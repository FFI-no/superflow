#include "superflow/utils/lock_queue.h"

#include "gtest/gtest.h"

#include <future>
#include <thread>

using namespace flow;

TEST(LockQueue, QueueSizeZeroThrows)
{
  ASSERT_THROW(LockQueue<int>(0), std::invalid_argument);
}

TEST(LockQueue, PushLvalue)
{
  LockQueue<int> impl(10);
  int val = 42;
  ASSERT_NO_FATAL_FAILURE(impl.push(val));
}

TEST(LockQueue, PushRvalue)
{
  LockQueue<int> impl(10);
  ASSERT_NO_FATAL_FAILURE(impl.push(42));
}

TEST(LockQueue, Queue_size_from_const_reference_works)
{
  LockQueue<int> impl(10);
  const auto& cref = impl;

  ASSERT_EQ(0u, cref.getQueueSize());
}

TEST(LockQueue, PushIncreasesQueueSize)
{
  LockQueue<int> impl(10);
  ASSERT_EQ(0u, impl.getQueueSize());
  impl.push(42);
  ASSERT_EQ(1u, impl.getQueueSize());
}

TEST(LockQueue, InitializerListInitializedQueue)
{
  LockQueue<int> impl(10, {42, 2, 3});
  EXPECT_EQ(3u, impl.getQueueSize());
  EXPECT_EQ(42, impl.pop());
}

TEST(LockQueue, TooLongInitializerListThrowsException)
{
  EXPECT_THROW(LockQueue<int>(2, {42, 2, 3}), std::range_error);
}

TEST(LockQueue, PopReturnsInsertedValue)
{
  LockQueue<int> impl(10);
  const int val = 42;

  // Test T pop()
  impl.push(val);
  impl.push(val + 1);
  EXPECT_EQ(val, impl.pop());
  EXPECT_EQ(val + 1, impl.pop());

  // Test void pop(T&)
  EXPECT_EQ(0u, impl.getQueueSize());
  impl.push(val);
  impl.push(val + 1);
  int res = 0;
  EXPECT_NO_FATAL_FAILURE(impl.pop(res));
  EXPECT_EQ(val, res);
  EXPECT_NO_FATAL_FAILURE(impl.pop(res));
  EXPECT_EQ(val + 1, res);
}

TEST(LockQueue, PopDecreasesQueueSize)
{
  LockQueue<int> impl(10);
  ASSERT_EQ(0u, impl.getQueueSize());
  impl.push(42);
  ASSERT_EQ(1u, impl.getQueueSize());
  impl.pop();
  ASSERT_EQ(0u, impl.getQueueSize());
}

TEST(LockQueue, PopHangsUntilPushAndDoesNotThrow)
{
  LockQueue<int> impl(10);
  int popped_value = 0;
  auto push_fn = [&impl]()
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    impl.push(42);
  };
  std::thread pusher(push_fn);
  ASSERT_TRUE(pusher.joinable());
  EXPECT_EQ(0, popped_value);

  EXPECT_NO_THROW(popped_value = impl.pop());

  pusher.join();
  EXPECT_EQ(42, popped_value);
}

TEST(LockQueue, PopHangsUntilTerminateAndThenThrows)
{
  LockQueue<int> impl(10);

  int popped_value = 42;

  auto term_fn = [&impl]()
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    EXPECT_NO_THROW(impl.terminate());
  };

  std::thread terminator(term_fn);
  ASSERT_TRUE(terminator.joinable());

  EXPECT_THROW(popped_value = impl.pop(), TerminatedException);
  terminator.join();
  EXPECT_EQ(42, popped_value);
}

TEST(LockQueue, TerminateTwiceDoesNotThrow)
{
  LockQueue<int> impl(10);
  EXPECT_NO_THROW(impl.terminate());
  EXPECT_NO_THROW(impl.terminate());
}

TEST(LockQueue, Queue_correctly_responds_if_it_is_terminated_or_not)
{
  std::mutex mu;
  std::condition_variable cv;
  std::atomic_bool tests_verified = false;

  LockQueue<int> impl(10);
  auto terminator = std::async(
    std::launch::async,
    [&impl, &mu, &cv, &tests_verified]()
    {
      std::unique_lock<std::mutex> mlock(mu);
      cv.wait(mlock, [&tests_verified]()
      { return tests_verified.load(); });
      impl.terminate();
    }
  );
  EXPECT_FALSE(impl.isTerminated());
  {
    std::unique_lock<std::mutex> mlock(mu);
    tests_verified = true;
    cv.notify_one();
  }

  terminator.wait();
  EXPECT_TRUE(impl.isTerminated());
}

TEST(LockQueue, PushMoreThanCapacityDoesNotIncreasesQueueSize)
{
  uint32_t queue_size = 10;
  LockQueue<uint32_t> impl(queue_size);
  ASSERT_EQ(0u, impl.getQueueSize());
  for (uint32_t i = 0; i < queue_size; ++i)
  {
    impl.push(i);
  }
  ASSERT_EQ(10u, impl.getQueueSize());
  impl.push(42);
  ASSERT_EQ(10u, impl.getQueueSize());
}

TEST(LockQueue, PushMoreThanCapacityDiscardsFront)
{
  uint32_t queue_size = 10;
  LockQueue<uint32_t> impl(queue_size);
  ASSERT_EQ(0u, impl.getQueueSize());
  for (uint32_t i = 0; i < queue_size; ++i)
  {
    impl.push(i);
  }
  ASSERT_EQ(10u, impl.getQueueSize());
  impl.push(42);
  auto front = impl.pop();
  ASSERT_EQ(1, front);
}

TEST(LockQueue, ClearQueueClearsQueue)
{
  uint32_t queue_size = 10;
  LockQueue<int> impl(queue_size);
  EXPECT_EQ(0u, impl.getQueueSize());
  for (uint32_t i = 0; i < queue_size; ++i)
  {
    impl.push(i);
  }
  EXPECT_EQ(queue_size, impl.getQueueSize());
  impl.clearQueue();
  EXPECT_EQ(0u, impl.getQueueSize());
}

TEST(LockQueue, MultiThreadPushQueueAlwaysHasOneElement)
{
  LockQueue<int> queue{1};
  queue.push(-1);

  bool is_running = true;

  std::vector<std::future<void>> workers;
  constexpr int num_workers = 10;

  for (int i = 0; i < num_workers; ++i) {
    workers.push_back(std::async(std::launch::async, [&is_running, &queue, i]()
    {
      while (is_running) {
        queue.push(i);
      }
    }));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds{10});

  for (int i = 0; i < 10000; ++i) {
    const size_t queue_size = queue.getQueueSize();

    if (queue_size != 1) {
      is_running = false;
    }

    ASSERT_EQ(queue_size, 1);
  }

  is_running = false;
}

TEST(LockQueue, DTORTerminates)
{
  std::atomic_int worker_sum = 0;
  std::atomic_bool worker_finished = false;
  std::future<void> worker;

  {
    LockQueue<int> queue(10);

    worker = std::async(
      std::launch::async,
      [&queue, &worker_sum, &worker_finished]()
      {
        try { worker_sum += queue.pop(); }
        catch (TerminatedException& ) {}

        worker_finished = true;
      }
    );

    std::this_thread::sleep_for(std::chrono::milliseconds{5});
    ASSERT_FALSE(worker_finished);
  }

  worker.wait();
  ASSERT_EQ(worker_sum, 0);
  ASSERT_TRUE(worker_finished);
}
