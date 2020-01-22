/* Copyright (c) 2018-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifdef _WIN32
#warning Try to work around https://bugs.python.org/issue11566
#define _hypot hypot
#endif

#if defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
#endif

#include <pybind11/functional.h>
#include <pybind11/pybind11.h> // Must come before our own stuff
#include <pybind11/stl.h>

#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif

#include "src/kernel/context/Context.hpp"
#include <simgrid/Exception.hpp>
#include <simgrid/s4u/Actor.hpp>
#include <simgrid/s4u/Comm.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Exec.hpp>
#include <simgrid/s4u/Host.hpp>
#include <simgrid/s4u/Mailbox.hpp>
#include <simgrid/version.h>

#include <memory>
#include <string>
#include <vector>

namespace py = pybind11;
using simgrid::s4u::Actor;
using simgrid::s4u::ActorPtr;
using simgrid::s4u::Engine;
using simgrid::s4u::Host;
using simgrid::s4u::Mailbox;

XBT_LOG_NEW_DEFAULT_CATEGORY(python, "python");

namespace {

static std::string get_simgrid_version()
{
  int major;
  int minor;
  int patch;
  sg_version_get(&major, &minor, &patch);
  return simgrid::xbt::string_printf("%i.%i.%i", major, minor, patch);
}

/* Classes GilScopedAcquire and GilScopedRelease have the same purpose as pybind11::gil_scoped_acquire and
 * pybind11::gil_scoped_release.  Refer to the manual of pybind11 for details:
 * https://pybind11.readthedocs.io/en/stable/advanced/misc.html#global-interpreter-lock-gil
 *
 * The pybind11 versions are however too sophisticated (using TLS for example) and don't work well with all kinds of
 * contexts.
 * See also https://github.com/pybind/pybind11/issues/1276, which may be related.
 *
 * Briefly, GilScopedAcquire can be used on actor creation to acquire a new PyThreadState.  The PyThreadState has to be
 * released for context switches (i.e. before simcalls). That's the purpose of GilScopedRelease.
 *
 * Like their pybind11 counterparts, both classes use a RAII pattern.
 */
class XBT_PRIVATE GilScopedAcquire {
  static PyThreadState* acquire()
  {
    PyThreadState* state = PyThreadState_New(PyInterpreterState_Head());
    PyEval_AcquireThread(state);
    return state;
  }
  static void release(PyThreadState* state)
  {
    PyEval_ReleaseThread(state);
    PyThreadState_Clear(state);
    PyThreadState_Delete(state);
  }

  std::unique_ptr<PyThreadState, decltype(&release)> thread_state{acquire(), &release};

public:
  void reset() { thread_state.reset(); }
};

class XBT_PRIVATE GilScopedRelease {
  std::unique_ptr<PyThreadState, decltype(&PyEval_RestoreThread)> thread_state{PyEval_SaveThread(),
                                                                               &PyEval_RestoreThread};
};

} // namespace

PYBIND11_DECLARE_HOLDER_TYPE(T, boost::intrusive_ptr<T>)

