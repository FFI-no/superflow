// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/curses/window.h"
#include "superflow/proxel_status.h"

namespace flow::curses
{
class ProxelWindow
{
public:
  ProxelWindow();

  ProxelWindow(
      int x,
      int y,
      int width,
      size_t max_ports_shown = 0
  );

  void renderStatus(
      const std::string& name,
      const ProxelStatus& status
  );

  static int getHeight();

  static constexpr int port_window_width = 10;
  static constexpr int port_window_height = 2;

private:
  static constexpr int inner_height = 6;

  int x_;
  int y_;
  int width_;
  size_t max_ports_shown_;

  Window window_;
  std::map<std::string, Window> port_windows_;

  static std::vector<std::string> getLines(const std::string& str, size_t max_line_length);

  static std::vector<std::string> split(const std::string& str, char sep);

  static std::vector<std::string> split(const std::string& str, size_t chunk_size);

  static nccpp::Color getStateColor(ProxelStatus::State state);
};
}
