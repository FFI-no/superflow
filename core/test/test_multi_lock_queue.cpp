#include "superflow/utils/multi_lock_queue.h"

#include "gtest/gtest.h"

#include <chrono>
#include <future>
#include <numeric>
#include <thread>

using namespace flow;

using namespace std::chrono_literals;

TEST(MultiLockQueue, PushLvalue)
{
  constexpr size_t queue_size = 10;
  MultiLockQueue<int, int> multi_queue{queue_size};

  constexpr int key = 13;
  const int val = 42;

  ASSERT_NO_FATAL_FAILURE(multi_queue.push(key, val));
}

TEST(MultiLockQueue, PushRvalue)
{
  constexpr size_t queue_size = 10;
  MultiLockQueue<int, int> multi_queue{queue_size};

  constexpr int key = 21;

  ASSERT_NO_FATAL_FAILURE(multi_queue.push(key, 42));
}

TEST(MultiLockQueue, PushToInitedQueue)
{
  constexpr size_t queue_size = 10;
  constexpr int key = 21;
  MultiLockQueue<int, int> multi_queue{queue_size, {key}};

  ASSERT_NO_FATAL_FAILURE(multi_queue.push(key, 42));
}

TEST(MultiLockQueue, PushToMultipleQueues)
{
  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size};

  constexpr int val_offset = 13;

  for (int key = 0; key < 10; ++key)
  {
    ASSERT_NO_FATAL_FAILURE(multi_queue.push(key, key + val_offset));
  }
}

TEST(MultiLockQueue, PopReadyReturnsInsertedValues)
{
  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size};

  constexpr int val_offset = 13;

  for (int key = 0; key < 10; ++key)
  {
    ASSERT_NO_FATAL_FAILURE(multi_queue.push(key, key + val_offset));
  }

  const auto values = multi_queue.popReady();

  for (const auto& kv : values)
  {
    const auto key = kv.first;
    const auto val = kv.second;

    ASSERT_EQ(val, key + val_offset);
  }
}

TEST(MultiLockQueue, PopAtLeastOneReturnsInsertedValues)
{
  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size};

  constexpr int val_offset = 13;

  for (int key = 0; key < 10; ++key)
  {
    ASSERT_NO_FATAL_FAILURE(multi_queue.push(key, key + val_offset));
  }

  const auto values = multi_queue.popAtLeastOne();

  for (const auto& kv : values)
  {
    const auto key = kv.first;
    const auto val = kv.second;

    ASSERT_EQ(val, key + val_offset);
  }
}

TEST(MultiLockQueue, PopAllReturnsInsertedValues)
{
  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size};

  constexpr int val_offset = 13;

  for (int key = 0; key < 10; ++key)
  {
    ASSERT_NO_FATAL_FAILURE(multi_queue.push(key, key + val_offset));
  }

  const auto values = multi_queue.popAll();

  for (const auto& kv : values)
  {
    const auto key = kv.first;
    const auto val = kv.second;

    ASSERT_EQ(val, key + val_offset);
  }
}

TEST(MultiLockQueue, PeekReadyReturnsInsertedValues)
{
  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size};

  constexpr int val_offset = 13;

  for (int key = 0; key < 10; ++key)
  {
    ASSERT_NO_FATAL_FAILURE(multi_queue.push(key, key + val_offset));
  }

  const auto values = multi_queue.peekReady();

  for (const auto& kv : values)
  {
    const auto key = kv.first;
    const auto val = kv.second;

    ASSERT_EQ(val, key + val_offset);
  }
}

TEST(MultiLockQueue, PeekAtLeastOneReturnsInsertedValues)
{
  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size};

  constexpr int val_offset = 13;

  for (int key = 0; key < 10; ++key)
  {
    ASSERT_NO_FATAL_FAILURE(multi_queue.push(key, key + val_offset));
  }

  const auto values = multi_queue.peekAtLeastOne();

  for (const auto& kv : values)
  {
    const auto key = kv.first;
    const auto val = kv.second;

    ASSERT_EQ(val, key + val_offset);
  }
}

