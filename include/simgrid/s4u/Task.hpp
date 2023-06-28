#ifndef SIMGRID_S4U_TASK_H_
#define SIMGRID_S4U_TASK_H_

#include <simgrid/s4u/Activity.hpp>
#include <simgrid/s4u/Io.hpp>
#include <xbt/Extendable.hpp>

#include <atomic>
#include <deque>
#include <map>
#include <memory>
#include <set>

namespace simgrid::s4u {

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

class XBT_PUBLIC Token : public xbt::Extendable<Token> {};

class Task {
  std::string name_;
  double amount_;
  int queued_firings_ = 0;
  int count_        = 0;
  bool working_     = false;

  std::set<Task*> successors_                 = {};
  std::map<Task*, unsigned int> predecessors_ = {};
  std::atomic_int_fast32_t refcount_{0};

  bool ready_to_run() const;
  void receive(Task* source);

  std::shared_ptr<Token> token_ = nullptr;
  std::deque<std::map<TaskPtr, std::shared_ptr<Token>>> tokens_received_;
  ActivityPtr previous_activity_;
  ActivityPtr current_activity_;

protected:
  explicit Task(const std::string& name);
  virtual ~Task()     = default;

  virtual void fire();
  void complete();

  void set_current_activity (ActivityPtr a) { current_activity_ = a; }

  inline static xbt::signal<void(Task*)> on_start;
  xbt::signal<void(Task*)> on_this_start;
  inline static xbt::signal<void(Task*)> on_completion;
  xbt::signal<void(Task*)> on_this_completion;

public:
  const std::string& get_name() const { return name_; }
  const char* get_cname() const { return name_.c_str(); }
  void set_amount(double amount);
  double get_amount() const { return amount_; }
  int get_count() const { return count_; }

  void set_token(std::shared_ptr<Token> token);
  std::shared_ptr<Token> get_next_token_from(TaskPtr t);

  void add_successor(TaskPtr t);
  void remove_successor(TaskPtr t);
  void remove_all_successors();
  const std::set<Task*>& get_successors() const { return successors_; }

  void enqueue_firings(int n);

  /** Add a callback fired before this task activity starts */
  void on_this_start_cb(const std::function<void(Task*)>& func){ on_this_start.connect(func); }
  /** Add a callback fired before a task activity starts.
   * Triggered after the on_this_start function**/
  static void on_start_cb(const std::function<void(Task*)>& cb) { on_start.connect(cb); }
  /** Add a callback fired before this task activity ends */
  void on_this_completion_cb(const std::function<void(Task*)>& func) { on_this_completion.connect(func); };
  /** Add a callback fired after a task activity ends.
   * Triggered after the on_this_end function, but before sending tokens to successors.**/
  static void on_completion_cb(const std::function<void(Task*)>& cb) { on_completion.connect(cb); }

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

class CommTask : public Task {
  Host* source_;
  Host* destination_;

  explicit CommTask(const std::string& name);
  void fire() override;

public:
  static CommTaskPtr init(const std::string& name);
  static CommTaskPtr init(const std::string& name, double bytes, Host* source, Host* destination);

  CommTaskPtr set_source(Host* source);
  Host* get_source() const { return source_; }
  CommTaskPtr set_destination(Host* destination);
  Host* get_destination() const { return destination_; }
  CommTaskPtr set_bytes(double bytes);
  double get_bytes() const { return get_amount(); }
};

class ExecTask : public Task {
  Host* host_;

  explicit ExecTask(const std::string& name);
  void fire() override;

public:
  static ExecTaskPtr init(const std::string& name);
  static ExecTaskPtr init(const std::string& name, double flops, Host* host);

  ExecTaskPtr set_host(Host* host);
  Host* get_host() const { return host_; }
  ExecTaskPtr set_flops(double flops);
  double get_flops() const { return get_amount(); }
};

class IoTask : public Task {
  Disk* disk_;
  Io::OpType type_;
  explicit IoTask(const std::string& name);
  void fire() override;

public:
  static IoTaskPtr init(const std::string& name);
  static IoTaskPtr init(const std::string& name, double bytes, Disk* disk, Io::OpType type);

  IoTaskPtr set_disk(Disk* disk);
  Disk* get_disk() const { return disk_; }
  IoTaskPtr set_bytes(double bytes);
  double get_bytes() const { return get_amount(); }
  IoTaskPtr set_op_type(Io::OpType type);
  Io::OpType get_op_type() const { return type_; }
};
} // namespace simgrid::s4u
#endif
