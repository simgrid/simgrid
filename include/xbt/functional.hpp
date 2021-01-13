/* Copyright (c) 2015-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_FUNCTIONAL_HPP
#define XBT_FUNCTIONAL_HPP

#include <xbt/sysdep.h>

#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <array>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace simgrid {
namespace xbt {

template <class F> class MainFunction {
  F code_;
  std::shared_ptr<const std::vector<std::string>> args_;

public:
  MainFunction(F code, std::vector<std::string>&& args)
      : code_(std::move(code)), args_(std::make_shared<const std::vector<std::string>>(std::move(args)))
  {
  }
  void operator()() const
  {
    const int argc                = args_->size();
    std::vector<std::string> args = *args_;
    std::vector<char*> argv(args.size() + 1); // argv[argc] is nullptr
    std::transform(begin(args), end(args), begin(argv), [](std::string& s) { return &s.front(); });
    code_(argc, argv.data());
  }
};

template <class F> inline std::function<void()> wrap_main(F code, std::vector<std::string>&& args)
{
  return MainFunction<F>(std::move(code), std::move(args));
}

template <class F> inline std::function<void()> wrap_main(F code, int argc, const char* const argv[])
{
  std::vector<std::string> args(argv, argv + argc);
  return MainFunction<F>(std::move(code), std::move(args));
}

namespace bits {
template <class F, class Tuple, std::size_t... I>
constexpr auto apply(F&& f, Tuple&& t, std::index_sequence<I...>)
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
 *  @endcode
 **/
template <class F, class Tuple>
constexpr auto apply(F&& f, Tuple&& t) -> decltype(
    simgrid::xbt::bits::apply(std::forward<F>(f), std::forward<Tuple>(t),
                              std::make_index_sequence<std::tuple_size<typename std::decay_t<Tuple>>::value>()))
{
  return simgrid::xbt::bits::apply(std::forward<F>(f), std::forward<Tuple>(t),
                                   std::make_index_sequence<std::tuple_size<typename std::decay_t<Tuple>>::value>());
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
  // Placeholder for some class type:
  struct whatever {};

  // Union used for storage:
  using TaskUnion =
      typename std::aligned_union_t<0, void*, std::pair<void (*)(), void*>, std::pair<void (whatever::*)(), whatever*>>;

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
  using call_function = R (*)(TaskUnion&, Args...);
  // Destroy the function (of needed):
  using destroy_function = void (*)(TaskUnion&);
  // Move the function (otherwise memcpy):
  using move_function = void (*)(TaskUnion& dest, TaskUnion& src);

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
  Task() = default;
  explicit Task(std::nullptr_t) { /* Nothing to do */}
  ~Task()
  {
    this->clear();
  }

  Task(Task const&) = delete;

  Task(Task&& that) noexcept
  {
    if (that.vtable_ && that.vtable_->move)
      that.vtable_->move(buffer_, that.buffer_);
    else
      std::memcpy(&buffer_, &that.buffer_, sizeof(buffer_));
    vtable_      = std::move(that.vtable_);
    that.vtable_ = nullptr;
  }
  Task& operator=(Task const& that) = delete;
  Task& operator=(Task&& that) noexcept
  {
    this->clear();
    if (that.vtable_ && that.vtable_->move)
      that.vtable_->move(buffer_, that.buffer_);
    else
      std::memcpy(&buffer_, &that.buffer_, sizeof(buffer_));
    vtable_      = std::move(that.vtable_);
    that.vtable_ = nullptr;
    return *this;
  }

private:
  template <class F> typename std::enable_if_t<canSBO<F>()> init(F code)
  {
    const static TaskVtable vtable {
      // Call:
      [](TaskUnion& buffer, Args... args) {
        auto* src = reinterpret_cast<F*>(&buffer);
        F code = std::move(*src);
        src->~F();
        // NOTE: std::forward<Args>(args)... is correct.
        return code(std::forward<Args>(args)...);
      },
      // Destroy:
      std::is_trivially_destructible<F>::value ?
      static_cast<destroy_function>(nullptr) :
      [](TaskUnion& buffer) {
        auto* code = reinterpret_cast<F*>(&buffer);
        code->~F();
      },
      // Move:
      [](TaskUnion& dst, TaskUnion& src) {
        auto* src_code = reinterpret_cast<F*>(&src);
        auto* dst_code = reinterpret_cast<F*>(&dst);
        new(dst_code) F(std::move(*src_code));
        src_code->~F();
      }
    };
    new(&buffer_) F(std::move(code));
    vtable_ = &vtable;
  }

  template <class F> typename std::enable_if_t<not canSBO<F>()> init(F code)
  {
    const static TaskVtable vtable {
      // Call:
      [](TaskUnion& buffer, Args... args) {
        // Delete F when we go out of scope:
        std::unique_ptr<F> code(*reinterpret_cast<F**>(&buffer));
        // NOTE: std::forward<Args>(args)... is correct.
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
  template <class F> explicit Task(F code) { this->init(std::move(code)); }

  operator bool() const { return vtable_ != nullptr; }
  bool operator!() const { return vtable_ == nullptr; }

  R operator()(Args... args)
  {
    if (vtable_ == nullptr)
      throw std::bad_function_call();
    const TaskVtable* vtable = vtable_;
    vtable_ = nullptr;
    // NOTE: std::forward<Args>(args)... is correct.
    // see C++ [func.wrap.func.inv] for an example
    return vtable->call(buffer_, std::forward<Args>(args)...);
  }
};

template<class F, class... Args>
class TaskImpl {
  F code_;
  std::tuple<Args...> args_;
  using result_type = decltype(simgrid::xbt::apply(std::move(code_), std::move(args_)));

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

template <class F, class... Args> auto make_task(F code, Args... args) -> Task<decltype(code(std::move(args)...))()>
{
  TaskImpl<F, Args...> task(std::move(code), std::make_tuple(std::move(args)...));
  return Task<decltype(code(std::move(args)...))()>(std::move(task));
}

} // namespace xbt
} // namespace simgrid
#endif
