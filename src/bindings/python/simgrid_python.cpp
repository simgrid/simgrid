/* Copyright (c) 2018-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include <pybind11/pybind11.h> // Must come before our own stuff

#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "simgrid/instr.h"
#include "simgrid/kernel/ProfileBuilder.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/modelchecker.h"
#include "simgrid/plugins/carbon_footprint.h"
#include "simgrid/plugins/energy.h"
#include "simgrid/plugins/file_system.h"
#include "simgrid/plugins/live_migration.h"
#include "simgrid/plugins/load.h"
#include "src/bindings/python/PythonContextHooks.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/context/ContextSwapped.hpp"
#include "src/plugins/vm/VmLiveMigration.hpp"
#include <simgrid/Exception.hpp>
#include <simgrid/s4u/ActivitySet.hpp>
#include <simgrid/s4u/Actor.hpp>
#include <simgrid/s4u/Barrier.hpp>
#include <simgrid/s4u/Comm.hpp>
#include <simgrid/s4u/Disk.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Exec.hpp>
#include <simgrid/s4u/Host.hpp>
#include <simgrid/s4u/Io.hpp>
#include <simgrid/s4u/Link.hpp>
#include <simgrid/s4u/Mailbox.hpp>
#include <simgrid/s4u/Mess.hpp>
#include <simgrid/s4u/MessageQueue.hpp>
#include <simgrid/s4u/Mutex.hpp>
#include <simgrid/s4u/NetZone.hpp>
#include <simgrid/s4u/Semaphore.hpp>
#include <simgrid/s4u/Task.hpp>
#include <simgrid/s4u/VirtualMachine.hpp>
#include <simgrid/version.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace py = pybind11;
using simgrid::s4u::Activity;
using simgrid::s4u::ActivityPtr;
using simgrid::s4u::ActivitySet;
using simgrid::s4u::ActivitySetPtr;
using simgrid::s4u::Actor;
using simgrid::s4u::ActorPtr;
using simgrid::s4u::Barrier;
using simgrid::s4u::BarrierPtr;
using simgrid::s4u::Comm;
using simgrid::s4u::CommPtr;
using simgrid::s4u::CommTask;
using simgrid::s4u::CommTaskPtr;
using simgrid::s4u::Disk;
using simgrid::s4u::Engine;
using simgrid::s4u::ExecTask;
using simgrid::s4u::ExecTaskPtr;
using simgrid::s4u::Host;
using simgrid::s4u::Io;
using simgrid::s4u::IoTask;
using simgrid::s4u::IoTaskPtr;
using simgrid::s4u::Link;
using simgrid::s4u::Mailbox;
using simgrid::s4u::Mess;
using simgrid::s4u::MessageQueue;
using simgrid::s4u::MessPtr;
using simgrid::s4u::Mutex;
using simgrid::s4u::MutexPtr;
using simgrid::s4u::Semaphore;
using simgrid::s4u::SemaphorePtr;
using simgrid::s4u::Task;
using simgrid::s4u::TaskPtr;
using simgrid::s4u::VirtualMachine;

XBT_LOG_NEW_DEFAULT_CATEGORY(python, "python");

#ifdef SIMGRID_HAVE_PYTHON_SMPI
void SMPI_bindings(py::module& m);
#endif

namespace {

std::string get_simgrid_version()
{
  int major;
  int minor;
  int patch;
  sg_version_get(&major, &minor, &patch);
  return simgrid::xbt::string_printf("%i.%i.%i", major, minor, patch);
}
} // namespace

PYBIND11_DECLARE_HOLDER_TYPE(T, boost::intrusive_ptr<T>)

namespace {

/* Activities cannot be copied, so the callbacks get them as smart pointers instead of references */
template <class A> std::function<void(A const&)> wrap_activity_cb(py::object cb, const char* cb_name)
{
  cb.inc_ref(); // keep alive after return
  return [cb_p = cb.ptr(), cb_name](A const& activity) {
    try {
      const auto fun = py::reinterpret_borrow<py::function>(cb_p);
      fun(boost::intrusive_ptr<A>(const_cast<A*>(&activity)));
    } catch (const py::error_already_set& e) {
      xbt_die("Error while executing the %s lambda: %s", cb_name, e.what());
    }
  };
}

/* Bind the methods that are common to all activities (Comm, Exec, Io). `what` names the activity in the docstrings */
template <class A, class... Options> void bind_activity_common(py::class_<A, Options...>& cls, const std::string& what)
{
  cls.def_property(
         "name", [](const A* self) { return self->get_name(); },
         [](A* self, const std::string& name) { self->set_name(name); },
         ("The name of this " + what + ", used in the logs (read/write property).").c_str())
      .def_property_readonly("state_str", &A::get_state_str,
                             ("The state of this " + what + ", as a string (read-only property).").c_str())
      .def_property_readonly("start_time", &A::get_start_time, ("Time at which this " + what + " started.").c_str())
      .def_property_readonly("finish_time", &A::get_finish_time,
                             ("Time at which this " + what + " finished.").c_str())
      .def_property_readonly("is_suspended", &A::is_suspended,
                             ("Whether this " + what + " is suspended (read-only property).").c_str())
      .def_property_readonly(
          "is_assigned", &A::is_assigned,
          ("Whether this " + what + " is assigned to the resources it needs to start (read-only property).").c_str())
      .def_property_readonly("dependencies_solved", &A::dependencies_solved,
                             ("Whether all the predecessors of this " + what +
                              " are completed, so that it can start (read-only property).")
                                 .c_str())
      .def("add_successor", &A::add_successor, py::arg("activity"),
           ("Declare that the given activity cannot start before the completion of this " + what + ".").c_str())
      .def("remove_successor", &A::remove_successor, py::arg("activity"),
           "Remove a dependency previously declared with add_successor().")
      .def("set_tracing_category", &A::set_tracing_category, py::arg("category"),
           "Set a user-defined tracing category. Must be called before start().")
      .def("start", &A::start,
           ("Start this " + what +
            ". It only actually starts once it is assigned to its resources and all its dependencies are solved.")
               .c_str())
      .def("cancel", &A::cancel, ("Cancel this " + what + ".").c_str())
      .def("test", &A::test, ("Test whether this " + what + " is terminated.").c_str())
      .def("wait", &A::wait, ("Block until the completion of this " + what + ".").c_str())
      .def("wait_for", &A::wait_for, py::arg("timeout"),
           ("Block until the completion of this " + what +
            ", or raises TimeoutException after the specified timeout.")
               .c_str())
      .def("wait_until", &A::wait_until, py::arg("time_limit"),
           ("Block until the completion of this " + what + ", or raises TimeoutException after the specified time.")
               .c_str())
      .def("suspend", &A::suspend,
           ("Blocks the progression of this " + what + " until it gets resumed with resume().").c_str())
      .def("resume", &A::resume,
           ("Unblock the progression of this " + what + " if it was suspended previously with suspend().").c_str())
      .def("detach", py::overload_cast<>(&A::detach),
           ("Start this " + what + ", and ignore its result. It can be completely forgotten after that.").c_str())
      .def(
          "on_this_start_cb",
          [](A* self, py::object cb) { self->on_this_start_cb(wrap_activity_cb<A>(cb, "on_this_start")); },
          py::arg("cb"), ("Add a callback fired when this " + what + " starts, with the " + what + " as parameter.").c_str())
      .def(
          "on_this_completion_cb",
          [](A* self, py::object cb) { self->on_this_completion_cb(wrap_activity_cb<A>(cb, "on_this_completion")); },
          py::arg("cb"),
          ("Add a callback fired when this " + what + " completes (either normally, cancelled or failed), with the " +
           what + " as parameter.")
              .c_str())
      .def(
          "on_this_suspend_cb",
          [](A* self, py::object cb) { self->on_this_suspend_cb(wrap_activity_cb<A>(cb, "on_this_suspend")); },
          py::arg("cb"),
          ("Add a callback fired when this " + what + " is suspended, with the " + what + " as parameter.").c_str())
      .def(
          "on_this_resume_cb",
          [](A* self, py::object cb) { self->on_this_resume_cb(wrap_activity_cb<A>(cb, "on_this_resume")); },
          py::arg("cb"),
          ("Add a callback fired when this " + what + " is resumed, with the " + what + " as parameter.").c_str())
      .def(
          "on_this_veto_cb",
          [](A* self, py::object cb) { self->on_this_veto_cb(wrap_activity_cb<A>(cb, "on_this_veto")); },
          py::arg("cb"),
          ("Add a callback fired each time this " + what +
           " fails to start because of a veto (unsolved dependency or unassigned resource), with the " + what +
           " as parameter.")
              .c_str())
      .def_static(
          "on_start_cb", [](py::object cb) { A::on_start_cb(wrap_activity_cb<A>(cb, "on_start")); }, py::arg("cb"),
          ("Add a callback fired when any " + what + " starts, with the " + what + " as parameter.").c_str())
      .def_static(
          "on_completion_cb", [](py::object cb) { A::on_completion_cb(wrap_activity_cb<A>(cb, "on_completion")); },
          py::arg("cb"),
          ("Add a callback fired when any " + what + " completes (either normally, cancelled or failed), with the " +
           what + " as parameter.")
              .c_str())
      .def_static(
          "on_suspend_cb", [](py::object cb) { A::on_suspend_cb(wrap_activity_cb<A>(cb, "on_suspend")); },
          py::arg("cb"),
          ("Add a callback fired when any " + what + " is suspended, with the " + what + " as parameter.").c_str())
      .def_static(
          "on_resume_cb", [](py::object cb) { A::on_resume_cb(wrap_activity_cb<A>(cb, "on_resume")); },
          py::arg("cb"),
          ("Add a callback fired when any " + what + " is resumed, with the " + what + " as parameter.").c_str())
      .def_static(
          "on_veto_cb", [](py::object cb) { A::on_veto_cb(wrap_activity_cb<A>(cb, "on_veto")); }, py::arg("cb"),
          ("Add a callback fired each time any " + what +
           " fails to start because of a veto (unsolved dependency or unassigned resource), with the " + what +
           " as parameter.")
              .c_str());
}

/* User data attached to a SimGrid object (through xbt::Extendable). We keep a reference on the stored Python object */
template <class T> py::object get_py_data(const T* obj)
{
  auto* data = obj->template get_data<PyObject>();
  return data == nullptr ? py::none() : py::reinterpret_borrow<py::object>(data);
}
template <class T> void set_py_data(T* obj, py::object data)
{
  auto* old = obj->template get_data<PyObject>();
  obj->set_data(data.is_none() ? nullptr : data.inc_ref().ptr());
  Py_XDECREF(old);
}

/* Signals of the resources (such as VirtualMachine) pass them by reference. The Python callbacks get a pointer */
template <class R, class Ref = R const&> std::function<void(Ref)> wrap_resource_cb(py::object cb, const char* cb_name)
{
  cb.inc_ref(); // keep alive after return
  return [cb_p = cb.ptr(), cb_name](Ref resource) {
    try {
      const auto fun = py::reinterpret_borrow<py::function>(cb_p);
      fun(const_cast<R*>(&resource));
    } catch (const py::error_already_set& e) {
      xbt_die("Error while executing the %s lambda: %s", cb_name, e.what());
    }
  };
}

void ensure_live_migration_plugin()
{
  if (not simgrid::plugin::vm::VmMigrationExt::EXTENSION_ID.valid())
    throw std::runtime_error("The live migration plugin is not initialized. Please call "
                             "sg_vm_live_migration_plugin_init() before loading the platform.");
}

void ensure_file_system_plugin()
{
  if (not simgrid::s4u::FileSystemDiskExt::EXTENSION_ID.valid())
    throw std::runtime_error("The file system plugin is not initialized. Please call sg_storage_file_system_init() "
                             "before loading the platform.");
}

/* Python view of a simgrid::s4u::File. Closing a File destroys it, so we keep track of it to make sure that a closed
 * file is not used anymore (as for the Python files, using it raises a ValueError). */
class PyFile {
  simgrid::s4u::File* file_;

public:
  py::object data = py::none();

  explicit PyFile(simgrid::s4u::File* file) : file_(file) {}
  simgrid::s4u::File* get() const
  {
    if (file_ == nullptr)
      throw py::value_error("I/O operation on closed file.");
    return file_;
  }
  bool closed() const { return file_ == nullptr; }
  void close()
  {
    if (file_ != nullptr)
      file_->close();
    file_ = nullptr;
  }
};
} // namespace