TEST(MultiLockQueue, PeekAllReturnsInsertedValues)
{
  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size};

  constexpr int val_offset = 13;

  for (int key = 0; key < 10; ++key)
  {
    ASSERT_NO_FATAL_FAILURE(multi_queue.push(key, key + val_offset));
  }

  const auto values = multi_queue.peekAll();

  for (const auto& kv : values)
  {
    const auto key = kv.first;
    const auto val = kv.second;

    ASSERT_EQ(val, key + val_offset);
  }
}

TEST(MultiLockQueue, PopAllDoesNotBlockForUninitedQueues)
{
  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size};

  constexpr int val_offset = 13;

  std::vector<std::future<void>> workers;
  std::atomic_size_t finished_workers{0};

  std::promise<void> unblock;
  const auto blocker = unblock.get_future().share();

  std::promise<void> worker_promise;
  const auto workers_done = worker_promise.get_future();

  for (int key = 0; key < 10; ++key)
  {
    workers.push_back(
      std::async(
        std::launch::async,
        [key, &multi_queue, &blocker, &worker_promise, &finished_workers]() {
          blocker.wait();
          multi_queue.push(key, key + val_offset);
          if (++finished_workers == 10)
          {
            worker_promise.set_value();
          }
        }));
  }

  ASSERT_EQ(multi_queue.getNumQueues(), 0);

  {
    const auto values = multi_queue.popAll();

    ASSERT_TRUE(values.empty());
  }

  unblock.set_value();
  workers_done.wait();

  {
    const auto values = multi_queue.popAll();

    ASSERT_EQ(values.size(), workers.size());

    for (const auto& kv : values)
    {
      const auto key = kv.first;
      const auto val = kv.second;

      ASSERT_EQ(val, key + val_offset);
    }
  }
}

TEST(MultiLockQueue, PopAllBlocksForCtorInitedQueues)
{
  constexpr size_t num_keys = 10;
  std::vector<int> keys(num_keys);
  std::iota(keys.begin(), keys.end(), 0);

  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size, keys};

  constexpr int val_offset = 13;

  std::vector<std::future<void>> workers;
  std::promise<void> unblock;
  const auto blocker = unblock.get_future().share();

  for (const auto key : keys)
  {
    workers.push_back(
      std::async(
        std::launch::async,
        [key, &blocker, &multi_queue]() {
          blocker.wait();
          multi_queue.push(key, key + val_offset);
        }));
  }

  auto future_values = std::async(std::launch::async, [&multi_queue]{ return multi_queue.popAll(); });

  ASSERT_EQ(future_values.wait_for(10ms), std::future_status::timeout);
  unblock.set_value();
  const auto values = future_values.get();

  ASSERT_EQ(values.size(), workers.size());

  for (const auto& kv : values)
  {
    const auto key = kv.first;
    const auto val = kv.second;

    ASSERT_EQ(val, key + val_offset);
  }
}

TEST(MultiLockQueue, PopAllBlocksForDynamicallyInitedQueues)
{
  constexpr size_t num_keys = 10;
  std::vector<int> keys(num_keys);
  std::iota(keys.begin(), keys.end(), 0);

  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size};

  constexpr int val_offset = 13;

  std::vector<std::future<void>> workers;
  std::promise<void> unblock;
  const auto blocker = unblock.get_future().share();

  for (const auto key : keys)
  {
    // push dummy values in order to init queue dynamically
    multi_queue.push(key, key + val_offset - 1);

    workers.push_back(
        std::async(
            std::launch::async,
            [key, &blocker, &multi_queue]()
            {
              blocker.wait();
              multi_queue.push(key, key + val_offset);
            }
        )
    );
  }

  // pop dummy values
  multi_queue.popAll();

  ASSERT_FALSE(multi_queue.hasAny());

  auto future_values = std::async(std::launch::async, [&multi_queue]{ return multi_queue.popAll(); });
  ASSERT_EQ(future_values.wait_for(10ms), std::future_status::timeout);

  unblock.set_value();
  const auto values = future_values.get();

  ASSERT_EQ(values.size(), workers.size());

  for (const auto& kv : values)
  {
    const auto key = kv.first;
    const auto val = kv.second;

    ASSERT_EQ(val, key + val_offset);
  }
}

