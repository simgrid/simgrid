/* Copyright (c) 2006-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_ACTIVITY_HPP
#define SIMGRID_S4U_ACTIVITY_HPP

#include <algorithm>
#include <atomic>
#include <set>
#include <simgrid/forward.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <xbt/Extendable.hpp>
#include <xbt/asserts.h>
#include <xbt/signal.hpp>
#include <xbt/utility.hpp>

#ifndef DOXYGEN
XBT_LOG_EXTERNAL_CATEGORY(s4u_activity);
#endif

namespace simgrid {

extern template class XBT_PUBLIC xbt::Extendable<s4u::Activity>;

namespace s4u {

/** @brief Activities
 *
 * This class is the ancestor of every activities that an actor can undertake.
 * That is, activities are all the things that do take time to the actor in the simulated world.
 */
class XBT_PUBLIC Activity : public xbt::Extendable<Activity> {

#ifndef DOXYGEN
  friend ActivitySet;
  friend Comm;
  friend Exec;
  friend Io;
  friend Mess;
  friend kernel::activity::ActivityImpl;
  friend std::vector<ActivityPtr> create_DAG_from_dot(const std::string& filename);
  friend std::vector<ActivityPtr> create_DAG_from_DAX(const std::string& filename);
  friend std::vector<ActivityPtr> create_DAG_from_json(const std::string& filename);
#endif

public:
  // enum class State { ... }
  XBT_DECLARE_ENUM_CLASS(State, INITED, STARTING, STARTED, FAILED, CANCELED, FINISHED);

  virtual bool is_assigned() const = 0;
  bool dependencies_solved() const { return dependencies_.empty(); }
  bool has_no_successor() const { return successors_.empty(); }
  const std::set<ActivityPtr>& get_dependencies() const { return dependencies_; }
  const std::vector<ActivityPtr>& get_successors() const { return successors_; }

protected:
  Activity()  = default;
  virtual ~Activity() = default;
  void destroy();

  void release_dependencies()
  {
    while (not successors_.empty()) {
      ActivityPtr b = successors_.back();
      XBT_CVERB(s4u_activity, "Remove a dependency from '%s' on '%s'", get_cname(), b->get_cname());
      b->dependencies_.erase(this);
      if (b->dependencies_solved()) {
        b->start();
      }
      successors_.pop_back();
    }
  }

  void add_successor(ActivityPtr a)
  {
    if(this == a)
      throw std::invalid_argument("Cannot be its own successor");

    if (std::any_of(begin(successors_), end(successors_), [a](ActivityPtr const& i) { return i.get() == a.get(); }))
      throw std::invalid_argument("Dependency already exists");

    successors_.push_back(a);
    a->dependencies_.insert({this});
  }

  void remove_successor(ActivityPtr a)
  {
    if(this == a)
      throw std::invalid_argument("Cannot ask to remove itself from successors list");

    auto p =
        std::find_if(successors_.begin(), successors_.end(), [a](ActivityPtr const& i) { return i.get() == a.get(); });
    if (p == successors_.end())
      throw std::invalid_argument("Dependency does not exist. Can not be removed.");

    successors_.erase(p);
    a->dependencies_.erase({this});
  }

  static std::set<Activity*>* vetoed_activities_;

  /** Set the [remaining] amount of work that this Activity will entail
   *
   * It is forbidden to change the amount of work once the Activity is started */
  Activity* set_remaining(double remains);

  virtual void fire_on_start() const           = 0;
  virtual void fire_on_this_start() const      = 0;
  virtual void fire_on_completion() const      = 0;
  virtual void fire_on_this_completion() const = 0;
  virtual void fire_on_suspend() const         = 0;
  virtual void fire_on_this_suspend() const    = 0;
  virtual void fire_on_resume() const          = 0;
  virtual void fire_on_this_resume() const     = 0;
  virtual void fire_on_veto()                  = 0;
  virtual void fire_on_this_veto()             = 0;

public:
  void start()
  {
    state_ = State::STARTING;
    if (dependencies_solved() && is_assigned()) {
      XBT_CVERB(s4u_activity, "'%s' is assigned to a resource and all dependencies are solved. Let's start", get_cname());
      do_start();
    } else {
      if (vetoed_activities_ != nullptr)
        vetoed_activities_->insert(this);
      fire_on_veto();
      fire_on_this_veto();
    }
  }

  void complete(Activity::State state)
  {
    // Ensure that the current activity remains alive until the end of the function, even if its last reference is
    // released by the on_completion() callbacks.
    ActivityPtr keepalive(this);
    state_ = state;
    fire_on_completion();
    fire_on_this_completion();
    if (state == State::FINISHED)
      release_dependencies();
  }

