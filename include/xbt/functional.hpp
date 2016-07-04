/* Copyright (c) 2015-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_FUNCTIONAL_HPP
#define XBT_FUNCTIONAL_HPP

#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <array>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
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
  void operator()() const
  {
    const int argc = args_->size();
    std::vector<std::string> args = *args_;
    std::unique_ptr<char*[]> argv(new char*[argc + 1]);
    for (int i = 0; i != argc; ++i)
      argv[i] = args[i].empty() ? const_cast<char*>(""): &args[i].front();
    argv[argc] = nullptr;
    code_(argc, argv.get());
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

  // Placeholder for some class type:
  struct whatever {};

  // Union used for storage:
#if 0
  typedef typename std::aligned_union<0,
    void*,
    std::pair<void(*)(),void*>,
    std::pair<void(whatever::*)(), whatever*>
  >::type TaskUnion;
#else
  union TaskUnion {
    void* ptr;
    std::pair<void(*)(),void*> funcptr;
    std::pair<void(whatever::*)(), whatever*> memberptr;
    char any1[sizeof(std::pair<void(*)(),void*>)];
    char any2[sizeof(std::pair<void(whatever::*)(), whatever*>)];
    TaskUnion() {}
    ~TaskUnion() {}
  };
#endif

  // Is F suitable for small buffer optimization?
  template<class F>
  static constexpr bool canSBO()
  {
    return sizeof(F) <= sizeof(TaskUnion) &&
      alignof(F) <= alignof(TaskUnion);
  }

  static_assert(canSBO<std::reference_wrapper<whatever>>(),
    "SBO not working for reference_wrapper");

  // Call (and possibly destroy) the function:
  typedef R (*call_function)(TaskUnion&, Args...);
  // Destroy the function (of needed):
  typedef void (*destroy_function)(TaskUnion&);
  // Move the function (otherwise memcpy):
  typedef void (*move_function)(TaskUnion& dest, TaskUnion& src);

  // Vtable of functions for manipulating whatever is in the TaskUnion:
  struct TaskVtable {
    call_function call;
    destroy_function destroy;
    move_function move;
  };

  TaskUnion buffer_;
  const TaskVtable* vtable_ = nullptr;

  void clear()
  {
    if (vtable_ && vtable_->destroy)
      vtable_->destroy(buffer_);
  }

public:

  Task() {}
  Task(std::nullptr_t) {}
  ~Task()
  {
    this->clear();
  }

  Task(Task const&) = delete;

  Task(Task&& that)
  {
    if (that.vtable_ && that.vtable_->move)
      that.vtable_->move(buffer_, that.buffer_);
    else
      std::memcpy(&buffer_, &that.buffer_, sizeof(buffer_));
    vtable_ = that.vtable_;
    that.vtable_ = nullptr;
  }
  Task& operator=(Task that)
  {
    this->clear();
    if (that.vtable_ && that.vtable_->move)
      that.vtable_->move(buffer_, that.buffer_);
    else
      std::memcpy(&buffer_, &that.buffer_, sizeof(buffer_));
    vtable_ = that.vtable_;
    that.vtable_ = nullptr;
    return *this;
  }

private:

  template<class F>
  typename std::enable_if<canSBO<F>()>::type
  init(F code)
  {
    const static TaskVtable vtable {
      // Call:
      [](TaskUnion& buffer, Args... args) -> R {
        F* src = reinterpret_cast<F*>(&buffer);
        F code = std::move(*src);
        src->~F();
        code(std::forward<Args>(args)...);
      },
      // Destroy:
      std::is_trivially_destructible<F>::value ?
      static_cast<destroy_function>(nullptr) :
      [](TaskUnion& buffer) {
        F* code = reinterpret_cast<F*>(&buffer);
        code->~F();
      },
      // Move:
      [](TaskUnion& dst, TaskUnion& src) {
        F* src_code = reinterpret_cast<F*>(&src);
        F* dst_code = reinterpret_cast<F*>(&dst);
        new(dst_code) F(std::move(*src_code));
        src_code->~F();
      }
    };
    new(&buffer_) F(std::move(code));
    vtable_ = &vtable;
  }

  template<class F>
  typename std::enable_if<!canSBO<F>()>::type
  init(F code)
  {
    const static TaskVtable vtable {
      // Call:
      [](TaskUnion& buffer, Args... args) -> R {
        // Delete F when we go out of scope:
        std::unique_ptr<F> code(*reinterpret_cast<F**>(&buffer));
        return (*code)(std::forward<Args>(args)...);
      },
      // Destroy:
      [](TaskUnion& buffer) {
        F* code = *reinterpret_cast<F**>(&buffer);
        delete code;
      },
      // Move:
      nullptr
    };
    *reinterpret_cast<F**>(&buffer_) = new F(std::move(code));
    vtable_ = &vtable;
  }

public:

  template<class F>
  Task(F code)
  {
    this->init(std::move(code));
  }

  operator bool() const { return vtable_ != nullptr; }
  bool operator!() const { return vtable_ == nullptr; }

  R operator()(Args... args)
  {
    if (vtable_ == nullptr)
      throw std::bad_function_call();
    const TaskVtable* vtable = vtable_;
    vtable_ = nullptr;
    return vtable->call(buffer_, std::forward<Args>(args)...);
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
