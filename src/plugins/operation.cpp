#include <simgrid/Exception.hpp>
#include <simgrid/plugins/operation.hpp>
#include <simgrid/s4u/Comm.hpp>
#include <simgrid/s4u/Exec.hpp>
#include <simgrid/simix.hpp>

#include "src/simgrid/module.hpp"

SIMGRID_REGISTER_PLUGIN(operation, "Battery management", nullptr)
/** @defgroup plugin_operation plugin_operation Plugin Operation

  @beginrst

This is the operation plugin, enabling management of Operations.
To activate this plugin, first call :cpp:func:`Operation::init`.

Operations are designed to represent workflows, i.e, graphs of Operations.
Operations can only be instancied using either
:cpp:func:`simgrid::plugins::ExecOp::create` or :cpp:func:`simgrid::plugins::CommOp::create`
An ExecOp is an Execution Operation. Its underlying Activity is an :ref:`Exec <API_s4u_Exec>`.
A CommOp is a Communication Operation. Its underlying Activity is a :ref:`Comm <API_s4u_Comm>`.

  @endrst
 */
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(Operation, kernel, "Logging specific to the operation plugin");

namespace simgrid::plugins {

xbt::signal<void(Operation*)> Operation::on_start;
xbt::signal<void(Operation*)> Operation::on_end;

Operation::Operation(const std::string& name, double amount) : name_(name), amount_(amount) {}

std::string Operation::get_name()
{
  return name_;
}

/**
 *  @param predecessor The Operation to add.
 *  @brief Add a predecessor to this Operation.
 */
void Operation::add_predecessor(Operation* predecessor)
{
  if (predecessors_.find(predecessor) == predecessors_.end())
    simgrid::kernel::actor::simcall_answered([this, predecessor] { predecessors_[predecessor] = 0; });
}

/**
 *  @param predecessor The Operation to remove.
 *  @brief Remove a predecessor from this Operation.
 */
void Operation::remove_predecessor(Operation* predecessor)
{
  simgrid::kernel::actor::simcall_answered([this, predecessor] { predecessors_.erase(predecessor); });
}

/**
 *  @brief Return True if the Operation can start a new Activity.
 *  @note The Operation is ready if not already doing something and there is at least one execution waiting in queue.
 */
bool Operation::ready_to_run() const
{
  if (working_ or queued_execs_ <= 0)
    return false;
  else
    return true;
}

/**
 *  @param source The sender.
 *  @brief Receive a token from another Operation.
 *  @note Check upon reception if the Operation has received a token from each of its predecessors,
 * and in this case consumes those tokens and enqueue an execution.
 */
void Operation::receive(Operation* source)
{
  XBT_DEBUG("Operation %s received a token from %s", name_.c_str(), source->name_.c_str());
  auto it = predecessors_.find(source);
  simgrid::kernel::actor::simcall_answered([this, it] {
    it->second++;
    bool enough_tokens = true;
    for (auto const& [key, val] : predecessors_)
      if (val < 1) {
        enough_tokens = false;
        break;
      }
    if (enough_tokens) {
      for (auto [key, val] : predecessors_)
        val--;
      enqueue_execs(1);
    }
  });
}

/**
 *  @brief Operation routine when finishing an execution.
 *  @note Set its working status as false.
 * Add 1 to its count of finished executions.
 * Call the on_this_end func.
 * Fire on_end callback.
 * Send a token to each of its successors.
 * Start a new execution if possible.
 */
void Operation::complete()
{
  simgrid::kernel::actor::simcall_answered([this] {
    working_ = false;
    count_++;
  });
  end_func_(this);
  Operation::on_end(this);
  for (auto const& op : successors_)
    op->receive(this);
  if (ready_to_run())
    execute();
}

/** @ingroup plugin_operation
 *  @brief Init the Operation plugin.
 *  @note Add a completion callback to all Activities to call Operation::complete().
 */
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

/** @ingroup plugin_operation
 *  @param n The number of executions to enqueue.
 *  @brief Enqueue executions.
 *  @note Immediatly starts an execution if possible.
 */
void Operation::enqueue_execs(int n)
{
  simgrid::kernel::actor::simcall_answered([this, n] {
    queued_execs_ += n;
    if (ready_to_run())
      execute();
  });
}

/** @ingroup plugin_operation
 *  @param amount The amount to set.
 *  @brief Set the amout of work to do.
 *  @note Amount in flop for ExecOp and in bytes for CommOp.
 */
void Operation::set_amount(double amount)
{
  simgrid::kernel::actor::simcall_answered([this, amount] { amount_ = amount; });
}

/** @ingroup plugin_operation
 *  @param successor The Operation to add.
 *  @brief Add a successor to this Operation.
 *  @note It also adds this as a predecessor of successor.
 */
void Operation::add_successor(OperationPtr successor)
{
  simgrid::kernel::actor::simcall_answered([this, successor] { successors_.insert(successor.get()); });
  successor->add_predecessor(this);
}

/** @ingroup plugin_operation
 *  @param successor The Operation to remove.
 *  @brief Remove a successor from this Operation.
 *  @note It also remove this from the predecessors of successor.
 */
void Operation::remove_successor(OperationPtr successor)
{
  simgrid::kernel::actor::simcall_answered([this, successor] { successors_.erase(successor.get()); });
  successor->remove_predecessor(this);
}

/** @ingroup plugin_operation
 *  @param func The function to set.
 *  @brief Set a function to be called before each execution.
 *  @note The function is called before the underlying Activity starts.
 */
void Operation::on_this_start(std::function<void(Operation*)> func)
{
  simgrid::kernel::actor::simcall_answered([this, func] { start_func_ = func; });
}

/** @ingroup plugin_operation
 *  @param func The function to set.
 *  @brief Set a function to be called after each execution.
 *  @note The function is called after the underlying Activity ends, but before sending tokens to successors.
 */
void Operation::on_this_end(std::function<void(Operation*)> func)
{
  simgrid::kernel::actor::simcall_answered([this, func] { end_func_ = func; });
}

/** @ingroup plugin_operation
 *  @brief Return the number of completed executions.
 */
int Operation::get_count()
{
  return count_;
}

/**
 *  @brief Default constructor.
 */
ExecOp::ExecOp(const std::string& name, double flops, simgrid::s4u::Host* host) : Operation(name, flops), host_(host) {}

/** @ingroup plugin_operation
 *  @brief Smart Constructor.
 */
ExecOpPtr ExecOp::create(const std::string& name, double flops, simgrid::s4u::Host* host)
{
  auto op = ExecOpPtr(new ExecOp(name, flops, host));
  return op;
}

/**
 *  @brief Do one execution of the Operation.
 *  @note Call the on_this_start() func. Set its working status as true.
 *  Create and start the underlying Activity.
 */
void ExecOp::execute()
{
  start_func_(this);
  Operation::on_start(this);
  simgrid::kernel::actor::simcall_answered([this] {
    working_      = true;
    queued_execs_ = std::max(queued_execs_ - 1, 0);
  });
  simgrid::s4u::ExecPtr exec = simgrid::s4u::Exec::init();
  exec->set_name(name_);
  exec->set_flops_amount(amount_);
  exec->set_host(host_);
  exec->start();
  exec->extension_set(new ExtendedAttributeActivity());
  exec->extension<ExtendedAttributeActivity>()->operation_ = this;
  simgrid::kernel::actor::simcall_answered([this, exec] { current_activity_ = exec; });
}

/** @ingroup plugin_operation
 *  @param host The host to set.
 *  @brief Set a new host.
 */
void ExecOp::set_host(simgrid::s4u::Host* host)
{
  simgrid::kernel::actor::simcall_answered([this, host] { host_ = host; });
}

/**
 *  @brief Default constructor.
 */
CommOp::CommOp(const std::string& name, double bytes, simgrid::s4u::Host* source, simgrid::s4u::Host* destination)
    : Operation(name, bytes), source_(source), destination_(destination)
{
}

/** @ingroup plugin_operation
 *  @brief Smart constructor.
 */
CommOpPtr CommOp::create(const std::string& name, double bytes, simgrid::s4u::Host* source,
                         simgrid::s4u::Host* destination)
{
  auto op = CommOpPtr(new CommOp(name, bytes, source, destination));
  return op;
}

/**
 *  @brief Do one execution of the Operation.
 *  @note Call the on_this_start() func. Set its working status as true.
 *  Create and start the underlying Activity.
 */
void CommOp::execute()
{
  start_func_(this);
  Operation::on_start(this);
  simgrid::kernel::actor::simcall_answered([this] {
    working_      = true;
    queued_execs_ = std::max(queued_execs_ - 1, 0);
  });
  simgrid::s4u::CommPtr comm = simgrid::s4u::Comm::sendto_init(source_, destination_);
  comm->set_name(name_);
  comm->set_payload_size(amount_);
  comm->start();
  comm->extension_set(new ExtendedAttributeActivity());
  comm->extension<ExtendedAttributeActivity>()->operation_ = this;
  simgrid::kernel::actor::simcall_answered([this, comm] { current_activity_ = comm; });
}

/** @ingroup plugin_operation
 *  @param source The host to set.
 *  @brief Set a new source host.
 */
void CommOp::set_source(simgrid::s4u::Host* source)
{
  simgrid::kernel::actor::simcall_answered([this, source] { source_ = source; });
}

/** @ingroup plugin_operation
 *  @param destination The host to set.
 *  @brief Set a new destination host.
 */
void CommOp::set_destination(simgrid::s4u::Host* destination)
{
  simgrid::kernel::actor::simcall_answered([this, destination] { destination_ = destination; });
}

} // namespace simgrid::plugins

simgrid::xbt::Extension<simgrid::s4u::Activity, simgrid::plugins::ExtendedAttributeActivity>
    simgrid::plugins::ExtendedAttributeActivity::EXTENSION_ID;
bool simgrid::plugins::Operation::inited_ = false;
