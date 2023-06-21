#include <memory>
#include <simgrid/Exception.hpp>
#include <simgrid/s4u/Task.hpp>
#include <simgrid/s4u/Comm.hpp>
#include <simgrid/s4u/Exec.hpp>
#include <simgrid/s4u/Io.hpp>
#include <simgrid/simix.hpp>

#include "src/simgrid/module.hpp"

SIMGRID_REGISTER_PLUGIN(task, "Battery management", nullptr)
/**
  @beginrst


Tasks are designed to represent dataflows, i.e, graphs of Tasks.
Tasks can only be instancied using either
:cpp:func:`simgrid::s4u::ExecTask::init` or :cpp:func:`simgrid::s4u::CommTask::init`
An ExecTask is an Execution Task. Its underlying Activity is an :ref:`Exec <API_s4u_Exec>`.
A CommTask is a Communication Task. Its underlying Activity is a :ref:`Comm <API_s4u_Comm>`.

  @endrst
 */
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(Task, kernel, "Logging specific to the task plugin");

namespace simgrid::s4u {

Task::Task(const std::string& name) : name_(name) {}

/**
 *  @brief Return True if the Task can start a new Activity.
 *  @note The Task is ready if not already doing something and there is at least one execution waiting in queue.
 */
bool Task::ready_to_run() const
{
  return not working_ && queued_firings_ > 0;
}

/**
 *  @param source The sender.
 *  @brief Receive a token from another Task.
 *  @note Check upon reception if the Task has received a token from each of its predecessors,
 * and in this case consumes those tokens and enqueue an execution.
 */
void Task::receive(Task* source)
{
  XBT_DEBUG("Task %s received a token from %s", name_.c_str(), source->name_.c_str());
  auto source_count = predecessors_[source]++;
  if (tokens_received_.size() <= queued_firings_ + source_count)
    tokens_received_.push_back({});
  tokens_received_[queued_firings_ + source_count][source] = source->token_;
  bool enough_tokens = true;
  for (auto const& [key, val] : predecessors_)
    if (val < 1) {
      enough_tokens = false;
      break;
    }
  if (enough_tokens) {
    for (auto& [key, val] : predecessors_)
      val--;
    enqueue_firings(1);
  }
}

/**
 *  @brief Task routine when finishing an execution.
 *  @note Set its working status as false.
 * Add 1 to its count of finished executions.
 * Call the on_this_end func.
 * Fire on_end callback.
 * Send a token to each of its successors.
 * Start a new execution if possible.
 */
void Task::complete()
{
  xbt_assert(Actor::is_maestro());
  working_ = false;
  count_++;
  on_this_completion(this);
  on_completion(this);
  if (current_activity_)
    previous_activity_ = std::move(current_activity_);
  for (auto const& t : successors_)
    t->receive(this);
  if (ready_to_run())
    fire();
}

/** @param n The number of firings to enqueue.
 *  @brief Enqueue firing.
 *  @note Immediatly fire an activity if possible.
 */
void Task::enqueue_firings(int n)
{
  simgrid::kernel::actor::simcall_answered([this, n] {
    queued_firings_ += n;
    if (ready_to_run())
      fire();
  });
}

/** @param amount The amount to set.
 *  @brief Set the amout of work to do.
 *  @note Amount in flop for ExecTask and in bytes for CommTask.
 */
void Task::set_amount(double amount)
{
  simgrid::kernel::actor::simcall_answered([this, amount] { amount_ = amount; });
}

/** @param token The token to set.
 *  @brief Set the token to send to successors.
 *  @note The token is passed to each successor after the task end, i.e., after the on_end callback.
 */
void Task::set_token(std::shared_ptr<Token> token)
{
  simgrid::kernel::actor::simcall_answered([this, token] { token_ = token; });
}

/** @return Map of tokens received for the next execution.
 *  @note If there is no queued execution for this task the map might not exist or be partially empty.
 */
std::shared_ptr<Token> Task::get_next_token_from(TaskPtr t)
{
  return tokens_received_.front()[t];
}

void Task::fire() {
  on_this_start(this);
  on_start(this);
  working_ = true;
  queued_firings_ = std::max(queued_firings_ - 1, 0);
  if (tokens_received_.size() > 0)
    tokens_received_.pop_front();
}

/** @param successor The Task to add.
 *  @brief Add a successor to this Task.
 *  @note It also adds this as a predecessor of successor.
 */
void Task::add_successor(TaskPtr successor)
{
  simgrid::kernel::actor::simcall_answered([this, successor_p = successor.get()] {
    successors_.insert(successor_p);
    successor_p->predecessors_.try_emplace(this, 0);
  });
}

/** @param successor The Task to remove.
 *  @brief Remove a successor from this Task.
 *  @note It also remove this from the predecessors of successor.
 */
void Task::remove_successor(TaskPtr successor)
{
  simgrid::kernel::actor::simcall_answered([this, successor_p = successor.get()] {
    successor_p->predecessors_.erase(this);
    successors_.erase(successor_p);
  });
}

void Task::remove_all_successors()
{
  simgrid::kernel::actor::simcall_answered([this] {
    while (not successors_.empty()) {
      auto* successor = *(successors_.begin());
      successor->predecessors_.erase(this);
      successors_.erase(successor);
    }
  });
}

/**
 *  @brief Default constructor.
 */
ExecTask::ExecTask(const std::string& name) : Task(name) {}

/** @ingroup plugin_task
 *  @brief Smart Constructor.
 */
ExecTaskPtr ExecTask::init(const std::string& name)
{
  return ExecTaskPtr(new ExecTask(name));
}

/** @ingroup plugin_task
 *  @brief Smart Constructor.
 */
ExecTaskPtr ExecTask::init(const std::string& name, double flops, Host* host)
{
  return init(name)->set_flops(flops)->set_host(host);
}

/**
 *  @brief Do one execution of the Task.
 *  @note Call the on_this_start() func. Set its working status as true.
 *  Init and start the underlying Activity.
 */
void ExecTask::fire()
{
  Task::fire();
  auto exec = Exec::init()->set_name(get_name())->set_flops_amount(get_amount())->set_host(host_);
  exec->start();
  exec->on_this_completion_cb([this](Exec const&) { this->complete(); });
  set_current_activity(exec);
}

/** @ingroup plugin_task
 *  @param host The host to set.
 *  @brief Set a new host.
 */
ExecTaskPtr ExecTask::set_host(Host* host)
{
  kernel::actor::simcall_answered([this, host] { host_ = host; });
  return this;
}

/** @ingroup plugin_task
 *  @param flops The amount of flops to set.
 */
ExecTaskPtr ExecTask::set_flops(double flops)
{
  kernel::actor::simcall_answered([this, flops] { set_amount(flops); });
  return this;
}

/**
 *  @brief Default constructor.
 */
CommTask::CommTask(const std::string& name) : Task(name) {}

/** @ingroup plugin_task
 *  @brief Smart constructor.
 */
CommTaskPtr CommTask::init(const std::string& name)
{
  return CommTaskPtr(new CommTask(name));
}

/** @ingroup plugin_task
 *  @brief Smart constructor.
 */
CommTaskPtr CommTask::init(const std::string& name, double bytes, Host* source, Host* destination)
{
  return init(name)->set_bytes(bytes)->set_source(source)->set_destination(destination);
}

/**
 *  @brief Do one execution of the Task.
 *  @note Call the on_this_start() func. Set its working status as true.
 *  Init and start the underlying Activity.
 */
void CommTask::fire()
{
  Task::fire();
  auto comm = Comm::sendto_init(source_, destination_)->set_name(get_name())->set_payload_size(get_amount());
  comm->start();
  comm->on_this_completion_cb([this](Comm const&) { this->complete(); });
  set_current_activity(comm);
}

/** @ingroup plugin_task
 *  @param source The host to set.
 *  @brief Set a new source host.
 */
CommTaskPtr CommTask::set_source(Host* source)
{
  kernel::actor::simcall_answered([this, source] { source_ = source; });
  return this;
}

/** @ingroup plugin_task
 *  @param destination The host to set.
 *  @brief Set a new destination host.
 */
CommTaskPtr CommTask::set_destination(Host* destination)
{
  kernel::actor::simcall_answered([this, destination] { destination_ = destination; });
  return this;
}

/** @ingroup plugin_task
 *  @param bytes The amount of bytes to set.
 */
CommTaskPtr CommTask::set_bytes(double bytes)
{
  kernel::actor::simcall_answered([this, bytes] { set_amount(bytes); });
  return this;
}

/**
 *  @brief Default constructor.
 */
IoTask::IoTask(const std::string& name) : Task(name) {}

/** @ingroup plugin_task
 *  @brief Smart Constructor.
 */
IoTaskPtr IoTask::init(const std::string& name)
{
  return IoTaskPtr(new IoTask(name));
}

/** @ingroup plugin_task
 *  @brief Smart Constructor.
 */
IoTaskPtr IoTask::init(const std::string& name, double bytes, Disk* disk, Io::OpType type)
{
  return init(name)->set_bytes(bytes)->set_disk(disk)->set_op_type(type);
}

/** @ingroup plugin_task
 *  @param disk The disk to set.
 *  @brief Set a new disk.
 */
IoTaskPtr IoTask::set_disk(Disk* disk)
{
  kernel::actor::simcall_answered([this, disk] { disk_ = disk; });
  return this;
}

/** @ingroup plugin_task
 *  @param bytes The amount of bytes to set.
 */
IoTaskPtr IoTask::set_bytes(double bytes)
{
  kernel::actor::simcall_answered([this, bytes] { set_amount(bytes); });
  return this;
}

/** @ingroup plugin_task */
IoTaskPtr IoTask::set_op_type(Io::OpType type)
{
  kernel::actor::simcall_answered([this, type] { type_ = type; });
  return this;
}

void IoTask::fire()
{
  Task::fire();
  auto io = Io::init()->set_name(get_name())->set_size(get_amount())->set_disk(disk_)->set_op_type(type_);
  io->start();
  io->on_this_completion_cb([this](Io const&) { this->complete(); });
  set_current_activity(io);
}

} // namespace simgrid::s4u
