#ifndef SIMGRID_PLUGINS_OPERATION_H_
#define SIMGRID_PLUGINS_OPERATION_H_

#include <simgrid/s4u/Activity.hpp>
#include <simgrid/s4u/Io.hpp>
#include <xbt/Extendable.hpp>

#include <atomic>
#include <map>
#include <memory>
#include <set>

namespace simgrid::plugins {

class Operation;
using OperationPtr = boost::intrusive_ptr<Operation>;
XBT_PUBLIC void intrusive_ptr_release(Operation* o);
XBT_PUBLIC void intrusive_ptr_add_ref(Operation* o);
class ExecOp;
using ExecOpPtr = boost::intrusive_ptr<ExecOp>;
XBT_PUBLIC void intrusive_ptr_release(ExecOp* e);
XBT_PUBLIC void intrusive_ptr_add_ref(ExecOp* e);
class CommOp;
using CommOpPtr =  boost::intrusive_ptr<CommOp>;
XBT_PUBLIC void intrusive_ptr_release(CommOp* c);
XBT_PUBLIC void intrusive_ptr_add_ref(CommOp* c);
class IoOp;
using IoOpPtr =  boost::intrusive_ptr<IoOp>;
XBT_PUBLIC void intrusive_ptr_release(IoOp* i);
XBT_PUBLIC void intrusive_ptr_add_ref(IoOp* i);

struct ExtendedAttributeActivity {
  static simgrid::xbt::Extension<simgrid::s4u::Activity, ExtendedAttributeActivity> EXTENSION_ID;
  Operation* operation_;
};

class Operation {
  static bool inited_;
  std::set<Operation*> successors_                 = {};
  std::map<Operation*, unsigned int> predecessors_ = {};

  void add_predecessor(Operation* predecessor);
  void remove_predecessor(Operation* predecessor);
  bool ready_to_run() const;
  void receive(Operation* source);
  void complete();

protected:
  std::string name_;
  double amount_;
  int queued_execs_ = 0;
  int count_        = 0;
  bool working_     = false;
  s4u::ActivityPtr current_activity_;
  std::vector<std::function<void(Operation*)>> end_func_handlers_;
  std::vector<std::function<void(Operation*)>> start_func_handlers_;
  explicit Operation(const std::string& name);
  virtual ~Operation()   = default;
  virtual void execute() = 0;

  static xbt::signal<void(Operation*)> on_start;
  static xbt::signal<void(Operation*)> on_end;
  std::atomic_int_fast32_t refcount_{0};

public:
  static void init();
  const std::string& get_name() const { return name_; }
  const char* get_cname() const { return name_.c_str(); }
  void enqueue_execs(int n);
  void set_amount(double amount);
  double get_amount() const { return amount_; }
  void add_successor(OperationPtr op);
  void remove_successor(OperationPtr op);
  void remove_all_successors();
  const std::set<Operation*>& get_successors() const { return successors_ ;}
  void on_this_start(const std::function<void(Operation*)>& func);
  void on_this_end(const std::function<void(Operation*)>& func);
  int get_count() const;

  /** Add a callback fired before an operation activity start.
   * Triggered after the on_this_start function**/
  static void on_start_cb(const std::function<void(Operation*)>& cb) { on_start.connect(cb); }
  /** Add a callback fired after an operation activity end.
   * Triggered after the on_this_end function, but before
   * sending tokens to successors.**/
  static void on_end_cb(const std::function<void(Operation*)>& cb) { on_end.connect(cb); }

#ifndef DOXYGEN
  friend void intrusive_ptr_release(Operation* o)
  {
    if (o->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
      std::atomic_thread_fence(std::memory_order_acquire);
      delete o;
    }
  }
  friend void intrusive_ptr_add_ref(Operation* o) { o->refcount_.fetch_add(1, std::memory_order_relaxed); }
#endif
};

class ExecOp : public Operation {
  s4u::Host* host_;

  explicit ExecOp(const std::string& name);
  void execute() override;

public:
  static ExecOpPtr init(const std::string& name);
  static ExecOpPtr init(const std::string& name, double flops, s4u::Host* host);
  ExecOpPtr set_host(s4u::Host* host);
  s4u::Host* get_host() const { return host_; }
  ExecOpPtr set_flops(double flops);
  double get_flops() const { return get_amount(); }
  friend void inline intrusive_ptr_release(ExecOp* e) { intrusive_ptr_release(static_cast<Operation*>(e)); }
  friend void inline intrusive_ptr_add_ref(ExecOp* e) { intrusive_ptr_add_ref(static_cast<Operation*>(e)); }
};

class CommOp : public Operation {
  s4u::Host* source_;
  s4u::Host* destination_;

  explicit CommOp(const std::string& name);
  void execute() override;

public:
  static CommOpPtr init(const std::string& name);
  static CommOpPtr init(const std::string& name, double bytes, s4u::Host* source,
                        s4u::Host* destination);
  CommOpPtr set_source(s4u::Host* source);
  s4u::Host* get_source() const { return source_; }
  CommOpPtr set_destination(s4u::Host* destination);
  s4u::Host* get_destination() const { return destination_; }
  CommOpPtr set_bytes(double bytes);
  double get_bytes() const { return get_amount(); }
  friend void inline intrusive_ptr_release(CommOp* c) { intrusive_ptr_release(static_cast<Operation*>(c)); }
  friend void inline intrusive_ptr_add_ref(CommOp* c) { intrusive_ptr_add_ref(static_cast<Operation*>(c)); }
};

class IoOp : public Operation {
  s4u::Disk* disk_;
  s4u::Io::OpType type_;
  explicit IoOp(const std::string& name);
  void execute() override;

public:
  static IoOpPtr init(const std::string& name);
  static IoOpPtr init(const std::string& name, double bytes, s4u::Disk* disk, s4u::Io::OpType type);
  IoOpPtr set_disk(s4u::Disk* disk);
  s4u::Disk* get_disk() const { return disk_; }
  IoOpPtr set_bytes(double bytes);
  double get_bytes() { return get_amount(); }
  IoOpPtr set_op_type(s4u::Io::OpType type);

  friend void inline intrusive_ptr_release(IoOp* i) { intrusive_ptr_release(static_cast<Operation*>(i)); }
  friend void inline intrusive_ptr_add_ref(IoOp* i) { intrusive_ptr_add_ref(static_cast<Operation*>(i)); }
};
} // namespace simgrid::plugins
#endif
