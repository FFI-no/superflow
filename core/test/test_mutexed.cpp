#include "superflow/utils/mutexed.h"
#include "superflow/utils/blocker.h"
#include "gtest/gtest.h"

#include <future>


TEST(Mutexed, conforms_to_cpp_named_requirement_Lockable)
{
  class A{};
  const flow::Mutexed<A> mutexed;

  EXPECT_NO_FATAL_FAILURE(std::scoped_lock lock{mutexed});
}

TEST(Mutexed, mutexed_object_retains_its_properties)
{
  const flow::Mutexed<std::string> mutexed_str{"42"};

  const std::string equal{"42"};

  EXPECT_EQ(mutexed_str, equal);
}

TEST(Mutexed, example_using_Mutexed_instead_of_adding_a_mutex_to_a_non_threadsafe_type)
{
  struct ThreeVariables
  {
    int a;
    int b;
    int c;
  };

  flow::Mutexed<ThreeVariables> threevar;

  {
    std::scoped_lock lock{threevar};// protect all variables while writing
    threevar.a = 1;
    threevar.b = 2;
    threevar.c = threevar.a + threevar.b;
  }

  int result = 0;
  {
    std::scoped_lock lock{threevar};// protect all variables while writing
    result = threevar.a + threevar.b;
  }

  EXPECT_EQ(result, 3);
}

TEST(Mutexed, assign_other_Mutexed)
{
  flow::Mutexed<std::string> str1("ice cream");
  flow::Mutexed<std::string> str2("you scream");

  ASSERT_NO_FATAL_FAILURE(str1 = str2);
  EXPECT_EQ("you scream", str1);
}

TEST(Mutexed, assign_from_T)
{
  flow::Mutexed<std::string> str("ice cream");
  {
    std::lock_guard mlock(str);
    ASSERT_NO_FATAL_FAILURE(str = "you scream");
  }
  EXPECT_EQ("you scream", str);
}

TEST(Mutexed, scoped_lock)
{
  flow::Mutexed<std::string> str("ice cream");
  {
    std::scoped_lock mlock(str);
    ASSERT_NO_FATAL_FAILURE(str = "you scream");
  }
  EXPECT_EQ("you scream", str);
}

TEST(Mutexed, move_assign_from_T)
{
  flow::Mutexed<std::string> mu_str("ice cream");
  {
    std::lock_guard mlock(mu_str);
    std::string str{"beef"};
    ASSERT_NO_FATAL_FAILURE(mu_str = std::move(str));
  }
  EXPECT_EQ("beef", mu_str);
}

TEST(Mutexed, const_slice)
{
  const flow::Mutexed<std::string> mu_str("original");
  auto sliced = mu_str.slice();
  sliced = "sliced";
  EXPECT_NE(sliced, mu_str);
}

TEST(Mutexed, mutable_slice)
{
  flow::Mutexed<std::string> mu_str("original");
  auto& sliced = mu_str.slice();
  sliced = "sliced";
  EXPECT_EQ(sliced, mu_str);
  EXPECT_EQ("sliced", mu_str);
}

TEST(Mutexed, load)
{
  const flow::Mutexed<std::string> mu_str("original");
  auto copy = mu_str.load();
  const auto& ref = mu_str.load();
  copy = "a copy";
  mu_str.load() = "temporary discarded, no effect";
  EXPECT_EQ("original", mu_str);
  EXPECT_EQ("a copy", copy);
  EXPECT_EQ("original", ref);
}

TEST(Mutexed, load_must_wait)
{
  std::promise<void> promise;
  flow::Mutexed<std::string> mutexed("original");
  std::string value = "some value";
  auto fu = std::async(
    std::launch::async,
    [&promise, &mutexed, &value]
    {
      promise.get_future().wait();//< mutexed is locked

      value = mutexed.load();//< will try to lock, so must wait until 'new value' is set
    }
  );

  {
    std::scoped_lock lock{mutexed};
    promise.set_value();
    ASSERT_NO_FATAL_FAILURE(mutexed = "new value");
    // lock is still in scope, so 'value' is not yet overwritten
    EXPECT_EQ("some value", value);
  }

  fu.get();
  EXPECT_EQ("new value", value);
  EXPECT_EQ("new value", mutexed);
}

TEST(Mutexed, read_function_call_can_read_value)
{
  const flow::Mutexed<std::string> mutexed{"original"};
  std::string copied_value;
  mutexed.read(
    [&copied_value](auto&& mutexed_value)
    { copied_value = mutexed_value; }
  );

  EXPECT_EQ("original", copied_value);
}

TEST(Mutexed, write_function_call_can_overwrite_value)
{
  flow::Mutexed<std::string> mutexed{"original"};
  mutexed.write(
    [](auto&& mutexed_value)
    { mutexed_value = "new value"; }
  );

  EXPECT_EQ("new value", mutexed);
}