TEST(MultiLockQueue, PopAllRemovesElements)
{
  constexpr size_t num_keys = 10;
  std::vector<int> keys(num_keys);
  std::iota(keys.begin(), keys.end(), 0);

  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size};

  constexpr int val_offset = 13;

  for (const auto key : keys)
  {
    multi_queue.push(key, key + val_offset - 1);
  }

  ASSERT_TRUE(multi_queue.hasAny());

  // pop dummy values
  multi_queue.popAll();

  ASSERT_FALSE(multi_queue.hasAny());
}

TEST(MultiLockQueue, PopAtLeastOneBlocksForFirstValueForCtorInitedQueues)
{
  constexpr size_t num_keys = 10;
  std::vector<int> keys(num_keys);
  std::iota(keys.begin(), keys.end(), 0);

  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size, keys};

  constexpr int val_offset = 13;

  std::promise<void> unblock;
  const auto blocker = unblock.get_future().share();

  const auto worker = std::async(
      std::launch::async,
      [keys, &blocker, &multi_queue]()
      {
        blocker.wait();
        multi_queue.push(keys.front(), keys.front() + val_offset);
      }
  );


  auto future_values = std::async(std::launch::async, [&multi_queue]{ return multi_queue.popAtLeastOne(); });
  ASSERT_EQ(future_values.wait_for(10ms), std::future_status::timeout);

  unblock.set_value();
  const auto values = future_values.get();
  ASSERT_EQ(values.size(), 1);

  for (const auto& kv : values)
  {
    const auto key = kv.first;
    const auto val = kv.second;

    ASSERT_EQ(val, key + val_offset);
  }
}

TEST(MultiLockQueue, PopAtLeastOneReturnsForNewValue)
{
  constexpr size_t num_keys = 10;
  std::vector<int> keys(num_keys);
  std::iota(keys.begin(), keys.end(), 0);

  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size};

  constexpr int val_offset = 13;

  std::vector<std::future<void>> workers;

  std::promise<void> unblock;
  const auto blocker = unblock.get_future().share();

  for (const auto key : keys)
  {
    // push dummy values in order to init queue
    multi_queue.push(key, key + val_offset - 1);

    workers.push_back(
        std::async(
            std::launch::async,
            [key, &blocker, &multi_queue]()
            {
              blocker.wait();
              multi_queue.push(key, key + val_offset);
            }
        )
    );
  }

  // pop dummy values
  multi_queue.popAll();
  ASSERT_FALSE(multi_queue.hasAny());

  auto future_values = std::async(std::launch::async, [&multi_queue]{ return multi_queue.popAtLeastOne(); });
  ASSERT_EQ(future_values.wait_for(10ms), std::future_status::timeout);
  unblock.set_value();

  const auto values = future_values.get();
  ASSERT_FALSE(values.empty());

  for (const auto& kv : values)
  {
    const auto key = kv.first;
    const auto val = kv.second;

    ASSERT_EQ(val, key + val_offset);
  }

}

TEST(MultiLockQueue, PopAtLeastOneBlocksForAtLeastOneNewValue)
{
  constexpr size_t num_keys = 10;
  std::vector<int> keys(num_keys);
  std::iota(keys.begin(), keys.end(), 0);

  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size};

  constexpr int val_offset = 13;

  for (const auto key : keys)
  {
    // push dummy values in order to init queue
    multi_queue.push(key, key + val_offset - 1);
  }

  // pop dummy values
  multi_queue.popAll();

  multi_queue.push(0, val_offset);

  const auto values = multi_queue.popAtLeastOne();

  ASSERT_EQ(values.size(), 1);

  for (const auto& kv : values)
  {
    const auto key = kv.first;
    const auto val = kv.second;

    ASSERT_EQ(val, key + val_offset);
  }
}

