// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "testing/dummy_value_adapter.h"

#include "superflow/loader/proxel_library.h"
#include "proxels/lib_path.h"
#include "proxels/fauxade.h"

#include "gtest/gtest.h"

using namespace flow;

TEST(ProxelLibrary, rtld_now_missing_dependency)
{
  /// libdependent_library.so depends on libmissing_dependency.so, but libmissing_dependency.so is not on the linker path.
  /// rtld_now should cause the loading of libdependent_library.so to fail.
  EXPECT_THROW(
    std::ignore = load::ProxelLibrary(flow::test::local::lib_dir, "dependent_library"),
    std::invalid_argument
  );
}

TEST(ProxelLibrary, rtld_now_dependencies_resolved)
{
  /// libmissing_dependency.so has no dependencies that are not on the default linker path.
  /// It should thus load just fine.
  EXPECT_NO_THROW(
    std::ignore = load::ProxelLibrary(flow::test::local::lib_dir, "missing_dependency")
  );
}
