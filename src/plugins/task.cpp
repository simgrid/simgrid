#include <simgrid/Exception.hpp>
#include <simgrid/plugins/task.hpp>
#include <simgrid/s4u/Comm.hpp>
#include <simgrid/s4u/Exec.hpp>
#include <simgrid/s4u/Io.hpp>
#include <simgrid/simix.hpp>

#include "src/simgrid/module.hpp"

SIMGRID_REGISTER_PLUGIN(task, "Battery management", nullptr)
/** @defgroup plugin_task plugin_task Plugin Task

  @beginrst

This is the task plugin, enabling management of Tasks.
To activate this plugin, first call :cpp:func:`Task::init`.

Tasks are designed to represent dataflows, i.e, graphs of Tasks.
Tasks can only be instancied using either
:cpp:func:`simgrid::plugins::ExecTask::init` or :cpp:func:`simgrid::plugins::CommTask::init`
An ExecTask is an Execution Task. Its underlying Activity is an :ref:`Exec <API_s4u_Exec>`.
A CommTask is a Communication Task. Its underlying Activity is a :ref:`Comm <API_s4u_Comm>`.

  @endrst
 */
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(Task, kernel, "Logging specific to the task plugin");

namespace simgrid::plugins {

xbt::Extension<s4u::Activity, ExtendedAttributeActivity> ExtendedAttributeActivity::EXTENSION_ID;

xbt::signal<void(Task*)> Task::on_start;
xbt::signal<void(Task*)> Task::on_end;

Task::Task(const std::string& name) : name_(name) {}

/**
 *  @brief Return True if the Task can start a new Activity.
 *  @note The Task is ready if not already doing something and there is at least one execution waiting in queue.
 */
bool Task::ready_to_run() const
{
  return not working_ && queued_execs_ > 0;
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
  if (tokens_received_.size() <= queued_execs_ + source_count)
    tokens_received_.push_back({});
  tokens_received_[queued_execs_ + source_count][source] = source->token_;
  bool enough_tokens = true;
  for (auto const& [key, val] : predecessors_)
    if (val < 1) {
      enough_tokens = false;
      break;
    }
  if (enough_tokens) {
    for (auto& [key, val] : predecessors_)
      val--;
    enqueue_execs(1);
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
  xbt_assert(s4u::Actor::is_maestro());
  working_ = false;
  count_++;
  on_this_end_(this);
  Task::on_end(this);
  if (current_activity_)
    previous_activity_ = std::move(current_activity_);
  for (auto const& t : successors_)
    t->receive(this);
  if (ready_to_run())
    fire();
}

/** @ingroup plugin_task
 *  @brief Init the Task plugin.
 *  @note Add a completion callback to all Activities to call Task::complete().
 */
void Task::init()
{
  static bool inited = false;
  if (inited)
    return;

  inited                                  = true;
  ExtendedAttributeActivity::EXTENSION_ID = simgrid::s4u::Activity::extension_create<ExtendedAttributeActivity>();
  simgrid::s4u::Exec::on_completion_cb(
      [](simgrid::s4u::Exec const& exec) { exec.extension<ExtendedAttributeActivity>()->task_->complete(); });
  simgrid::s4u::Comm::on_completion_cb(
      [](simgrid::s4u::Comm const& comm) { comm.extension<ExtendedAttributeActivity>()->task_->complete(); });
  simgrid::s4u::Io::on_completion_cb(
      [](simgrid::s4u::Io const& io) { io.extension<ExtendedAttributeActivity>()->task_->complete(); });
}

/** @ingroup plugin_task
 *  @param n The number of executions to enqueue.
 *  @brief Enqueue executions.
 *  @note Immediatly starts an execution if possible.
 */
void Task::enqueue_execs(int n)
{
  simgrid::kernel::actor::simcall_answered([this, n] {
    queued_execs_ += n;
    if (ready_to_run())
      fire();
  });
}

/** @ingroup plugin_task
 *  @param amount The amount to set.
 *  @brief Set the amout of work to do.
 *  @note Amount in flop for ExecTask and in bytes for CommTask.
 */
void Task::set_amount(double amount)
{
  simgrid::kernel::actor::simcall_answered([this, amount] { amount_ = amount; });
}

/** @ingroup plugin_task
 *  @param token The token to set.
 *  @brief Set the token to send to successors.
 *  @note The token is passed to each successor after the task end, i.e., after the on_end callback.
 */
void Task::set_token(std::shared_ptr<void> token)
{
  simgrid::kernel::actor::simcall_answered([this, token] { token_ = token; });
}

/** @ingroup plugin_task
 *  @return Map of tokens received for the next execution.
 *  @note If there is no queued execution for this task the map might not exist or be partially empty.
 */
std::map<TaskPtr, std::shared_ptr<void>> Task::get_tokens() const
{
  return tokens_received_.front();
}

/** @ingroup plugin_task
 *  @param successor The Task to add.
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

/** @ingroup plugin_task
 *  @param successor The Task to remove.
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

/** @ingroup plugin_task
 *  @param func The function to set.
 *  @brief Set a function to be called before each execution.
 *  @note The function is called before the underlying Activity starts.
 */
void Task::on_this_start_cb(const std::function<void(Task*)>& func)
{
  simgrid::kernel::actor::simcall_answered([this, &func] { on_this_start_.connect(func); });
}

/** @ingroup plugin_task
 *  @param func The function to set.
 *  @brief Set a function to be called after each execution.
 *  @note The function is called after the underlying Activity ends, but before sending tokens to successors.
 */
void Task::on_this_end_cb(const std::function<void(Task*)>& func)
{
  simgrid::kernel::actor::simcall_answered([this, &func] { on_this_end_.connect(func); });
}

/** @ingroup plugin_task
 *  @brief Return the number of completed executions.
 */
int Task::get_count() const
{
  return count_;
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
ExecTaskPtr ExecTask::init(const std::string& name, double flops, s4u::Host* host)
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
  on_this_start_(this);
  Task::on_start(this);
  working_          = true;
  queued_execs_     = std::max(queued_execs_ - 1, 0);
  if (tokens_received_.size() > 0)
      tokens_received_.pop_front();
  s4u::ExecPtr exec = s4u::Exec::init();
  exec->set_name(name_);
  exec->set_flops_amount(amount_);
  exec->set_host(host_);
  exec->start();
  exec->extension_set(new ExtendedAttributeActivity());
  exec->extension<ExtendedAttributeActivity>()->task_ = this;
  current_activity_                                   = exec;
}

/** @ingroup plugin_task
 *  @param host The host to set.
 *  @brief Set a new host.
 */
ExecTaskPtr ExecTask::set_host(s4u::Host* host)
{
  kernel::actor::simcall_answered([this, host] { host_ = host; });
  return this;
}

/** @ingroup plugin_task
 *  @param flops The amount of flops to set.
 */
ExecTaskPtr ExecTask::set_flops(double flops)
{
  kernel::actor::simcall_answered([this, flops] { amount_ = flops; });
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
CommTaskPtr CommTask::init(const std::string& name, double bytes, s4u::Host* source, s4u::Host* destination)
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
  on_this_start_(this);
  Task::on_start(this);
  working_          = true;
  queued_execs_     = std::max(queued_execs_ - 1, 0);
  if (tokens_received_.size() > 0)
      tokens_received_.pop_front();
  s4u::CommPtr comm = s4u::Comm::sendto_init(source_, destination_);
  comm->set_name(name_);
  comm->set_payload_size(amount_);
  comm->start();
  comm->extension_set(new ExtendedAttributeActivity());
  comm->extension<ExtendedAttributeActivity>()->task_ = this;
  current_activity_                                   = comm;
}

/** @ingroup plugin_task
 *  @param source The host to set.
 *  @brief Set a new source host.
 */
CommTaskPtr CommTask::set_source(s4u::Host* source)
{
  kernel::actor::simcall_answered([this, source] { source_ = source; });
  return this;
}

/** @ingroup plugin_task
 *  @param destination The host to set.
 *  @brief Set a new destination host.
 */
CommTaskPtr CommTask::set_destination(s4u::Host* destination)
{
  kernel::actor::simcall_answered([this, destination] { destination_ = destination; });
  return this;
}

/** @ingroup plugin_task
 *  @param bytes The amount of bytes to set.
 */
CommTaskPtr CommTask::set_bytes(double bytes)
{
  kernel::actor::simcall_answered([this, bytes] { amount_ = bytes; });
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
IoTaskPtr IoTask::init(const std::string& name, double bytes, s4u::Disk* disk, s4u::Io::OpType type)
{
  return init(name)->set_bytes(bytes)->set_disk(disk)->set_op_type(type);
}

/** @ingroup plugin_task
 *  @param disk The disk to set.
 *  @brief Set a new disk.
 */
IoTaskPtr IoTask::set_disk(s4u::Disk* disk)
{
  kernel::actor::simcall_answered([this, disk] { disk_ = disk; });
  return this;
}

/** @ingroup plugin_task
 *  @param bytes The amount of bytes to set.
 */
IoTaskPtr IoTask::set_bytes(double bytes)
{
  kernel::actor::simcall_answered([this, bytes] { amount_ = bytes; });
  return this;
}

/** @ingroup plugin_task */
IoTaskPtr IoTask::set_op_type(s4u::Io::OpType type)
{
  kernel::actor::simcall_answered([this, type] { type_ = type; });
  return this;
}

void IoTask::fire()
{
  on_this_start_(this);
  Task::on_start(this);
  working_      = true;
  queued_execs_ = std::max(queued_execs_ - 1, 0);
  if (tokens_received_.size() > 0)
      tokens_received_.pop_front();
  s4u::IoPtr io = s4u::Io::init();
  io->set_name(name_);
  io->set_size(amount_);
  io->set_disk(disk_);
  io->set_op_type(type_);
  io->start();
  io->extension_set(new ExtendedAttributeActivity());
  io->extension<ExtendedAttributeActivity>()->task_ = this;
  current_activity_                                 = io;
}

} // namespace simgrid::plugins
