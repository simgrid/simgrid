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
#include <xbt/asserts.h>

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

  std::map<std::string, double> amount_              = {{"instance_0", 0}, {"dispatcher", 0}, {"collector", 0}};
  std::map<std::string, int> queued_firings_         = {{"instance_0", 0}, {"dispatcher", 0}, {"collector", 0}};
  std::map<std::string, int> running_instances_      = {{"instance_0", 0}, {"dispatcher", 0}, {"collector", 0}};
  std::map<std::string, int> count_                  = {{"instance_0", 0}, {"dispatcher", 0}, {"collector", 0}};
  std::map<std::string, int> parallelism_degree_     = {{"instance_0", 1}, {"dispatcher", 1}, {"collector", 1}};
  std::map<std::string, int> internal_bytes_to_send_ = {{"instance_0", 0}, {"dispatcher", 0}};

  std::function<std::string()> load_balancing_function_;

  std::set<Task*> successors_                 = {};
  std::map<Task*, unsigned int> predecessors_ = {};
  std::atomic_int_fast32_t refcount_{0};

  bool ready_to_run(std::string instance);
  void receive(Task* source);

  std::shared_ptr<Token> token_ = nullptr;
  std::deque<std::map<TaskPtr, std::shared_ptr<Token>>> tokens_received_;
  std::map<std::string, std::deque<ActivityPtr>> current_activities_ = {
      {"instance_0", {}}, {"dispatcher", {}}, {"collector", {}}};

  inline static xbt::signal<void(Task*)> on_start;
  xbt::signal<void(Task*)> on_this_start;
  inline static xbt::signal<void(Task*)> on_completion;
  xbt::signal<void(Task*)> on_this_completion;

protected:
  explicit Task(const std::string& name);
  virtual ~Task() = default;

  virtual void fire(std::string instance);
  void complete(std::string instance);

  void store_activity(ActivityPtr a, std::string instance) { current_activities_[instance].push_back(a); }

  virtual void add_instances(int n);
  virtual void remove_instances(int n);

public:
  void set_name(std::string name);
  const std::string& get_name() const { return name_; }
  const char* get_cname() const { return name_.c_str(); }
  void set_amount(double amount, std::string instance = "instance_0");
  double get_amount(std::string instance = "instance_0") const { return amount_.at(instance); }
  int get_count(std::string instance = "collector") const { return count_.at(instance); }
  void set_parallelism_degree(int n, std::string instance = "all");
  int get_parallelism_degree(std::string instance = "instance_0") const { return parallelism_degree_.at(instance); }
  void set_internal_bytes(int bytes, std::string instance = "instance_0");
  double get_internal_bytes(std::string instance = "instance_0") const { return internal_bytes_to_send_.at(instance); }
  void set_load_balancing_function(std::function<std::string()> func);

  void set_token(std::shared_ptr<Token> token);
  std::shared_ptr<Token> get_next_token_from(TaskPtr t) const { return tokens_received_.front().at(t); }

  void add_successor(TaskPtr t);
  void remove_successor(TaskPtr t);
  void remove_all_successors();
  const std::set<Task*>& get_successors() const { return successors_; }

  void enqueue_firings(int n);

  /** Add a callback fired before this task activity starts */
  void on_this_start_cb(const std::function<void(Task*)>& func) { on_this_start.connect(func); }
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
  void fire(std::string instance) override;

public:
  static CommTaskPtr init(const std::string& name);
  static CommTaskPtr init(const std::string& name, double bytes, Host* source, Host* destination);

  CommTaskPtr set_source(Host* source);
  Host* get_source() const { return source_; }
  CommTaskPtr set_destination(Host* destination);
  Host* get_destination() const { return destination_; }
  CommTaskPtr set_bytes(double bytes);
  double get_bytes() const { return get_amount("instance_0"); }
};

class ExecTask : public Task {
  std::map<std::string, Host*> host_ = {{"instance_0", nullptr}, {"dispatcher", nullptr}, {"collector", nullptr}};

  explicit ExecTask(const std::string& name);
  void fire(std::string instance) override;

public:
  static ExecTaskPtr init(const std::string& name);
  static ExecTaskPtr init(const std::string& name, double flops, Host* host);

  ExecTaskPtr set_host(Host* host, std::string instance = "all");
  Host* get_host(std::string instance = "instance_0") const { return host_.at(instance); }
  ExecTaskPtr set_flops(double flops, std::string instance = "instance_0");
  double get_flops(std::string instance = "instance_0") const { return get_amount(instance); }

  void add_instances(int n) override;
  void remove_instances(int n) override;
};

class IoTask : public Task {
  Disk* disk_;
  Io::OpType type_;
  explicit IoTask(const std::string& name);
  void fire(std::string instance) override;

public:
  static IoTaskPtr init(const std::string& name);
  static IoTaskPtr init(const std::string& name, double bytes, Disk* disk, Io::OpType type);

  IoTaskPtr set_disk(Disk* disk);
  Disk* get_disk() const { return disk_; }
  IoTaskPtr set_bytes(double bytes);
  double get_bytes() const { return get_amount("instance_0"); }
  IoTaskPtr set_op_type(Io::OpType type);
  Io::OpType get_op_type() const { return type_; }
};
} // namespace simgrid::s4u
#endif