PYBIND11_MODULE(simgrid, m)
{
  m.doc() = "SimGrid userspace API";

  m.attr("simgrid_version") = get_simgrid_version();

  // Internal exception used to kill actors and sweep the RAII chimney (free objects living on the stack)
  static py::object pyForcefulKillEx(py::register_exception<simgrid::ForcefulKillException>(m, "ActorKilled"));

  /* this_actor namespace */
  m.def_submodule("this_actor", "Bindings of the s4u::this_actor namespace.")
      .def("info", [](const char* s) { XBT_INFO("%s", s); }, "Display a logging message of 'info' priority.")
      .def("error", [](const char* s) { XBT_ERROR("%s", s); }, "Display a logging message of 'error' priority.")
      .def("execute", py::overload_cast<double, double>(&simgrid::s4u::this_actor::execute),
           "Block the current actor, computing the given amount of flops at the given priority, "
           "see :cpp:func:`void simgrid::s4u::this_actor::execute(double, double)`",
           py::arg("flops"), py::arg("priority") = 1)
      .def("exec_init", py::overload_cast<double>(&simgrid::s4u::this_actor::exec_init))
      .def("get_host", &simgrid::s4u::this_actor::get_host, "Retrieves host on which the current actor is located")
      .def("set_host", &simgrid::s4u::this_actor::set_host,
           "Moves the current actor to another host, see :cpp:func:`void simgrid::s4u::this_actor::set_host()`",
           py::arg("dest"))
      .def("sleep_for", static_cast<void (*)(double)>(&simgrid::s4u::this_actor::sleep_for),
           "Block the actor sleeping for that amount of seconds, "
           "see :cpp:func:`void simgrid::s4u::this_actor::sleep_for`",
           py::arg("duration"))
      .def("sleep_until", static_cast<void (*)(double)>(&simgrid::s4u::this_actor::sleep_until),
           "Block the actor sleeping until the specified timestamp, "
           "see :cpp:func:`void simgrid::s4u::this_actor::sleep_until`",
           py::arg("duration"))
      .def("suspend", &simgrid::s4u::this_actor::suspend,
           "Suspend the current actor, that is blocked until resume()ed by another actor. "
           "see :cpp:func:`void simgrid::s4u::this_actor::suspend`")
      .def("yield_", &simgrid::s4u::this_actor::yield,
           "Yield the actor, see :cpp:func:`void simgrid::s4u::this_actor::yield()`")
      .def("exit", &simgrid::s4u::this_actor::exit, "kill the current actor")
      .def("on_exit",
           [](py::object fun) {
             simgrid::s4u::this_actor::on_exit([fun](bool /*failed*/) {
               try {
                 fun();
               } catch (const py::error_already_set& e) {
                 xbt_die("Error while executing the on_exit lambda: %s", e.what());
               }
             });
           },
           "");

  /* Class Engine */
  py::class_<Engine>(m, "Engine", "Simulation Engine, see :ref:`class s4u::Engine <API_s4u_Engine>`")
      .def(py::init([](std::vector<std::string> args) {
        static char noarg[] = {'\0'};
        int argc            = args.size();
        std::unique_ptr<char* []> argv(new char*[argc + 1]);
        for (int i = 0; i != argc; ++i)
          argv[i] = args[i].empty() ? noarg : &args[i].front();
        argv[argc] = nullptr;
        // Currently this can be dangling, we should wrap this somehow.
        return new simgrid::s4u::Engine(&argc, argv.get());
      }))
      .def_static("get_clock", &Engine::get_clock,
                  "The simulation time, ie the amount of simulated seconds since the simulation start.")
      .def("get_all_hosts", &Engine::get_all_hosts, "Returns the list of all hosts found in the platform")
      .def("load_platform", &Engine::load_platform,
           "Load a platform file describing the environment, see :cpp:func:`simgrid::s4u::Engine::load_platform()`")
      .def("load_deployment", &Engine::load_deployment,
           "Load a deployment file and launch the actors that it contains, see "
           ":cpp:func:`simgrid::s4u::Engine::load_deployment()`")
      .def("run", &Engine::run, py::call_guard<GilScopedRelease>(), "Run the simulation")
      .def("register_actor",
           [](Engine* e, const std::string& name, py::object fun_or_class) {
             e->register_actor(name, [fun_or_class](std::vector<std::string> args) {
               try {
                 /* Convert the std::vector into a py::tuple */
                 py::tuple params(args.size() - 1);
                 for (size_t i = 1; i < args.size(); i++)
                   params[i - 1] = py::cast(args[i]);

                 py::object res = fun_or_class(*params);

                 /* If I was passed a class, I just built an instance, so I need to call it now */
                 if (py::isinstance<py::function>(res))
                   res();
               } catch (const py::error_already_set& ex) {
                 if (ex.matches(pyForcefulKillEx)) {
                   XBT_VERB("Actor killed");
                   /* Stop here that ForcefulKill exception which was meant to free the RAII stuff on the stack */
                 } else {
                   throw;
                 }
               }
             });
           },
           "Registers the main function of an actor, see :cpp:func:`simgrid::s4u::Engine::register_actor()`");

  /* Class Host */
  py::class_<simgrid::s4u::Host, std::unique_ptr<Host, py::nodelete>>(
      m, "Host", "Simulation Engine, see :ref:`class s4u::Host <API_s4u_Host>`")
      .def("by_name", &Host::by_name, "Retrieves a host from its name, or die")
      .def("get_pstate_count", &Host::get_pstate_count,
           "Retrieve the cound of defined pstate levels, see :cpp:func:`simgrid::s4u::Host::get_pstate_count`")
      .def("get_pstate_speed", &Host::get_pstate_speed,
           "Retrieve the maximal speed at the given pstate, see :cpp:func:`simgrid::s4u::Host::get_pstate_speed`")
      .def_property("pstate", &Host::get_pstate, &Host::set_pstate, "The current pstate")

      .def("current", &Host::current,
           "Retrieves the host on which the running actor is located, see :cpp:func:`simgrid::s4u::Host::current()`")
      .def_property_readonly("name",
                             [](const Host* self) {
                               return std::string(self->get_name().c_str()); // Convert from xbt::string because of MC
                             },
                             "The name of this host")
      .def_property_readonly(
          "load", &Host::get_load,
          "Returns the current computation load (in flops per second). This is the currently achieved speed. "
          "See :cpp:func:`simgrid::s4u::Host::get_load()`")
      .def_property_readonly(
          "speed", &Host::get_speed,
          "The peak computing speed in flops/s at the current pstate, taking the external load into account. "
          "This is the max potential speed. See :cpp:func:`simgrid::s4u::Host::get_speed()`");

  /* Class Mailbox */
  py::class_<simgrid::s4u::Mailbox, std::unique_ptr<Mailbox, py::nodelete>>(
      m, "Mailbox", "Mailbox, see :ref:`class s4u::Mailbox <API_s4u_Mailbox>`")
      .def("__str__", [](const Mailbox* self) { return std::string("Mailbox(") + self->get_cname() + ")"; },
           "Textual representation of the Mailbox`")
      .def("by_name", &Mailbox::by_name,
           "Retrieve a Mailbox from its name, see :cpp:func:`simgrid::s4u::Mailbox::by_name()`")
      .def_property_readonly("name",
                             [](const Mailbox* self) {
                               return std::string(self->get_name().c_str()); // Convert from xbt::string because of MC
                             },
                             "The name of that mailbox, see :cpp:func:`simgrid::s4u::Mailbox::get_name()`")
      .def("put",
           [](Mailbox* self, py::object data, int size) {
             data.inc_ref();
             self->put(data.ptr(), size);
           },
           "Blocking data transmission, see :cpp:func:`void simgrid::s4u::Mailbox::put(void*, uint64_t)`")
      .def("put_async",
           [](Mailbox* self, py::object data, int size) {
             data.inc_ref();
             return self->put_async(data.ptr(), size);
           },
           "Non-blocking data transmission, see :cpp:func:`void simgrid::s4u::Mailbox::put_async(void*, uint64_t)`")
      .def("get",
           [](Mailbox* self) {
             py::object data = pybind11::reinterpret_steal<py::object>(static_cast<PyObject*>(self->get()));
             data.dec_ref();
             return data;
           },
           "Blocking data reception, see :cpp:func:`void* simgrid::s4u::Mailbox::get()`");

  /* Class Comm */
  py::class_<simgrid::s4u::Comm, simgrid::s4u::CommPtr>(m, "Comm",
                                                        "Communication, see :ref:`class s4u::Comm <API_s4u_Comm>`")
      .def("test", &simgrid::s4u::Comm::test,
           "Test whether the communication is terminated, see :cpp:func:`simgrid::s4u::Comm::test()`")
      .def("wait", &simgrid::s4u::Comm::wait,
           "Block until the completion of that communication, see :cpp:func:`simgrid::s4u::Comm::wait()`")
      .def("wait_all", &simgrid::s4u::Comm::wait_all,
           "Block until the completion of all communications in the list, see "
           ":cpp:func:`simgrid::s4u::Comm::wait_all()`")
      .def("wait_any", &simgrid::s4u::Comm::wait_any,
           "Block until the completion of any communication in the list and return the index of the terminated one, "
           "see :cpp:func:`simgrid::s4u::Comm::wait_any()`");

  /* Class Exec */
  py::class_<simgrid::s4u::Exec, simgrid::s4u::ExecPtr>(m, "Exec",
                                                        "Execution, see :ref:`class s4u::Exec <API_s4u_Exec>`")
      .def_property_readonly("remaining", &simgrid::s4u::Exec::get_remaining,
                             "Amount of flops that remain to be computed until completion, see "
                             ":cpp:func:`simgrid::s4u::Exec::get_remaining()`")
      .def_property_readonly("remaining_ratio", &simgrid::s4u::Exec::get_remaining_ratio,
                             "Amount of work remaining until completion from 0 (completely done) to 1 (nothing done "
                             "yet). See :cpp:func:`simgrid::s4u::Exec::get_remaining_ratio()`")
      .def_property("host",
                    [](simgrid::s4u::ExecPtr self) {
                      simgrid::s4u::ExecSeqPtr seq = boost::dynamic_pointer_cast<simgrid::s4u::ExecSeq>(self);
                      if (seq != nullptr)
                        return seq->get_host();
                      xbt_throw_unimplemented(__FILE__, __LINE__,
                                              "host of parallel executions is not implemented in python yet.");
                    },
                    &simgrid::s4u::Exec::set_host,
                    "Host on which this execution runs. See :cpp:func:`simgrid::s4u::ExecSeq::get_host()`")
      .def("test", &simgrid::s4u::Exec::test,
           "Test whether the execution is terminated, see :cpp:func:`simgrid::s4u::Exec::test()`")
      .def("cancel", &simgrid::s4u::Exec::cancel, "Cancel that execution, see :cpp:func:`simgrid::s4u::Exec::cancel()`")
      .def("start", &simgrid::s4u::Exec::start, "Start that execution, see :cpp:func:`simgrid::s4u::Exec::start()`")
      .def("wait", &simgrid::s4u::Exec::wait,
           "Block until the completion of that execution, see :cpp:func:`simgrid::s4u::Exec::wait()`");

  /* Class Actor */
  py::class_<simgrid::s4u::Actor, ActorPtr>(m, "Actor",
                                            "An actor is an independent stream of execution in your distributed "
                                            "application, see :ref:`class s4u::Actor <API_s4u_Actor>`")
      .def("create",
           [](py::str name, Host* host, py::object fun, py::args args) {
             return simgrid::s4u::Actor::create(name, host, [fun, args]() {
               try {
                 fun(*args);
               } catch (const py::error_already_set& ex) {
                 if (ex.matches(pyForcefulKillEx)) {
                   XBT_VERB("Actor killed");
                   /* Stop here that ForcefulKill exception which was meant to free the RAII stuff on the stack */
                 } else {
                   throw;
                 }
               }
             });
           },
           "Create an actor from a function or an object.")
      .def_property("host", &Actor::get_host, &Actor::set_host, "The host on which this actor is located")
      .def_property_readonly("name", &Actor::get_cname, "The name of this actor.")
      .def_property_readonly("pid", &Actor::get_pid, "The PID (unique identifier) of this actor.")
      .def_property_readonly("ppid", &Actor::get_ppid,
                             "The PID (unique identifier) of the actor that created this one.")
      .def("by_pid", &Actor::by_pid, "Retrieve an actor by its PID")
      .def("daemonize", &Actor::daemonize,
           "This actor will be automatically terminated when the last non-daemon actor finishes (more info in the C++ "
           "documentation).")
      .def("is_daemon", &Actor::is_daemon,
           "Returns True if that actor is a daemon and will be terminated automatically when the last non-daemon actor "
           "terminates.")
      .def("join", py::overload_cast<double>(&Actor::join),
           "Wait for the actor to finish (more info in the C++ documentation).", py::arg("timeout"))
      .def("kill", &Actor::kill, "Kill that actor")
      .def("kill_all", &Actor::kill_all, "Kill all actors but the caller.")
      .def("self", &Actor::self, "Retrieves the current actor.")
      .def("is_suspended", &Actor::is_suspended, "Returns True if that actor is currently suspended.")
      .def("suspend", &Actor::suspend, "Suspend that actor, that is blocked until resume()ed by another actor.")
      .def("resume", &Actor::resume, "Resume that actor, that was previously suspend()ed.");
}
