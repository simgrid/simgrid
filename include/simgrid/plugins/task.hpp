#ifndef SIMGRID_PLUGINS_TASK_H_
#define SIMGRID_PLUGINS_TASK_H_

#include <simgrid/s4u/Activity.hpp>
#include <simgrid/s4u/Io.hpp>
#include <xbt/Extendable.hpp>

#include <atomic>
#include <map>
#include <memory>
#include <set>

namespace simgrid::plugins {

class Task;
using TaskPtr = boost::intrusive_ptr<Task>;
XBT_PUBLIC void intrusive_ptr_release(Task* o);
XBT_PUBLIC void intrusive_ptr_add_ref(Task* o);
class ExecTask;
using ExecTaskPtr = boost::intrusive_ptr<ExecTask>;
class CommTask;
using CommTaskPtr = boost::intrusive_ptr<CommTask>;
class IoTask;
using IoTaskPtr = boost::intrusive_ptr<IoTask>;

struct ExtendedAttributeActivity {
  static simgrid::xbt::Extension<simgrid::s4u::Activity, ExtendedAttributeActivity> EXTENSION_ID;
  Task* task_;
};

class Task {
  std::set<Task*> successors_                 = {};
  std::map<Task*, unsigned int> predecessors_ = {};

  bool ready_to_run() const;
  void receive(Task* source);
  void complete();

protected:
  std::string name_;
  double amount_;
  int queued_execs_ = 0;
  int count_        = 0;
  bool working_     = false;
  s4u::ActivityPtr current_activity_;
  xbt::signal<void(Task*)> on_this_start_;
  xbt::signal<void(Task*)> on_this_end_;
  explicit Task(const std::string& name);
  virtual ~Task()     = default;
  virtual void fire() = 0;

  static xbt::signal<void(Task*)> on_start;
  static xbt::signal<void(Task*)> on_end;
  std::atomic_int_fast32_t refcount_{0};

public:
  static void init();
  const std::string& get_name() const { return name_; }
  const char* get_cname() const { return name_.c_str(); }
  void enqueue_execs(int n);
  void set_amount(double amount);
  double get_amount() const { return amount_; }
  void add_successor(TaskPtr t);
  void remove_successor(TaskPtr t);
  void remove_all_successors();
  const std::set<Task*>& get_successors() const { return successors_; }
  void on_this_start_cb(const std::function<void(Task*)>& func);
  void on_this_end_cb(const std::function<void(Task*)>& func);
  int get_count() const;

  /** Add a callback fired before a task activity start.
   * Triggered after the on_this_start function**/
  static void on_start_cb(const std::function<void(Task*)>& cb) { on_start.connect(cb); }
  /** Add a callback fired after a task activity end.
   * Triggered after the on_this_end function, but before
   * sending tokens to successors.**/
  static void on_end_cb(const std::function<void(Task*)>& cb) { on_end.connect(cb); }

#ifndef DOXYGEN
  friend void intrusive_ptr_release(Task* o)
  {
    if (o->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
      std::atomic_thread_fence(std::memory_order_acquire);
      delete o;
    }
  }
  friend void intrusive_ptr_add_ref(Task* o) { o->refcount_.fetch_add(1, std::memory_order_relaxed); }
#endif
};

class ExecTask : public Task {
  s4u::Host* host_;

  explicit ExecTask(const std::string& name);
  void fire() override;

public:
  static ExecTaskPtr init(const std::string& name);
  static ExecTaskPtr init(const std::string& name, double flops, s4u::Host* host);
  ExecTaskPtr set_host(s4u::Host* host);
  s4u::Host* get_host() const { return host_; }
  ExecTaskPtr set_flops(double flops);
  double get_flops() const { return get_amount(); }
};

class CommTask : public Task {
  s4u::Host* source_;
  s4u::Host* destination_;

  explicit CommTask(const std::string& name);
  void fire() override;

public:
  static CommTaskPtr init(const std::string& name);
  static CommTaskPtr init(const std::string& name, double bytes, s4u::Host* source, s4u::Host* destination);
  CommTaskPtr set_source(s4u::Host* source);
  s4u::Host* get_source() const { return source_; }
  CommTaskPtr set_destination(s4u::Host* destination);
  s4u::Host* get_destination() const { return destination_; }
  CommTaskPtr set_bytes(double bytes);
  double get_bytes() const { return get_amount(); }
};

class IoTask : public Task {
  s4u::Disk* disk_;
  s4u::Io::OpType type_;
  explicit IoTask(const std::string& name);
  void fire() override;

public:
  static IoTaskPtr init(const std::string& name);
  static IoTaskPtr init(const std::string& name, double bytes, s4u::Disk* disk, s4u::Io::OpType type);
  IoTaskPtr set_disk(s4u::Disk* disk);
  s4u::Disk* get_disk() const { return disk_; }
  IoTaskPtr set_bytes(double bytes);
  double get_bytes() { return get_amount(); }
  IoTaskPtr set_op_type(s4u::Io::OpType type);
  s4u::Io::OpType get_op_type() { return type_; }
};
} // namespace simgrid::plugins
#endif
