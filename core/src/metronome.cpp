#include "superflow/utils/metronome.h"

namespace flow
{
using Clock = std::chrono::steady_clock;

Metronome::Metronome(
  const Functional& func,
  const Duration period
)
  : has_stopped_{false}
{
  const auto first_time_point = Clock::now();

  worker_ = std::async(
    std::launch::async,
    [this, func, period, first_time_point]()
    {
      auto time_point = first_time_point;

      while (!has_stopped_)
      {
        time_point += period;

        {
          std::unique_lock lock{mutex_};
          const bool has_stopped = cv_.wait_until(lock, time_point,
            [this]{ return has_stopped_; }
          );
          if (has_stopped)
          { break; }
        }

        const auto duration_since_begin = Clock::now() - first_time_point;

        func(duration_since_begin);
      }
    }
  );
}

Metronome::~Metronome()
{
  stop();
}

void Metronome::get()
{
  if (worker_.valid())
  {
    worker_.get();
  }
  else
  {
    throw std::runtime_error{"Cannot call get() on Metronome with invalid state"};
  }
}

void Metronome::check()
{
  const auto state = worker_.wait_for(std::chrono::seconds{0});

  if (state != std::future_status::timeout)
  {
    get();
  }
}

void Metronome::stop() noexcept
{
  {
    std::scoped_lock lock{mutex_};

    if (has_stopped_)
    { return; }

    has_stopped_ = true;
  }

  cv_.notify_one();
}
}
