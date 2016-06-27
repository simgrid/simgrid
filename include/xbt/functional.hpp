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

namespace bits {

  // Compute the maximum size taken by any of the given types:
  template <class... T> struct max_size;
  template <>
  struct max_size<> : public std::integral_constant<std::size_t, 0> {};
  template <class T>
  struct max_size<T> : public std::integral_constant<std::size_t, sizeof(T)> {};
  template <class T, class... U>
  struct max_size<T, U...> : public std::integral_constant<std::size_t,
    (sizeof(T) > max_size<U...>::value) ? sizeof(T) : max_size<U...>::value
  > {};

  struct whatever {};

  // What we can store in a Task:
  typedef void* ptr_callback;
  struct funcptr_callback {
    // Placeholder for any function pointer:
    void(*callback)();
    void* data;
  };
  struct member_funcptr_callback {
    // Placeholder for any pointer to member function:
    void (whatever::* callback)();
    whatever* data;
  };
  constexpr std::size_t any_size = max_size<
    ptr_callback,
    funcptr_callback,
    member_funcptr_callback
  >::value;
  typedef std::array<char, any_size> any_callback;

  // Union of what we can store in a Task:
  union TaskErasure {
    ptr_callback ptr;
    funcptr_callback funcptr;
    member_funcptr_callback member_funcptr;
    any_callback any;
  };

  // Can we copy F in Task (or do we have to use the heap)?
  template<class F>
  constexpr bool isUsableDirectlyInTask()
  {
    // TODO, detect availability std::is_trivially_copyable / workaround
#if 1
    // std::is_trivially_copyable is not available before GCC 5.
    return false;
#else
    // The only types we can portably store directly in the Task are the
    // trivially copyable ones (we can memcpy) which are small enough to fit:
    return std::is_trivially_copyable<F>::value &&
      sizeof(F) <= sizeof(bits::any_callback);
#endif
  }

}

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

  typedef bits::TaskErasure TaskErasure;
  struct TaskErasureVtable {
    // Call (and possibly destroy) the function:
    R (*call)(TaskErasure&, Args...);
    // Destroy the function:
    void (*destroy)(TaskErasure&);
  };

  TaskErasure code_;
  const TaskErasureVtable* vtable_ = nullptr;

public:
  Task() {}
  Task(std::nullptr_t) {}
  ~Task()
  {
    if (vtable_ && vtable_->destroy)
      vtable_->destroy(code_);
  }

  Task(Task const&) = delete;
  Task& operator=(Task const&) = delete;

  Task(Task&& that)
  {
    std::memcpy(&code_, &that.code_, sizeof(code_));
    vtable_ = that.vtable_;
    that.vtable_ = nullptr;
  }
  Task& operator=(Task&& that)
  {
    if (vtable_ && vtable_->destroy)
      vtable_->destroy(code_);
    std::memcpy(&code_, &that.code_, sizeof(code_));
    vtable_ = that.vtable_;
    that.vtable_ = nullptr;
    return *this;
  }

  template<class F,
    typename = typename std::enable_if<bits::isUsableDirectlyInTask<F>()>::type>
  Task(F const& code)
  {
    const static TaskErasureVtable vtable {
      // Call:
      [](TaskErasure& erasure, Args... args) -> R {
        // We need to wrap F un a union because F might not have a default
        // constructor: this is especially the case for lambdas.
        union no_ctor {
          no_ctor() {}
          ~no_ctor() {}
          F code ;
        } code;
        if (!std::is_empty<F>::value)
          // AFAIU, this is safe as per [basic.types]:
          std::memcpy(&code.code, erasure.any.data(), sizeof(code.code));
        code.code(std::forward<Args>(args)...);
      },
      // Destroy:
      nullptr
    };
    if (!std::is_empty<F>::value)
      std::memcpy(code_.any.data(), &code, sizeof(code));
    vtable_ = &vtable;
  }

  template<class F,
    typename = typename std::enable_if<!bits::isUsableDirectlyInTask<F>()>::type>
  Task(F code)
  {
    const static TaskErasureVtable vtable {
      // Call:
      [](TaskErasure& erasure, Args... args) -> R {
        // Delete F when we go out of scope:
        std::unique_ptr<F> code(static_cast<F*>(erasure.ptr));
        (*code)(std::forward<Args>(args)...);
      },
      // Destroy:
      [](TaskErasure& erasure) {
        F* code = static_cast<F*>(erasure.ptr);
        delete code;
      }
    };
    code_.ptr = new F(std::move(code));
    vtable_ = &vtable;
  }

  template<class F>
  Task(std::reference_wrapper<F> code)
  {
    const static TaskErasureVtable vtable {
      // Call:
      [](TaskErasure& erasure, Args... args) -> R {
        F* code = static_cast<F*>(erasure.ptr);
        (*code)(std::forward<Args>(args)...);
      },
      // Destroy:
      nullptr
    };
    code.code_.ptr = code.get();
    vtable_ = &vtable;
  }

  // TODO, Task(funcptr)
  // TODO, Task(funcptr, data)
  // TODO, Task(method, object)
  // TODO, Task(stateless lambda)

  operator bool() const { return vtable_ != nullptr; }
  bool operator!() const { return vtable_ == nullptr; }

  R operator()(Args... args)
  {
    if (!vtable_)
      throw std::bad_function_call();
    const TaskErasureVtable* vtable = vtable_;
    vtable_ = nullptr;
    return vtable->call(code_, std::forward<Args>(args)...);
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