TEST(MultiLockQueue, PopAtLeastOneReturnsAllNewValues)
{
  constexpr size_t num_keys = 10;
  std::vector<int> keys(num_keys);
  std::iota(keys.begin(), keys.end(), 0);

  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size};

  constexpr int val_offset = 13;

  for (const auto key : keys)
  {
    // push dummy values in order to init queue
    multi_queue.push(key, key + val_offset - 1);
  }

  // pop dummy values
  multi_queue.popAll();

  constexpr int num_new = 4;
  for (int key = 0; key < num_new; ++key)
  {
    multi_queue.push(key, key + val_offset);
  }

  const auto values = multi_queue.popAtLeastOne();

  ASSERT_EQ(values.size(), static_cast<size_t>(num_new));

  for (const auto& kv : values)
  {
    const auto key = kv.first;
    const auto val = kv.second;

    ASSERT_EQ(val, key + val_offset);
  }
}

TEST(MultiLockQueue, PopAtLeastOneRemovesElements)
{
  constexpr size_t num_keys = 10;
  std::vector<int> keys(num_keys);
  std::iota(keys.begin(), keys.end(), 0);

  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size};

  constexpr int val_offset = 13;

  for (const auto key : keys)
  {
    multi_queue.push(key, key + val_offset - 1);
  }

  ASSERT_TRUE(multi_queue.hasAny());

  // pop dummy values
  multi_queue.popAtLeastOne();

  ASSERT_FALSE(multi_queue.hasAny());
}

TEST(MultiLockQueue, PopReadyDoesNotBlockForUninitedQueues)
{
  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size};

  constexpr int val_offset = 13;

  std::vector<std::future<void>> workers;

  std::promise<void> unblock;
  const auto blocker = unblock.get_future().share();

  for (int key = 0; key < 10; ++key)
  {
    workers.push_back(
        std::async(
            std::launch::async,
            [key, &blocker, &multi_queue]()
            {
              blocker.wait();
              multi_queue.push(key, key + val_offset);
            }
        )
    );
  }

  const auto values = multi_queue.popReady();
  ASSERT_TRUE(values.empty());
  unblock.set_value();
}

TEST(MultiLockQueue, PopReadyDoesNotBlocksForCtorInitedQueues)
{
  constexpr size_t num_keys = 10;
  std::vector<int> keys(num_keys);
  std::iota(keys.begin(), keys.end(), 0);

  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size, keys};

  constexpr int val_offset = 13;

  std::vector<std::future<void>> workers;

  std::promise<void> unblock;
  const auto blocker = unblock.get_future().share();

  for (const auto key : keys)
  {
    workers.push_back(
        std::async(
            std::launch::async,
            [key, &blocker, &multi_queue]()
            {
              blocker.wait();
              multi_queue.push(key, key + val_offset);
            }
        )
    );
  }

  const auto values = multi_queue.popReady();
  ASSERT_TRUE(values.empty());
  unblock.set_value();
}

TEST(MultiLockQueue, PopReadyReturnsAllNewValues)
{
  constexpr size_t num_keys = 10;
  std::vector<int> keys(num_keys);
  std::iota(keys.begin(), keys.end(), 0);

  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size};

  constexpr int val_offset = 13;

  for (const auto key : keys)
  {
    multi_queue.push(key, key + val_offset);
  }

  const auto values = multi_queue.popReady();

  ASSERT_EQ(values.size(), num_keys);

  for (const auto& kv : values)
  {
    const auto key = kv.first;
    const auto val = kv.second;

    ASSERT_EQ(val, key + val_offset);
  }
}

TEST(MultiLockQueue, PopReadyRemovesElements)
{
  constexpr size_t num_keys = 10;
  std::vector<int> keys(num_keys);
  std::iota(keys.begin(), keys.end(), 0);

  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size};

  constexpr int val_offset = 13;

  for (const auto key : keys)
  {
    multi_queue.push(key, key + val_offset - 1);
  }

  ASSERT_TRUE(multi_queue.hasAny());

  // pop dummy values
  multi_queue.popReady();

  ASSERT_FALSE(multi_queue.hasAny());
}

