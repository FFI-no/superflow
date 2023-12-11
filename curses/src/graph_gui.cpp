// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/curses/graph_gui.h"

#include "superflow/utils/signal_waiter.h"

#include "ncursescpp/ncursescpp.hpp"

#include <chrono>
#include <thread>

namespace flow::curses
{
namespace
{
[[nodiscard]] std::map<std::string, Proxel::Ptr> getProxelSet(
  const Graph& graph,
  const std::map<std::string, ProxelStatus>& non_blacklisted_statuses
);

/// \brief Returns the status for all proxels that are not blacklisted, and _all_ crashed proxels
/// (even if they are blacklisted)
std::map<std::string, ProxelStatus> getProxelStatuses(
  const Graph& graph,
  const std::unordered_set<std::string>& blacklisted_proxels
);
}

GraphGUI::GraphGUI(const size_t max_ports_shown)
  : max_ports_shown_{max_ports_shown}
{}

void GraphGUI::spin(
  const Graph& graph,
  const std::unordered_set<std::string>& blacklisted_proxels
)
{
  nccpp::ncurses().nodelay(true);
  SignalWaiter waiter{{SIGINT, SIGTERM}};

  while (nccpp::ncurses().getch() != 'q' && !waiter.hasGottenSignal())
  {
    spinOnce(graph, blacklisted_proxels);
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }
}

void GraphGUI::spinOnce(
  const Graph& graph,
  const std::unordered_set<std::string>& blacklisted_proxels
)
{
  const auto statuses = getProxelStatuses(graph, blacklisted_proxels);
  const auto proxels = getProxelSet(graph, statuses);

  const auto minimum_window_width = getMinimumWindowWidth(static_cast<int>(getMaxNumPorts(statuses)));

  nccpp::ncurses().cbreak(true);
  nccpp::ncurses().echo(false);
  curs_set(0);

  if (proxelSetHasChanged(proxels)
      || guiSizeHasChanged()
      || minimum_window_width != minimum_window_width_)
  {
    last_proxels_ = proxels;
    minimum_window_width_ = minimum_window_width;
    nccpp::ncurses().get_maxyx(height_, width_);

    // clear all windows
    nccpp::ncurses().clear();
    nccpp::ncurses().start_color();
    nccpp::ncurses().bkgd(
      static_cast<int>(nccpp::ncurses().color_to_attr(
        {
          nccpp::colors::white,
          nccpp::colors::black
        }
      ))
    );

    windows_ = createWindows(
      proxels,
      width_,
      height_,
      minimum_window_width_,
      max_ports_shown_
    );
  }

  for (const auto& kv : statuses)
  {
    const std::string& name = kv.first;
    const ProxelStatus& status = kv.second;

    windows_[name].renderStatus(name, status);
  }

  nccpp::ncurses().move(height_ - 1, width_ - 1);
  nccpp::ncurses().refresh();
}

bool GraphGUI::guiSizeHasChanged() const
{
  int width;
  int height;

  nccpp::ncurses().get_maxyx(height, width);

  return width != width_ || height != height_;
}

bool GraphGUI::proxelSetHasChanged(const ProxelSet& proxels) const
{
  if (last_proxels_.size() != proxels.size())
  {
    return true;
  }

  auto old_it = last_proxels_.begin();
  auto new_it = proxels.begin();

  while (old_it != last_proxels_.end() && new_it != proxels.end())
  {
    if (old_it->first != new_it->first
        || old_it->second != new_it->second)
    {
      return true;
    }

    ++old_it;
    ++new_it;
  }

  return false;
}

GraphGUI::WindowSet GraphGUI::createWindows(
    const ProxelSet& proxels,
    const int width,
    const int height,
    const int minimum_window_width,
    const size_t max_ports_shown
)
{
  const auto num_proxels = static_cast<int>(proxels.size());
  const int num_cols = getNumCols(num_proxels, width, height, minimum_window_width);
  const int num_rows = (num_proxels + num_cols - 1) / num_cols;
  const int window_width = getWindowWidth(num_cols, width, height);
  const int window_height = ProxelWindow::getHeight();

  WindowSet windows;

  auto it = proxels.begin();

  for (int i = 0; i < num_rows; ++i)
  {
    for (int j = 0; j < num_cols; ++j)
    {
      if (it == proxels.end())
      {
        break;
      }

      const int x = (j + 1) * window_h_padding + j * window_width;
      const int y = (i + 1) * window_v_padding + i * window_height;

      ProxelWindow window{
          x,
          y,
          window_width,
          max_ports_shown
      };

      const std::string& name = it->first;

      windows[name] = std::move(window);
      ++it;
    }
  }

  return windows;
}

size_t GraphGUI::getMaxNumPorts(const StatusSet& statuses) const
{
  size_t max_num_ports = 0;

  for (const auto& kv : statuses)
  {
    max_num_ports = std::max(max_num_ports, kv.second.ports.size());
  }

  if (max_ports_shown_ > 0)
  {
    max_num_ports = std::min(max_num_ports, max_ports_shown_);
  }

  return max_num_ports;
}

int GraphGUI::getMinimumWindowWidth(const int max_num_ports)
{
  return std::max(
      2 + max_num_ports * ProxelWindow::port_window_width,
      20
  );
}

int GraphGUI::getWindowWidth(
    const int num_cols,
    const int width,
    const int)
{
  return (width - (num_cols + 1) * window_h_padding) / num_cols;
}

int GraphGUI::getNumCols(
    const int num_proxels,
    const int width,
    const int height,
    const int minimum_window_width
)
{
  for (int num_cols = num_proxels; num_cols > 1; --num_cols)
  {
    if (getWindowWidth(num_cols, width, height) >= minimum_window_width)
    {
      return num_cols;
    }
  }

  return 1;
}

namespace
{
std::map<std::string, Proxel::Ptr> getProxelSet(
  const Graph& graph,
  const std::map<std::string, ProxelStatus>& non_blacklisted_statuses
)
{
  std::map<std::string, Proxel::Ptr> proxels;

  for (const auto& kv : non_blacklisted_statuses)
  {
    const std::string& name = kv.first;
    proxels[name] = graph.getProxel(name);
  }

  return proxels;
}

std::map<std::string, ProxelStatus> getProxelStatuses(
  const Graph& graph,
  const std::unordered_set<std::string>& blacklisted_proxels
)
{
  auto statuses = graph.getProxelStatuses();

  for (auto it = statuses.begin(); it != statuses.end();)
  {
    const bool is_blacklisted = blacklisted_proxels.find(it->first) != blacklisted_proxels.end();
    const bool is_crashed = it->second.state == ProxelStatus::State::Crashed;

    if (!is_crashed && is_blacklisted)
    {
      it = statuses.erase(it);
    }
    else
    {
      ++it;
    }
  }

  return statuses;
}
}
}
