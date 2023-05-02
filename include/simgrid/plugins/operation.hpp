#ifndef SIMGRID_PLUGINS_OPERATION_H_
#define SIMGRID_PLUGINS_OPERATION_H_

#include <simgrid/s4u/Activity.hpp>
#include <xbt/Extendable.hpp>

#include <map>
#include <memory>
#include <set>

namespace simgrid::plugins {

class Operation;
using OperationPtr = std::shared_ptr<Operation>;
class ExecOp;
using ExecOpPtr = std::shared_ptr<ExecOp>;
class CommOp;
using CommOpPtr = std::shared_ptr<CommOp>;

class ExtendedAttributeActivity {
public:
  static simgrid::xbt::Extension<simgrid::s4u::Activity, ExtendedAttributeActivity> EXTENSION_ID;
  Operation* operation_;

  ExtendedAttributeActivity(){};
};

class Operation {
private:
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
  simgrid::s4u::ActivityPtr current_activity_;
  std::function<void(Operation*)> end_func_   = [](Operation*) {};
  std::function<void(Operation*)> start_func_ = [](Operation*) {};
  Operation(const std::string& name);
  virtual ~Operation()   = default;
  virtual void execute() = 0;

  static xbt::signal<void(Operation*)> on_start;
  static xbt::signal<void(Operation*)> on_end;

public:
  static void init();
  std::string get_name();
  void enqueue_execs(int n);
  void set_amount(double amount);
  void add_successor(OperationPtr op);
  void remove_successor(OperationPtr op);
  void on_this_start(std::function<void(Operation*)> func);
  void on_this_end(std::function<void(Operation*)> func);
  int get_count();

  /** Add a callback fired before an operation activity start.
   * Triggered after the on_this_start function**/
  static void on_start_cb(const std::function<void(Operation*)>& cb) { on_start.connect(cb); }
  /** Add a callback fired after an operation activity end.
   * Triggered after the on_this_end function, but before
   * sending tokens to successors.**/
  static void on_end_cb(const std::function<void(Operation*)>& cb) { on_end.connect(cb); }
};

class ExecOp : public Operation {
private:
  simgrid::s4u::Host* host_;

  ExecOp(const std::string& name);
  void execute() override;

public:
  static ExecOpPtr init(const std::string& name);
  static ExecOpPtr init(const std::string& name, double flops, simgrid::s4u::Host* host);
  void set_host(simgrid::s4u::Host* host);
  void set_flops(double flops);
};

class CommOp : public Operation {
private:
  simgrid::s4u::Host* source_;
  simgrid::s4u::Host* destination_;

  CommOp(const std::string& name);
  void execute() override;

public:
  static CommOpPtr init(const std::string& name);
  static CommOpPtr init(const std::string& name, double bytes, simgrid::s4u::Host* source,
                        simgrid::s4u::Host* destination);
  void set_source(simgrid::s4u::Host* source);
  void set_destination(simgrid::s4u::Host* destination);
  void set_bytes(double bytes);
};
} // namespace simgrid::plugins
#endif