TEST(Mutexed, multiple_read_function_calls_are_still_exclusive)
{
  const flow::Mutexed<std::string> mutexed{"original"};

  std::promise<void> all_readers_started;
  std::atomic_size_t active_readers{0};
  std::atomic_size_t simultaneous_readers{0};

  std::promise<void> release_readers;
  auto must_wait = release_readers.get_future().share();

  std::vector<std::future<void>> readers;

  constexpr size_t num_readers{10};
  for (size_t i{0}; i < num_readers; ++i)
  {
    readers.emplace_back(
      std::async(
        std::launch::async,
        [&all_readers_started, &must_wait, &active_readers, &simultaneous_readers, &mutexed]
        {
          if (++active_readers == num_readers)
          { all_readers_started.set_value(); }
          // All reader threads have certainly been created

          mutexed.read(
            [&must_wait, &active_readers, &simultaneous_readers](auto&&)
            {
              ++simultaneous_readers;
              // Should never be more than one
              ASSERT_EQ(simultaneous_readers, 1);
              must_wait.wait();
              --simultaneous_readers;
              --active_readers;
            }
          );
        }
      )
    );
  }

  { // Assert that all readers have certainly been created
    using namespace std::chrono_literals;
    const auto future_status = all_readers_started.get_future().wait_for(1s);
    ASSERT_NE(future_status, std::future_status::timeout);

    // All are locked out of 'read' except the first.
    ASSERT_EQ(active_readers, num_readers);
    EXPECT_EQ(simultaneous_readers, 1);
  }

  release_readers.set_value();

  for (auto& reader: readers)
  { reader.wait(); }

  ASSERT_EQ(active_readers, 0);
  ASSERT_EQ(simultaneous_readers, 0);
}

TEST(Mutexed, write_must_wait_for_multiple_read)
{
  flow::Mutexed<std::string> mutexed{"original"};

  std::promise<void> release_readers;
  auto must_wait = release_readers.get_future().share();

  std::atomic_size_t active_workers{0};

  std::vector<std::future<void>> workers;
  constexpr size_t num_readers{10};
  {
    std::promise<void> all_readers_started;
    for (size_t i{0}; i < num_readers; ++i)
    {
      workers.emplace_back(
        std::async(
          std::launch::async,
          [&all_readers_started, &must_wait, &active_workers, &mutexed]
          {
            if (++active_workers == num_readers)
            { all_readers_started.set_value(); }
            // All reader threads have certainly been created

            mutexed.read(
              [&must_wait, &active_workers](auto&&)
              {
                must_wait.wait();
                --active_workers;
              }
            );
          }
        )
      );
    }

    { // Assert that all readers have certainly been created
      using namespace std::chrono_literals;
      const auto future_status = all_readers_started.get_future().wait_for(1s);
      ASSERT_NE(future_status, std::future_status::timeout);

      EXPECT_EQ(active_workers, num_readers);
    }
  }

  {
    std::promise<void> writer_started;
    workers.emplace_back(
      std::async(
        std::launch::async,
        [&writer_started, &active_workers, &mutexed]
        {
          ++active_workers;
          writer_started.set_value();
          mutexed.write(
            [&active_workers](auto&& str)
            { str = std::to_string(active_workers); }
          );
          --active_workers;
        }
      )
    );

    { // Assert that the writer has certainly been created
      using namespace std::chrono_literals;
      const auto future_status = writer_started.get_future().wait_for(1s);
      ASSERT_NE(future_status, std::future_status::timeout);

      // Readers are still blocked in 'read', so writer is also blocked.
      ASSERT_EQ(active_workers, num_readers + 1);
    }
  }

  release_readers.set_value();
  for (auto& worker: workers)
  { worker.wait(); }

  EXPECT_EQ(active_workers, 0);
  EXPECT_EQ("1", mutexed);
}


TEST(Mutexed, store_must_wait_for_multiple_read)
{
  flow::Mutexed<std::string> mutexed{"original"};

  std::promise<void> release_readers;
  auto must_wait = release_readers.get_future().share();

  std::atomic_size_t active_workers{0};

  std::vector<std::future<void>> workers;
  constexpr size_t num_readers{10};
  {
    std::promise<void> all_readers_started;
    for (size_t i{0}; i < num_readers; ++i)
    {
      workers.emplace_back(
        std::async(
          std::launch::async,
          [&all_readers_started, &must_wait, &active_workers, &mutexed]
          {
            if (++active_workers == num_readers)
            { all_readers_started.set_value(); }
            // All reader threads have certainly been created

            mutexed.read(
              [&must_wait, &active_workers](auto&&)
              {
                must_wait.wait();
                --active_workers;
              }
            );
          }
        )
      );
    }

    { // Assert that all readers have certainly been created
      using namespace std::chrono_literals;
      const auto future_status = all_readers_started.get_future().wait_for(1s);
      ASSERT_NE(future_status, std::future_status::timeout);

      EXPECT_EQ(active_workers, num_readers);
    }
  }

  {
    std::promise<void> writer_started;
    workers.emplace_back(
      std::async(
        std::launch::async,
        [&writer_started, &active_workers, &mutexed]
        {
          ++active_workers;
          writer_started.set_value();
          mutexed.store("store");
          mutexed.store(mutexed.load() + "," + std::to_string(active_workers));
          --active_workers;
        }
      )
    );

    { // Assert that the writer has certainly been created
      using namespace std::chrono_literals;
      const auto future_status = writer_started.get_future().wait_for(1s);
      ASSERT_NE(future_status, std::future_status::timeout);

      // when writer is eventually started,
      // the first reader is blocked and the rest are locked out
      ASSERT_EQ(active_workers, num_readers + 1);
    }
  }

  release_readers.set_value();
  for (auto& worker: workers)
  { worker.wait(); }

  EXPECT_EQ(active_workers, 0);
  EXPECT_EQ("store,1", mutexed);
}
