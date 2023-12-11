#include "testing/dummy_value_adapter.h"

namespace flow::test
{
  bool DummyValueAdapter::hasKey(const std::string& key) const
  { return key == key_; }
}
