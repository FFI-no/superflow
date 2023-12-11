// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/policy.h"
#include "superflow/utils/lock_queue.h"
#include <optional>

namespace flow
{
template<typename T, GetMode M, LeakPolicy L>
struct QueueGetter
{
  static_assert(M == GetMode::Blocking || M == GetMode::Latched, "Selected GetMode is not available for this Port");
};

template<typename T, LeakPolicy L>
struct QueueGetter<T, GetMode::Blocking, L>
{
  static std::optional<T> get(LockQueue<T, L>& queue)
  {
    try
    {
      return queue.pop();
    }
    catch (const TerminatedException&)
    { return std::nullopt; }
  }

  static bool hasNext(const LockQueue<T>& queue)
  {
    return !queue.isEmpty();
  }

  void clear()
  { /* noop */ }
};

template<typename T, LeakPolicy L>
struct QueueGetter<T, GetMode::Latched, L>
{
  std::optional<T> get(LockQueue<T, L>& queue)
  {
    try
    {
      if (!opt.has_value() || !queue.isEmpty())
      { opt = queue.pop(); }

      return opt;
    }
    catch (const TerminatedException&)
    { return std::nullopt; }
  }

  bool hasNext(const LockQueue<T>& queue) const
  { return opt.has_value() || !queue.isEmpty(); }

  void clear()
  {
    opt = std::nullopt;
  }

private:
  std::optional<T> opt;
};
}
