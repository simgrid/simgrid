#ifndef SIMGRID_PLUGINS_OPERATION_H_
#define SIMGRID_PLUGINS_OPERATION_H_

#include <simgrid/s4u/Activity.hpp>
#include <xbt/Extendable.hpp>

#include <map>
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
  std::function<void(Operation*)> end_func         = [](Operation*) {};

  void add_predecessor(Operation* predecessor);
  bool ready_to_run() const;
  void receive(Operation* source);
  void complete();

protected:
  std::string name_;
  double amount_;
  int iteration_limit_ = -1;
  int iteration_count_ = 0;
  bool working_        = false;
  simgrid::s4u::ActivityPtr current_activity_;
  Operation(const std::string& name, double amount);
  ~Operation() = default;
  void consume();
  void call_end();

public:
  static void init();
  std::string get_name();
  void set_iteration_limit(unsigned int n);
  void add_successor(OperationPtr op);
  void on_end(std::function<void(Operation*)> func);
  virtual void execute() = 0;
};

class ExecOp : public Operation {
private:
  simgrid::s4u::Host* host_;

  ExecOp(const std::string& name, double flops, simgrid::s4u::Host* host);

public:
  static ExecOpPtr create(const std::string& name, double flops, simgrid::s4u::Host* host);
  void execute();
};

class CommOp : public Operation {
private:
  simgrid::s4u::Host* source_;
  simgrid::s4u::Host* destination_;

  CommOp(const std::string& name, double bytes, simgrid::s4u::Host* source, simgrid::s4u::Host* destination);

public:
  static CommOpPtr create(const std::string& name, double bytes, simgrid::s4u::Host* source,
                          simgrid::s4u::Host* destination);
  void execute();
};
} // namespace simgrid::plugins
#endif