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

xbt::signal<void(Task*)> Task::on_start;
xbt::signal<void(Task*)> Task::on_end;

Task::Task(const std::string& name) : name_(name) {}

/**
 *  @param predecessor The Task to add.
 *  @brief Add a predecessor to this Task.
 */
void Task::add_predecessor(Task* predecessor)
{
  if (predecessors_.find(predecessor) == predecessors_.end())
    simgrid::kernel::actor::simcall_answered([this, predecessor] { predecessors_[predecessor] = 0; });
}

/**
 *  @param predecessor The Task to remove.
 *  @brief Remove a predecessor from this Task.
 */
void Task::remove_predecessor(Task* predecessor)
{
  simgrid::kernel::actor::simcall_answered([this, predecessor] { predecessors_.erase(predecessor); });
}

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
      for (auto& [key, val] : predecessors_)
        val--;
      enqueue_execs(1);
    }
  });
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
  simgrid::kernel::actor::simcall_answered([this] {
    working_ = false;
    count_++;
  });
  for (auto const& end_func : end_func_handlers_)
    end_func(this);
  Task::on_end(this);
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
  if (Task::inited_)
    return;
  Task::inited_                           = true;
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
 *  @param successor The Task to add.
 *  @brief Add a successor to this Task.
 *  @note It also adds this as a predecessor of successor.
 */
void Task::add_successor(TaskPtr successor)
{
  simgrid::kernel::actor::simcall_answered([this, successor] { successors_.insert(successor.get()); });
  successor->add_predecessor(this);
}

/** @ingroup plugin_task
 *  @param successor The Task to remove.
 *  @brief Remove a successor from this Task.
 *  @note It also remove this from the predecessors of successor.
 */
void Task::remove_successor(TaskPtr successor)
{
  simgrid::kernel::actor::simcall_answered([this, successor] { successors_.erase(successor.get()); });
  successor->remove_predecessor(this);
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
void Task::on_this_start(const std::function<void(Task*)>& func)
{
  simgrid::kernel::actor::simcall_answered([this, &func] { start_func_handlers_.push_back(func); });
}

/** @ingroup plugin_task
 *  @param func The function to set.
 *  @brief Set a function to be called after each execution.
 *  @note The function is called after the underlying Activity ends, but before sending tokens to successors.
 */
void Task::on_this_end(const std::function<void(Task*)>& func)
{
  simgrid::kernel::actor::simcall_answered([this, &func] { end_func_handlers_.push_back(func); });
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
  for (auto start_func : start_func_handlers_)
    start_func(this);
  Task::on_start(this);
  kernel::actor::simcall_answered([this] {
    working_      = true;
    queued_execs_ = std::max(queued_execs_ - 1, 0);
  });
  s4u::ExecPtr exec = s4u::Exec::init();
  exec->set_name(name_);
  exec->set_flops_amount(amount_);
  exec->set_host(host_);
  exec->start();
  exec->extension_set(new ExtendedAttributeActivity());
  exec->extension<ExtendedAttributeActivity>()->task_ = this;
  kernel::actor::simcall_answered([this, exec] { current_activity_ = exec; });
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
  for (auto start_func : start_func_handlers_)
    start_func(this);
  Task::on_start(this);
  kernel::actor::simcall_answered([this] {
    working_      = true;
    queued_execs_ = std::max(queued_execs_ - 1, 0);
  });
  s4u::CommPtr comm = s4u::Comm::sendto_init(source_, destination_);
  comm->set_name(name_);
  comm->set_payload_size(amount_);
  comm->start();
  comm->extension_set(new ExtendedAttributeActivity());
  comm->extension<ExtendedAttributeActivity>()->task_ = this;
  kernel::actor::simcall_answered([this, comm] { current_activity_ = comm; });
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
  for (auto start_func : start_func_handlers_)
    start_func(this);
  Task::on_start(this);
  kernel::actor::simcall_answered([this] {
    working_      = true;
    queued_execs_ = std::max(queued_execs_ - 1, 0);
  });
  s4u::IoPtr io = s4u::Io::init();
  io->set_name(name_);
  io->set_size(amount_);
  io->set_disk(disk_);
  io->set_op_type(type_);
  io->start();
  io->extension_set(new ExtendedAttributeActivity());
  io->extension<ExtendedAttributeActivity>()->task_ = this;
  kernel::actor::simcall_answered([this, io] { current_activity_ = io; });
}

} // namespace simgrid::plugins

simgrid::xbt::Extension<simgrid::s4u::Activity, simgrid::plugins::ExtendedAttributeActivity>
    simgrid::plugins::ExtendedAttributeActivity::EXTENSION_ID;
bool simgrid::plugins::Task::inited_ = false;
