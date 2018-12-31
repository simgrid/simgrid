/* Copyright (c) 2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <simgrid/config.h>
#include <xbt/log.h>
#include <xbt/string.hpp>

#include <simgrid/s4u/Actor.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Host.hpp>
#include <simgrid/s4u/Mailbox.hpp>

#include <boost/intrusive_ptr.hpp>

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

  /* this_actor namespace */
  py::module m2 = m.def_submodule("this_actor", "Bindings of the s4u::this_actor namespace.");
  m2.def("info", [](char* s) { XBT_INFO("%s", s); }, "Display a logging message of default priority.");
  m2.def("execute", py::overload_cast<double>(&simgrid::s4u::this_actor::execute),
         "Block the current actor, computing the given amount of flops, see :cpp:func:`void "
         "simgrid::s4u::this_actor::execute(double)`");
  m2.def("execute", py::overload_cast<double, double>(&simgrid::s4u::this_actor::execute),
         "Block the current actor, computing the given amount of flops at the given priority, see :cpp:func:`void "
         "simgrid::s4u::this_actor::execute(double, double)`");
  m2.def("yield_", &simgrid::s4u::this_actor::yield,
         "Yield the actor, see :cpp:func:`void simgrid::s4u::this_actor::yield()`");

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
      .def("load_platform", &Engine::load_platform,
          "Load a platform file describing the environment, see :cpp:func:`simgrid::s4u::Engine::load_platform()`")
      .def("load_deployment", &Engine::load_deployment,
          "Load a deployment file and launch the actors that it contains, see :cpp:func:`simgrid::s4u::Engine::load_deployment()`")
      .def("run", &Engine::run, "Run the simulation")
      .def("register_actor", [](Engine*, std::string name, py::object obj) {
        simgrid::simix::register_function(name,
            [obj](std::vector<std::string> args) -> simgrid::simix::ActorCode {
          return [obj, args]() {
            /* Convert the std::vector into a py::tuple */
            py::tuple params(args.size()-1);
            for (size_t i=1; i<args.size(); i++)
              params[i-1] = py::cast(args[i]);

            PyObject *result = PyObject_CallObject(obj.ptr(), params.ptr());
            if (!result)
                throw pybind11::error_already_set();

            /* If I was passed a class, I just built an instance, so I need to call it now */
            if (PyCallable_Check(result)) {
              py::object obj2 = pybind11::reinterpret_steal<py::object>(pybind11::handle(static_cast<PyObject*>(result)));
              obj2();
            }
          };
        });
      }, "Registers the main function of an actor, see :cpp:func:`simgrid::s4u::Engine::register_function()`")
      ;

  /* Class Host */
  py::class_<simgrid::s4u::Host, std::unique_ptr<Host, py::nodelete>>(m, "Host", "Simulation Engine, see :ref:`class s4u::Host <API_s4u_Host>`").def(
      "by_name", &Host::by_name, "Retrieve a host from its name, or die");

  /* Class Mailbox */
  py::class_<simgrid::s4u::Mailbox, std::unique_ptr<Mailbox, py::nodelete>>(m, "Mailbox", "Mailbox, see :ref:`class s4u::Mailbox <API_s4u_Mailbox>`")
      .def("by_name", &Mailbox::by_name, "Retrieve a Mailbox from its name, see :cpp:func:`simgrid::s4u::Mailbox::by_name()`")
      .def("get_name", &Mailbox::get_name, "Retrieves the name of that host, see :cpp:func:`simgrid::s4u::Mailbox::get_name()`")
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
                                            ""
                                            "An actor is an independent stream of execution in your distributed "
                                            "application, see :ref:`class s4u::Actor <API_s4u_Actor>`")

      .def("create",
           [](py::args args, py::kwargs kwargs) {
             xbt_assert(args.size() > 2,
                        "Creating an actor takes at least 3 parameters: name, host, and main function.");
             return simgrid::s4u::Actor::create(args[0].cast<std::string>(), args[1].cast<Host*>(), [args]() {
               py::tuple funargs(args.size() - 3);
               for (size_t i = 3; i < args.size(); i++)
                 funargs[i - 3] = args[i];

               PyObject* result = PyObject_CallObject(args[2].ptr(), funargs.ptr());
               if (!result)
                 throw pybind11::error_already_set();
             });
           },
           "Create an actor from a function or an object, see :cpp:func:`simgrid::s4u::Actor::create()`");
}
