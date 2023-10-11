#include <cstddef>
#include <memory>
#include <simgrid/Exception.hpp>
#include <simgrid/s4u/Activity.hpp>
#include <simgrid/s4u/Comm.hpp>
#include <simgrid/s4u/Disk.hpp>
#include <simgrid/s4u/Exec.hpp>
#include <simgrid/s4u/Io.hpp>
#include <simgrid/s4u/Task.hpp>
#include <simgrid/simix.hpp>
#include <string>
#include <xbt/asserts.h>

#include "src/simgrid/module.hpp"

SIMGRID_REGISTER_PLUGIN(task, "Battery management", nullptr)
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(Task, kernel, "Logging specific to the task plugin");

namespace simgrid::s4u {

Task::Task(const std::string& name) : name_(name) {}

/** @param instance The Task instance to check.
 *  @brief Return True if this Task instance can start.
 */
bool Task::ready_to_run(std::string instance)
{
  return running_instances_[instance] < parallelism_degree_[instance] && queued_firings_[instance] > 0;
}

/** @param source The sender.
 *  @brief Receive a token from another Task.
 *  @note Check upon reception if the Task has received a token from each of its predecessors,
 * and in this case consumes those tokens and enqueue an execution.
 */
void Task::receive(Task* source)
{
  XBT_DEBUG("Task %s received a token from %s", name_.c_str(), source->name_.c_str());
  predecessors_[source]++;
  if (source->token_ != nullptr)
    tokens_received_[source].push_back(source->token_);
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

/** @param instance The Taks instance to complete.
 *  @brief Task instance routine when finishing an execution of an instance.
 *  @note The dispatcher instance enqueues a firing for the next instance.
 *        The collector instance triggers the on_completion signals and sends tokens to successors.
 *        Others instances enqueue a firing of the collector instance.
 */
void Task::complete(std::string instance)
{
  xbt_assert(Actor::is_maestro());
  running_instances_[instance]--;
  count_[instance]++;
  if (instance == "collector") {
    on_this_completion(this);
    on_completion(this);
    for (auto const& t : successors_)
      t->receive(this);
  } else if (instance == "dispatcher") {
    auto next_instance = load_balancing_function_();
    xbt_assert(next_instance != "dispatcher" and next_instance != "collector", "Invalid instance selected: %s",
               next_instance.c_str());
    queued_firings_[next_instance] = queued_firings_.at(next_instance) + 1;
    while (ready_to_run(next_instance))
      fire(next_instance);
  } else {
    queued_firings_["collector"]++;
    while (ready_to_run("collector"))
      fire("collector");
  }
  if (ready_to_run(instance))
    fire(instance);
}

/** @param n The new parallelism degree of the Task instance.
 *  @param instance The Task instance to modify.
 *  @note You can use instance "all" to modify the parallelism degree of all instances of this Task.
 *        When increasing the degree new executions are started if there is queued firings.
 *        When decreasing the degree instances already running are NOT stopped.
 */
void Task::set_parallelism_degree(int n, std::string instance)
{
  xbt_assert(n > 0, "Parallelism degree must be above 0.");
  simgrid::kernel::actor::simcall_answered([this, n, &instance] {
    if (instance == "all") {
      for (auto& [key, value] : parallelism_degree_) {
        parallelism_degree_[key] = n;
        while (ready_to_run(key))
          fire(key);
      }
    } else {
      parallelism_degree_[instance] = n;
      while (ready_to_run(instance))
        fire(instance);
    }
  });
}

/** @param bytes The internal bytes of the Task instance.
 *  @param instance The Task instance to modify.
 *  @note Internal bytes are used for Comms between the dispatcher and instance_n,
 *        and between instance_n and the collector if they are not on the same host.
 */
void Task::set_internal_bytes(int bytes, std::string instance)
{
  simgrid::kernel::actor::simcall_answered([this, bytes, &instance] { internal_bytes_to_send_[instance] = bytes; });
}

/** @param func The load balancing function.
 *  @note The dispatcher uses this function to determine which instance to trigger next.
 */
void Task::set_load_balancing_function(std::function<std::string()> func)
{
  simgrid::kernel::actor::simcall_answered([this, func] { load_balancing_function_ = func; });
}

/** @param n The number of firings to enqueue.
 */
void Task::enqueue_firings(int n)
{
  simgrid::kernel::actor::simcall_answered([this, n] {
    queued_firings_["dispatcher"] += n;
    while (ready_to_run("dispatcher"))
      fire("dispatcher");
  });
}

/** @param name The new name to set.
 *  @brief Set the name of the Task.
 */
void Task::set_name(std::string name)
{
  name_ = name;
}

/** @param amount The amount to set.
 *  @param instance The Task instance to modify.
 *  @note Amount in flop for ExecTask and in bytes for CommTask.
 */
void Task::set_amount(double amount, std::string instance)
{
  simgrid::kernel::actor::simcall_answered([this, amount, &instance] { amount_[instance] = amount; });
}

/** @param token The token to set.
 *  @brief Set the token to send to successors.
 *  @note The token is passed to each successor after the Task instance collector end, i.e., after the on_completion
 * callback.
 */
void Task::set_token(std::shared_ptr<Token> token)
{
  simgrid::kernel::actor::simcall_answered([this, token] { token_ = token; });
}

/** @param t The Task to deque a token from.
 */
void Task::deque_token_from(TaskPtr t)
{
  simgrid::kernel::actor::simcall_answered([this, &t] { tokens_received_[t].pop_front(); });
}

void Task::fire(std::string instance)
{
  if ((int)current_activities_[instance].size() > parallelism_degree_[instance]) {
    current_activities_[instance].pop_front();
  }
  if (instance != "dispatcher" and instance != "collector") {
    on_this_start(this);
    on_start(this);
  }
  running_instances_[instance]++;
  queued_firings_[instance] = std::max(queued_firings_[instance] - 1, 0);
}

/** @param successor The Task to add as a successor.
 *  @note It also adds this as a predecessor of successor.
 */
void Task::add_successor(TaskPtr successor)
{
  simgrid::kernel::actor::simcall_answered([this, successor_p = successor.get()] {
    successors_.insert(successor_p);
    successor_p->predecessors_.try_emplace(this, 0);
  });
}

/** @param successor The Task to remove from the successors of this Task.
 *  @note It also remove this from the predecessors of successor.
 */
void Task::remove_successor(TaskPtr successor)
{
  simgrid::kernel::actor::simcall_answered([this, successor_p = successor.get()] {
    successor_p->predecessors_.erase(this);
    successors_.erase(successor_p);
  });
}

/** @brief Remove all successors from this Task.
 */
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

/** @param n The number of instances to add to this Task (>=0).
 *  @note Instances goes always from instance_0 to instance_x,
 *        where x is the current number of instance.
 */
void Task::add_instances(int n)
{
  xbt_assert(n >= 0, "Cannot add a negative number of instances (provided: %d)", n);
  int instance_count = (int)amount_.size() - 2;
  for (int i = instance_count; i < n + instance_count; i++) {
    amount_["instance_" + std::to_string(i)]                 = amount_.at("instance_0");
    queued_firings_["instance_" + std::to_string(i)]         = 0;
    running_instances_["instance_" + std::to_string(i)]      = 0;
    count_["instance_" + std::to_string(i)]                  = 0;
    parallelism_degree_["instance_" + std::to_string(i)]     = parallelism_degree_.at("instance_0");
    current_activities_["instance_" + std::to_string(i)]     = {};
    internal_bytes_to_send_["instance_" + std::to_string(i)] = internal_bytes_to_send_.at("instance_0");
    ;
  }
}

/** @param n The number of instances to remove from this Task (>=0).
 *  @note Instances goes always from instance_0 to instance_x,
 *        where x is the current number of instance.
 *        Running instances cannot be removed.
 */
void Task::remove_instances(int n)
{
  int instance_count = (int)amount_.size() - 2;
  xbt_assert(n >= 0, "Cannot remove a negative number of instances (provided: %d)", n);
  xbt_assert(instance_count - n > 0, "The number of instances must be above 0 (instances: %d, provided: %d)",
             instance_count, n);
  for (int i = instance_count - 1; i >= instance_count - n; i--) {
    xbt_assert(running_instances_.at("instance_" + std::to_string(i)) == 0,
               "Cannot remove a running instance (instances: %d)", i);
    amount_.erase("instance_" + std::to_string(i));
    queued_firings_.erase("instance_" + std::to_string(i));
    running_instances_.erase("instance_" + std::to_string(i));
    count_.erase("instance_" + std::to_string(i));
    parallelism_degree_.erase("instance_" + std::to_string(i));
    current_activities_.erase("instance_" + std::to_string(i));
  }
}

/**
 *  @brief Default constructor.
 */
ExecTask::ExecTask(const std::string& name) : Task(name)
{
  set_load_balancing_function([]() { return "instance_0"; });
}

/**
 *  @brief Smart Constructor.
 */
ExecTaskPtr ExecTask::init(const std::string& name)
{
  return ExecTaskPtr(new ExecTask(name));
}

/**
 *  @brief Smart Constructor.
 */
ExecTaskPtr ExecTask::init(const std::string& name, double flops, Host* host)
{
  return init(name)->set_flops(flops)->set_host(host);
}

/** @param instance The Task instance to fire.
 *  @note Only the dispatcher instance triggers the on_start signal.
 *        Comms are created if hosts differ between dispatcher and the instance to fire,
 *        or between the instance and the collector.
 */
void ExecTask::fire(std::string instance)
{
  Task::fire(instance);
  if (instance == "dispatcher" or instance == "collector") {
    auto exec = Exec::init()
                    ->set_name(get_name() + "_" + instance)
                    ->set_flops_amount(get_amount(instance))
                    ->set_host(host_[instance]);
    exec->start();
    exec->on_this_completion_cb([this, instance](Exec const&) { complete(instance); });
    store_activity(exec, instance);
  } else {
    auto exec = Exec::init()->set_name(get_name())->set_flops_amount(get_amount())->set_host(host_[instance]);
    if (host_["dispatcher"] == host_[instance]) {
      exec->start();
      store_activity(exec, instance);
    } else {
      auto comm = Comm::sendto_init(host_["dispatcher"], host_[instance])
                      ->set_name(get_name() + "_dispatcher_to_" + instance)
                      ->set_payload_size(get_internal_bytes("dispatcher"));
      comm->add_successor(exec);
      comm->start();
      store_activity(comm, instance);
    }
    if (host_[instance] == host_["collector"]) {
      exec->on_this_completion_cb([this, instance](Exec const&) { complete(instance); });
      if (host_["dispatcher"] != host_[instance])
        store_activity(exec, instance);
    } else {
      auto comm = Comm::sendto_init(host_[instance], host_["collector"])
                      ->set_name(get_name() + instance + "_to_collector")
                      ->set_payload_size(get_internal_bytes(instance));
      exec->add_successor(comm);
      comm->on_this_completion_cb([this, instance](Comm const&) { complete(instance); });
      comm.detach();
    }
  }
}

/** @param host The host to set.
 *  @param instance The Task instance to modify.
 *  @brief Set a new host.
 */
ExecTaskPtr ExecTask::set_host(Host* host, std::string instance)
{
  kernel::actor::simcall_answered([this, host, &instance] {
    if (instance == "all")
      for (auto& [key, value] : host_)
        host_[key] = host;
    else
      host_[instance] = host;
  });
  return this;
}

/** @param flops The amount of flops to set.
 *  @param instance The Task instance to modify.
 */
ExecTaskPtr ExecTask::set_flops(double flops, std::string instance)
{
  kernel::actor::simcall_answered([this, flops, &instance] { set_amount(flops, instance); });
  return this;
}

/** @param n The number of instances to add to this Task (>=0).
    @note Instances goes always from instance_0 to instance_x,
          where x is the current number of instance.
 */
void ExecTask::add_instances(int n)
{
  Task::add_instances(n);
  int instance_count = (int)host_.size() - 2;
  for (int i = instance_count; i < n + instance_count; i++)
    host_["instance_" + std::to_string(i)] = host_.at("instance_0");
}

/** @param n The number of instances to remove from this Task (>=0).
    @note Instances goes always from instance_0 to instance_x,
          where x is the current number of instance.
          Running instance cannot be removed.
 */
void ExecTask::remove_instances(int n)
{
  Task::remove_instances(n);
  int instance_count = (int)host_.size() - 2;
  for (int i = instance_count - 1; i >= instance_count - n; i--)
    host_.erase("instance_" + std::to_string(i));
}

/**
 *  @brief Default constructor.
 */
CommTask::CommTask(const std::string& name) : Task(name)
{
  set_load_balancing_function([]() { return "instance_0"; });
}

/**
 *  @brief Smart constructor.
 */
CommTaskPtr CommTask::init(const std::string& name)
{
  return CommTaskPtr(new CommTask(name));
}

/**
 *  @brief Smart constructor.
 */
CommTaskPtr CommTask::init(const std::string& name, double bytes, Host* source, Host* destination)
{
  return init(name)->set_bytes(bytes)->set_source(source)->set_destination(destination);
}

/** @param instance The Task instance to fire.
 *  @note Only the dispatcher instance triggers the on_start signal.
 */
void CommTask::fire(std::string instance)
{
  Task::fire(instance);
  if (instance == "dispatcher" or instance == "collector") {
    auto exec = Exec::init()
                    ->set_name(get_name() + "_" + instance)
                    ->set_flops_amount(get_amount(instance))
                    ->set_host(instance == "dispatcher" ? source_ : destination_);
    exec->start();
    exec->on_this_completion_cb([this, instance](Exec const&) { complete(instance); });
    store_activity(exec, instance);
  } else {
    auto comm = Comm::sendto_init(source_, destination_)->set_name(get_name())->set_payload_size(get_amount());
    comm->start();
    comm->on_this_completion_cb([this, instance](Comm const&) { complete(instance); });
    store_activity(comm, instance);
  }
}

/**
 *  @param source The host to set.
 *  @brief Set a new source host.
 */
CommTaskPtr CommTask::set_source(Host* source)
{
  kernel::actor::simcall_answered([this, source] { source_ = source; });
  return this;
}

/**
 *  @param destination The host to set.
 *  @brief Set a new destination host.
 */
CommTaskPtr CommTask::set_destination(Host* destination)
{
  kernel::actor::simcall_answered([this, destination] { destination_ = destination; });
  return this;
}

/**
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
IoTask::IoTask(const std::string& name) : Task(name)
{
  set_load_balancing_function([]() { return "instance_0"; });
}

/**
 *  @brief Smart Constructor.
 */
IoTaskPtr IoTask::init(const std::string& name)
{
  return IoTaskPtr(new IoTask(name));
}

/**
 *  @brief Smart Constructor.
 */
IoTaskPtr IoTask::init(const std::string& name, double bytes, Disk* disk, Io::OpType type)
{
  return init(name)->set_bytes(bytes)->set_disk(disk)->set_op_type(type);
}

/**
 *  @param disk The disk to set.
 */
IoTaskPtr IoTask::set_disk(Disk* disk)
{
  kernel::actor::simcall_answered([this, disk] { disk_ = disk; });
  return this;
}

/**
 *  @param bytes The amount of bytes to set.
 */
IoTaskPtr IoTask::set_bytes(double bytes)
{
  kernel::actor::simcall_answered([this, bytes] { set_amount(bytes); });
  return this;
}

/**
 *  @param type The op type to set.
 */
IoTaskPtr IoTask::set_op_type(Io::OpType type)
{
  kernel::actor::simcall_answered([this, type] { type_ = type; });
  return this;
}

/** @param instance The Task instance to fire.
 *  @note Only the dispatcher instance triggers the on_start signal.
 */
void IoTask::fire(std::string instance)
{
  Task::fire(instance);
  if (instance == "dispatcher" or instance == "collector") {
    auto exec = Exec::init()
                    ->set_name(get_name() + "_" + instance)
                    ->set_flops_amount(get_amount(instance))
                    ->set_host(disk_->get_host());
    exec->start();
    exec->on_this_completion_cb([this, instance](Exec const&) { complete(instance); });
    store_activity(exec, instance);
  } else {
    auto io = Io::init()->set_name(get_name())->set_size(get_amount())->set_disk(disk_)->set_op_type(type_);
    io->start();
    io->on_this_completion_cb([this, instance](Io const&) { complete(instance); });
    store_activity(io, instance);
  }
}
} // namespace simgrid::s4u