PYBIND11_MODULE(simgrid, m)
{
  m.doc() = "SimGrid userspace API";

  m.attr("simgrid_version") = get_simgrid_version();

  m.def("MC_assert", &MC_assert, "Assertion for the model-checker: Defines a safety property to verify");
  m.def("MC_random", &MC_random,
        "Explore every branches where that function returns a value between min and max (inclusive)");

  /* Tracing: host user-variables.
   * time defaults to -1, meaning "now": the actual current clock is read at call time (inside the lambda),
   * not when the module is loaded, unlike what a plain py::arg(...)=simgrid_get_clock() default would do. */
  m.def("declare_host_variable", &simgrid::instr::declare_host_variable, py::arg("variable"), py::arg("color") = "",
        "Declare a new user variable associated to hosts, and optionally give a color to use in the trace.");
  m.def(
      "set_host_variable",
      [](const std::string& host, const std::string& variable, double value, double time) {
        simgrid::instr::set_host_variable(host, variable, value, time >= 0 ? time : simgrid_get_clock());
      },
      py::arg("host"), py::arg("variable"), py::arg("value"), py::arg("time") = -1,
      "Set the value of a given host variable, possibly at a given timestamp (defaults to the current time).");
  m.def(
      "add_host_variable",
      [](const std::string& host, const std::string& variable, double value, double time) {
        simgrid::instr::add_host_variable(host, variable, value, time >= 0 ? time : simgrid_get_clock());
      },
      py::arg("host"), py::arg("variable"), py::arg("value"), py::arg("time") = -1,
      "Add a value to a given host variable, possibly at a given timestamp (defaults to the current time).");
  m.def(
      "sub_host_variable",
      [](const std::string& host, const std::string& variable, double value, double time) {
        simgrid::instr::sub_host_variable(host, variable, value, time >= 0 ? time : simgrid_get_clock());
      },
      py::arg("host"), py::arg("variable"), py::arg("value"), py::arg("time") = -1,
      "Subtract a value from a given host variable, possibly at a given timestamp (defaults to the current time).");
  m.def("get_host_variables", &simgrid::instr::get_host_variables,
        "Retrieve the set of user variables declared for hosts.");

  /* Tracing: link user-variables */
  m.def("declare_link_variable", &simgrid::instr::declare_link_variable, py::arg("variable"), py::arg("color") = "",
        "Declare a new user variable associated to links, and optionally give a color to use in the trace.");
  m.def(
      "set_link_variable",
      [](const std::string& link, const std::string& variable, double value, double time) {
        simgrid::instr::set_link_variable(link, variable, value, time >= 0 ? time : simgrid_get_clock());
      },
      py::arg("link"), py::arg("variable"), py::arg("value"), py::arg("time") = -1,
      "Set the value of a given link variable, possibly at a given timestamp (defaults to the current time).");
  m.def(
      "add_link_variable",
      [](const std::string& link, const std::string& variable, double value, double time) {
        simgrid::instr::add_link_variable(link, variable, value, time >= 0 ? time : simgrid_get_clock());
      },
      py::arg("link"), py::arg("variable"), py::arg("value"), py::arg("time") = -1,
      "Add a value to a given link variable, possibly at a given timestamp (defaults to the current time).");
  m.def(
      "sub_link_variable",
      [](const std::string& link, const std::string& variable, double value, double time) {
        simgrid::instr::sub_link_variable(link, variable, value, time >= 0 ? time : simgrid_get_clock());
      },
      py::arg("link"), py::arg("variable"), py::arg("value"), py::arg("time") = -1,
      "Subtract a value from a given link variable, possibly at a given timestamp (defaults to the current time).");
  m.def(
      "set_link_variable",
      [](const std::string& src, const std::string& dst, const std::string& variable, double value, double time) {
        simgrid::instr::set_link_variable(src, dst, variable, value, time >= 0 ? time : simgrid_get_clock());
      },
      py::arg("src"), py::arg("dst"), py::arg("variable"), py::arg("value"), py::arg("time") = -1,
      "Set the value of a given link variable for the link(s) between src and dst, possibly at a given timestamp "
      "(defaults to the current time).");
  m.def(
      "add_link_variable",
      [](const std::string& src, const std::string& dst, const std::string& variable, double value, double time) {
        simgrid::instr::add_link_variable(src, dst, variable, value, time >= 0 ? time : simgrid_get_clock());
      },
      py::arg("src"), py::arg("dst"), py::arg("variable"), py::arg("value"), py::arg("time") = -1,
      "Add a value to a given link variable for the link(s) between src and dst, possibly at a given timestamp "
      "(defaults to the current time).");
  m.def(
      "sub_link_variable",
      [](const std::string& src, const std::string& dst, const std::string& variable, double value, double time) {
        simgrid::instr::sub_link_variable(src, dst, variable, value, time >= 0 ? time : simgrid_get_clock());
      },
      py::arg("src"), py::arg("dst"), py::arg("variable"), py::arg("value"), py::arg("time") = -1,
      "Subtract a value from a given link variable for the link(s) between src and dst, possibly at a given "
      "timestamp (defaults to the current time).");
  m.def("get_link_variables", &simgrid::instr::get_link_variables,
        "Retrieve the set of user variables declared for links.");

  // Internal exception used to kill actors and sweep the RAII chimney (free objects living on the stack)
  static py::object pyForcefulKillEx(py::register_exception<simgrid::ForcefulKillException>(m, "ActorKilled"));

  py::register_exception<simgrid::NetworkFailureException>(m, "NetworkFailureException");
  py::register_exception<simgrid::TimeoutException>(m, "TimeoutException");
  py::register_exception<simgrid::HostFailureException>(m, "HostFailureException");
  py::register_exception<simgrid::StorageFailureException>(m, "StorageFailureException");
  py::register_exception<simgrid::VmFailureException>(m, "VmFailureException");
  py::register_exception<simgrid::CancelException>(m, "CancelException");
  py::register_exception<simgrid::AssertionError>(m, "AssertionError");

  /* Declare all the classes and enums before defining any function or method, so that the signatures generated by
   * pybind11 (and thus the documentation) refer to the Python names (e.g., simgrid.Host) instead of the C++ ones
   * (e.g., simgrid::s4u::Host). The members of each class are defined further below. */
  py::class_<Engine> engine(m, "Engine", "Simulation Engine");
  py::class_<simgrid::s4u::NetZone, std::unique_ptr<simgrid::s4u::NetZone, py::nodelete>> netzone(
      m, "NetZone",
      "A netzone is a network container, in charge of routing information between elements (hosts) and to the "
      "nearby netzones. In SimGrid, there is a hierarchy of netzones, with a unique root zone (that you can "
      "retrieve from the Engine).");
  py::class_<simgrid::s4u::ClusterCallbacks> cluster_callbacks( // XBT_ATTRIB_DEPRECATED_v401
      m, "ClusterCallbacks", "Callbacks used to create cluster zones");
  py::class_<simgrid::s4u::FatTreeParams> fat_tree_params( // XBT_ATTRIB_DEPRECATED_v401
      m, "FatTreeParams", "Parameters to create a Fat-Tree zone");
  py::class_<simgrid::s4u::DragonflyParams> dragonfly_params( // XBT_ATTRIB_DEPRECATED_v401
      m, "DragonflyParams", "Parameters to create a Dragonfly zone");
  py::class_<simgrid::s4u::Host, std::unique_ptr<Host, py::nodelete>> host(
      m, "Host",
      "Some physical resource with computing and networking capabilities on which Actors execute. All hosts are "
      "automatically created when the platform is loaded. You cannot create a host yourself. You can retrieve a "
      "particular host using Host.by_name() and actors can retrieve the host on which they run using Host.current() "
      "or this_actor.get_host().");
  py::enum_<simgrid::s4u::Host::SharingPolicy> host_sharing_policy(host, "SharingPolicy");
  py::class_<VirtualMachine, Host, std::unique_ptr<VirtualMachine, py::nodelete>> vm(
      m, "VirtualMachine",
      "A virtual machine is an execution container that runs on a physical machine (PM, a :py:class:`~simgrid.Host`) "
      "and that can be moved between PMs. A VM is also a :py:class:`~simgrid.Host`, so you can start actors on it. "
      "Create VMs with :py:func:`Host.create_vm() <simgrid.Host.create_vm>`.");
  py::enum_<VirtualMachine::State> vm_state(vm, "State", "The possible states of a virtual machine.");
  py::class_<Disk, std::unique_ptr<simgrid::s4u::Disk, py::nodelete>> disk(
      m, "Disk",
      "Disk represent the disk resources associated to a host. By default, SimGrid does not keep track of the "
      "actual data being written but only computes the time taken by the corresponding data movement.");
  py::enum_<simgrid::s4u::Disk::SharingPolicy> disk_sharing_policy(disk, "SharingPolicy");
  py::enum_<simgrid::s4u::Disk::Operation> disk_operation(disk, "Operation");
  py::class_<simgrid::kernel::routing::NetPoint, std::unique_ptr<simgrid::kernel::routing::NetPoint, py::nodelete>>
      netpoint(m, "NetPoint", "NetPoint object");
  py::class_<Link, std::unique_ptr<Link, py::nodelete>> link(m, "Link",
                                                             "A Link represents the network facilities between hosts.");
  py::enum_<Link::SharingPolicy> link_sharing_policy(link, "SharingPolicy");
  py::class_<simgrid::s4u::LinkInRoute> linkinroute(m, "LinkInRoute", "Abstraction to add link in routes");
  py::enum_<simgrid::s4u::LinkInRoute::Direction> linkinroute_direction(linkinroute, "Direction");
  py::class_<simgrid::s4u::SplitDuplexLink, Link, std::unique_ptr<simgrid::s4u::SplitDuplexLink, py::nodelete>>
      split_duplex_link(m, "SplitDuplexLink", "Network split-duplex link");
  py::class_<simgrid::s4u::Mailbox, std::unique_ptr<Mailbox, py::nodelete>> mailbox(m, "Mailbox",
                                                                                    "Network rendez-vous points.");
  py::class_<MessageQueue, std::unique_ptr<MessageQueue, py::nodelete>> mqueue(
      m, "MessageQueue",
      "Rendez-vous points for the exchange of messages between actors. Unlike the :py:class:`~simgrid.Mailbox`, "
      "the messages exchanged through a message queue do not involve any network communication: they are "
      "delivered instantly, as soon as both the sender and the receiver are ready.");
  py::class_<Activity, ActivityPtr> activity(
      m, "Activity",
      "An Activity is the ancestor of every activity that an actor can undertake, i.e., anything that takes time in "
      "the simulated world. See the relevant subclasses (Comm, Exec, Io) for the activities that you can actually "
      "create.");
  py::class_<Comm, CommPtr, Activity> comm(
      m, "Comm",
      "Represents all communications, be they asynchronous or not, that you can start, test and wait for. Most "
      "communications are created by exchanging data through a :py:class:`~simgrid.Mailbox`. You can also create "
      "direct point-to-point communications between two arbitrary hosts, bypassing the mailbox and actor "
      "mechanisms, with sendto(), sendto_init() and sendto_async().");
  py::class_<Mess, MessPtr, Activity> mess(
      m, "Mess",
      "Represents the messages exchanged through a :py:class:`~simgrid.MessageQueue`, be they asynchronous or not, "
      "that you can start, test and wait for.");
  py::class_<simgrid::s4u::Io, simgrid::s4u::IoPtr, Activity> io(
      m, "Io",
      "I/O activities, representing an asynchronous disk access. They are generated from the read/write methods "
      "of :py:class:`~simgrid.Disk`.");
  py::enum_<simgrid::s4u::Io::OpType> io_op_type(m, "IoOpType");
  py::class_<simgrid::s4u::Exec, simgrid::s4u::ExecPtr, Activity> exec(
      m, "Exec",
      "Computation Activity, representing an asynchronous execution. Most of them are created with "
      "this_actor.exec_init() or Host.exec_init(), and represent a classical (sequential) execution. This can be "
      "used to simulate some computation occurring in another thread when the calling actor is not blocked "
      "during the execution.");
  py::class_<Semaphore, SemaphorePtr> semaphore(
      m, "Semaphore",
      "A classical semaphore, but blocking in the simulation world. It is strictly impossible to use a real "
      "semaphore here, because it would block the whole simulation. Instead, you should use the present class, "
      "that offers a very similar interface.");
  py::class_<Mutex, MutexPtr> mutex(
      m, "Mutex",
      "A classical mutex, but blocking in the simulation world. By default, mutexes are not recursive: if an "
      "actor tries to lock the same object twice, it deadlocks with itself. Pass True to the constructor to get "
      "a recursive mutex instead.");
  py::class_<Barrier, BarrierPtr> barrier(
      m, "Barrier",
      "A classical barrier, but blocking in the simulation world. A fixed number of actors block in wait() until "
      "all of them reached that point, after which they are all unblocked together.");
  py::class_<simgrid::s4u::Actor, ActorPtr> actor(m, "Actor",
                                                  "An actor is an independent stream of execution in your distributed "
                                                  "application. See the C++ documentation for details.");
  py::class_<PyFile> file(m, "File",
                          "A file, as provided by the file system plugin (see :py:func:`sg_storage_file_system_init`). "
                          "Files are opened with :py:func:`File.open` and must be closed with :py:func:`File.close` "
                          "or, preferably, used as context managers: ``with File.open(path) as file: ...``.");
  py::class_<Task, TaskPtr> task(
      m, "Task",
      "Repeatable activities, used to simulate Dataflow applications. A Dataflow is a graph of Tasks, where each "
      "Task has a set of successors and predecessors. When a Task completes, it sends a Token to each of its "
      "successors; a Task only fires once it received a token from each of its predecessors. See the concrete "
      "subclasses that you can actually create: CommTask, ExecTask and IoTask.\n\n"
      "Each Task is internally composed of several instances: a dispatcher, a collector, and instance_0 to "
      "instance_n. The dispatcher relies on a user-provided load balancing function to select the instance that "
      "fires next; once that instance completes, it fires the collector in turn. To start a Dataflow without "
      "waiting for any token, use enqueue_firings().");
  py::class_<CommTask, CommTaskPtr, Task> comm_task(
      m, "CommTask",
      "A repeatable communication, i.e. a Task representing a communication in a Dataflow. See Task for the "
      "general Task concepts (dispatcher, collector, instances, tokens).");
  py::class_<ExecTask, ExecTaskPtr, Task> exec_task(
      m, "ExecTask",
      "A repeatable execution, i.e. a Task representing a computation in a Dataflow. See Task for the general "
      "Task concepts (dispatcher, collector, instances, tokens).");
  py::class_<IoTask, IoTaskPtr, Task> io_task(
      m, "IoTask",
      "A repeatable I/O, i.e. a Task representing a disk access in a Dataflow. See Task for the general Task "
      "concepts (dispatcher, collector, instances, tokens).");
  py::class_<ActivitySet, ActivitySetPtr> activity_set(
      m, "ActivitySet",
      "A container of activities, allowing to wait for the completion of any or all activities in the set. This "
      "is somehow similar to the select(2) system call under UNIX, allowing you to wait for the next event about "
      "these activities.");

  /* this_actor namespace */
  m.def_submodule("this_actor", "Bindings of the s4u::this_actor namespace. See the C++ documentation for details.")
      .def(
          "debug", [](const char* s) { XBT_DEBUG("%s", s); }, "Display a logging message of 'debug' priority.")
      .def(
          "info", [](const char* s) { XBT_INFO("%s", s); }, "Display a logging message of 'info' priority.")
      .def(
          "warning", [](const char* s) { XBT_WARN("%s", s); }, "Display a logging message of 'warning' priority.")
      .def(
          "error", [](const char* s) { XBT_ERROR("%s", s); }, "Display a logging message of 'error' priority.")
      .def("execute", py::overload_cast<double, double>(&simgrid::s4u::this_actor::execute),
           "Block the current actor, computing the given amount of flops at the given priority.", py::arg("flops"),
           py::arg("priority") = 1)
      .def("exec_init", py::overload_cast<double>(&simgrid::s4u::this_actor::exec_init))
      .def("exec_async", py::overload_cast<double>(&simgrid::s4u::this_actor::exec_async))
      .def("parallel_execute", &simgrid::s4u::this_actor::parallel_execute,
           "Run a parallel task (requires the 'ptask_L07' model)")
      .def("thread_execute", py::overload_cast<Host*, double, int>(&simgrid::s4u::this_actor::thread_execute),
           py::arg("host"), py::arg("flops"), py::arg("thread_count"),
           "Block the current actor, computing the given amount of flops with several threads on the given host. "
           "Each thread computes the given amount of flops, using one core of the host.")
      .def("thread_execute", py::overload_cast<double, int>(&simgrid::s4u::this_actor::thread_execute),
           py::arg("flops"), py::arg("thread_count"),
           "Block the current actor, computing the given amount of flops with several threads on the current host. "
           "Each thread computes the given amount of flops, using one core of the host.")
      .def("thread_execute_async",
           py::overload_cast<Host*, double, int>(&simgrid::s4u::this_actor::thread_execute_async), py::arg("host"),
           py::arg("flops"), py::arg("thread_count"),
           "Start a multi-threaded execution on the given host, and return the corresponding Exec without waiting "
           "for its completion.")
      .def("thread_execute_async", py::overload_cast<double, int>(&simgrid::s4u::this_actor::thread_execute_async),
           py::arg("flops"), py::arg("thread_count"),
           "Start a multi-threaded execution on the current host, and return the corresponding Exec without waiting "
           "for its completion.")
      .def("exec_init",
           py::overload_cast<const std::vector<simgrid::s4u::Host*>&, const std::vector<double>&,
                             const std::vector<double>&>(&simgrid::s4u::this_actor::exec_init),
           "Initiate a parallel task (requires the 'ptask_L07' model)")
      .def("get_engine", &simgrid::s4u::this_actor::get_engine, "Returns the engine instance this actor is running in.")
      .def("get_host", &simgrid::s4u::this_actor::get_host, "Retrieves host on which the current actor is located")
      .def("set_host", &simgrid::s4u::this_actor::set_host,
           "Moves the current actor to another host.\n\n"
           "If the actor is currently blocked on an execution activity, the activity is also migrated to the new "
           "host. If it's blocked on another kind of activity, an error is raised as the mandated code is not "
           "written yet. Please report that bug if you need it.\n\n"
           "Asynchronous activities started by the actor are not migrated automatically, so you have to take care "
           "of this yourself (only you knows which ones should be migrated).",
           py::arg("dest"))
      .def("sleep_for", static_cast<void (*)(double)>(&simgrid::s4u::this_actor::sleep_for),
           "Block the actor sleeping for that amount of seconds.", py::arg("duration"))
      .def("sleep_until", static_cast<void (*)(double)>(&simgrid::s4u::this_actor::sleep_until),
           "Block the actor sleeping until the specified timestamp.", py::arg("duration"))
      .def("suspend", &simgrid::s4u::this_actor::suspend,
           "Suspend the current actor, that is blocked until resume()ed by another actor.")
      .def("yield_", &simgrid::s4u::this_actor::yield,
           "Yield the current actor, give the control to the other actors. It will be executed again in the next "
           "scheduling round.")
      .def("exit", &simgrid::s4u::this_actor::exit, "Kill the current actor.")
      .def(
          "on_exit",
          [](py::object fun) {
            fun.inc_ref(); // keep alive after return
            simgrid::s4u::this_actor::on_exit([fun_p = fun.ptr()](bool failed) {
              try {
                const auto fun = py::reinterpret_borrow<py::function>(fun_p);
                fun(failed);
              } catch (const py::error_already_set& e) {
                xbt_die("Error while executing the on_exit lambda: %s", e.what());
              }
            });
          },
          "Define a lambda to be called when the actor ends. It takes a bool parameter indicating whether the actor "
          "was killed. If False, the actor finished peacefully.")
      .def("get_pid", &simgrid::s4u::this_actor::get_pid, "Retrieves PID of the current actor")
      .def("get_ppid", &simgrid::s4u::this_actor::get_ppid,
           "Retrieves PPID of the current actor (i.e., the PID of its parent).")
      .def("get_name", &simgrid::s4u::this_actor::get_cname, "Retrieves the name of this actor.");

  /* Class Engine */
  engine
      .def(py::init([](std::vector<std::string> args) {
             auto argc = static_cast<int>(args.size());
             std::vector<char*> argv(args.size() + 1); // argv[argc] is nullptr
             std::transform(begin(args), end(args), begin(argv), [](std::string& s) { return &s.front(); });
             // Currently this can be dangling, we should wrap this somehow.
             auto* e = new simgrid::s4u::Engine(&argc, argv.data());
             // Register per-actor Python state save/restore.
             // Deferred here (not at module import) because the Engine must exist first.
             auto* swapped = dynamic_cast<simgrid::kernel::context::SwappedContextFactory*>(
                 e->get_impl()->get_context_factory());
             if (swapped != nullptr) {
               // Raw or Boost factory: allocate PythonActorState per actor and register the
               // context-switch hook to save/restore Python interpreter state on each switch.
               simgrid::s4u::Actor::on_creation_cb([](simgrid::s4u::Actor& actor) {
                 actor.get_impl()->py_state = new simgrid::python::PythonActorState();
               });
               // Free memory when actor is destroyed. Set py_state to nullptr after deletion to prevent
               // any stale accesses during final cleanup context switches.
               simgrid::s4u::Actor::on_destruction_cb([](simgrid::s4u::Actor const& actor) {
                 auto* py_state = static_cast<simgrid::python::PythonActorState*>(actor.get_impl()->py_state);
                 delete py_state;
                 const_cast<simgrid::s4u::Actor&>(actor).get_impl()->py_state = nullptr;
               });
               // Allocate state for maestro now (it was created before on_creation_cb was registered).
               // Maestro is not destroyed via on_destruction_cb (ActorImpl::~ActorImpl skips the
               // signal for maestro), so we free its py_state explicitly when the simulation ends.
               auto* maestro        = e->get_impl()->get_maestro();
               maestro->py_state    = new simgrid::python::PythonActorState();
               simgrid::s4u::Engine::on_simulation_end_cb([maestro]() {
                 delete static_cast<simgrid::python::PythonActorState*>(maestro->py_state);
                 maestro->py_state = nullptr;
               });
               swapped->register_before_context_switch_hook(
                   [](simgrid::kernel::context::SwappedContext* from, simgrid::kernel::context::SwappedContext* to) {
                     auto* from_state = from->get_actor()->py_state;
                     auto* to_state = to->get_actor()->py_state;
                     if (from_state && to_state)
                       simgrid::python::py_switch_state(
                           static_cast<simgrid::python::PythonActorState*>(from_state),
                           static_cast<simgrid::python::PythonActorState*>(to_state));
                   });
             }
             return e;
           }),
           "Create the simulation engine, passing the command-line parameters of your main function.")
      .def_property_readonly_static(
          "clock", [](py::object /* self */) { return Engine::get_clock(); },
          "The simulation time, ie the amount of simulated seconds since the simulation start.")
      .def_property_readonly_static(
          "instance", [](py::object /* self */) { return Engine::get_instance(); }, "Retrieve the engine singleton")
      .def("host_by_name", &Engine::host_by_name_or_null,
           "Retrieve a host by its name, or None if it does not exist in the platform.")
      .def("link_by_name", &Engine::link_by_name_or_null, py::arg("name"),
           "Retrieve a link by its name, or None if it does not exist in the platform.")
      .def_property_readonly("all_hosts", &Engine::get_all_hosts, "Returns the list of all hosts found in the platform")
      .def_property_readonly("all_links", &Engine::get_all_links, "Returns the list of all links found in the platform")
      .def_property_readonly("all_netpoints", &Engine::get_all_netpoints,
                             "Returns the list of all netpoints found in the platform")
      .def_property_readonly("all_netzones", &Engine::get_all_netzones,
                             "Returns the list of all netzones found in the platform")
      .def_property_readonly("all_actors", &Engine::get_all_actors,
                             "Returns the list of all actors found in the platform")
      .def_property_readonly("netzone_root", &Engine::get_netzone_root,
                             "Retrieve the root netzone, containing all others.")
      .def("netpoint_by_name", &Engine::netpoint_by_name_or_null,
           "Retrieve a netpoint by its name, or None if it does not exist in the platform.")
      .def("netzone_by_name", &Engine::netzone_by_name_or_null,
           "Retrieve a netzone by its name, or None if it does not exist in the platform.")
      .def_static("set_config", py::overload_cast<const std::string&>(&Engine::set_config),
                  "Set a configuration variable to the given value. Do --help on any SimGrid binary to see the list "
                  "of currently existing configuration variables.")
      .def("load_platform", &Engine::load_platform,
           "Creates a new platform, including hosts, links, and the routing table (see also: the documentation on "
           "platform files).")
      .def("load_deployment", &Engine::load_deployment, "Load a deployment file and launch the actors that it contains")
      .def("mailbox_by_name_or_create", &Engine::mailbox_by_name_or_create, py::arg("name"),
           "Find a mailbox from its name or create one if it does not exist")
      .def("message_queue_by_name_or_create", &Engine::message_queue_by_name_or_create, py::arg("name"),
           "Find a message queue from its name or create one if it does not exist")
      .def("run", &Engine::run, "Run the simulation until its end")
      .def("run_until", py::overload_cast<double>(&Engine::run_until, py::const_),
           "Run the simulation until the given date, given in seconds since the simulation start",
           py::arg("max_date") = -1)
      .def(
          "register_actor",
          [](Engine* e, const std::string& name, py::object fun_or_class) {
            fun_or_class.inc_ref(); // keep alive after return
            e->register_actor(name, [fun_or_class_p = fun_or_class.ptr()](std::vector<std::string> args) {
              try {
                /* Convert the std::vector into a py::tuple */
                py::tuple params(args.size() - 1);
                for (size_t i = 1; i < args.size(); i++)
                  params[i - 1] = py::cast(args[i]);

                const auto fun_or_class = py::reinterpret_borrow<py::object>(fun_or_class_p);
                py::object res          = fun_or_class(*params);
                /* If I was passed a class, I just built an instance, so I need to call it now */
                if (py::isinstance<py::function>(res))
                  res();
              } catch (const py::error_already_set& ex) {
                if (ex.matches(pyForcefulKillEx)) {
                  XBT_VERB("Actor killed");
                  simgrid::ForcefulKillException::do_throw(); // Forward that ForcefulKill exception
                }
                throw;
              }
            });
          },
          "Registers the main function of an actor (used for the deployment file)")
      .def(
          "add_actor", // XBT_ATTRIB_DEPRECATED_v403
          [](std::string const& name) {
            PyErr_WarnEx(PyExc_DeprecationWarning, "add_actor) is deprecated, use Host.add_actor().", 2);
            throw std::logic_error("Please call Host.add_actor() instead");
          },
          "Create an actor from a function or an object. See the :ref:`example <s4u_ex_actors_create>`.")
      .def("set_log_control", [](Engine*, const std::string& settings) { xbt_log_control_set(settings.c_str()); });

  /* Class Netzone */
  netzone
      .def_static(
          "create_full_zone",
          [](std::string const& name) {
            PyErr_WarnEx(PyExc_DeprecationWarning, "create_full_zone() is deprecated, use Netzone.add_netzone_full().",
                         2);
            throw std::logic_error("Please call Netzone.add_netzone_full() instead");
          },
          "Creates a zone of type FullZone") // XBT_ATTRIB_DEPRECATED_v401
      .def_static("create_torus_zone", [](std::string const& name) {
            PyErr_WarnEx(PyExc_DeprecationWarning,
                         "create_torus_zone() is deprecated, use Netzone.add_netzone_fatTree().", 2);
            throw std::logic_error("Please call Netzone.add_netzone_torus() instead");
        },
        "Creates a cluster of type Torus") // XBT_ATTRIB_DEPRECATED_v401
      .def_static("create_fatTree_zone", [](std::string const& name) {
            PyErr_WarnEx(PyExc_DeprecationWarning,
                         "create_fatTree_zone() is deprecated, use Netzone.add_netzone_fatTree().", 2);
            throw std::logic_error("Please call Netzone.add_netzone_fatTree() instead");
          },
          "Creates a cluster of type Fat-Tree") // XBT_ATTRIB_DEPRECATED_v401
      .def_static("create_dragonfly_zone", [](std::string const& name) {
            PyErr_WarnEx(PyExc_DeprecationWarning,
                         "create_dragonfly_zone() is deprecated, use Netzone.add_netzone_dragonfly().", 2);
            throw std::logic_error("Please call Netzone.add_netzone_dragonfly() instead");
          }, 
          "Creates a cluster of type Dragonfly")
      .def_static(
          "create_star_zone",
          [](std::string const& name) {
            PyErr_WarnEx(PyExc_DeprecationWarning, "create_star_zone() is deprecated, use Netzone.add_netzone_star().",
                         2);
            throw std::logic_error("Please call Netzone.add_netzone_star() instead");
          },
          "Creates a zone of type Star") // XBT_ATTRIB_DEPRECATED_v401
      .def_static(
          "create_floyd_zone",
          [](std::string const& name) {
            PyErr_WarnEx(PyExc_DeprecationWarning,
                         "create_floyd_zone() is deprecated, use Netzone.add_netzone_floyd().", 2);
            throw std::logic_error("Please call Netzone.add_netzone_floyd() instead");
          },
          "Creates a zone of type Floyd") // XBT_ATTRIB_DEPRECATED_v401
      .def_static(
          "create_dijkstra_zone",
          [](std::string const& name) {
            PyErr_WarnEx(PyExc_DeprecationWarning,
                         "create_dijkstra_zone() is deprecated, use Netzone.add_netzone_dijkstra().", 2);
            throw std::logic_error("Please call Netzone.add_netzone_dijkstra() instead");
          },
          "Creates a zone of type Dijkstra") // XBT_ATTRIB_DEPRECATED_v401
      .def_static(
          "create_vivaldi_zone",
          [](std::string const& name) {
            PyErr_WarnEx(PyExc_DeprecationWarning,
                         "create_vivaldi_zone() is deprecated, use Netzone.add_netzone_vivaldi().", 2);
            throw std::logic_error("Please call Netzone.add_netzone_vivaldi() instead");
          },
          "Creates a zone of type Vivaldi") // XBT_ATTRIB_DEPRECATED_v401
      .def_static(
          "create_empty_zone",
          [](std::string const& name) {
            PyErr_WarnEx(PyExc_DeprecationWarning,
                         "create_empty_zone() is deprecated, use Netzone.add_netzone_empty().", 2);
            throw std::logic_error("Please call Netzone.add_netzone_empty() instead");
          },
          "Creates a zone of type Empty") // XBT_ATTRIB_DEPRECATED_v401
      .def_static(
          "create_wifi_zone",
          [](std::string const& name) {
            PyErr_WarnEx(PyExc_DeprecationWarning, "create_wifi_zone() is deprecated, use Netzone.add_netzone_wifi().",
                         2);
            throw std::logic_error("Please call Netzone.add_netzone_wifi() instead");
          },
          "Creates a zone of type Wi-Fi") // XBT_ATTRIB_DEPRECATED_v401
      .def("set_host_cb", [](simgrid::s4u::NetZone* zone, py::object cb) -> simgrid::s4u::NetZone* {
            cb.inc_ref(); // keep alive after return
            return zone->set_host_cb([cb_p = cb.ptr()](simgrid::s4u::NetZone* zone,
                                                       const std::vector<unsigned long>& coord,
                                                       unsigned long id) -> simgrid::s4u::Host* {
              try {
                const auto fun = py::reinterpret_borrow<py::function>(cb_p);
                return py::cast<simgrid::s4u::Host*>(fun(zone, coord, id));
              } catch (const py::error_already_set& e) {
                xbt_die("Error while executing the set_host_cb lambda : %s", e.what());
              }
            });
        }, "Set a host callback to a cluster zone")
      .def("set_netzone_cb", [](simgrid::s4u::NetZone* zone, py::object cb) -> simgrid::s4u::NetZone* {
            cb.inc_ref(); // keep alive after return
            return zone->set_netzone_cb([cb_p = cb.ptr()](simgrid::s4u::NetZone* zone,
                                                          const std::vector<unsigned long>& coord,
                                                          unsigned long id) -> simgrid::s4u::NetZone* {
              try {
                const auto fun = py::reinterpret_borrow<py::function>(cb_p);
                return py::cast<simgrid::s4u::NetZone*>(fun(zone, coord, id));
              } catch (const py::error_already_set& e) {
                xbt_die("Error while executing the set_netzone_cb lambda : %s", e.what());
              }
            });
        }, "Set a netzone callback to a cluster zone")
      .def("set_loopback_cb", [](simgrid::s4u::NetZone* zone, py::object cb) -> simgrid::s4u::NetZone* {
            cb.inc_ref(); // keep alive after return
            return zone->set_loopback_cb([cb_p = cb.ptr()](simgrid::s4u::NetZone* zone,
                                                           const std::vector<unsigned long>& coord,
                                                           unsigned long id) -> simgrid::s4u::Link* {
              try {
                const auto fun = py::reinterpret_borrow<py::function>(cb_p);
                return py::cast<simgrid::s4u::Link*>(fun(zone, coord, id));
              } catch (const py::error_already_set& e) {
                xbt_die("Error while executing the set_loopback_cb lambda : %s", e.what());
              }
            });
        }, "Set a loopback callback to a cluster zone")
      .def("set_limiter_cb", [](simgrid::s4u::NetZone* zone, py::object cb) -> simgrid::s4u::NetZone* {
            cb.inc_ref(); // keep alive after return
            return zone->set_limiter_cb([cb_p = cb.ptr()](simgrid::s4u::NetZone* zone,
                                                          const std::vector<unsigned long>& coord,
                                                          unsigned long id) -> simgrid::s4u::Link* {
              try {
                const auto fun = py::reinterpret_borrow<py::function>(cb_p);
                return py::cast<simgrid::s4u::Link*>(fun(zone, coord, id));
              } catch (const py::error_already_set& e) {
                xbt_die("Error while executing the set_limiter_cb lambda : %s", e.what());
              }
            });
        }, "Set a limiter callback to a cluster zone")
      .def("add_netzone_torus", 
               py::overload_cast<const std::string&, const std::vector<unsigned long>&,
               const std::string&, const std::string&, 
               simgrid::s4u::Link::SharingPolicy>(&simgrid::s4u::NetZone::add_netzone_torus), 
               "Creates a cluster of type Torus")
      .def("add_netzone_torus", 
               py::overload_cast<const std::string&, const std::vector<unsigned long>&,
               double, double, simgrid::s4u::Link::SharingPolicy>(&simgrid::s4u::NetZone::add_netzone_torus),
               "Creates a cluster of type Torus")
      .def("add_netzone_fatTree", 
               py::overload_cast<const std::string&, unsigned int, const std::vector<unsigned int>&,
               const std::vector<unsigned int>&, const std::vector<unsigned int>&,
               const std::string&, const std::string&, simgrid::s4u::Link::SharingPolicy>
               (&simgrid::s4u::NetZone::add_netzone_fatTree), 
               "Creates a cluster of type Fat-Tree")
      .def("add_netzone_fatTree", 
               py::overload_cast<const std::string&, unsigned int, const std::vector<unsigned int>&,
               const std::vector<unsigned int>&, const std::vector<unsigned int>&,
               double, double, simgrid::s4u::Link::SharingPolicy>(&simgrid::s4u::NetZone::add_netzone_fatTree),
               "Creates a cluster of type Fat-Tree")
      .def("add_netzone_dragonfly", 
               py::overload_cast<const std::string&,const std::pair<unsigned int, unsigned int>&,
               const std::pair<unsigned int, unsigned int>&, const std::pair<unsigned int, unsigned int>&,
               unsigned int, double, double, simgrid::s4u::Link::SharingPolicy>
               (&simgrid::s4u::NetZone::add_netzone_dragonfly), "Creates a cluster of type Dragonfly")
      .def("add_netzone_dragonfly", 
               py::overload_cast<const std::string&,const std::pair<unsigned int, unsigned int>&,
               const std::pair<unsigned int, unsigned int>&, const std::pair<unsigned int, unsigned int>&,
               unsigned int, const std::string&, const std::string&, simgrid::s4u::Link::SharingPolicy>
               (&simgrid::s4u::NetZone::add_netzone_dragonfly), "Creates a cluster of type Dragonfly")
      .def("add_netzone_full", &simgrid::s4u::NetZone::add_netzone_full, "Creates a zone of type FullZone")
      .def("add_netzone_star", &simgrid::s4u::NetZone::add_netzone_star, "Creates a zone of type Star")
      .def("add_netzone_floyd", &simgrid::s4u::NetZone::add_netzone_floyd, "Creates a zone of type Floyd")
      .def("add_netzone_dijkstra", &simgrid::s4u::NetZone::add_netzone_dijkstra, "Creates a zone of type Dijkstra")
      .def("add_netzone_vivaldi", &simgrid::s4u::NetZone::add_netzone_vivaldi, "Creates a zone of type Vivaldi")
      .def("add_netzone_empty", &simgrid::s4u::NetZone::add_netzone_empty, "Creates a zone of type Empty")
      .def("add_netzone_wifi", &simgrid::s4u::NetZone::add_netzone_wifi, "Creates a zone of type Wi-Fi")
      .def("add_component", &simgrid::s4u::NetZone::add_component, "Add NetPoint component to NetZone")
      .def("add_route",
           py::overload_cast<const simgrid::s4u::Host*, const simgrid::s4u::Host*,
                             const std::vector<simgrid::s4u::LinkInRoute>&, bool>(&simgrid::s4u::NetZone::add_route),
           "Add a route between 2 hosts, and its symmetrical route in the other direction")
      .def("add_route",
           py::overload_cast<const simgrid::s4u::Host*, const simgrid::s4u::Host*,
                             const std::vector<const simgrid::s4u::Link*>&>(&simgrid::s4u::NetZone::add_route),
           "Add a route between 2 hosts, and its symmetrical route in the other direction")
      .def("add_route",
           py::overload_cast<const simgrid::s4u::NetZone*, const simgrid::s4u::NetZone*,
                             const std::vector<simgrid::s4u::LinkInRoute>&, bool>(&simgrid::s4u::NetZone::add_route),
           "Add a route between 2 netzones, and its symmetrical route in the other direction. The gateway of each "
           "zone gets used.")
      .def("add_route",
           py::overload_cast<const simgrid::s4u::NetZone*, const simgrid::s4u::NetZone*,
                             const std::vector<const simgrid::s4u::Link*>&>(&simgrid::s4u::NetZone::add_route),
           "Add a route between 2 netzones, and its symmetrical route in the other direction. The gateway of each "
           "zone gets used.")
      .def("add_bypass_route", &simgrid::s4u::NetZone::add_bypass_route, "Add bypass route to NetZone.")
      .def("add_host", py::overload_cast<const std::string&, double>(&simgrid::s4u::NetZone::add_host),
           "Adds a host")
      .def("add_host", py::overload_cast<const std::string&, const std::string&>(&simgrid::s4u::NetZone::add_host),
           "Adds a host")
      .def("add_host", py::overload_cast<const std::string&, const std::vector<double>&>(&simgrid::s4u::NetZone::add_host),
           "Adds a host")
      .def("add_host", py::overload_cast<const std::string&, const std::vector<std::string>&>(&simgrid::s4u::NetZone::add_host),
           "Adds a host")
      .def("create_host", [](std::string const& name) {
            PyErr_WarnEx(PyExc_DeprecationWarning, "create_host() is deprecated, use Netzone.add_host().", 2);
            throw std::logic_error("Please call Netzone.add_host() instead");
            },
            "Creates a host") // XBT_ATTRIB_DEPRECATED_v401 
      .def("add_link", py::overload_cast<const std::string&, double>(&simgrid::s4u::NetZone::add_link),
           "Adds a network link")
      .def("add_link",
           py::overload_cast<const std::string&, const std::string&>(&simgrid::s4u::NetZone::add_link),
           "Adds a network link")
      .def("add_link",
           py::overload_cast<const std::string&, const std::vector<double>&>(&simgrid::s4u::NetZone::add_link),
           "Adds a network link")
      .def("add_link",
           py::overload_cast<const std::string&, const std::vector<std::string>&>(&simgrid::s4u::NetZone::add_link),
           "Adds a network link")
      .def("create_link", [](std::string const& name) {
            PyErr_WarnEx(PyExc_DeprecationWarning, "create_link() is deprecated, use Netzone.add_link().", 2);
            throw std::logic_error("Please call Netzone.add_link() instead");
            },
            "Creates a link") // XBT_ATTRIB_DEPRECATED_v401 
      .def("add_split_duplex_link",
           py::overload_cast<const std::string&, double, double>(&simgrid::s4u::NetZone::add_split_duplex_link),
           "Adds a split-duplex link")
      .def("add_split_duplex_link",
           py::overload_cast<const std::string&, const std::string&, const std::string&>(
               &simgrid::s4u::NetZone::add_split_duplex_link),
           "Adds a split-duplex link")
      .def("add_split_duplex_link",
          [](simgrid::s4u::NetZone& net, const std::string& name, double bw) {
            return net.add_split_duplex_link(name, bw);
          },
          "Adds a split-duplex link")
      .def("add_split_duplex_link",
          [](simgrid::s4u::NetZone& net, const std::string& name, const std::string& bw) {
            return net.add_split_duplex_link(name, bw);
          },
          "Adds a split-duplex link")
      .def("create_split_duplex_link", [](std::string const& name) {
            PyErr_WarnEx(PyExc_DeprecationWarning,
                         "create_split_duplex_link() is deprecated, use Netzone.add_split_duplex_link().", 2);
            throw std::logic_error("Please call Netzone.add_split_duplex_link() instead");
          },
          "Creates a split-duplex link") // XBT_ATTRIB_DEPRECATED_v401 
      .def("add_router", &simgrid::s4u::NetZone::add_router, "Adds a router")
      .def("create_router", [](std::string const& name) {
            PyErr_WarnEx(PyExc_DeprecationWarning, "create_router() is deprecated, use Netzone.add_router().", 2);
            throw std::logic_error("Please call Netzone.add_router() instead");
          },
          "Creates a router") // XBT_ATTRIB_DEPRECATED_v401 
      .def_property_readonly("parent", &simgrid::s4u::NetZone::get_parent,
                             "The parent netzone, or None for the root netzone (read-only property).")
      // Keep `set_parent` method for backward compatibility.
      .def("get_children", &simgrid::s4u::NetZone::get_children, "Retrieve the list of direct children netzones.")
      .def("get_property", &simgrid::s4u::NetZone::get_property,
           "Retrieve the property value (or None if not set).")
      .def("get_properties", &simgrid::s4u::NetZone::get_properties, "Retrieve all the properties of this netzone.")
      .def("set_property", &simgrid::s4u::NetZone::set_property, "Set a property (old values will be overwritten)")
      .def("set_gateway", py::overload_cast<const simgrid::s4u::Host*>(&simgrid::s4u::NetZone::set_gateway),
           "Specify the gateway of this zone, to be used for inter-zone routes")
      .def("set_gateway", py::overload_cast<simgrid::kernel::routing::NetPoint*>(&simgrid::s4u::NetZone::set_gateway),
           "Specify the gateway of this zone, to be used for inter-zone routes")
      .def_property_readonly("netpoint", &simgrid::s4u::NetZone::get_netpoint,
                             "Retrieve the netpoint associated to this zone")
      .def("seal", &simgrid::s4u::NetZone::seal, "Seals this netzone, finishing its configuration")
      .def_property_readonly("name", &simgrid::s4u::NetZone::get_name,
                             "The name of this network zone (read-only property).")
      .def_property_readonly("all_hosts", &simgrid::s4u::NetZone::get_all_hosts,
                             "The list of all hosts included in this netzone, and its children netzones (read-only "
                             "property).")
      .def_property_readonly("host_count", &simgrid::s4u::NetZone::get_host_count,
                             "The amount of hosts included in this netzone, and its children netzones (read-only "
                             "property).")
      .def(
          "__repr__", [](const simgrid::s4u::NetZone net) { return "NetZone(" + net.get_name() + ")"; },
          "Textual representation of the NetZone");

  /* Class ClusterCallbacks */ // XBT_ATTRIB_DEPRECATED_v401
  cluster_callbacks
      .def(py::init<const std::function<simgrid::s4u::ClusterCallbacks::ClusterNetZoneCb>&,
                    const std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>&,
                    const std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>&>());

  /* Class FatTreeParams */ // XBT_ATTRIB_DEPRECATED_v401
  fat_tree_params
      .def(py::init<unsigned int, const std::vector<unsigned int>&, const std::vector<unsigned int>&,
                    const std::vector<unsigned int>&>());

  /* Class DragonflyParams */ // XBT_ATTRIB_DEPRECATED_v401
  dragonfly_params
      .def(py::init<const std::pair<unsigned int, unsigned int>&, const std::pair<unsigned int, unsigned int>&,
                    const std::pair<unsigned int, unsigned int>&, unsigned int>());

  /* Class Host */
  host.def_static("by_name", &Host::by_name, py::arg("name"), "Retrieves a host from its name, or die")
      .def_static("by_name_or_null", &Host::by_name_or_null, py::arg("name"),
                  "Retrieves a host from its name or None if there is no such host")
      .def(
          "route_to",
          [](const simgrid::s4u::Host* h, const simgrid::s4u::Host* to) {
            auto* list = new std::vector<Link*>();
            double bw  = 0;
            h->route_to(to, *list, &bw);
            return make_tuple(list, bw);
          },
          "Retrieves the list of links and the bandwidth between two hosts.")
      .def(
          "set_speed_profile",
          [](Host* h, const std::string& profile, double period) {
            h->set_speed_profile(simgrid::kernel::profile::ProfileBuilder::from_string("", profile, period));
          },
          "Specify a profile modeling the external load according to an exhaustive list. "
          "Each line of the profile describes timed events as ``date ratio``. "
          "For example, the following content describes an host which computational speed is initially 1 (i.e, 100%) "
          "and then halved after 42 seconds:\n\n"
          ".. code-block:: python\n\n"
          "   \"\"\"\n"
          "   0 1.0\n"
          "   42 0.5\n"
          "   \"\"\"\n\n"
          ".. warning:: Don't get fooled: bandwidth and latency profiles of links contain absolute values,"
          " while speed profiles of hosts contain ratios.\n\n"
          "The second function parameter is the periodicity: the time to wait after the last event to start again over "
          "the list. Set it to -1 to not loop over.")
      .def(
          "set_state_profile",
          [](Host* h, const std::string& profile, double period) {
            h->set_state_profile(simgrid::kernel::profile::ProfileBuilder::from_string("", profile, period));
          },
          "Specify a profile modeling the churn. "
          "Each line of the profile describes timed events as ``date boolean``, where the boolean (0 or 1) tells "
          "whether the host is on. "
          "For example, the following content describes a link which turns off at t=1 and back on at t=2:\n\n"
          ".. code-block:: python\n\n"
          "   \"\"\"\n"
          "   1.0 0\n"
          "   2.0 1\n"
          "   \"\"\"\n\n"
          "The second function parameter is the periodicity: the time to wait after the last event to start again over "
          "the list. Set it to -1 to not loop over.")
      .def_property_readonly("pstate_count", &Host::get_pstate_count,
                             "Returns the total count of pstates defined "
                             "for this host")
      .def("pstate_speed", &Host::get_pstate_speed,
           "Returns the peak computing speed in flops/s at the given pstate, NOT taking the external load into "
           "account")
      .def_property_readonly("netpoint", &Host::get_netpoint, "Retrieve the netpoint associated to this zone")
      .def_property_readonly("disks", &Host::get_disks, "The list of disks on this host (read-only).")
      .def_property_readonly("all_actors", &Host::get_all_actors, "Returns the list of all actors on the host")
      .def("get_disks", &Host::get_disks, "Returns the list of disks attached to this host")
      .def("get_disk_by_name", &Host::get_disk_by_name,
           "Retrieve a disk attached to this host from its name, or None if it does not exist")
      .def_property("core_count", &Host::get_core_count, py::cpp_function(&Host::set_core_count),
                    "The number of cores of the processor (read/write property).")
      .def("set_coordinates", &Host::set_coordinates,
           "Set the coordinates of this host, for use with the vivaldi routing model")
      .def("set_sharing_policy", &simgrid::s4u::Host::set_sharing_policy,
           "Describes how the CPU is shared between concurrent tasks. Note that the NONLINEAR callback is in the "
           "critical path of the solver, so it should be fast.",
           py::arg("policy"), py::arg("cb") = simgrid::s4u::NonLinearResourceCb())
      .def(
          "set_cpu_factor_cb",
          [](Host* h, py::object cb) {
            cb.inc_ref(); // keep alive after return
            h->set_cpu_factor_cb([cb_p = cb.ptr()](Host& host, double flops) -> double {
              try {
                const auto fun = py::reinterpret_borrow<py::function>(cb_p);
                return py::cast<double>(fun(&host, flops));
              } catch (const py::error_already_set& e) {
                xbt_die("Error while executing the set_cpu_factor_cb lambda: %s", e.what());
              }
            });
          },
          py::arg("cb"),
          "Specify a callback that computes a multiplicative factor applied to the speed of this host, to model the "
          "speed variability of the executions. The callback takes the host and the amount of flops of the "
          "execution as parameters, and returns the factor. Note that this callback is in the critical path of the "
          "solver, so it should be fast.")
      .def("create_vm", py::overload_cast<const std::string&, int>(&Host::create_vm), py::arg("name"),
           py::arg("core_amount"),
           "Create a :py:class:`~simgrid.VirtualMachine` on this host, with the given amount of cores. Call start() "
           "on the result to boot it.")
      .def("create_vm", py::overload_cast<const std::string&, int, size_t>(&Host::create_vm), py::arg("name"),
           py::arg("core_amount"), py::arg("ramsize"),
           "Create a :py:class:`~simgrid.VirtualMachine` on this host, with the given amount of cores and of RAM (in "
           "bytes). Call start() on the result to boot it.")
      .def_property_readonly(
          "consumed_energy", [](const Host* h) { return sg_host_get_consumed_energy(h); },
          "The total energy consumed by this host so far, in Joules (read-only property). Requires the host energy "
          "plugin (see :py:func:`sg_host_energy_plugin_init`).")
      .def_property_readonly(
          "current_consumption", [](const Host* h) { return sg_host_get_current_consumption(h); },
          "The current power consumption of this host, in Watts (read-only property). Requires the host energy "
          "plugin.")
      .def_property_readonly(
          "idle_consumption", [](const Host* h) { return sg_host_get_idle_consumption(h); },
          "The power dissipated by this host when idling at pstate 0, in Watts (read-only property). Requires the "
          "host energy plugin.")
      .def(
          "idle_consumption_at", [](const Host* h, int pstate) { return sg_host_get_idle_consumption_at(h, pstate); },
          py::arg("pstate"),
          "The power dissipated by this host when idling at the given pstate, in Watts. Requires the host energy "
          "plugin.")
      .def(
          "wattmin_at", [](const Host* h, int pstate) { return sg_host_get_wattmin_at(h, pstate); }, py::arg("pstate"),
          "The power dissipated by this host at the given pstate when its CPU is at 0 or epsilon% usage, in Watts. "
          "Requires the host energy plugin.")
      .def(
          "wattmax_at", [](const Host* h, int pstate) { return sg_host_get_wattmax_at(h, pstate); }, py::arg("pstate"),
          "The power dissipated by this host at the given pstate when its CPU is at 100% usage, in Watts. Requires "
          "the host energy plugin.")
      .def(
          "power_range_slope_at", [](const Host* h, int pstate) { return sg_host_get_power_range_slope_at(h, pstate); },
          py::arg("pstate"),
          "The slope of the power consumption between 0% and 100% CPU usage at the given pstate, in Watts. Requires "
          "the host energy plugin.")
      .def("exec_init", &Host::exec_init, py::arg("flops"),
           "Initialize an execution of the given amount of flops on this host. Call start() or wait() to start it.")
      .def("exec_async", &Host::exec_async, py::arg("flops"),
           "Start an execution of the given amount of flops on this host, and return it without waiting for its "
           "completion.")
      .def(
          "add_actor",
          [](Host* host, const std::string& name, py::object fun, py::args args) {
            fun.inc_ref();  // keep alive after return
            args.inc_ref(); // keep alive after return
            return host->add_actor(name, [fun_p = fun.ptr(), args_p = args.ptr()]() {
              try {
                const auto fun  = py::reinterpret_borrow<py::object>(fun_p);
                const auto args = py::reinterpret_borrow<py::args>(args_p);
                fun(*args);
              } catch (const py::error_already_set& ex) {
                if (ex.matches(pyForcefulKillEx)) {
                  XBT_VERB("Actor killed");
                  simgrid::ForcefulKillException::do_throw(); // Forward that ForcefulKill exception
                }
                throw;
              }
            });
          },
          "Create an actor from a function or an object. See the :ref:`example <s4u_ex_actors_create>`.")
      .def("add_disk", py::overload_cast<const std::string&, double, double>(&Host::add_disk),
           "Add a disk to this host")
      .def("add_disk", py::overload_cast<const std::string&, const std::string&, const std::string&>(&Host::add_disk),
           "Add a disk to this host")
      .def(
          "create_disk",
          [](std::string const& name) {
            PyErr_WarnEx(PyExc_DeprecationWarning, "create_disk) is deprecated, use Host.add_disk().", 2);
            throw std::logic_error("Please call Host.add_disk() instead");
          },
          "Creates a disk") // XBT_ATTRIB_DEPRECATED_v401
      .def("get_property", &Host::get_property, "Retrieve the value of a host property (or None if not set)")
      .def("get_properties", &Host::get_properties, "Retrieve all the properties of this host")
      .def("set_property", &Host::set_property, "Set a host property (old values will be overwritten)")
      .def("set_properties", &Host::set_properties,
           "Set several host properties at once, adding to the ones "
           "already set")
      .def_property("concurrency_limit", &Host::get_concurrency_limit, py::cpp_function(&Host::set_concurrency_limit),
                    "The max amount of executions that can take place on this host at the same time (read/write "
                    "property). Use -1 to set no limit.")
      .def("seal", &Host::seal,
           "Seals this host, finishing its configuration. No more configuration is allowed "
           "after the seal.")
      .def("turn_off", &Host::turn_off, "Turns this host off. All actors are forcefully stopped.")
      .def("turn_on", &Host::turn_on,
           "Turns this host on if it was previously off. This call does nothing if the host is already on. If it "
           "was off, all actors which were marked 'autorestart' on that host will be restarted automatically.")
      .def_property("pstate", &Host::get_pstate, py::cpp_function(&Host::set_pstate),
                    "The current pstate (read/write property).")
      .def_static("current", &Host::current, "Retrieves the host on which the running actor is located.")
      .def_property_readonly("is_on", &Host::is_on,
                             "Returns if that host is currently up and running (read-only property).")
      .def_property_readonly("actor_count", &Host::get_actor_count,
                             "Returns actor count placed on this Host (read-only property).")
      .def_property_readonly("name", &Host::get_name, "The name of this host (read-only property).")
      .def_property_readonly("load", &Host::get_load,
                             "Returns the current computation load (in flops per second), NOT taking the external load "
                             "into account. This is the currently achieved speed (read-only property).")
      .def_property_readonly(
          "speed", &Host::get_speed,
          "The peak computing speed in flops/s at the current pstate, NOT taking the external load into account. "
          "This is the max potential speed (read-only property).")
      .def_property_readonly("available_speed", &Host::get_available_speed,
                             "Get the available speed ratio, between 0 and 1.\n"
                             "This accounts for external load (see :py:func:`set_speed_profile() "
                             "<simgrid.Host.set_speed_profile>`) (read-only property).")
      .def_static(
          "on_creation_cb",
          [](py::object cb) {
            cb.inc_ref(); // keep alive after return
            Host::on_creation_cb([cb_p = cb.ptr()](Host& h) {
              try {
                const auto fun = py::reinterpret_borrow<py::function>(cb_p);
                fun(&h);
              } catch (const py::error_already_set& e) {
                xbt_die("Error while executing the on_creation lambda : %s", e.what());
              }
            });
          },
          "")
      .def(
          "__repr__", [](const Host* h) { return "Host(" + h->get_name() + ")"; },
          "Textual representation of the Host.");

  m.def("sg_host_load_plugin_init", [host]() {
    sg_host_load_plugin_init();

    static_cast<pybind11::class_<simgrid::s4u::Host, std::unique_ptr<simgrid::s4u::Host, pybind11::nodelete>>>(host)
        .def(
            "reset_load", [](const Host* h) { sg_host_load_reset(h); },
            "Reset counters of the host load plugin for this host.")
        .def_property_readonly(
            "current_load", [](const Host* h) { return sg_host_get_current_load(h); }, "Current load of the host.")
        .def_property_readonly(
            "avg_load", [](const Host* h) { return sg_host_get_avg_load(h); }, "Average load of the host.")
        .def_property_readonly(
            "idle_time", [](const Host* h) { return sg_host_get_idle_time(h); }, "Idle time of the host")
        .def_property_readonly(
            "total_idle_time", [](const Host* h) { return sg_host_get_total_idle_time(h); },
            "Total idle time of the host.")
        .def_property_readonly(
            "computed_flops", [](const Host* h) { return sg_host_get_computed_flops(h); },
            "Computed flops of the host.");
  });

  m.def("sg_host_carbon_footprint_plugin_init", &sg_host_carbon_footprint_plugin_init,
        "Initialize the host carbon footprint plugin (and the host energy plugin on which it relies). It must be "
        "called before the platform gets loaded.");
  m.def("sg_host_carbon_footprint_load_trace_file", &sg_host_carbon_footprint_load_trace_file, py::arg("trace_file"),
        "Load a CSV file giving the carbon intensity of the hosts over time. Each line is of the form "
        "``host_id,timestamp,carbon_intensity``.");
  m.def("sg_storage_file_system_init", &sg_storage_file_system_init,
        "Initialize the file system plugin, that provides the :py:class:`~simgrid.File` class. It must be called "
        "before the platform gets loaded.");
  m.def("sg_host_energy_plugin_init", &sg_host_energy_plugin_init,
        "Initialize the host energy plugin, that provides :py:attr:`Host.consumed_energy <simgrid.Host.consumed_energy>` "
        "and the related functions. It must be called before the platform gets loaded.");
  m.def("sg_host_energy_update_all", &sg_host_energy_update_all,
        "Update the energy consumption of all hosts. The consumption is lazily computed, so you need to call this "
        "before reading it from outside of an actor (e.g., after the end of the simulation).");
  m.def("sg_link_energy_plugin_init", &sg_link_energy_plugin_init,
        "Initialize the link energy plugin, that provides :py:attr:`Link.consumed_energy "
        "<simgrid.Link.consumed_energy>`. It must be called before the platform gets loaded.");
  m.def("sg_wifi_energy_plugin_init", &sg_wifi_energy_plugin_init,
        "Initialize the wifi energy plugin, that computes the energy dissipated by the wifi links. It must be called "
        "before the platform gets loaded.");
  m.def("sg_vm_live_migration_plugin_init", &sg_vm_live_migration_plugin_init,
        "Initialize the live migration plugin, that provides :py:func:`VirtualMachine.migrate() "
        "<simgrid.VirtualMachine.migrate>`. It must be called before the platform gets loaded.");

  host_sharing_policy
      .value("NONLINEAR", simgrid::s4u::Host::SharingPolicy::NONLINEAR)
      .value("LINEAR", simgrid::s4u::Host::SharingPolicy::LINEAR);

  /* Class VirtualMachine */
  vm.def("start", &VirtualMachine::start, "Immediately boot this virtual machine, which must be in the CREATED state.")
      .def("suspend", &VirtualMachine::suspend,
           "Suspend this virtual machine. The actors running on it are not scheduled anymore until resume() is called.")
      .def("resume", &VirtualMachine::resume, "Resume this virtual machine, previously suspended with suspend().")
      .def("shutdown", &VirtualMachine::shutdown,
           "Immediately shut down this virtual machine. The actors running on it are forcefully stopped.")
      .def("destroy", &VirtualMachine::destroy,
           "Immediately destroy this virtual machine, freeing its resources. It cannot be used afterward.")
      .def_property(
          "pm", &VirtualMachine::get_pm, [](VirtualMachine* self, Host* pm) { self->set_pm(pm); },
          "The physical machine (host) on which this virtual machine runs (read/write property). Changing it moves "
          "the VM instantly: use :py:func:`migrate` to simulate the cost of a live migration.")
      .def_property(
          "ramsize", &VirtualMachine::get_ramsize, [](VirtualMachine* self, size_t size) { self->set_ramsize(size); },
          "The amount of RAM dedicated to this virtual machine, in bytes (read/write property).")
      .def(
          "set_bound", [](VirtualMachine* self, double bound) { self->set_bound(bound); }, py::arg("bound"),
          "Set the CPU utilization bound of this virtual machine (in flops/s), that is the max amount of the "
          "underlying host's computing power that this VM (and the actors it hosts) can use. 0 means no bound.")
      .def_property_readonly("state", &VirtualMachine::get_state,
                             "The current state of this virtual machine (read-only property).")
      .def(
          "migrate",
          [](VirtualMachine* self, Host* dst_pm) {
            ensure_live_migration_plugin();
            sg_vm_migrate(self, dst_pm);
          },
          py::arg("dst_pm"),
          "Migrate this virtual machine to the given physical machine, simulating the transfer of its memory over the "
          "network. The calling actor is blocked until the end of the migration. Requires the live migration plugin "
          "(see :py:func:`sg_vm_live_migration_plugin_init`).")
      .def_static(
          "on_creation_cb",
          [](py::object cb) {
            VirtualMachine::on_creation_cb(wrap_resource_cb<VirtualMachine, VirtualMachine&>(cb, "on_creation"));
          },
          py::arg("cb"), "Add a callback fired when any VM is created, with the VM as parameter.")
      .def(
          "__repr__", [](const VirtualMachine* self) { return "VirtualMachine(" + self->get_name() + ")"; },
          "Textual representation of the VirtualMachine.");

  /* Signals of the VM life cycle: <name, static signal, per-VM signal> */
  using VmCbSetter     = void (*)(const std::function<void(VirtualMachine const&)>&);
  using VmThisCbSetter = void (VirtualMachine::*)(const std::function<void(VirtualMachine const&)>&);
  const std::vector<std::tuple<const char*, const char*, VmCbSetter, VmThisCbSetter>> vm_signals = {
      {"start", "starts booting", &VirtualMachine::on_start_cb, &VirtualMachine::on_this_start_cb},
      {"started", "is booted", &VirtualMachine::on_started_cb, &VirtualMachine::on_this_started_cb},
      {"shutdown", "is shut down", &VirtualMachine::on_shutdown_cb, &VirtualMachine::on_this_shutdown_cb},
      {"suspend", "is suspended", &VirtualMachine::on_suspend_cb, &VirtualMachine::on_this_suspend_cb},
      {"resume", "is resumed", &VirtualMachine::on_resume_cb, &VirtualMachine::on_this_resume_cb},
      {"destruction", "is destroyed", &VirtualMachine::on_destruction_cb, &VirtualMachine::on_this_destruction_cb},
      {"migration_start", "starts a migration", &VirtualMachine::on_migration_start_cb,
       &VirtualMachine::on_this_migration_start_cb},
      {"migration_end", "ends a migration", &VirtualMachine::on_migration_end_cb,
       &VirtualMachine::on_this_migration_end_cb}};
  for (auto const& [name, what, setter, this_setter] : vm_signals) {
    vm.def_static(
        (std::string("on_") + name + "_cb").c_str(),
        [setter, name](py::object cb) { setter(wrap_resource_cb<VirtualMachine>(cb, name)); }, py::arg("cb"),
        (std::string("Add a callback fired when any VM ") + what + ", with the VM as parameter.").c_str());
    vm.def(
        (std::string("on_this_") + name + "_cb").c_str(),
        [this_setter, name](VirtualMachine* self, py::object cb) {
          (self->*this_setter)(wrap_resource_cb<VirtualMachine>(cb, name));
        },
        py::arg("cb"), (std::string("Add a callback fired when this VM ") + what + ", with the VM as parameter.").c_str());
  }

  vm_state
      .value("CREATED", VirtualMachine::State::CREATED, "Created, but not started yet.")
      .value("RUNNING", VirtualMachine::State::RUNNING, "Started and running.")
      .value("SUSPENDED", VirtualMachine::State::SUSPENDED, "Suspended (see VirtualMachine.suspend()).")
      .value("DESTROYED", VirtualMachine::State::DESTROYED, "Destroyed: its resources have been freed.");

  /* Class Disk */
  disk.def("read", py::overload_cast<sg_size_t, double>(&simgrid::s4u::Disk::read, py::const_),
           "Blocking read of the given amount of bytes from this disk. Returns the amount of bytes actually read.",
           py::arg("size"), py::arg("priority") = 1)
      .def("write", py::overload_cast<sg_size_t, double>(&simgrid::s4u::Disk::write, py::const_),
           "Blocking write of the given amount of bytes to this disk. Returns the amount of bytes actually written.",
           py::arg("size"), py::arg("priority") = 1)
      .def("read_async", &simgrid::s4u::Disk::read_async,
           "Non-blocking read of the given amount of bytes from this "
           "disk")
      .def("write_async", &simgrid::s4u::Disk::write_async,
           "Non-blocking write of the given amount of bytes to this "
           "disk")
      .def("io_init", &simgrid::s4u::Disk::io_init, py::arg("size"), py::arg("type"),
           "Initialize an I/O operation (of the given IoOpType) of the given amount of bytes on this disk. Call "
           "start() or wait() to start it.")
      .def_property(
          "data", [](const Disk* d) { return get_py_data(d); }, [](Disk* d, py::object data) { set_py_data(d, data); },
          "Some user data attached to this disk, or None if not set (read/write property).")
      .def_property_readonly(
          "mount_point",
          [](const Disk* d) {
            ensure_file_system_plugin();
            return sg_disk_get_mount_point(d);
          },
          "The mount point of this disk (read-only property). Requires the file system plugin.")
      .def_property_readonly(
          "size",
          [](const Disk* d) {
            ensure_file_system_plugin();
            return sg_disk_get_size(d);
          },
          "The total size of this disk, in bytes (read-only property). Requires the file system plugin.")
      .def_property_readonly(
          "size_used",
          [](const Disk* d) {
            ensure_file_system_plugin();
            return sg_disk_get_size_used(d);
          },
          "The amount of bytes used on this disk (read-only property). Requires the file system plugin.")
      .def_property_readonly(
          "size_free",
          [](const Disk* d) {
            ensure_file_system_plugin();
            return sg_disk_get_size_free(d);
          },
          "The amount of bytes still available on this disk (read-only property). Requires the file system plugin.")
      .def_property("read_bandwidth", py::cpp_function{&simgrid::s4u::Disk::get_read_bandwidth},
                    py::cpp_function{&simgrid::s4u::Disk::set_read_bandwidth},
                    "The read bandwidth of this disk is the max speed at which read operations progress")
      .def_property("write_bandwidth", py::cpp_function{&simgrid::s4u::Disk::get_write_bandwidth},
                    py::cpp_function{&simgrid::s4u::Disk::set_write_bandwidth},
                    "The write bandwidth of this disk is the max speed at which write operations progress")
      .def_property("concurrency_limit", py::cpp_function{&simgrid::s4u::Disk::get_concurrency_limit},
                    py::cpp_function{&simgrid::s4u::Disk::set_concurrency_limit},
                    "The concurrency limit of this disk is the max amount of concurrent I/O operations (extra ones are "
                    "queued). Use -1 to set no limit.")
      .def("set_sharing_policy", &simgrid::s4u::Disk::set_sharing_policy,
           "Describes how this disk is shared between activities for the given operation (read, write, or both).",
           py::arg("op"), py::arg("policy"), py::arg("cb") = simgrid::s4u::NonLinearResourceCb())
      .def("seal", &simgrid::s4u::Disk::seal, "Seals this disk, finishing its configuration")
      .def("turn_on", &Disk::turn_on,
           "Turns this disk on if it was previously off. This call does nothing if the disk is already on.")
      .def("turn_off", &Disk::turn_off, "Turns this disk off. All I/O activities are forcefully stopped.")
      .def_property_readonly("name", &simgrid::s4u::Disk::get_name, "The name of this disk (read-only property).")
      .def_property_readonly("host", &simgrid::s4u::Disk::get_host,
                             "The Host to which this disk is attached (read-only property)")
      .def(
          "__repr__", [](const Disk* d) { return "Disk(" + d->get_name() + ")"; },
          "Textual representation of the Disk");
  disk_sharing_policy
      .value("NONLINEAR", simgrid::s4u::Disk::SharingPolicy::NONLINEAR)
      .value("LINEAR", simgrid::s4u::Disk::SharingPolicy::LINEAR);
  disk_operation
      .value("READ", simgrid::s4u::Disk::Operation::READ)
      .value("WRITE", simgrid::s4u::Disk::Operation::WRITE)
      .value("READWRITE", simgrid::s4u::Disk::Operation::READWRITE);

  /* Class NetPoint */

  /* Class Link */
  link.def("set_latency", py::overload_cast<const std::string&>(&Link::set_latency),
           "Set the latency of this link. Accepts values with units, such as '1s' or '7ms'.\nFull list of accepted "
           "units: w (week), d (day), h, s, ms, us, ns, ps.")
      .def("set_latency", py::overload_cast<double>(&Link::set_latency), "Set the latency as a float (in seconds).")
      // Keep `set_bandwidth` for backward compatibility.
      .def("set_bandwidth", &Link::set_bandwidth, "Set the bandwidth of this link (in bytes per second).")
      .def(
          "set_bandwidth_profile",
          [](Link* l, const std::string& profile, double period) {
            l->set_bandwidth_profile(simgrid::kernel::profile::ProfileBuilder::from_string("", profile, period));
          },
          "Specify a profile modeling the external load according to an exhaustive list. "
          "Each line of the profile describes timed events as ``date bandwidth`` (in bytes per second). "
          "For example, the following content describes a link which bandwidth changes to 40 Mb/s at t=4, and to 6 "
          "Mb/s at t=8:\n\n"
          ".. code-block:: python\n\n"
          "   \"\"\"\n"
          "   4.0 40000000\n"
          "   8.0 60000000\n"
          "   \"\"\"\n\n"
          ".. warning:: Don't get fooled: bandwidth and latency profiles of links contain absolute values,"
          " while speed profiles of hosts contain ratios.\n\n"
          "The second function parameter is the periodicity: the time to wait after the last event to start again over "
          "the list. Set it to -1 to not loop over.")
      .def(
          "set_latency_profile",
          [](Link* l, const std::string& profile, double period) {
            l->set_latency_profile(simgrid::kernel::profile::ProfileBuilder::from_string("", profile, period));
          },
          "Specify a profile modeling the external load according to an exhaustive list. "
          "Each line of the profile describes timed events as ``date latency`` (in seconds). "
          "For example, the following content describes a link which latency changes to 1ms (0.001s) at t=1, and to 2s "
          "at t=2:\n\n"
          ".. code-block:: python\n\n"
          "   \"\"\"\n"
          "   1.0 0.001\n"
          "   2.0 2\n"
          "   \"\"\"\n\n"
          ".. warning:: Don't get fooled: bandwidth and latency profiles of links contain absolute values,"
          " while speed profiles of hosts contain ratios.\n\n"
          "The second function parameter is the periodicity: the time to wait after the last event to start again over "
          "the list. Set it to -1 to not loop over.")
      .def(
          "set_state_profile",
          [](Link* l, const std::string& profile, double period) {
            l->set_state_profile(simgrid::kernel::profile::ProfileBuilder::from_string("", profile, period));
          },
          "Specify a profile modeling the churn. "
          "Each line of the profile describes timed events as ``date boolean``, where the boolean (0 or 1) tells "
          "whether the link is on. "
          "For example, the following content describes a link which turns off at t=1 and back on at t=2:\n\n"
          ".. code-block:: python\n\n"
          "   \"\"\"\n"
          "   1.0 0\n"
          "   2.0 1\n"
          "   \"\"\"\n\n"
          "The second function parameter is the periodicity: the time to wait after the last event to start again over "
          "the list. Set it to -1 to not loop over.")

      .def("turn_on", &Link::turn_on, "Turns this link on.")
      .def("turn_off", &Link::turn_off, "Turns this link off.")
      .def("is_on", &Link::is_on, "Checks whether this link is on.")
      .def_property_readonly("is_used", &Link::is_used, "Check if this link is used (at least one flow uses it).")
      .def("get_sharing_policy", &Link::get_sharing_policy, "Describes how this link is shared between flows.")
      .def("set_sharing_policy", &Link::set_sharing_policy,
           "Describes how this link is shared between flows. Note that the NONLINEAR callback is in the critical "
           "path of the solver, so it should be fast.")
      .def_property("concurrency_limit", &Link::get_concurrency_limit, py::cpp_function(&Link::set_concurrency_limit),
                    "The max amount of communications that can share this link at the same time (read/write "
                    "property). Use -1 to set no limit.")
      // Keep `set_concurrency_limit` method for backward compatibility.
      .def("set_concurrency_limit", &Link::set_concurrency_limit,
           "Set the number of communications that can share this link at the same time. Use this method to "
           "serialize communication flows going through this link. Use -1 to set no limit.")
      .def("set_host_wifi_rate", &Link::set_host_wifi_rate,
           "Set the level of communication speed of the given host on this wifi link. By default, the first level "
           "in the list is used, but this can be used to take the location of hosts into account, or even to model "
           "mobility. Asserts that the link is actually a wifi link.")
      .def_property_readonly(
          "consumed_energy", [](const Link* l) { return sg_link_get_consumed_energy(l); },
          "The total energy consumed by this link so far, in Joules (read-only property). Requires the link energy "
          "plugin (see :py:func:`sg_link_energy_plugin_init`).")
      .def_static("by_name", &Link::by_name, "Retrieves a link from its name, or dies")
      .def_static("by_name_or_null", &Link::by_name_or_null,
                  "Retrieve a link by its name, or None if it does not exist in the platform.")
      .def("seal", &Link::seal, "Seals this link, finishing its configuration")
      .def_property_readonly("name", &Link::get_name, "The name of this link")
      .def_property("bandwidth", &Link::get_bandwidth, py::cpp_function(&Link::set_bandwidth),
                    "The bandwidth (in bytes per second) (r/w property).")
      .def_property_readonly("latency", &Link::get_latency, "The latency (in seconds) (read-only property).")
      .def_property_readonly("load", &Link::get_load,
                             "Returns the current load (in bytes per second) (read-only property).")
      .def("get_property", &Link::get_property, "Retrieve the property value (or None if not set).")
      .def("get_properties", &Link::get_properties, "Retrieve all the properties of this link.")
      .def("set_property", &Link::set_property, "Set a property (old values will be overwritten).")
      .def("set_properties", &Link::set_properties,
           "Set several properties at once, adding to the ones already "
           "set.")
      .def(
          "__repr__", [](const Link* l) { return "Link(" + l->get_name() + ")"; },
          "Textual representation of the Link");
  link_sharing_policy
      .value("NONLINEAR", Link::SharingPolicy::NONLINEAR,
             "This policy takes a callback that specifies the maximal capacity as a function of the number of usage. "
             "See the examples with 'degradation' in their name.")
      .value("WIFI", Link::SharingPolicy::WIFI, "Pseudo-sharing policy requesting wifi-specific sharing.")
      .value("SPLITDUPLEX", Link::SharingPolicy::SPLITDUPLEX,
             "Each link is split in 2, UP and DOWN, one per direction. These links are SHARED.")
      .value("SHARED", Link::SharingPolicy::SHARED,
             "The bandwidth is shared between all comms using that link, regardless of their direction.")
      .value("FATPIPE", Link::SharingPolicy::FATPIPE,
             "Each comm can use the link fully, with no sharing (only a maximum). This is intended to represent the "
             "backbone links that cannot be saturated by concurrent links, but have a maximal bandwidth.");

  /* Class LinkInRoute */
  linkinroute.def(py::init<const Link*>());
  linkinroute.def(py::init<const Link*, simgrid::s4u::LinkInRoute::Direction>());
  linkinroute_direction
      .value("UP", simgrid::s4u::LinkInRoute::Direction::UP)
      .value("DOWN", simgrid::s4u::LinkInRoute::Direction::DOWN)
      .value("NONE", simgrid::s4u::LinkInRoute::Direction::NONE);

  /* Class Split-Duplex Link */
  split_duplex_link
      .def_property_readonly("link_up", &simgrid::s4u::SplitDuplexLink::get_link_up, "Get link direction up")
      .def_property_readonly("link_down", &simgrid::s4u::SplitDuplexLink::get_link_down, "Get link direction down");

  /* Class Mailbox */
  mailbox
      .def(
          "__repr__", [](const Mailbox* self) { return "Mailbox(" + self->get_name() + ")"; },
          "Textual representation of the Mailbox")
      .def_static("by_name", &Mailbox::by_name, py::arg("name"),
                  "Retrieve the mailbox associated to the given name. Mailboxes are created on demand.")
      .def_property_readonly("name", &Mailbox::get_name, "The name of that mailbox (read-only property).")
      .def_property_readonly("ready", &Mailbox::ready,
                             "Check if there is a communication ready to be consumed from a mailbox.")
      .def(
          "put",
          [](Mailbox* self, py::object data, uint64_t size, double timeout) {
            auto* data_ptr = data.inc_ref().ptr();
            self->put(data_ptr, size, timeout);
          },
          "Blocking data transmission with a timeout")
      .def(
          "put",
          [](Mailbox* self, py::object data, uint64_t size) {
            auto* data_ptr = data.inc_ref().ptr();
            self->put(data_ptr, size);
          },
          "Blocking data transmission")
      .def(
          "put_async",
          [](Mailbox* self, py::object data, uint64_t size) {
            auto* data_ptr = data.inc_ref().ptr();
            return self->put_async(data_ptr, size);
          },
          "Creates and start a data transmission to that mailbox.")
      .def(
          "put_init",
          [](Mailbox* self, py::object data, uint64_t size) {
            auto* data_ptr = data.inc_ref().ptr();
            return self->put_init(data_ptr, size);
          },
          "Creates (but don't start) a data transmission to that mailbox.")
      .def(
          "get", [](Mailbox* self) { return py::reinterpret_steal<py::object>(self->get<PyObject>()); },
          "Blocking data reception")
      .def(
          "get_async", [](Mailbox* self) -> CommPtr { return self->get_async(); },
          "Non-blocking data reception. Use data.get() to get the python object after the communication has finished")
      .def(
          "get_init", [](Mailbox* self) -> CommPtr { return self->get_init()->set_dst_data(nullptr, sizeof(void*)); },
          "Creates (but don't start) a data reception from that mailbox. Use get_payload() on the result to get the "
          "python object after the communication has finished.")
      .def("set_receiver", &Mailbox::set_receiver,
           "Declare that the specified actor is a permanent receiver on this mailbox. It means that the "
           "communications sent to this mailbox will start flowing to its host even before it does a get(). This "
           "models the real behavior of TCP and MPI communications, amongst other, and improves the accuracy of "
           "predictions, in particular if your application exhibits swarms of small messages.");

  /* Class MessageQueue */
  mqueue.def(
          "__repr__", [](const MessageQueue* self) { return "MessageQueue(" + self->get_name() + ")"; },
          "Textual representation of the MessageQueue")
      .def_static("by_name", &MessageQueue::by_name, py::arg("name"),
                  "Retrieve the message queue associated to the given name. Message queues are created on demand.")
      .def_property_readonly("name", &MessageQueue::get_name, "The name of that message queue (read-only property).")
      .def_property_readonly("empty", &MessageQueue::empty,
                             "Whether the message queue contains no queued messages (read-only property).")
      .def_property_readonly("size", &MessageQueue::size,
                             "The number of queued messages (read-only property).")
      .def(
          "put", [](MessageQueue* self, py::object data) { self->put(data.inc_ref().ptr()); }, py::arg("data"),
          "Blocking data transmission: put the data in the queue and wait for its reception.")
      .def(
          "put",
          [](MessageQueue* self, py::object data, double timeout) { self->put(data.inc_ref().ptr(), timeout); },
          py::arg("data"), py::arg("timeout"),
          "Blocking data transmission with a timeout. Raises TimeoutException if the data is not received in time.")
      .def(
          "put_async", [](MessageQueue* self, py::object data) { return self->put_async(data.inc_ref().ptr()); },
          py::arg("data"), "Creates and start a data transmission to that message queue.")
      .def(
          "put_init", [](MessageQueue* self, py::object data) { return self->put_init(data.inc_ref().ptr()); },
          py::arg("data"), "Creates (but don't start) a data transmission to that message queue.")
      .def(
          "get", [](MessageQueue* self) { return py::reinterpret_steal<py::object>(self->get<PyObject>()); },
          "Blocking data reception.")
      .def(
          "get",
          [](MessageQueue* self, double timeout) {
            return py::reinterpret_steal<py::object>(self->get<PyObject>(timeout));
          },
          py::arg("timeout"),
          "Blocking data reception with a timeout. Raises TimeoutException if no data is received in time.")
      .def(
          "get_async", [](MessageQueue* self) -> MessPtr { return self->get_async(); },
           "Non-blocking data reception. Use get_payload() on the result to get the python object once the message "
           "is received.")
      .def("get_init", &MessageQueue::get_init,
           "Creates (but don't start) a data reception from that message queue. Use get_payload() on the result to "
           "get the python object once the message is received.");

  /* Class Comm */
  bind_activity_common(comm, "communication");
  comm.def_property_readonly("dst_data_size", py::cpp_function{&Comm::get_dst_data_size},
                             "Retrieve the size of the received data.")
      .def_property_readonly("mailbox", py::cpp_function{&Comm::get_mailbox},
                             "Retrieve the mailbox on which this comm acts. This is None if the comm was created "
                             "through sendto().")
      .def_property_readonly("sender", py::cpp_function{&Comm::get_sender},
                             "Retrieve the actor which is sending this communication. Returns None if the "
                             "communication has not started yet, or if it was created with sendto().")
      .def_property_readonly("remaining", py::cpp_function{&Comm::get_remaining},
                             "Remaining amount of work that this Comm entails")
      .def("set_payload_size", &Comm::set_payload_size, py::arg("bytes"),
           "Specify the amount of bytes which exchange should be simulated.")
      .def("set_rate", &Comm::set_rate, py::arg("rate"),
           "Sets the maximal communication rate (in byte/sec). Must be done before start")
      .def(
          "get_payload",
          [](const Comm* self) { return py::reinterpret_steal<py::object>((PyObject*)self->get_payload()); },
          "Retrieve the message's payload of a get_async. You cannot call this until after the comm termination.")
      .def_static("sendto", &Comm::sendto, py::arg("from"), py::arg("to"), py::arg("simulated_size_in_bytes"),
                  "Do a blocking communication between two arbitrary hosts, bypassing the mailbox and actor "
                  "mechanisms.")
      .def_static("sendto_init", py::overload_cast<Host*, Host*>(&Comm::sendto_init), py::arg("from"), py::arg("to"),
                  "Creates a communication between the two given hosts, bypassing the mailbox mechanism.")
      .def_static("sendto_async", &Comm::sendto_async, py::arg("from"), py::arg("to"),
                  py::arg("simulated_size_in_bytes"),
                  "Do a blocking communication between two arbitrary hosts.\n\nThis initializes a communication that "
                  "completely bypass the mailbox and actors mechanism. There is really no limit on the hosts involved. "
                  "In particular, the actor does not have to be on one of the involved hosts.");

  /* Class Mess */
  bind_activity_common(mess, "message");
  mess.def_property_readonly("queue", &Mess::get_queue, "The message queue on which this message is exchanged.")
      .def_property_readonly("sender", &Mess::get_sender,
                             "The actor sending this message, or None if it is not known yet.")
      .def_property_readonly("receiver", &Mess::get_receiver,
                             "The actor receiving this message, or None if it is not known yet.")
      .def(
          "get_payload",
          [](const Mess* self) { return py::reinterpret_steal<py::object>(static_cast<PyObject*>(self->get_payload())); },
          "Retrieve the payload of a message obtained with get_async() or get_init(). You cannot call this until "
          "after the message reception.");

  /* Class Io */
  bind_activity_common(io, "I/O");
  io.def_property_readonly("remaining", py::cpp_function{&simgrid::s4u::Io::get_remaining},
                           "Amount of bytes that remain to be read or written until completion (read-only property).")
      .def("update_priority", &simgrid::s4u::Io::update_priority, py::arg("priority"),
           "Change the priority of this I/O while it is running. An I/O of priority 2 is twice as fast as an I/O of "
           "priority 1.");

  /* Class Exec */
  bind_activity_common(exec, "execution");
  exec.def_static("init", &simgrid::s4u::Exec::init,
                  "Create an execution that is not assigned to any host yet. Use set_flops_amount() to specify its "
                  "amount of work, and assign it to a host by setting its ``host`` property before (or after) starting "
                  "it: it only actually starts once assigned.")
      .def("set_flops_amount", &simgrid::s4u::Exec::set_flops_amount, py::arg("flops"),
           "Specify the amount of flops that this execution entails. Returns the execution itself.")
      .def("set_bound", &simgrid::s4u::Exec::set_bound, py::arg("bound"),
           "Set the maximal speed at which this execution can progress (in flops/s), even if the host is faster. 0 "
           "means no bound. Returns the execution itself.")
      .def_property_readonly("remaining", py::cpp_function{&simgrid::s4u::Exec::get_remaining},
                             "Amount of flops that remain to be computed until completion (read-only property).")
      .def_property_readonly("remaining_ratio", py::cpp_function{&simgrid::s4u::Exec::get_remaining_ratio},
                             "Amount of work remaining until completion from 0 (completely done) to 1 (nothing done "
                             "yet) (read-only property).")
      .def_property("host", &simgrid::s4u::Exec::get_host, &simgrid::s4u::Exec::set_host,
                    "Host on which this execution runs. Only the first host is returned for parallel executions. "
                    "Changing this value migrates the execution.");

  /* Class Semaphore */
  semaphore
      .def(py::init<>(&Semaphore::create), py::arg("capacity"), "Semaphore constructor.")
      .def("acquire", &Semaphore::acquire,
           "Acquire on the semaphore object with no timeout. Blocks until the semaphore is acquired.")
      .def("acquire_timeout", &Semaphore::acquire_timeout, py::arg("timeout"),
           "Acquire on the semaphore object with a timeout. Blocks until the semaphore is acquired, or returns "
           "true if it has not been acquired after the specified timeout.")
      .def("release", &Semaphore::release, "Release the semaphore.")
      .def_property_readonly("capacity", py::cpp_function{&Semaphore::get_capacity}, "Get the semaphore capacity.")
      .def_property_readonly("would_block", py::cpp_function{&Semaphore::would_block},
                             "Check whether trying to acquire the semaphore would block (in other word, checks whether "
                             "this semaphore has capacity).")
      // Allow semaphores to be automatically acquired/released with a context manager: `with semaphore: ...`
      .def("__enter__", &Semaphore::acquire)
      .def("__exit__",
           [](Semaphore* self, const py::object&, const py::object&, const py::object&) { self->release(); });

  /* Class Mutex */
  mutex
      .def(py::init<>(&Mutex::create), "Mutex constructor (pass True as a parameter to get a recursive Mutex).",
           py::arg("recursive") = false)
      .def("lock", &Mutex::lock, "Block until the mutex is acquired.")
      .def("try_lock", &Mutex::try_lock,
           "Try to acquire the mutex. Return true if the mutex was acquired, false otherwise.")
      .def("unlock", &Mutex::unlock, "Release the mutex.")
      // Allow mutexes to be automatically acquired/released with a context manager: `with mutex: ...`
      .def("__enter__", &Mutex::lock)
      .def("__exit__", [](Mutex* self, const py::object&, const py::object&, const py::object&) { self->unlock(); });

  /* Class Barrier */
  barrier
      .def(py::init<>(&Barrier::create), py::arg("expected_actors"), "Barrier constructor.")
      .def("wait", &Barrier::wait,
           "Blocks into the barrier. Every waiting actor is unblocked once the expected amount of actors reaches "
           "the barrier.");

  /* Class Actor */
  actor
      .def( // XBT_ATTRIB_DEPRECATED_v401
          "create",
          [](const std::string& name, Host* h, py::object fun, py::args args) {
            PyErr_WarnEx(PyExc_DeprecationWarning, "Actor.create() is deprecated, use Engine.add_actor().", 2);

            fun.inc_ref();  // keep alive after return
            args.inc_ref(); // keep alive after return
            return h->add_actor(name, [fun_p = fun.ptr(), args_p = args.ptr()]() {
              try {
                const auto fun  = py::reinterpret_borrow<py::object>(fun_p);
                const auto args = py::reinterpret_borrow<py::args>(args_p);
                fun(*args);
              } catch (const py::error_already_set& ex) {
                if (ex.matches(pyForcefulKillEx)) {
                  XBT_VERB("Actor killed");
                  simgrid::ForcefulKillException::do_throw(); // Forward that ForcefulKill exception
                }
                throw;
              }
            });
          },
          "Create an actor from a function or an object. See the :ref:`example <s4u_ex_actors_create>`.")
      .def_property(
          "host", &Actor::get_host, py::cpp_function(&Actor::set_host),
          "The host on which this actor is located. Changing this value migrates the actor.\n\n"
          "If the actor is currently blocked on an execution activity, the activity is also migrated to the new host. "
          "If it’s blocked on another kind of activity, an error is raised as the mandated code is not written yet. "
          "Please report that bug if you need it.\n\n"
          "Asynchronous activities started by the actor are not migrated automatically, so you have to take care of "
          "this yourself (only you knows which ones should be migrated). ")
      .def_property_readonly("name", &Actor::get_cname, "The name of this actor (read-only property).")
      .def_property_readonly("pid", &Actor::get_pid, "The PID (unique identifier) of this actor (read-only property).")
      .def_property_readonly("ppid", &Actor::get_ppid,
                             "The PID (unique identifier) of the actor that created this one (read-only property).")
      .def_static("by_pid", &Actor::by_pid, py::arg("pid"), "Retrieve an actor by its PID")
      .def("set_auto_restart", &Actor::set_auto_restart,
           "Specify whether the actor shall restart when its host reboots.")
      .def("daemonize", &Actor::daemonize,
           "This actor will be automatically terminated when the last non-daemon actor finishes.\n\n"
           "Daemons are killed as soon as the last regular actor disappears. If another regular actor gets "
           "restarted later on by a timer or when its host reboots, the daemons do not get restarted.")
      .def("is_daemon", &Actor::is_daemon,
           "Returns True if that actor is a daemon and will be terminated automatically when the last non-daemon actor "
           "terminates.")
      .def("join", py::overload_cast<double>(&Actor::join, py::const_),
           "Wait for the actor to finish. Blocks the calling actor until the joined actor is terminated.",
           py::arg("timeout") = -1)
      .def("kill", &Actor::kill, "Kill that actor")
      .def("self", &Actor::self, "Retrieves the current actor.")
      .def("is_suspended", &Actor::is_suspended, "Returns True if that actor is currently suspended.")
      .def("suspend", &Actor::suspend, "Suspend that actor, that is blocked until resume()ed by another actor.")
      .def("resume", &Actor::resume, "Resume that actor, that was previously suspend()ed.")
      .def_static("kill_all", &Actor::kill_all, "Kill all actors but the caller.")
      .def("get_property", &Actor::get_property, "Retrieve the value of an actor property (or None if not set)")
      .def("get_properties", &Actor::get_properties, "Retrieve all actor properties")
      .def("set_property", &Actor::set_property, "Set an actor property (old values will be overwritten)")
      .def(
          "__repr__", [](const ActorPtr a) { return "Actor(" + a->get_name() + ")"; },
          "Textual representation of the Actor");

  /* Enum Class IoOpType */
  io_op_type
      .value("READ", simgrid::s4u::Io::OpType::READ)
      .value("WRITE", simgrid::s4u::Io::OpType::WRITE);

  /* Class File (from the file system plugin) */
  file
      .def_static(
          "open",
          [](const std::string& fullpath, const Host* host) {
            ensure_file_system_plugin();
            return new PyFile(host == nullptr ? simgrid::s4u::File::open(fullpath, nullptr)
                                              : simgrid::s4u::File::open(fullpath, host, nullptr));
          },
          py::arg("fullpath"), py::arg("host") = py::none(),
          "Open the file of the given full path, creating it if it does not exist. The disk is searched on the "
          "given host (on the current host by default) from the mount point found in the path.")
      .def("close", &PyFile::close, "Close this file. It cannot be used afterward. Closing a file twice is harmless.")
      .def_property_readonly("closed", &PyFile::closed, "Whether this file is closed (read-only property).")
      .def("__enter__", [](py::object self) { return self; })
      .def("__exit__",
           [](PyFile* self, const py::object&, const py::object&, const py::object&) { self->close(); })
      .def_property_readonly(
          "path", [](const PyFile* self) { return self->get()->get_path(); },
          "The full path of this file (read-only property).")
      .def_property_readonly(
          "size", [](const PyFile* self) { return self->get()->size(); },
          "The size of this file, in bytes (read-only property).")
      .def_readwrite("data", &PyFile::data, "Some user data attached to this file, or None if not set.")
      .def(
          "read", [](const PyFile* self, sg_size_t size) { return self->get()->read(size); }, py::arg("size"),
          "Simulate a read of the given amount of bytes from the current position. Returns the amount of bytes "
          "actually read.")
      .def(
          "write",
          [](const PyFile* self, sg_size_t size, bool write_inside) { return self->get()->write(size, write_inside); },
          py::arg("size"), py::arg("write_inside") = false,
          "Simulate a write of the given amount of bytes at the current position. Returns the amount of bytes "
          "actually written. Unless ``write_inside`` is True, the file is truncated at the end of the write.")
      .def(
          "seek", [](const PyFile* self, sg_offset_t offset, int whence) { self->get()->seek(offset, whence); },
          py::arg("offset"), py::arg("whence") = SEEK_SET,
          "Move the current position to the given offset, relative to the position given by ``whence`` "
          "(os.SEEK_SET, os.SEEK_CUR or os.SEEK_END), as with Python files.")
      .def(
          "tell", [](const PyFile* self) { return self->get()->tell(); }, "Return the current position in the file.")
      .def(
          "move", [](const PyFile* self, const std::string& fullpath) { self->get()->move(fullpath); },
          py::arg("fullpath"), "Rename this file. It is forbidden to move the file to another mount point.")
      .def(
          "unlink", [](const PyFile* self) { self->get()->unlink(); },
          "Remove this file from the disk. The file is not closed.")
      .def(
          "remote_copy",
          [](const PyFile* self, Host* host, const std::string& fullpath) {
            return self->get()->remote_copy(host, fullpath);
          },
          py::arg("host"), py::arg("fullpath"), "Copy this file to the given full path on the given host.")
      .def(
          "remote_move",
          [](const PyFile* self, Host* host, const std::string& fullpath) {
            return self->get()->remote_move(host, fullpath);
          },
          py::arg("host"), py::arg("fullpath"),
          "Move this file to the given full path on the given host. The local file is removed.")
      .def(
          "dump", [](const PyFile* self) { self->get()->dump(); },
          "Display information about this file (path, size, mount point, disk and host) in the logs.")
      .def(
          "__repr__",
          [](const PyFile* self) {
            return self->closed() ? std::string("File(closed)") : "File(" + std::string(self->get()->get_path()) + ")";
          },
          "Textual representation of the File");

  /* Class Task */
  task
      .def_static(
          "on_start_cb",
          [](py::object cb) {
            cb.inc_ref(); // keep alive after return
            Task::on_start_cb([cb_p = cb.ptr()](Task* op) { py::reinterpret_borrow<py::function>(cb_p)(op); });
          },
          "Add a callback called when each task starts.")
      .def_static(
          "on_completion_cb",
          [](py::object cb) {
            cb.inc_ref(); // keep alive after return
            Task::on_completion_cb([cb_p = cb.ptr()](Task* op) { py::reinterpret_borrow<py::function>(cb_p)(op); });
          },
          "Add a callback called when each task ends.")
      .def_property_readonly("name", &Task::get_name, "The name of this task (read-only).")
      .def_property_readonly("successors", &Task::get_successors, "The successors of this task (read-only).")
      .def_property("amount", &Task::get_amount, &Task::set_amount, "The amount of work to do for this task.")
      .def(
          "get_count", [](const TaskPtr t) { return t->get_count("instance_0"); },
          "The execution count of this task instance_0.")
      .def(
          "get_count", [](const TaskPtr t, const std::string& instance) { return t->get_count(instance); },
          "The execution count of this task instance.")
      .def("enqueue_firings", py::overload_cast<int>(&Task::enqueue_firings), py::arg("n"),
           "Enqueue firings for this task.")
      .def("add_successor", py::overload_cast<TaskPtr>(&Task::add_successor), py::arg("op"),
           "Add a successor to this task.")
      .def("remove_successor", py::overload_cast<TaskPtr>(&Task::remove_successor), py::arg("op"),
           "Remove a successor of this task.")
      .def("remove_all_successors", &Task::remove_all_successors, "Remove all successors of this task.")
      .def("on_this_start_cb", py::overload_cast<const std::function<void(Task*)>&>(&Task::on_this_start_cb),
           py::arg("func"), "Add a callback called when this task starts.")
      .def("on_this_completion_cb", py::overload_cast<const std::function<void(Task*)>&>(&Task::on_this_completion_cb),
           py::arg("func"), "Add a callback called when this task ends.")
      .def(
          "__repr__", [](const TaskPtr op) { return "Task(" + op->get_name() + ")"; },
          "Textual representation of the Task");

  /* Class CommTask */
  comm_task
      .def_static("init", py::overload_cast<const std::string&>(&CommTask::init), py::arg("name"),
                  "Initiate the creation of a CommTask. Setters have to be called afterwards.")
      .def_static("init", py::overload_cast<const std::string&, double, Host*, Host*>(&CommTask::init), py::arg("name"),
                  py::arg("bytes"), py::arg("source"), py::arg("destination"),
                  "Initiate the creation of a CommTask, setting its amount of bytes, source and destination hosts.")
      .def_property("source", &CommTask::get_source, &CommTask::set_source, "The source of the communication.")
      .def_property("destination", &CommTask::get_destination, &CommTask::set_destination,
                    "The destination of the communication.")
      .def_property("bytes", &CommTask::get_bytes, &CommTask::set_bytes, "The amount of bytes to send.")
      .def(
          "__repr__", [](const CommTaskPtr c) { return "CommTask(" + c->get_name() + ")"; },
          "Textual representation of the CommTask");

  /* Class ExecTask */
  exec_task
      .def_static("init", py::overload_cast<const std::string&>(&ExecTask::init), py::arg("name"),
                  "Initiate the creation of an ExecTask. Setters have to be called afterwards.")
      .def_static("init", py::overload_cast<const std::string&, double, Host*>(&ExecTask::init), py::arg("name"),
                  py::arg("flops"), py::arg("host"),
                  "Initiate the creation of an ExecTask, setting its amount of flops and host.")
      .def_property("host", &ExecTask::get_host, &ExecTask::set_host, "The host of the execution.")
      .def_property("flops", &ExecTask::get_flops, &ExecTask::set_flops, "The amount of flops to execute.")
      .def(
          "__repr__", [](const ExecTaskPtr e) { return "ExecTask(" + e->get_name() + ")"; },
          "Textual representation of the ExecTask");

  /* Class IoTask */
  io_task
      .def_static("init", py::overload_cast<const std::string&>(&IoTask::init), py::arg("name"),
                  "Initiate the creation of an IoTask. Setters have to be called afterwards.")
      .def_static("init", py::overload_cast<const std::string&, double, Disk*, Io::OpType>(&IoTask::init),
                  py::arg("name"), py::arg("bytes"), py::arg("disk"), py::arg("type"),
                  "Initiate the creation of an IoTask, setting its amount of bytes, disk and operation type.")
      .def_property("disk", &IoTask::get_disk, &IoTask::set_disk, "The disk of the IO.")
      .def_property("bytes", &IoTask::get_bytes, &IoTask::set_bytes, "The amount of bytes to process.")
      .def_property("type", &IoTask::get_op_type, &IoTask::set_op_type, "The type of IO.")
      .def(
          "__repr__", [](const IoTaskPtr io) { return "IoTask(" + io->get_name() + ")"; },
          "Textual representation of the IoTask");

  /* Class ActivitySet */
  activity_set
      .def(py::init([](const std::vector<simgrid::s4u::ActivityPtr>& activities) {
             auto* ret = new ActivitySet();
             for (auto a : activities)
               ret->push(a);
             return ActivitySetPtr(ret);
           }),
           "Create an ActivitySet containing the given activities.")
      .def(py::init([]() { return ActivitySetPtr(new ActivitySet()); }), "Create an empty ActivitySet.")

      .def("push", &ActivitySet::push, py::arg("activity"), "Add an activity to the set")
      .def("erase", &ActivitySet::erase, py::arg("activity"), "Remove that activity from the set")
      .def_property_readonly("size", &ActivitySet::size, "Count of activities in the set")
      .def("empty", &ActivitySet::empty, "Returns whether the set is empty")
      .def("has_failed_activities", &ActivitySet::has_failed_activities,
           "Returns whether there is any failed activities")
      .def("get_failed_activity", &ActivitySet::get_failed_activity, "Returns a failed activity from the set, or None")

      .def("wait_all_for", &ActivitySet::wait_all_for, py::arg("timeout"),
           "Wait for the completion of all activities in the set, but not longer than the provided timeout")
      .def("wait_all", &ActivitySet::wait_all, "Wait for the completion of all activities in the set, endlessly")
      .def("test_any", &ActivitySet::test_any,
           "Returns the first terminated activity if any, or None if no activity is terminated")
      .def("wait_any_for", &ActivitySet::wait_any_for, py::arg("timeout"),
           "Wait for the completion of one activity in the set, but not longer than the provided timeout")
      .def("wait_any", &ActivitySet::wait_any, "Wait for the completion of one activity in the set, endlessly")

      .def(
          "__repr__", [](const ActivitySetPtr) { return "ActivitySet([...])"; },
          "Textual representation of the ActivitySet");

  // Initialize Python context state (capture PyThreadState at import time while GIL is held)
  simgrid::python::simgrid_initialize_python_context_state();

#ifdef SIMGRID_HAVE_PYTHON_SMPI
  SMPI_bindings(m);
#endif
}
