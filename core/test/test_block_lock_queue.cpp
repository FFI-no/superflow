#include "superflow/utils/lock_queue.h"

#include "gtest/gtest.h"

#include <future>
#include <thread>

using namespace flow;
using namespace std::chrono_literals;

TEST(BlockLockQueue, QueueSizeZeroThrows)
{
  ASSERT_THROW((LockQueue<int, LeakPolicy::PushBlocking>(0)), std::invalid_argument);
}

TEST(BlockLockQueue, PushLvalue)
{
  LockQueue<int, LeakPolicy::PushBlocking> impl(10);
  int val = 42;
  ASSERT_NO_FATAL_FAILURE(impl.push(val));
}

TEST(BlockLockQueue, PushRvalue)
{
  LockQueue<int, LeakPolicy::PushBlocking> impl(10);
  ASSERT_NO_FATAL_FAILURE(impl.push(42));
}

TEST(BlockLockQueue, Queue_size_from_const_reference_works)
{
  LockQueue<int, LeakPolicy::PushBlocking> impl(10);
  const auto& cref = impl;

  ASSERT_EQ(0u, cref.getQueueSize());
}

TEST(BlockLockQueue, PushIncreasesQueueSize)
{
  LockQueue<int, LeakPolicy::PushBlocking> impl(10);
  ASSERT_EQ(0u, impl.getQueueSize());
  impl.push(42);
  ASSERT_EQ(1u, impl.getQueueSize());
}

TEST(BlockLockQueue, InitializerListInitializedQueue)
{
  LockQueue<int, LeakPolicy::PushBlocking> impl(10, {42, 2, 3});
  EXPECT_EQ(3u, impl.getQueueSize());
  EXPECT_EQ(42, impl.pop());
}

TEST(BlockLockQueue, TooLongInitializerListThrowsException)
{
  EXPECT_THROW((LockQueue<int, LeakPolicy::PushBlocking>(2, {42, 2, 3})), std::range_error);
}