  static std::set<Activity*>* get_vetoed_activities() { return vetoed_activities_; }
  static void set_vetoed_activities(std::set<Activity*>* whereto) { vetoed_activities_ = whereto; }

#ifndef DOXYGEN
  Activity(Activity const&) = delete;
  Activity& operator=(Activity const&) = delete;
#endif

  /** Starts a previously created activity.
   *
   * This function is optional: you can call wait() even if you didn't call start()
   */
  virtual Activity* do_start() = 0;
  /** Tests whether the given activity is terminated yet. */
  virtual bool test();

  /** Blocks the current actor until the activity is terminated */
  Activity* wait() { return wait_for(-1.0); }
  /** Blocks the current actor until the activity is terminated, or until the timeout is elapsed\n
   *  Raises: timeout exception.*/
  Activity* wait_for(double timeout);
  /** Just like wait_for(), but the activity is first canceled if a timeout exception is raised.*/
  Activity* wait_for_or_cancel(double timeout);
  /** Blocks the current actor until the activity is terminated, or until the time limit is reached\n
   * Raises: timeout exception. */
  void wait_until(double time_limit);

  /** Cancel that activity */
  Activity* cancel();
  /** Retrieve the current state of the activity */
  Activity::State get_state() const { return state_; }
  /** Return a string representation of the activity's state (one of INITED, STARTING, STARTED, CANCELED, FINISHED,
   * DONE) */
  const char* get_state_str() const;
  bool is_canceled() const { return state_ == State::CANCELED; }
  bool is_failed() const { return state_ == State::FAILED; }
  bool is_done() const { return state_ == State::FINISHED; }
  void set_state(Activity::State state) { state_ = state; }
  void set_detached (bool detached) { detached_ = detached; }
  bool is_detached() const { return detached_;}

  /** Blocks the progression of this activity until it gets resumed */
  virtual Activity* suspend();
  /** Unblock the progression of this activity if it was suspended previously */
  virtual Activity* resume();
  /** Whether or not the progression of this activity is blocked */
  bool is_suspended() const { return suspended_; }

  virtual const char* get_cname() const       = 0;
  virtual const std::string& get_name() const = 0;

  /** Get the remaining amount of work that this Activity entails. When it's 0, it's done. */
  virtual double get_remaining() const;

  double get_start_time() const;
  double get_finish_time() const;
  void mark() { marked_ = true; }
  bool is_marked() const { return marked_; }

  /** Returns the internal implementation of this Activity */
  kernel::activity::ActivityImpl* get_impl() const { return pimpl_.get(); }

#ifndef DOXYGEN
  friend void intrusive_ptr_release(Activity* a)
  {
    if (a->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
      std::atomic_thread_fence(std::memory_order_acquire);
      delete a;
    }
  }
  friend void intrusive_ptr_add_ref(Activity* a) { a->refcount_.fetch_add(1, std::memory_order_relaxed); }
#endif
  Activity* add_ref()
  {
    intrusive_ptr_add_ref(this);
    return this;
  }
  void unref() { intrusive_ptr_release(this); }

private:
  kernel::activity::ActivityImplPtr pimpl_ = nullptr;
  Activity::State state_                   = Activity::State::INITED;
  double remains_                          = 0;
  bool suspended_                          = false;
  bool marked_                             = false;
  bool detached_                           = false;

  std::vector<ActivityPtr> successors_;
  std::set<ActivityPtr> dependencies_;
  std::atomic_int_fast32_t refcount_{0};
};

