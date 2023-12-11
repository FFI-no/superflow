#include "superflow/utils/shared_mutexed.h"
#include "gtest/gtest.h"

#include <future>

using namespace std::chrono_literals;

TEST(SharedMutexed, multiple_read_can_happen_simultaneously)
{
  const flow::SharedMutexed<std::string> mutexed{"original"};

  std::promise<void> release_readers;
  auto must_wait = release_readers.get_future().share();
  std::promise<void> all_readers_started;
  std::atomic_size_t active_readers{0};
  std::atomic_size_t simultaneous_readers{0};

  std::vector<std::future<std::string>> readers;

  constexpr size_t num_readers{10};
  for (size_t i{0}; i < num_readers; ++i)
  {
    readers.emplace_back(
      std::async(
        std::launch::async,
        [&all_readers_started, &must_wait, &active_readers, &simultaneous_readers, &mutexed]
        {
          return mutexed.read(
            [&all_readers_started, &must_wait, &active_readers,&simultaneous_readers](const auto& value)-> std::string
            {
              ++simultaneous_readers;

              if (++active_readers == num_readers)
              { all_readers_started.set_value(); }
              // All reader threads have certainly been created

              must_wait.wait();
              --simultaneous_readers;
              --active_readers;
              return value;
            }
          );
        }
      )
    );
  }

  { // Assert that all readers have certainly been created
    const auto future_status = all_readers_started.get_future().wait_for(1s);
    ASSERT_NE(future_status, std::future_status::timeout);

    EXPECT_EQ(active_readers, num_readers);
    EXPECT_EQ(simultaneous_readers.load(), num_readers);
  }

  release_readers.set_value();
  for (auto& reader: readers)
  { ASSERT_EQ("original", reader.get()); }

  EXPECT_EQ(active_readers, 0);
}

TEST(SharedMutexed, store_must_wait_for_multiple_read)
{
  flow::SharedMutexed<std::string> mutexed{"original"};

  std::promise<void> release_readers;
  auto must_wait = release_readers.get_future().share();
  std::atomic_size_t active_readers{0};

  std::vector<std::future<std::string>> readers;
  constexpr size_t num_readers{10};
  {
    std::promise<void> all_readers_started;
    for (size_t i{0}; i < num_readers; ++i)
    {
      readers.emplace_back(
        std::async(
          std::launch::async, [&all_readers_started, &must_wait, &active_readers, &mutexed]
          {
            return mutexed.read(
              [&all_readers_started, &must_wait, &active_readers](const auto& value) -> std::string
              {
                if (++active_readers == num_readers)
                { all_readers_started.set_value(); }
                // All reader threads have certainly been created

                must_wait.wait();
                --active_readers;
                return value;
              }
            );
          }
        )
      );
    }

    { // Assert that all readers have certainly been created
      const auto future_status = all_readers_started.get_future().wait_for(1s);
      ASSERT_NE(future_status, std::future_status::timeout);

      // They are all busy with 'read', but are blocked by the 'must_wait'
      ASSERT_EQ(active_readers, num_readers);
    }
  }

  std::promise<void> writer_started;
  auto writer = std::async(
    std::launch::async, [&writer_started, &active_readers, &mutexed]
    {
      writer_started.set_value();
      mutexed.store("store");
      mutexed.store(mutexed.load() + "," + std::to_string(active_readers));
    }
  );

  { // Assert that the writer has certainly been created
    const auto future_status = writer_started.get_future().wait_for(1s);
    ASSERT_NE(future_status, std::future_status::timeout);
  }

  // When writer is eventually started,
  // all readers are still blocked in 'read', so writer is also blocked.
  ASSERT_EQ(active_readers, num_readers);

  // All the readers must be served before writer can lock mutexed.
  // All readers will read the initial value
  release_readers.set_value();
  for (auto& reader: readers)
  {
    ASSERT_EQ("original", reader.get());
  }

  const auto future_status = writer.wait_for(1s);
  ASSERT_NE(future_status, std::future_status::timeout);

  EXPECT_EQ(active_readers, 0);
  EXPECT_EQ("store,0", mutexed);
}