TEST(MultiLockQueue, peekAll)
{
  constexpr size_t num_keys = 10;
  std::vector<int> keys(num_keys);
  std::iota(keys.begin(), keys.end(), 0);

  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size, keys};

  constexpr int val_offset = 13;

  std::vector<std::future<void>> workers;

  std::promise<void> unblock;
  const auto blocker = unblock.get_future().share();

  for (const auto key : keys)
  {
    workers.push_back(
        std::async(
            std::launch::async,
            [key, &blocker, &multi_queue]()
            {
              blocker.wait();
              multi_queue.push(key, key + val_offset);
            }
        )
    );
  }

  auto future_values = std::async(std::launch::async, [&multi_queue]{ return multi_queue.peekAll(); });
  ASSERT_EQ(future_values.wait_for(10ms), std::future_status::timeout);

  unblock.set_value();
  const auto values = future_values.get();
  ASSERT_EQ(values.size(), workers.size());

  for (const auto& kv : values)
  {
    const auto key = kv.first;
    const auto val = kv.second;

    ASSERT_EQ(val, key + val_offset);
  }

  ASSERT_TRUE(multi_queue.hasAll());
}

TEST(MultiLockQueue, peekAtLeastOne)
{
  constexpr size_t num_keys = 10;
  std::vector<int> keys(num_keys);
  std::iota(keys.begin(), keys.end(), 0);

  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size, keys};

  constexpr int val_offset = 13;

  std::promise<void> unblock;
  const auto blocker = unblock.get_future().share();

  const auto worker = std::async(
      std::launch::async,
      [keys, &blocker, &multi_queue]()
      {
        blocker.wait();
        multi_queue.push(keys.front(), keys.front() + val_offset);
      }
  );

  auto future_values = std::async(std::launch::async, [&multi_queue]{ return multi_queue.peekAtLeastOne(); });
  ASSERT_EQ(future_values.wait_for(10ms), std::future_status::timeout);
  unblock.set_value();

  const auto values = future_values.get();
  ASSERT_EQ(values.size(), 1);

  for (const auto& kv : values)
  {
    const auto key = kv.first;
    const auto val = kv.second;

    ASSERT_EQ(val, key + val_offset);
  }

  ASSERT_TRUE(multi_queue.hasAny());
}

TEST(MultiLockQueue, peekReady)
{
  constexpr size_t num_keys = 10;
  std::vector<int> keys(num_keys);
  std::iota(keys.begin(), keys.end(), 0);

  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size, keys};

  constexpr int val_offset = 13;

  {
    const auto values = multi_queue.peekReady();
    ASSERT_TRUE(values.empty());
  }

  constexpr int num_new = 4;

  for (int key = 0; key < num_new; ++key)
  {
    multi_queue.push(key, key + val_offset);
  }

  const auto values = multi_queue.peekReady();

  ASSERT_EQ(values.size(), num_new);

  for (const auto& kv : values)
  {
    const auto key = kv.first;
    const auto val = kv.second;

    ASSERT_EQ(val, key + val_offset);
  }

  ASSERT_TRUE(multi_queue.hasAny());
}

TEST(MultiLockQueue, ClearClearsAllQueues)
{
  constexpr size_t num_keys = 10;
  std::vector<int> keys(num_keys);
  std::iota(keys.begin(), keys.end(), 0);

  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size};

  constexpr int val_offset = 13;

  for (const auto key : keys)
  {
    multi_queue.push(key, key + val_offset - 1);
  }

  ASSERT_TRUE(multi_queue.hasAll());
  ASSERT_EQ(multi_queue.peekAll().size(), num_keys);

  multi_queue.clear();

  ASSERT_FALSE(multi_queue.hasAny());
  ASSERT_EQ(multi_queue.peekReady().size(), 0);
}

