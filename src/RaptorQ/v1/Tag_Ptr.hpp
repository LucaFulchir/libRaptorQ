/*
  Copyright (c) 2016, Matteo Bertello
  All rights reserved.

  Email:   bertello.matteo@gmail.com
  GitHub:  http://github.com/Corralx/Tag_Ptr
  Website: http://corralx.me
  Twitter: http://twitter.com/corralx

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  * The names of its contributors may not be used to endorse or promote
    products derived from this software without specific prior written
    permission.
*/

#pragma once

#include "common.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>


namespace {

template<size_t V>
struct log2
{
    enum { value = log2<V / 2>::value + 1 };
};

template<>
struct log2<1>
{
    enum { value = 0 };
};

}

namespace RaptorQ__v1 {
namespace Impl {

// Before use:
// FIXME: delete problem: overload delete?
// FIXME: variadic template constructor
// FIXME: get() returns T&
// FIXME: conversion to pointer operator?

template<typename T>
class RAPTORQ_LOCAL Tag_Ptr
{
    using pointer = T*;
    using element_type = T;

public:
    Tag_Ptr() : _ptr(nullptr) {}
    explicit Tag_Ptr(pointer ptr, uint8_t value = 0) : _ptr(ptr) { tag(value); }
    Tag_Ptr(const Tag_Ptr& o) : _ptr(o._ptr) {}
    ~Tag_Ptr() = default;

    Tag_Ptr& operator=(const Tag_Ptr& o)
    {
        _ptr = o._ptr;
        return *this;
    }

#pragma warning (disable: 4800)
    operator bool() const
    {
        return static_cast<bool>(_ptr_bits & ~static_cast<uintptr_t>(tag_mask));
    }
#pragma warning (default: 4800)

    T* get() const
    {
        return reinterpret_cast<pointer>(_ptr_bits & ~static_cast<uintptr_t> (tag_mask));
    }

    void reset(pointer p = nullptr)
    {
        _ptr = p;
    }

    uint8_t tag() const
    {
        return static_cast<uint8_t>(_ptr_bits & static_cast<uintptr_t>(tag_mask));
    }

    void tag(uint8_t value)
    {
        assert((value & tag_mask) == value);
        _ptr_bits = reinterpret_cast<uintptr_t>(get()) | static_cast<uintptr_t>(value & tag_mask);
    }

    void swap(Tag_Ptr& o)
    {
        pointer tmp = _ptr;
        o._ptr = _ptr;
        _ptr = tmp;
    }

    element_type& operator*() const { return *get(); }
    pointer operator->() const { return get(); }

    static constexpr uint8_t tag_bits = log2<alignof(element_type)>::value;
    static constexpr uint8_t tag_mask = alignof(element_type) - static_cast<uint8_t>(1);

private:
    union
    {
        pointer _ptr;
        uintptr_t _ptr_bits;
    };
};

template<typename T, typename... Args>
Tag_Ptr<T> make_tag(Args&&... args)
{
    return Tag_Ptr<T>(std::forward<Args>(args)...);
}

template<typename T>
std::ostream &operator<<(std::ostream& os, Tag_Ptr<T> ptr)
{
    return os << ptr.get();
}

template<typename T>
void swap(Tag_Ptr<T>& p1, Tag_Ptr<T>& p2)
{
    p1.swap(p2);
}

template<typename T, typename U>
bool operator==(const Tag_Ptr<T> lhs, const Tag_Ptr<U> rhs)
{
    return lhs.get() == rhs.get();
}

template<typename T, typename U>
bool operator!=(const Tag_Ptr<T> lhs, const Tag_Ptr<U> rhs)
{
    return lhs.get() != rhs.get();
}

template<typename T, typename U>
bool operator<(const Tag_Ptr<T> lhs, const Tag_Ptr<U> rhs)
{
    return lhs.get() < rhs.get();
}

template<typename T, typename U>
bool operator>(const Tag_Ptr<T> lhs, const Tag_Ptr<U> rhs)
{
    return lhs.get() > rhs.get();
}

template<typename T, typename U>
bool operator<=(const Tag_Ptr<T> lhs, const Tag_Ptr<U> rhs)
{
    return lhs.get() <= rhs.get();
}

template<typename T, typename U>
bool operator>=(const Tag_Ptr<T> lhs, const Tag_Ptr<U> rhs)
{
    return lhs.get() >= rhs.get();
}

} // namespace Impl
} // namespace RaptorQ__v1
