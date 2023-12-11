// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/utils/pimpl_h.h"

#include <string>
#include <vector>

namespace nccpp
{ struct Color; }

namespace flow::curses
{
class Window
{
public:
  Window(
      int x,
      int y,
      int width,
      int height
  );

  void render(
      const std::string& name,
      const std::vector<std::string>& lines,
      const nccpp::Color& color
  );

private:
  class impl;
  pimpl<impl> m_;
};
}
