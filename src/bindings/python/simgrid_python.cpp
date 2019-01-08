/* Copyright (c) 2018-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <pybind11/pybind11.h> // Must be first
#include <pybind11/stl.h>

#include "src/kernel/context/Context.hpp"
#include <simgrid/s4u/Actor.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Host.hpp>
#include <simgrid/s4u/Mailbox.hpp>

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

PYBIND11_DECLARE_HOLDER_TYPE(T, boost::intrusive_ptr<T>);

namespace {

static std::string get_simgrid_version()
{
  int major, minor, patch;
  sg_version_get(&major, &minor, &patch);
  return simgrid::xbt::string_printf("%i.%i.%i", major, minor, patch);
}

static std::string simgrid_version = get_simgrid_version();

} // namespace

PYBIND11_MODULE(simgrid, m)
{

  m.doc() = "SimGrid userspace API";

  m.attr("simgrid_version") = simgrid_version;

  // Internal exception used to kill actors and sweep the RAII chimney (free objects living on the stack)
  py::object pyStopRequestEx = py::register_exception<simgrid::kernel::context::Context::StopRequest>(m, "ActorKilled");

  /* this_actor namespace */
  void (*sleep_for_fun)(double) = &simgrid::s4u::this_actor::sleep_for; // pick the right overload
  void (*sleep_until_fun)(double) = &simgrid::s4u::this_actor::sleep_until;

  py::module m2 = m.def_submodule("this_actor", "Bindings of the s4u::this_actor namespace.");
  m2.def("info", [](char* s) { XBT_INFO("%s", s); }, "Display a logging message of default priority.");
  m2.def("error", [](char* s) { XBT_ERROR("%s", s); }, "Display a logging message of 'error' priority.");
  m2.def("execute", py::overload_cast<double, double>(&simgrid::s4u::this_actor::execute),
         "Block the current actor, computing the given amount of flops at the given priority, see :cpp:func:`void "
         "simgrid::s4u::this_actor::execute(double, double)`",
         py::arg("flops"), py::arg("priority") = 1);
  m2.def("get_host", &simgrid::s4u::this_actor::get_host, "Retrives host on which the current actor is located");
  m2.def("migrate", &simgrid::s4u::this_actor::migrate, "Moves the current actor to another host, see :cpp:func:`void simgrid::s4u::this_actor::migrate()`",
      py::arg("dest"));
  m2.def("sleep_for", sleep_for_fun,
      "Block the actor sleeping for that amount of seconds, see :cpp:func:`void simgrid::s4u::this_actor::sleep_for`", py::arg("duration"));
  m2.def("sleep_until", sleep_until_fun,
      "Block the actor sleeping until the specified timestamp, see :cpp:func:`void simgrid::s4u::this_actor::sleep_until`", py::arg("duration"));
  m2.def("suspend", &simgrid::s4u::this_actor::suspend, "Suspend the current actor, that is blocked until resume()ed by another actor. see :cpp:func:`void simgrid::s4u::this_actor::suspend`");
  m2.def("yield_", &simgrid::s4u::this_actor::yield,
         "Yield the actor, see :cpp:func:`void simgrid::s4u::this_actor::yield()`");
  m2.def("exit", &simgrid::s4u::this_actor::exit, "kill the current actor");
  m2.def("kill", [](py::int_ pid) { Actor::kill(pid); }, "Kill an actor by pid");
  m2.def("kill_all", &Actor::kill_all, "Kill all actors but the caller.");

  m2.def(
      "on_exit",
      [](py::function fun) { simgrid::s4u::this_actor::on_exit([fun](int ignored, void* data) { fun(); }, nullptr); },
      "");

  /* Class Engine */
  py::class_<Engine>(m, "Engine", "Simulation Engine, see :ref:`class s4u::Engine <API_s4u_Engine>`")
      .def(py::init([](std::vector<std::string> args) -> simgrid::s4u::Engine* {
        static char noarg[] = {'\0'};
        int argc            = args.size();
        std::unique_ptr<char* []> argv(new char*[argc + 1]);
        for (int i = 0; i != argc; ++i)
          argv[i] = args[i].empty() ? noarg : &args[i].front();
        argv[argc] = nullptr;
        // Currently this can be dangling, we should wrap this somehow.
        return new simgrid::s4u::Engine(&argc, argv.get());
      }))
      .def("get_all_hosts", &Engine::get_all_hosts, "Returns the list of all hosts found in the platform")
      .def("get_clock", &Engine::get_clock, "Retrieve the simulation time (in seconds)")
      .def("load_platform", &Engine::load_platform,
           "Load a platform file describing the environment, see :cpp:func:`simgrid::s4u::Engine::load_platform()`")
      .def("load_deployment", &Engine::load_deployment,
           "Load a deployment file and launch the actors that it contains, see "
           ":cpp:func:`simgrid::s4u::Engine::load_deployment()`")
      .def("run", &Engine::run, "Run the simulation")
      .def("register_actor",
           [pyStopRequestEx](Engine*, std::string name, py::object fun_or_class) {
             simgrid::simix::register_function(
                 name, [pyStopRequestEx, fun_or_class](std::vector<std::string> args) -> simgrid::simix::ActorCode {
                   return [pyStopRequestEx, fun_or_class, args]() {
                     try {
                       /* Convert the std::vector into a py::tuple */
                       py::tuple params(args.size() - 1);
                       for (size_t i = 1; i < args.size(); i++)
                         params[i - 1] = py::cast(args[i]);

                       py::object res = fun_or_class(*params);

                       /* If I was passed a class, I just built an instance, so I need to call it now */
                       if (py::isinstance<py::function>(res))
                         res();
                     } catch (py::error_already_set& ex) {
                       if (ex.matches(pyStopRequestEx)) {
                         XBT_VERB("Actor killed");
                         /* Stop here that StopRequest exception which was meant to free the RAII stuff on the stack */
                       } else {
                         throw;
                       }
                     }
                   };
                 });
           },
           "Registers the main function of an actor, see :cpp:func:`simgrid::s4u::Engine::register_function()`");

  /* Class Host */
  py::class_<simgrid::s4u::Host, std::unique_ptr<Host, py::nodelete>>(m, "Host", "Simulation Engine, see :ref:`class s4u::Host <API_s4u_Host>`")
      .def("by_name", &Host::by_name, "Retrieves a host from its name, or die")
      .def("current", &Host::current, "Retrieves the host on which the running actor is located, see :cpp:func:`simgrid::s4u::Host::current()`")
      .def_property_readonly("name", [](Host* self) -> const std::string {
          return std::string(self->get_name().c_str()); // Convert from xbt::string because of MC
        }, "The name of this host")
      .def_property_readonly("speed", &Host::get_speed,
          "The peak computing speed in flops/s at the current pstate, taking the external load into account, see :cpp:func:`simgrid::s4u::Host::get_speed()`");

  /* Class Mailbox */
  py::class_<simgrid::s4u::Mailbox, std::unique_ptr<Mailbox, py::nodelete>>(m, "Mailbox", "Mailbox, see :ref:`class s4u::Mailbox <API_s4u_Mailbox>`")
      .def("by_name", &Mailbox::by_name, "Retrieve a Mailbox from its name, see :cpp:func:`simgrid::s4u::Mailbox::by_name()`")
      .def_property_readonly("name", [](Mailbox* self) -> const std::string {
         return std::string(self->get_name().c_str()); // Convert from xbt::string because of MC
      }, "The name of that mailbox, see :cpp:func:`simgrid::s4u::Mailbox::get_name()`")
      .def("put", [](Mailbox self, py::object data, int size) {
        data.inc_ref();
        self.put(data.ptr(), size);
      }, "Blocking data transmission, see :cpp:func:`void simgrid::s4u::Mailbox::put(void*, uint64_t)`")
      .def("get", [](Mailbox self) -> py::object {
         py::object data = pybind11::reinterpret_steal<py::object>(pybind11::handle(static_cast<PyObject*>(self.get())));
         data.dec_ref();
         return data;
      }, "Blocking data reception, see :cpp:func:`void* simgrid::s4u::Mailbox::get()`");

  /* Class Actor */
  py::class_<simgrid::s4u::Actor, ActorPtr>(m, "Actor",
                                            "An actor is an independent stream of execution in your distributed "
                                            "application, see :ref:`class s4u::Actor <API_s4u_Actor>`")

      .def("create",
           [pyStopRequestEx](py::str name, py::object host, py::object fun, py::args args) {

             return simgrid::s4u::Actor::create(name, host.cast<Host*>(), [fun, args, pyStopRequestEx]() {

               try {
                 fun(*args);
               } catch (py::error_already_set& ex) {
                 if (ex.matches(pyStopRequestEx)) {
                   XBT_VERB("Actor killed");
                   /* Stop here that StopRequest exception which was meant to free the RAII stuff on the stack */
                 } else {
                   throw;
                 }
               }
             });
           },
           "Create an actor from a function or an object, see :cpp:func:`simgrid::s4u::Actor::create()`")
      .def_property("host", &Actor::get_host, &Actor::migrate, "The host on which this actor is located")
      .def_property_readonly("pid", &Actor::get_pid, "The PID (unique identifier) of this actor.")
      .def("daemonize", &Actor::daemonize,
           "This actor will be automatically terminated when the last non-daemon actor finishes, see :cpp:func:`void "
           "simgrid::s4u::Actor::daemonize()`")
      .def("join", py::overload_cast<double>(&Actor::join),
           "Wait for the actor to finish, see :cpp:func:`void simgrid::s4u::Actor::join(double)`", py::arg("timeout"))
      .def("kill", [](py::int_ pid) { Actor::kill(pid); }, "Kill an actor by pid")
      .def("kill", [](ActorPtr act) { act->kill(); }, "Kill that actor")
      .def("kill_all", &Actor::kill_all, "Kill all actors but the caller.")
      .def("migrate", &Actor::migrate,
           "Moves that actor to another host, see :cpp:func:`void simgrid::s4u::Actor::migrate()`", py::arg("dest"))
      .def("self", &Actor::self, "Retrieves the current actor, see :cpp:func:`void simgrid::s4u::Actor::self()`")
      .def("is_suspended", &Actor::is_suspended, "Returns True if that actor is currently suspended.")
      .def("suspend", &Actor::suspend, "Suspend that actor, that is blocked until resume()ed by another actor.")
      .def("resume", &Actor::resume, "Resume that actor, that was previously suspend()ed.");
}
