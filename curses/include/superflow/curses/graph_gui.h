// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#pragma once

#include "superflow/curses/proxel_window.h"
#include "superflow/graph.h"

#include <map>
#include <string>
#include <unordered_set>

namespace flow::curses
{
class GraphGUI
{
public:
  explicit GraphGUI(size_t max_ports_shown = 0);

  void spin(
    const Graph& graph,
    const std::unordered_set<std::string>& blacklisted_proxels = {}
  );

  void spinOnce(
    const Graph& graph,
    const std::unordered_set<std::string>& blacklisted_proxels = {}
  );

private:
  using ProxelSet = std::map<std::string, Proxel::Ptr>;
  using StatusSet = std::map<std::string, ProxelStatus>;
  using WindowSet = std::map<std::string, ProxelWindow>;

  static constexpr int window_h_padding = 2;
  static constexpr int window_v_padding = 4;

  ProxelSet last_proxels_;
  WindowSet windows_;

  size_t max_ports_shown_;
  int width_ = -1;
  int height_ = -1;
  int minimum_window_width_ = 0;

  [[nodiscard]] bool guiSizeHasChanged() const;

  [[nodiscard]] bool proxelSetHasChanged(const ProxelSet& proxels) const;

  static WindowSet createWindows(
      const ProxelSet& proxels,
      int width,
      int height,
      int minimum_window_width,
      size_t max_ports_shown
  );

  [[nodiscard]] size_t getMaxNumPorts(const StatusSet& statuses) const;

  static int getMinimumWindowWidth(int max_num_ports);

  static int getWindowWidth(int num_cols, int width, int height);

  static int getNumCols(
      int num_proxels,
      int width,
      int height,
      int minimum_window_width
  );
};
}
