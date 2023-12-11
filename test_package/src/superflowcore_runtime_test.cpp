#include "superflow/utils/metronome.h"
#include <chrono>

int main()
{
  flow::Metronome metronome{[](const auto) {}, std::chrono::microseconds{1}};
}