template <class AnyActivity> class Activity_T : public Activity {
  std::string name_             = "unnamed";
  std::string tracing_category_ = "";
  std::function<void(void*)> clean_fun_; // used if detached

  static xbt::signal<void(AnyActivity const&)> on_start;
  xbt::signal<void(AnyActivity const&)> on_this_start;
  static xbt::signal<void(AnyActivity const&)> on_completion;
  xbt::signal<void(AnyActivity const&)> on_this_completion;
  static xbt::signal<void(AnyActivity const&)> on_suspend;
  xbt::signal<void(AnyActivity const&)> on_this_suspend;
  static xbt::signal<void(AnyActivity const&)> on_resume;
  xbt::signal<void(AnyActivity const&)> on_this_resume;
  static xbt::signal<void(AnyActivity&)> on_veto;
  xbt::signal<void(AnyActivity&)> on_this_veto;

protected:
  void fire_on_start() const override;
  void fire_on_this_start() const override { on_this_start(static_cast<const AnyActivity&>(*this)); }
  void fire_on_completion() const override;
  void fire_on_this_completion() const override { on_this_completion(static_cast<const AnyActivity&>(*this)); }
  void fire_on_suspend() const override;
  void fire_on_this_suspend() const override { on_this_suspend(static_cast<const AnyActivity&>(*this)); }
  void fire_on_resume() const override;
  void fire_on_this_resume() const override { on_this_resume(static_cast<const AnyActivity&>(*this)); }
  void fire_on_veto() override;
  void fire_on_this_veto() override { on_this_veto(static_cast<AnyActivity&>(*this)); }

public:
  /*! \static Add a callback fired when any activity starts (no veto) */
  static void on_start_cb(const std::function<void(AnyActivity const&)>& cb);
  /*!  Add a callback fired when this specific activity starts (no veto) */
  void on_this_start_cb(const std::function<void(AnyActivity const&)>& cb) { on_this_start.connect(cb); }
  /*! \static Add a callback fired when any activity completes (either normally, cancelled or failed) */
  static void on_completion_cb(const std::function<void(AnyActivity const&)>& cb);
  /*! Add a callback fired when this specific activity completes (either normally, cancelled or failed) */
  void on_this_completion_cb(const std::function<void(AnyActivity const&)>& cb) { on_this_completion.connect(cb); }
  /*! \static Add a callback fired when any activity is suspended */
  static void on_suspend_cb(const std::function<void(AnyActivity const&)>& cb);
  /*! Add a callback fired when this specific activity is suspended */
  void on_this_suspend_cb(const std::function<void(AnyActivity const&)>& cb) { on_this_suspend.connect(cb); }
  /*! \static Add a callback fired when any activity is resumed after being suspended */
  static void on_resume_cb(const std::function<void(AnyActivity const&)>& cb);
  /*! Add a callback fired when this specific activity is resumed after being suspended */
  void on_this_resume_cb(const std::function<void(AnyActivity const&)>& cb) { on_this_resume.connect(cb); }
  /*! \static Add a callback fired each time that any activity fails to start because of a veto (e.g., unsolved
   *  dependency or no resource assigned) */
  static void on_veto_cb(const std::function<void(AnyActivity&)>& cb);
  /*! Add a callback fired each time that this specific activity fails to start because of a veto (e.g., unsolved
   *  dependency or no resource assigned) */
  void on_this_veto_cb(const std::function<void(AnyActivity&)>& cb) { on_this_veto.connect(cb); }

  AnyActivity* add_successor(ActivityPtr a)
  {
    Activity::add_successor(a);
    return static_cast<AnyActivity*>(this);
  }
  AnyActivity* remove_successor(ActivityPtr a)
  {
    Activity::remove_successor(a);
    return static_cast<AnyActivity*>(this);
  }
  AnyActivity* set_name(std::string_view name)
  {
    name_ = name;
    return static_cast<AnyActivity*>(this);
  }
  const std::string& get_name() const override { return name_; }
  const char* get_cname() const override { return name_.c_str(); }

  AnyActivity* set_tracing_category(std::string_view category)
  {
    xbt_assert(get_state() == State::INITED || get_state() == State::STARTING,
               "Cannot change the tracing category of an activity after its start");
    tracing_category_ = category;
    return static_cast<AnyActivity*>(this);
  }
  const std::string& get_tracing_category() const { return tracing_category_; }
  const std::function<void(void*)>& get_clean_function() const { return clean_fun_; }

  AnyActivity* start()
  {
    Activity::start();
    return static_cast<AnyActivity*>(this);
  }
   /** Start the activity, and ignore its result. It can be completely forgotten after that. */
  AnyActivity* detach()
  {
    xbt_assert(get_state() == State::INITED || get_state() == State::STARTING,
               "You cannot use %s() once your activity is %s (not implemented)", __func__, get_state_str());

    set_detached(true);
    return start();
  }

  AnyActivity* detach(const std::function<void(void*)>& clean_function)
  {
    clean_fun_ = clean_function;
    return detach();
  }

  AnyActivity* cancel() { return static_cast<AnyActivity*>(Activity::cancel()); }
  AnyActivity* wait() { return wait_for(-1.0); }
  virtual AnyActivity* wait_for(double timeout) {
    return static_cast<AnyActivity*>(Activity::wait_for(timeout));
  }

#ifndef DOXYGEN
  /* The refcounting is done in the ancestor class, Activity, but we want each of the classes benefiting of the CRTP
   * (Exec, Comm, etc) to have smart pointers too, so we define these methods here, that forward the ptr_release and
   * add_ref to the Activity class. Hopefully, the "inline" helps to not hinder the perf here.
   */
  friend void inline intrusive_ptr_release(AnyActivity* a) { intrusive_ptr_release(static_cast<Activity*>(a)); }
  friend void inline intrusive_ptr_add_ref(AnyActivity* a) { intrusive_ptr_add_ref(static_cast<Activity*>(a)); }
#endif
};

} // namespace s4u
} // namespace simgrid

#endif /* SIMGRID_S4U_ACTIVITY_HPP */
