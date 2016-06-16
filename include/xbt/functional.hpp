/* Copyright (c) 2015-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_FUNCTIONAL_HPP
#define XBT_FUNCTIONAL_HPP

#include <cstdlib>

#include <exception>
#include <functional>
#include <future>
#include <utility>

#include <xbt/sysdep.h>
#include <xbt/utility.hpp>

namespace simgrid {
namespace xbt {

class args {
private:
  int argc_ = 0;
  char** argv_ = nullptr;
public:

  // Main constructors
  args() {}

  void assign(int argc, const char*const* argv)
  {
    clear();
    char** new_argv = xbt_new(char*,argc + 1);
    for (int i = 0; i < argc; i++)
      new_argv[i] = xbt_strdup(argv[i]);
    new_argv[argc] = nullptr;
    this->argc_ = argc;
    this->argv_ = new_argv;
  }
  args(int argc, const char*const* argv)
  {
    this->assign(argc, argv);
  }

  char** to_argv() const
  {
    const int argc = argc_;
    char** argv = xbt_new(char*, argc + 1);
    for (int i=0; i< argc; i++)
      argv[i] = xbt_strdup(argv_[i]);
    argv[argc] = nullptr;
    return argv;
  }

  // Free
  void clear()
  {
    for (int i = 0; i < this->argc_; i++)
      std::free(this->argv_[i]);
    std::free(this->argv_);
    this->argc_ = 0;
    this->argv_ = nullptr;
  }
  ~args() { clear(); }

  // Copy
  args(args const& that)
  {
    this->assign(that.argc(), that.argv());
  }
  args& operator=(args const& that)
  {
    this->assign(that.argc(), that.argv());
    return *this;
  }

  // Move:
  args(args&& that) : argc_(that.argc_), argv_(that.argv_)
  {
    that.argc_ = 0;
    that.argv_ = nullptr;
  }
  args& operator=(args&& that)
  {
    this->argc_ = that.argc_;
    this->argv_ = that.argv_;
    that.argc_ = 0;
    that.argv_ = nullptr;
    return *this;
  }

  int    argc()            const { return argc_; }
  char** argv()                  { return argv_; }
  const char*const* argv() const { return argv_; }
  char* operator[](std::size_t i) { return argv_[i]; }
};

template<class F> inline
std::function<void()> wrapMain(F code, std::shared_ptr<simgrid::xbt::args> args)
{
  return [=]() {
    code(args->argc(), args->argv());
  };
}

template<class F> inline
std::function<void()> wrapMain(F code, simgrid::xbt::args args)
{
  return wrapMain(std::move(code),
    std::unique_ptr<simgrid::xbt::args>(new simgrid::xbt::args(std::move(args))));
}

template<class F> inline
std::function<void()> wrapMain(F code, int argc, const char*const* argv)
{
  return wrapMain(std::move(code), args(argc, argv));
}

namespace bits {
template <class F, class Tuple, std::size_t... I>
constexpr auto apply(F&& f, Tuple&& t, simgrid::xbt::index_sequence<I...>)
  -> decltype(std::forward<F>(f)(std::get<I>(std::forward<Tuple>(t))...))
{
  return std::forward<F>(f)(std::get<I>(std::forward<Tuple>(t))...);
}
}

/** Call a functional object with the values in the given tuple (from C++17)
 *
 *  @code{.cpp}
 *  int foo(int a, bool b);
 *
 *  auto args = std::make_tuple(1, false);
 *  int res = apply(foo, args);
 *  @encode
 **/
template <class F, class Tuple>
constexpr auto apply(F&& f, Tuple&& t)
  -> decltype(simgrid::xbt::bits::apply(
    std::forward<F>(f),
    std::forward<Tuple>(t),
    simgrid::xbt::make_index_sequence<
      std::tuple_size<typename std::decay<Tuple>::type>::value
    >()))
{
  return simgrid::xbt::bits::apply(
    std::forward<F>(f),
    std::forward<Tuple>(t),
    simgrid::xbt::make_index_sequence<
      std::tuple_size<typename std::decay<Tuple>::type>::value
    >());
}

}
}

#endif
