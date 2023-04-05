#include <simgrid/Exception.hpp>
#include <simgrid/plugins/operation.hpp>
#include <simgrid/s4u/Comm.hpp>
#include <simgrid/s4u/Exec.hpp>

#include "src/simgrid/module.hpp"

SIMGRID_REGISTER_PLUGIN(operation, "Battery management", nullptr)
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(Operation, kernel, "Logging specific to the operation plugin");

namespace simgrid::plugins {
Operation::Operation(const std::string& name, double amount) : name_(name), amount_(amount) {}

std::string Operation::get_name()
{
  return this->name_;
}

void Operation::add_predecessor(Operation* predecessor)
{
  this->predecessors_[predecessor] = 0;
}

bool Operation::ready_to_run() const
{
  if (this->working_ or (this->iteration_count_ != -1 and this->iteration_count_ >= this->iteration_limit_))
    return false;
  for (auto const& [key, val] : this->predecessors_)
    if (val < 1)
      return false;
  return true;
}

void Operation::receive(Operation* source)
{
  auto it = this->predecessors_.find(source);
  it->second++;
  if (this->ready_to_run())
    this->execute();
}

void Operation::complete()
{
  working_ = false;
  call_end();
  for (auto const& op : this->successors_)
    op->receive(this);
  if (ready_to_run())
    execute();
}

void Operation::consume()
{
  for (auto const& [key, val] : predecessors_)
    predecessors_[key] = predecessors_[key] > 0 ? predecessors_[key] - 1 : 0;
}

void Operation::call_end()
{
  end_func(this);
}

void Operation::init()
{
  if (Operation::inited_)
    return;
  Operation::inited_                      = true;
  ExtendedAttributeActivity::EXTENSION_ID = simgrid::s4u::Activity::extension_create<ExtendedAttributeActivity>();
  simgrid::s4u::Activity::on_completion_cb([&](simgrid::s4u::Activity const& activity) {
    activity.extension<ExtendedAttributeActivity>()->operation_->complete();
  });
}

void Operation::set_iteration_limit(unsigned int n)
{
  iteration_limit_ = n;
}

void Operation::add_successor(OperationPtr op)
{
  successors_.insert(op.get());
  op->add_predecessor(this);
}

void Operation::on_end(std::function<void(Operation*)> func)
{
  end_func = func;
}

ExecOp::ExecOp(const std::string& name, double flops, simgrid::s4u::Host* host) : Operation(name, flops), host_(host) {}

ExecOpPtr ExecOp::create(const std::string& name, double flops, simgrid::s4u::Host* host)
{
  auto op = ExecOpPtr(new ExecOp(name, flops, host));
  return op;
}

void ExecOp::execute()
{
  iteration_count_++;
  working_ = true;
  consume();
  simgrid::s4u::ExecPtr exec = simgrid::s4u::Exec::init();
  exec->set_name(name_);
  exec->set_flops_amount(amount_);
  exec->set_host(host_);
  exec->start();
  exec->extension_set(new ExtendedAttributeActivity());
  exec->extension<ExtendedAttributeActivity>()->operation_ = this;
  current_activity_                                        = exec;
}

CommOp::CommOp(const std::string& name, double bytes, simgrid::s4u::Host* source, simgrid::s4u::Host* destination)
    : Operation(name, bytes), source_(source), destination_(destination)
{
}

CommOpPtr CommOp::create(const std::string& name, double bytes, simgrid::s4u::Host* source,
                         simgrid::s4u::Host* destination)
{
  auto op = CommOpPtr(new CommOp(name, bytes, source, destination));
  return op;
}

void CommOp::execute()
{
  iteration_count_++;
  working_ = true;
  consume();
  simgrid::s4u::CommPtr comm = simgrid::s4u::Comm::sendto_init(source_, destination_);
  comm->set_name(name_);
  comm->set_payload_size(amount_);
  comm->start();
  comm->extension_set(new ExtendedAttributeActivity());
  comm->extension<ExtendedAttributeActivity>()->operation_ = this;
  current_activity_                                        = comm;
}
} // namespace simgrid::plugins

simgrid::xbt::Extension<simgrid::s4u::Activity, simgrid::plugins::ExtendedAttributeActivity>
    simgrid::plugins::ExtendedAttributeActivity::EXTENSION_ID;
bool simgrid::plugins::Operation::inited_ = false;