TEST(MultiLockQueue, DontBlockWhenAllQueuesRemoved)
{
  constexpr size_t num_keys = 10;
  std::vector<int> keys(num_keys);
  std::iota(keys.begin(), keys.end(), 0);

  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size, keys};

  multi_queue.removeAllQueues();
  ASSERT_TRUE(multi_queue.hasAll());

  // should not block
  multi_queue.popAll();
}

TEST(MultiLockQueue, DontBlockForRemovedQueues)
{
  constexpr size_t num_keys = 10;
  std::vector<int> keys(num_keys);
  std::iota(keys.begin(), keys.end(), 0);

  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size, keys};

  constexpr size_t num_remove_queues = 3;

  for (const auto key : keys)
  {
    if (static_cast<size_t>(key) < num_remove_queues)
    {
      multi_queue.removeQueue(key);
    } else
    {
      multi_queue.push(key, key) ;
    }
  }

  ASSERT_TRUE(multi_queue.hasAll());

  const auto values = multi_queue.peekAll();
  ASSERT_EQ(values.size(), static_cast<size_t>(num_keys - num_remove_queues));

  for (const auto kv : values)
  {
    const auto key = kv.first;

    ASSERT_GE(key, num_remove_queues);
  }
}

TEST(MultiLockQueue, addQueue)
{
  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size};

  ASSERT_TRUE(multi_queue.hasAll());

  multi_queue.addQueue(0);

  ASSERT_FALSE(multi_queue.hasAll());
}

TEST(MultiLockQueue, terminateThrowsOnPopAndPeek)
{
  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size};

  ASSERT_FALSE(multi_queue.isTerminated());

  multi_queue.terminate();

  ASSERT_TRUE(multi_queue.isTerminated());

  ASSERT_THROW(multi_queue.peekReady(), TerminatedException);
  ASSERT_THROW(multi_queue.peekAtLeastOne(), TerminatedException);
  ASSERT_THROW(multi_queue.peekAll(), TerminatedException);

  ASSERT_THROW(multi_queue.popReady(), TerminatedException);
  ASSERT_THROW(multi_queue.popAtLeastOne(), TerminatedException);
  ASSERT_THROW(multi_queue.popAll(), TerminatedException);
}

TEST(MultiLockQueue, QueuesRespectMaxQueueSize)
{
  constexpr size_t num_keys = 10;
  std::vector<int> keys(num_keys);
  std::iota(keys.begin(), keys.end(), 0);

  constexpr size_t queue_size = 4;
  MultiLockQueue<int, int> multi_queue{queue_size, keys};

  for (size_t i = 0; i < queue_size + 1; ++i)
  {
    for (const auto key : keys)
    {
      multi_queue.push(key, 0);
    }
  }

  for (size_t i = 0; i < queue_size; ++i)
  {
    ASSERT_TRUE(multi_queue.hasAll());

    const auto values = multi_queue.popAll();

    ASSERT_EQ(values.size(), num_keys);
  }

  ASSERT_FALSE(multi_queue.hasAny());
}

TEST(MultiLockQueue, AddExistingQueueDoesNotClear)
{
  const std::vector<int> keys = {0};

  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size, keys};

  multi_queue.push(keys.front(), 0);

  ASSERT_TRUE(multi_queue.hasAll());

  multi_queue.addQueue(keys.front());

  ASSERT_TRUE(multi_queue.hasAll());
}

TEST(MultiLockQueue, RemoveNonExistingQueueDoesNothing)
{
  const std::vector<int> keys = {0};

  constexpr size_t queue_size = 1;
  MultiLockQueue<int, int> multi_queue{queue_size, keys};

  multi_queue.push(keys.front(), 0);

  ASSERT_TRUE(multi_queue.hasAll());

  constexpr int non_existing_key = 42;
  ASSERT_NO_THROW(multi_queue.removeQueue(non_existing_key));

  ASSERT_TRUE(multi_queue.hasAll());
}
