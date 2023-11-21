#ifndef SIMGRID_S4U_TASK_H_
#define SIMGRID_S4U_TASK_H_

#include <simgrid/forward.h>
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

class XBT_PUBLIC Token : public xbt::Extendable<Token> {};

/** Task class */
class XBT_PUBLIC Task {

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
  std::map<TaskPtr, std::deque<std::shared_ptr<Token>>> tokens_received_;
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

  void store_activity(ActivityPtr a, const std::string& instance) { current_activities_[instance].push_back(a); }

  virtual void add_instances(int n);
  virtual void remove_instances(int n);

public:
  /** @param name The new name of this Task */
  void set_name(std::string name);
  /** Retrieves the name of that Task as a C++ string */
  const std::string& get_name() const { return name_; }
  /** Retrieves the name of that Task as a C string */
  const char* get_cname() const { return name_.c_str(); }
  /** @param amount The new amount of work this instance of this Task has to do
   *  @note In flops for ExecTasks instances and in bytes for CommTasks instances. In flops for dispatcher and collector
   * instances */
  void set_amount(double amount, std::string instance = "instance_0");
  /** @return Amout of work this instance of this Task has to process */
  double get_amount(std::string instance = "instance_0") const { return amount_.at(instance); }
  /** @return Amount of queued firings for this instance of this Task */
  int get_queued_firings(std::string instance = "instance_0") const { return queued_firings_.at(instance); }
  /** @return Amount currently running of this instance of this Task */
  int get_running_count(std::string instance = "instance_0") const { return running_instances_.at(instance); }
  /** @return Number of times this instance of this Task has been completed */
  int get_count(std::string instance = "collector") const { return count_.at(instance); }
  /** @param n The parallelism degree to set
   *  @brief The parallelism degree defines how many of this instance can run in parallel. */
  void set_parallelism_degree(int n, std::string instance = "all");
  /** @return Parallelism degree of this instance of this Task */
  int get_parallelism_degree(std::string instance = "instance_0") const { return parallelism_degree_.at(instance); }
  /** @param bytes The amount of bytes this instance has to send to the next instance of this Task
   *  @note This amount is used when the host is different between the dispatcher and the instance doing the work of the
   * Task, or between the instance and the collector. */
  void set_internal_bytes(int bytes, std::string instance = "instance_0");
  /** @return Amount of bytes this instance of the Task has to send to the next instance */
  double get_internal_bytes(std::string instance = "instance_0") const { return internal_bytes_to_send_.at(instance); }
  /** @param func The new balancing function
   *  @note This function is used by the dispatcher to determine which instance will effectively do the work. This
   * function must return the name of the instance as a string. The default balancing function always returns
   * "instance_0" */
  void set_load_balancing_function(std::function<std::string()> func);
  /** @param token The new token */
  void set_token(std::shared_ptr<Token> token);
  /** @param t A Smart pointer to a Task
   *  @return Oldest token received by this Task that was sent by Task t */
  std::shared_ptr<Token> get_token_from(TaskPtr t) const { return tokens_received_.at(t).front(); }
  /** @param t A Smart pointer to a Task
   *  @return All tokens received by this Task that were sent by Task t */
  std::deque<std::shared_ptr<Token>> get_tokens_from(TaskPtr t) const { return tokens_received_.at(t); }
  /** @param t A Smart pointer to a Task
   *   @brief Pop the oldest token received by this Task that was sent by Task t */
  void deque_token_from(TaskPtr t);
  /** @param t A Smart pointer to a Task
   *  @brief Add t as a successor of this Task */
  void add_successor(TaskPtr t);
  /** @param t A Smart pointer to a Task
   *  @brief Remove t from the successors of this Task */
  void remove_successor(TaskPtr t);
  /** @brief Remove all successors from this Task */
  void remove_all_successors();
  /** @return All successors of this Task */
  const std::set<Task*>& get_successors() const { return successors_; }
  /** @param n The number of firings to enqueue */
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

/** CommTask class */
class CommTask : public Task {
  Host* source_;
  Host* destination_;

  explicit CommTask(const std::string& name);
  void fire(std::string instance) override;

public:
  static CommTaskPtr init(const std::string& name);
  static CommTaskPtr init(const std::string& name, double bytes, Host* source, Host* destination);

  /** @param source The new source Host of this CommTask
   *  @return A Smart pointer to this CommTask */
  CommTaskPtr set_source(Host* source);
  /** @return A pointer to the source Host of this CommTask */
  Host* get_source() const { return source_; }
  /** @param destination The new destination of this CommTask
   *  @return A Smart pointer to the destination Host of this CommTask */
  CommTaskPtr set_destination(Host* destination);
  /** @return A pointer to the destination Host of this CommTask */
  Host* get_destination() const { return destination_; }
  /** @param bytes The amount of bytes this CommTask has to send */
  CommTaskPtr set_bytes(double bytes);
  /** @return The amout of bytes this CommTask has to send */
  double get_bytes() const { return get_amount("instance_0"); }
};

/** ExecTask class */
class ExecTask : public Task {
  std::map<std::string, Host*> host_ = {{"instance_0", nullptr}, {"dispatcher", nullptr}, {"collector", nullptr}};

  explicit ExecTask(const std::string& name);
  void fire(std::string instance) override;

public:
  static ExecTaskPtr init(const std::string& name);
  static ExecTaskPtr init(const std::string& name, double flops, Host* host);

  /** @param host The new host of this instance of this ExecTask
   *  @return a Smart pointer to this ExecTask */
  ExecTaskPtr set_host(Host* host, std::string instance = "all");
  /** @return A pointer to the host of this instance of this ExecTask */
  Host* get_host(std::string instance = "instance_0") const { return host_.at(instance); }
  /** @param flops The new amount of flops this instance of this Task has to execute
   *  @return A Smart pointer to this ExecTask */
  ExecTaskPtr set_flops(double flops, std::string instance = "instance_0");
  /** @return The amount of flops this instance of this ExecTask has to execute */
  double get_flops(std::string instance = "instance_0") const { return get_amount(instance); }
  /** @param n The number of instances to add to this ExecTask */
  void add_instances(int n) override;
  /** @param n The number of isntances to remove from this ExecTask */
  void remove_instances(int n) override;
};

/** IoTask class */
class IoTask : public Task {
  Disk* disk_;
  Io::OpType type_;
  explicit IoTask(const std::string& name);
  void fire(std::string instance) override;

public:
  static IoTaskPtr init(const std::string& name);
  static IoTaskPtr init(const std::string& name, double bytes, Disk* disk, Io::OpType type);

  /** @param disk The new disk of this IoTask
   * @return A Smart pointer to this IoTask */
  IoTaskPtr set_disk(Disk* disk);
  /** @return A pointer to the disk of this IoTask */
  Disk* get_disk() const { return disk_; }
  /** @param bytes The new amount of bytes this IoTask has to write or read
   *  @return A Smart pointer to this IoTask */
  IoTaskPtr set_bytes(double bytes);
  /** @return The amount of bytes this IoTask has to write or read */
  double get_bytes() const { return get_amount("instance_0"); }
  /** @param type The type of operation this IoTask has to do
   *  @return A Smart pointer to this IoTask */
  IoTaskPtr set_op_type(Io::OpType type);
  /** @return The type of operation this IoTask has to to */
  Io::OpType get_op_type() const { return type_; }
};
} // namespace simgrid::s4u
#endif
