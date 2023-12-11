// Copyright (c) 2020 Forsvarets forskningsinstitutt (FFI). All rights reserved.
// https://herbsutter.com/gotw/_101/

#pragma once

#include <memory>

namespace flow
{
/// \brief Helper class for the pimpl pattern.
/// The code is taken from Herb Sutter's
/// <a href="https://herbsutter.com/gotw/_101/">GotW#101</a>, with slight modifications.
/// Read <a href="https://herbsutter.com/gotw/_100/">GotW#100</a>for good practices and more about the Pimpl Idiom.
/// <br>The \b pimpl_h.h file should be included from the header file of the class owning the pimpl object.
/// <br>The \b pimpl_impl.h sould be included from the source file (cpp). Remember the explicit template instantiation!
///
/// \b my_class.h
/// \code{.cpp}
/// #pragma once
/// #include "superflow/utils/pimpl_h.h"
/// class MyClass
/// {
///   // ...
/// private:
///   class impl;     // forward declare
///   pimpl<impl> m_; // instead of std::unique_ptr<impl>
///   // ...
/// };
/// \endcode
/// \b my_class.cpp
/// \code{.cpp}
/// #include "mylib/my_class.h"
/// #include "superflow/utils/pimpl_impl.h"
///
/// // Easy to forget, but strictly required for the code to compile
/// template class flow::pimpl<MyClass::impl>;
///
/// // The impl.
/// class MyClass::impl
/// {
///   impl(impl ctor arguments);
///   void func();
/// };
///
/// MyClass::MyClass(ctor args)
///   : m_{impl ctor arguments} // instead of std::make_unique<impl>
/// {
///   m_->func(); // Access the impl
/// }
/// \endcode
/// \tparam T The (typically private inner) class to be pimp'ed.
/// \see https://herbsutter.com/gotw/_100/
/// \see https://herbsutter.com/gotw/_101/
/// \see https://stackoverflow.com/questions/8595471/does-the-gotw-101-solution-actually-solve-anything
template<typename T>
class pimpl
{
private:
  std::unique_ptr<T> m;
public:

  template<typename ...Args>
  pimpl(Args&& ...);

  ~pimpl();

  pimpl& operator=(pimpl&&) noexcept;

  T* operator->() const;

  T& operator*() const;
};
}