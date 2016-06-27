/* Copyright (c) 2015-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_FUNCTIONAL_HPP
#define XBT_FUNCTIONAL_HPP

#include <cstddef>
#include <cstdlib>

#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <xbt/sysdep.h>
#include <xbt/utility.hpp>

namespace simgrid {
namespace xbt {

template<class F>
class MainFunction {
private:
  F code_;
  std::shared_ptr<const std::vector<std::string>> args_;
public:
  MainFunction(F code, std::vector<std::string> args) :
    code_(std::move(code)),
    args_(std::make_shared<const std::vector<std::string>>(std::move(args)))
  {}
  int operator()() const
  {
    const int argc = args_->size();
    std::vector<std::string> args = *args_;
    std::unique_ptr<char*[]> argv(new char*[argc + 1]);
    for (int i = 0; i != argc; ++i)
      argv[i] = args[i].empty() ? const_cast<char*>(""): &args[i].front();
    argv[argc] = nullptr;
    return code_(argc, argv.get());
  }
};

template<class F> inline
std::function<void()> wrapMain(F code, std::vector<std::string> args)
{
  return MainFunction<F>(std::move(code), std::move(args));
}

template<class F> inline
std::function<void()> wrapMain(F code, int argc, const char*const argv[])
{
  std::vector<std::string> args(argv, argv + argc);
  return MainFunction<F>(std::move(code), std::move(args));
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

template<class T> class Task;

/** Type-erased run-once task
 *
 *  * Like std::function but callable only once.
 *    However, it works with move-only types.
 *
 *  * Like std::packaged_task<> but without the shared state.
 */
template<class R, class... Args>
class Task<R(Args...)> {
private:
  // Type-erasure for the code:
  class Base {
  public:
    virtual ~Base() {}
    virtual R operator()(Args...) = 0;
  };
  template<class F>
  class Impl : public Base {
  public:
    Impl(F&& code) : code_(std::move(code)) {}
    Impl(F const& code) : code_(code) {}
    ~Impl() override {}
    R operator()(Args... args) override
    {
      return code_(std::forward<Args>(args)...);
    }
  private:
    F code_;
  };
  std::unique_ptr<Base> code_;
public:
  Task() {}
  Task(std::nullptr_t) {}

  template<class F>
  Task(F&& code) :
    code_(new Impl<F>(std::forward<F>(code))) {}

  operator bool() const { return code_ != nullptr; }
  bool operator!() const { return code_ == nullptr; }

  template<class... OtherArgs>
  R operator()(OtherArgs&&... args)
  {
    std::unique_ptr<Base> code = std::move(code_);
    return (*code)(std::forward<OtherArgs>(args)...);
  }
};

template<class F, class... Args>
class TaskImpl {
private:
  F code_;
  std::tuple<Args...> args_;
  typedef decltype(simgrid::xbt::apply(std::move(code_), std::move(args_))) result_type;
public:
  TaskImpl(F code, std::tuple<Args...> args) :
    code_(std::move(code)),
    args_(std::move(args))
  {}
  result_type operator()()
  {
    return simgrid::xbt::apply(std::move(code_), std::move(args_));
  }
};

template<class F, class... Args>
auto makeTask(F code, Args... args)
-> Task< decltype(code(std::move(args)...))() >
{
  TaskImpl<F, Args...> task(std::move(code), std::make_tuple(std::move(args)...));
  return std::move(task);
}

}
}

#endif