TEST(BlockLockQueue, PopReturnsInsertedValue)
{
  LockQueue<int, LeakPolicy::PushBlocking> impl(10);
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

TEST(BlockLockQueue, PopDecreasesQueueSize)
{
  LockQueue<int, LeakPolicy::PushBlocking> impl(10);
  ASSERT_EQ(0u, impl.getQueueSize());
  impl.push(42);
  ASSERT_EQ(1u, impl.getQueueSize());
  impl.pop();
  ASSERT_EQ(0u, impl.getQueueSize());
}

TEST(BlockLockQueue, PopHangsUntilPushAndDoesNotThrow)
{
  LockQueue<int, LeakPolicy::PushBlocking> impl(10);
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

TEST(BlockLockQueue, PopHangsUntilTerminateAndThenThrows)
{
  LockQueue<int, LeakPolicy::PushBlocking> impl(10);

  int popped_value = 42;

  std::promise<void> terminated;

  auto term_fn = [&impl, &terminated]()
  {
    EXPECT_NO_THROW(impl.terminate());
    terminated.set_value();
  };

  std::thread terminator(term_fn);
  {
    const auto future_status = terminated.get_future().wait_for(1s);
    ASSERT_NE(future_status, std::future_status::timeout);
  }
  ASSERT_TRUE(terminator.joinable());

  EXPECT_THROW(popped_value = impl.pop(), TerminatedException);
  terminator.join();
  EXPECT_EQ(42, popped_value);
}

TEST(BlockLockQueue, PushHangsUntilTerminateAndThenThrows)
{
  LockQueue<int, LeakPolicy::PushBlocking> impl(2);

  impl.push(1);
  impl.push(2);
  std::promise<void> pushed;

  auto blocked_push = std::async(std::launch::async, [&impl, &pushed]
  {
    EXPECT_NO_THROW(impl.push(3));
    pushed.set_value();
    impl.push(4);
  });

  auto future = pushed.get_future();
  EXPECT_EQ(future.wait_for(10ms), std::future_status::timeout);

  impl.pop();

  EXPECT_EQ(future.wait_for(1s), std::future_status::ready);
  EXPECT_NO_THROW(impl.terminate());
  EXPECT_EQ(blocked_push.wait_for(1s), std::future_status::ready);
  EXPECT_THROW(blocked_push.get(), TerminatedException);
}

TEST(BlockLockQueue, TerminateTwiceDoesNotThrow)
{
  LockQueue<int, LeakPolicy::PushBlocking> impl(10);
  EXPECT_NO_THROW(impl.terminate());
  EXPECT_NO_THROW(impl.terminate());
}

TEST(BlockLockQueue, Queue_correctly_responds_if_it_is_terminated_or_not)
{
  LockQueue<int, LeakPolicy::PushBlocking> impl(10);
  std::promise<void> terminate;
  std::promise<void> terminated;

  auto term_fn = [&impl, &terminate, &terminated]()
  {
    terminate.get_future().wait();
    EXPECT_NO_THROW(impl.terminate());
    terminated.set_value();
  };

  std::thread terminator(term_fn);
  ASSERT_TRUE(terminator.joinable());

  EXPECT_FALSE(impl.isTerminated());
  terminate.set_value();

  terminated.get_future().wait();
  EXPECT_TRUE(impl.isTerminated());

  terminator.join();
}

TEST(BlockLockQueue, PushMoreThanCapacityBlocksPush)
{
  constexpr uint32_t queue_size = 10;
  LockQueue<uint32_t, LeakPolicy::PushBlocking> impl(queue_size);
  ASSERT_EQ(0u, impl.getQueueSize());
  for (uint32_t i = 0; i < queue_size; ++i)
  {
    impl.push(i);
  }
  ASSERT_EQ(10u, impl.getQueueSize());

  auto pusher = std::async(std::launch::async, [&impl]{
    impl.push(42);
  });

  EXPECT_EQ(pusher.wait_for(10ms), std::future_status::timeout);
  ASSERT_EQ(10u, impl.getQueueSize());

  const auto value = impl.pop();
  EXPECT_EQ(0, value);

  EXPECT_EQ(pusher.wait_for(1s), std::future_status::ready);
  EXPECT_NO_FATAL_FAILURE(pusher.get());
  EXPECT_EQ(10u, impl.getQueueSize());
}

TEST(BlockLockQueue, PushMoreThanCapacityDoesntDiscard)
{
  constexpr uint32_t queue_size = 10;
  LockQueue<uint32_t, LeakPolicy::PushBlocking> impl(queue_size);
  ASSERT_EQ(0u, impl.getQueueSize());
  for (uint32_t i = 0; i < queue_size; ++i)
  {
    impl.push(i);
  }
  ASSERT_EQ(10u, impl.getQueueSize());

  auto pusher = std::async(std::launch::async, [&impl]{
    ASSERT_NO_THROW(impl.push(42));
  });

  EXPECT_EQ(pusher.wait_for(10ms), std::future_status::timeout);
  ASSERT_EQ(10u, impl.getQueueSize());

  for (uint32_t i = 0; i < queue_size; ++i)
  {
    ASSERT_EQ(i, impl.pop());
  }

  EXPECT_EQ(pusher.wait_for(1s), std::future_status::ready);
  ASSERT_EQ(1u, impl.getQueueSize());
  ASSERT_EQ(42, impl.pop());
  EXPECT_NO_FATAL_FAILURE(pusher.get());
}

TEST(BlockLockQueue, ClearQueueClearsQueue)
{
  uint32_t queue_size = 10;
  LockQueue<int, LeakPolicy::PushBlocking> impl(queue_size);
  EXPECT_EQ(0u, impl.getQueueSize());
  for (uint32_t i = 0; i < queue_size; ++i)
  {
    impl.push(i);
  }
  EXPECT_EQ(queue_size, impl.getQueueSize());
  impl.clearQueue();
  EXPECT_EQ(0u, impl.getQueueSize());
}

TEST(BlockLockQueue, MultiThreadPushQueueAlwaysHasOneElement)
{
  LockQueue<int, LeakPolicy::PushBlocking> queue{1};
  queue.push(-1);

  std::vector<std::future<void>> workers;
  constexpr int num_workers = 10;

  std::promise<void> all_workers_launched;
  std::atomic_size_t num_workers_launched{0};

  for (int i = 0; i < num_workers; ++i)
  {
    workers.push_back(
      std::async(
        std::launch::async,
        [&queue, i, &num_workers_launched, &all_workers_launched]()
        {
          if (++num_workers_launched == num_workers)
          { all_workers_launched.set_value(); }

          while (!queue.isTerminated())
          {
            queue.push(i);
          }
        }
      )
    );
  }

  EXPECT_EQ(all_workers_launched.get_future().wait_for(1s), std::future_status::ready);

  for (int i = 0; i < 10000; ++i)
  {
    const size_t queue_size = queue.getQueueSize();

    if (queue_size != 1)
    {
      queue.terminate();
    }

    ASSERT_EQ(queue_size, 1);
  }

  queue.terminate();
}

TEST(BlockLockQueue, DTORTerminates)
{
  std::atomic_int worker_sum = 0;
  std::atomic_bool worker_finished = false;
  std::future<void> worker;

  {
    LockQueue<int, LeakPolicy::PushBlocking> queue(10);
    std::promise<void> worker_certainly_launched;
    worker = std::async(
      std::launch::async,
      [&queue, &worker_sum, &worker_certainly_launched, &worker_finished]()
      {
        worker_certainly_launched.set_value();
        try { worker_sum += queue.pop(); }
        catch (TerminatedException& ) {}

        worker_finished = true;
      }
    );

    EXPECT_NE(worker_certainly_launched.get_future().wait_for(1s), std::future_status::timeout);
    ASSERT_FALSE(worker_finished);
  }

  worker.wait();
  ASSERT_EQ(worker_sum, 0);
  ASSERT_TRUE(worker_finished);
}

TEST(BlockLockQueue, Notify)
{
  LockQueue<int, LeakPolicy::PushBlocking> queue(1, {42});

  std::atomic_bool pusher_finished = false;

  auto pusher = std::async(
    std::launch::async,
    [&queue, &pusher_finished]()
    {
      queue.push(100);
      pusher_finished = true;
    }
  );

  // The 'pusher' must wait while the PushBlocking queue is at max capacity.
  EXPECT_EQ(pusher.wait_for(5ms), std::future_status::timeout);
  EXPECT_FALSE(pusher_finished);

  struct {
    std::atomic_size_t num_started_poppers = 0;
    std::atomic_size_t num_terminated_poppers = 0;
    std::atomic_size_t num_successful_poppers = 0;
    std::promise<void> all_started;
  } captured;

  constexpr int num_poppers = 10;
  std::vector<std::future<void>> poppers;

  for (int i = 0; i < num_poppers; ++i)
  {
    poppers.push_back(
      std::async(
        std::launch::async, [&queue, &captured]()
        {
          if (++captured.num_started_poppers == num_poppers)
          { captured.all_started.set_value(); }
          // All poppers have certainly been created

          try
          { // num_poppers will all try to pop. While the queue is empty, they must sleep
            queue.pop();
            ++captured.num_successful_poppers;
          } // ... unless the queue is terminated
          catch (TerminatedException&)
          { ++captured.num_terminated_poppers; }
        }));
  }

  {
    const auto future_status = pusher.wait_for(1s);
    ASSERT_NE(future_status, std::future_status::timeout);
    ASSERT_TRUE(pusher_finished);
  } // All poppers have certainly been created


  { // And the pusher are certainly finished
    const auto future_status = captured.all_started.get_future().wait_for(1s);
    ASSERT_NE(future_status, std::future_status::timeout);
  }

  // Initial value and pusher's value are popped
  EXPECT_EQ(2, captured.num_successful_poppers);

  // All other poppers are blocked
  EXPECT_EQ(0, captured.num_terminated_poppers);

  queue.terminate();

  for (const auto& popper : poppers)
  {
    const auto future_status =  popper.wait_for(1s);
    ASSERT_NE(future_status, std::future_status::timeout);
  }
  EXPECT_EQ(num_poppers-2, captured.num_terminated_poppers);
  EXPECT_EQ(2, captured.num_successful_poppers);
}
