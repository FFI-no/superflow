// Copyright (c) 2020 Forsvarets forskningsinstitutt (FFI). All rights reserved.
// https://herbsutter.com/gotw/_101/

#pragma once

#include <utility>

namespace flow
{
template<typename T>
template<typename ...Args>
pimpl<T>::pimpl(Args&& ...args)
    : m{std::make_unique<T>(std::forward<Args>(args)...)}
{}

template<typename T>
pimpl<T>::~pimpl() = default;

template<typename T>
pimpl <T>& pimpl<T>::operator=(pimpl&&) noexcept = default;

template<typename T>
T* pimpl<T>::operator->() const
{ return m.get(); }

template<typename T>
T& pimpl<T>::operator*() const
{ return *m.get(); }
}