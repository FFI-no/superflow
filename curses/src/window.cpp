// Copyright 2019, Forsvarets forskningsinstitutt. All rights reserved.
#include "superflow/curses/window.h"
#include "superflow/utils/pimpl_impl.h"
#include "ncursescpp/ncursescpp.hpp"
#include <iomanip>
#include <sstream>

// https://stackoverflow.com/a/2351155
// You can use explicit instantiation to create an instantiation of
// a templated class or function without actually using it in your code.
// Because this is useful when you are creating library files that
// use templates for distribution, uninstantiated template definitions
// are not put into object files.
template class flow::pimpl<flow::curses::Window::impl>;

namespace flow::curses
{
class Window::impl
{
public:
  impl(int x, int y, int width, int height);

  static constexpr int content_col_offset = 1;
  static constexpr int header_col_offset = 2;

  int width_;
  int height_;
  nccpp::Window window_;

  void setColor(const nccpp::Color& color);

  void renderBox(int background_color);

  void renderName(const std::string& name);

  void renderLine(const std::string& line, int row_offset);

  [[nodiscard]] std::string shortenString(
      const std::string& str,
      int col_offset,
      bool pad = false
  ) const;
};

Window::Window(const int x, const int y, const int width, const int height)
    : m_{x, y, width, height}
{}

void Window::render(
    const std::string& name,
    const std::vector<std::string>& lines,
    const nccpp::Color& color
)
{
  m_->setColor(color);
  m_->renderBox(color.background);
  m_->renderName(name);

  for (size_t i = 0; i < lines.size(); ++i)
  {
    const int row_offset = 1 + static_cast<int>(i);

    m_->renderLine(lines[i], row_offset);
  }

  for (auto i = static_cast<int>(lines.size()); i < m_->height_; ++i)
  {
    const int row_offset = 1 + i;

    m_->renderLine("", row_offset);
  }

  m_->window_.refresh();
}

Window::impl::impl(const int x, const int y, const int width, const int height)
    : width_{width}
    , height_{height}
    , window_{height + 2, width, y, x}
{}

void Window::impl::setColor(const nccpp::Color& color)
{
  const auto attr = static_cast<int>(nccpp::ncurses().color_to_attr(color));

  window_.bkgd(attr);
  window_.attron(attr);
}

void Window::impl::renderBox(const int)
{
  window_.box('|', '-');
}

void Window::impl::renderName(const std::string& name)
{
  const std::string header = shortenString(name, header_col_offset);

  window_.mvprintw(0, header_col_offset, header.c_str());
}

void Window::impl::renderLine(const std::string& line, int row_offset)
{
  const std::string content = shortenString(line, content_col_offset, true);

  window_.mvprintw(row_offset, content_col_offset, content.c_str());
}

std::string Window::impl::shortenString(
    const std::string& str,
    const int col_offset,
    const bool pad) const
{
  const auto width = static_cast<size_t>(width_ - 2 * col_offset);
  std::ostringstream ss;

  if (str.length() <= width)
  {
    if (!pad || str.length() == width)
    {
      return str;
    }

    ss << str
       << std::setfill(' ') << std::setw(static_cast<int>(width - str.length())) << "";
  } else
  {
    const auto first_half_width = static_cast<size_t>(width / 2);
    const auto last_half_width = width - first_half_width;

    ss << str.substr(0, first_half_width)
       << str.substr(str.length() - last_half_width, last_half_width);
  }

  return ss.str();
}
}
