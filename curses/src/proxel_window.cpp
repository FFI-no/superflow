// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/curses/proxel_window.h"
#include "ncursescpp/ncursescpp.hpp"

#include <iomanip>
#include <sstream>

namespace flow::curses
{
ProxelWindow::ProxelWindow()
  : ProxelWindow{0, 0, 0, 0}
{}

ProxelWindow::ProxelWindow(
  const int x,
  const int y,
  const int width,
  const size_t max_ports_shown
)
    : x_{x}
    , y_{y}
    , width_{width}
    , max_ports_shown_{max_ports_shown}
    , window_{x, y, width, inner_height}
{}

void ProxelWindow::renderStatus(
    const std::string& name,
    const ProxelStatus& status
)
{
  std::vector<std::string> lines;

  {
    std::ostringstream ss;
    ss << "state: " << status.state;

    lines.push_back(ss.str());
  }

  {
    const size_t max_info = 5;

    const auto max_line_length = static_cast<size_t>(width_ - 2);
    std::vector<std::string> info_lines = getLines(status.info, max_line_length);
    const size_t num_info = std::min(max_info, info_lines.size());

    std::move(
        info_lines.begin(),
        std::next(info_lines.begin(), num_info),
        std::back_inserter(lines)
    );
  }

  const auto color = getStateColor(status.state);
  window_.render(name, lines, color);

  {
    constexpr int width = 10;
    constexpr int height = 2;
    int i = 0;
    for (const auto& kv : status.ports)
    {
      if (max_ports_shown_ > 0 && i >= static_cast<int>(max_ports_shown_))
      {
        break;
      }

      Window port_window(
          x_ + i * width + 1,
          y_ + inner_height + 1,
          width,
          height);

      std::vector<std::string> port_lines;

      const PortStatus& port_status = kv.second;

      if (port_status.num_connections != PortStatus::undefined)
      {
        std::ostringstream ss;
        ss << "C:"
           << std::setfill(' ') << std::setw(width - 4)
           << port_status.num_connections;

        port_lines.push_back(ss.str());
      }

      if (port_status.num_transactions != PortStatus::undefined)
      {
        std::ostringstream ss;
        ss << "T:"
           << std::setfill(' ') << std::setw(width - 4)
           << port_status.num_transactions;

        port_lines.push_back(ss.str());
      }

      port_window.render(kv.first, port_lines, color);
      ++i;
    }
  }
}

int ProxelWindow::getHeight()
{
  return inner_height + 2;
}

std::vector<std::string> ProxelWindow::getLines(
    const std::string& str,
    const size_t max_line_length)
{
  const std::vector<std::string> raw_lines = split(str, '\n');

  std::vector<std::string> lines;

  for (const std::string& raw_line : raw_lines)
  {
    std::vector<std::string> chunks = split(raw_line, max_line_length);

    std::move(chunks.begin(), chunks.end(), std::back_inserter(lines));
  }

  return lines;
}

std::vector<std::string> ProxelWindow::split(
    const std::string& str,
    const char sep)
{
  std::istringstream ss{str};
  std::vector<std::string> lines;

  for (std::string line; std::getline(ss, line, sep);)
  {
    lines.push_back(std::move(line));
  }

  return lines;
}

std::vector<std::string> ProxelWindow::split(
    const std::string& str,
    const size_t chunk_size)
{
  std::vector<std::string> chunks;

  for (size_t i = 0; i < str.length(); i += chunk_size)
  {
    chunks.push_back(str.substr(i, chunk_size));
  }

  return chunks;
}

nccpp::Color ProxelWindow::getStateColor(const ProxelStatus::State state)
{
  using nccpp::colors::red;
  using nccpp::colors::green;
  using nccpp::colors::blue;
  using nccpp::colors::yellow;
  using nccpp::colors::black;
  using nccpp::colors::white;

  switch (state)
  {
    case ProxelStatus::State::Running:
      return {green, black};
    case ProxelStatus::State::Crashed:
      return {white, red};
    case ProxelStatus::State::AwaitingInput:
      return {yellow, black};
    case ProxelStatus::State::Paused:
      return {blue, black};
    default:
      return {white, black};
  }
}
}
