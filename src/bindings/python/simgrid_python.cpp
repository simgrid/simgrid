/* Copyright (c) 2018-2021. The SimGrid Team. All rights reserved.          */

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

#ifndef NDEBUG
/* Many tests are failing after pybind11 commit ad6bf5cd39ca64b4a9bf846b84b11c4c8df1c8e1 "Adding PyGILState_Check() in
 *  object_api<>::operator(). (#2919)".
 * See https://github.com/pybind/pybind11/commit/ad6bf5cd39ca64b4a9bf846b84b11c4c8df1c8e1
 *
 * The failing tests are mostly those with boost/raw/sysv contexts. As a workaround, define NDEBUG before pybind11
 * includes.
 */
#define NDEBUG
#define NDEBUG_LOCALLY_DEFINED
#endif

#include <pybind11/pybind11.h> // Must come before our own stuff

#include <pybind11/functional.h>
#include <pybind11/stl.h>

#ifdef NDEBUG_LOCALLY_DEFINED
#undef NDEBUG_LOCALLY_DEFINED
#undef NDEBUG
#endif

#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif

#include "simgrid/kernel/routing/NetPoint.hpp"
#include "src/kernel/context/Context.hpp"
#include <simgrid/Exception.hpp>
#include <simgrid/s4u/Actor.hpp>
#include <simgrid/s4u/Comm.hpp>
#include <simgrid/s4u/Disk.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Exec.hpp>
#include <simgrid/s4u/Host.hpp>
#include <simgrid/s4u/Link.hpp>
#include <simgrid/s4u/Mailbox.hpp>
#include <simgrid/s4u/NetZone.hpp>
#include <simgrid/version.h>

#include <algorithm>
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

std::string get_simgrid_version()
{
  int major;
  int minor;
  int patch;
  sg_version_get(&major, &minor, &patch);
  return simgrid::xbt::string_printf("%i.%i.%i", major, minor, patch);
}

/** @brief Wrap for mailbox::get_async */
class PyGetAsync {
  std::unique_ptr<PyObject*> data = std::make_unique<PyObject*>();

public:
  PyObject** get() const { return data.get(); }
};

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
      .def(
          "info", [](const char* s) { XBT_INFO("%s", s); }, "Display a logging message of 'info' priority.")
      .def(
          "error", [](const char* s) { XBT_ERROR("%s", s); }, "Display a logging message of 'error' priority.")
      .def("execute", py::overload_cast<double, double>(&simgrid::s4u::this_actor::execute),
           py::call_guard<GilScopedRelease>(),
           "Block the current actor, computing the given amount of flops at the given priority.", py::arg("flops"),
           py::arg("priority") = 1)
      .def("exec_init", py::overload_cast<double>(&simgrid::s4u::this_actor::exec_init),
           py::call_guard<GilScopedRelease>())
      .def("get_host", &simgrid::s4u::this_actor::get_host, "Retrieves host on which the current actor is located")
      .def("set_host", &simgrid::s4u::this_actor::set_host, py::call_guard<GilScopedRelease>(),
           "Moves the current actor to another host.", py::arg("dest"))
      .def("sleep_for", static_cast<void (*)(double)>(&simgrid::s4u::this_actor::sleep_for),
           py::call_guard<GilScopedRelease>(), "Block the actor sleeping for that amount of seconds.",
           py::arg("duration"))
      .def("sleep_until", static_cast<void (*)(double)>(&simgrid::s4u::this_actor::sleep_until),
           py::call_guard<GilScopedRelease>(), "Block the actor sleeping until the specified timestamp.",
           py::arg("duration"))
      .def("suspend", &simgrid::s4u::this_actor::suspend, py::call_guard<GilScopedRelease>(),
           "Suspend the current actor, that is blocked until resume()ed by another actor.")
      .def("yield_", &simgrid::s4u::this_actor::yield, py::call_guard<GilScopedRelease>(), "Yield the actor")
      .def("exit", &simgrid::s4u::this_actor::exit, py::call_guard<GilScopedRelease>(), "kill the current actor")
      .def(
          "on_exit",
          [](py::object fun) {
            fun.inc_ref(); // FIXME: why is this needed for tests like actor-kill and actor-lifetime?
            simgrid::s4u::this_actor::on_exit([fun](bool /*failed*/) {
              GilScopedAcquire py_context; // need a new context for callback
              try {
                fun();
              } catch (const py::error_already_set& e) {
                std::string what = e.what();
                py_context.reset();
                xbt_die("Error while executing the on_exit lambda: %s", what.c_str());
              }
            });
          },
          py::call_guard<GilScopedRelease>(), "");

  /* Class Engine */
  py::class_<Engine>(m, "Engine", "Simulation Engine")
      .def(py::init([](std::vector<std::string> args) {
        auto argc           = static_cast<int>(args.size());
        std::vector<char*> argv(args.size() + 1); // argv[argc] is nullptr
        std::transform(begin(args), end(args), begin(argv), [](std::string& s) { return &s.front(); });
        // Currently this can be dangling, we should wrap this somehow.
        return new simgrid::s4u::Engine(&argc, argv.data());
      }))
      .def_static("get_clock", &Engine::get_clock,
                  "The simulation time, ie the amount of simulated seconds since the simulation start.")
      .def("get_all_hosts", &Engine::get_all_hosts, "Returns the list of all hosts found in the platform")
      .def("load_platform", &Engine::load_platform, "Load a platform file describing the environment")
      .def("load_deployment", &Engine::load_deployment, "Load a deployment file and launch the actors that it contains")
      .def("run", &Engine::run, py::call_guard<GilScopedRelease>(), "Run the simulation")
      .def(
          "register_actor",
          [](Engine* e, const std::string& name, py::object fun_or_class) {
            e->register_actor(name, [fun_or_class](std::vector<std::string> args) {
              GilScopedAcquire py_context;
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
                bool ffk = ex.matches(pyForcefulKillEx);
                py_context.reset();
                if (ffk) {
                  XBT_VERB("Actor killed");
                  simgrid::ForcefulKillException::do_throw(); // Forward that ForcefulKill exception
                }
                throw;
              }
            });
          },
          "Registers the main function of an actor");

  /* Class Netzone */
  py::class_<simgrid::s4u::NetZone, std::unique_ptr<simgrid::s4u::NetZone, py::nodelete>>(m, "NetZone",
                                                                                          "Networking Zones")
      .def_static("create_full_zone", &simgrid::s4u::create_full_zone, "Creates a zone of type FullZone")
      .def_static("create_torus_zone", &simgrid::s4u::create_torus_zone, "Creates a cluster of type Torus")
      .def_static("create_fatTree_zone", &simgrid::s4u::create_fatTree_zone, "Creates a cluster of type Fat-Tree")
      .def_static("create_dragonfly_zone", &simgrid::s4u::create_dragonfly_zone, "Creates a cluster of type Dragonfly")
      .def_static("create_star_zone", &simgrid::s4u::create_star_zone, "Creates a zone of type Star")
      .def_static("create_floyd_zone", &simgrid::s4u::create_floyd_zone, "Creates a zone of type Floyd")
      .def_static("create_dijkstra_zone", &simgrid::s4u::create_floyd_zone, "Creates a zone of type Dijkstra")
      .def_static("create_vivaldi_zone", &simgrid::s4u::create_vivaldi_zone, "Creates a zone of type Vivaldi")
      .def_static("create_empty_zone", &simgrid::s4u::create_empty_zone, "Creates a zone of type Empty")
      .def_static("create_wifi_zone", &simgrid::s4u::create_wifi_zone, "Creates a zone of type Wi-Fi")
      .def("add_route",
           py::overload_cast<simgrid::kernel::routing::NetPoint*, simgrid::kernel::routing::NetPoint*,
                             simgrid::kernel::routing::NetPoint*, simgrid::kernel::routing::NetPoint*,
                             const std::vector<simgrid::s4u::LinkInRoute>&, bool>(&simgrid::s4u::NetZone::add_route),
           "Add a route between 2 netpoints")
      .def("create_host", py::overload_cast<const std::string&, double>(&simgrid::s4u::NetZone::create_host),
           "Creates a host")
      .def("create_host",
           py::overload_cast<const std::string&, const std::string&>(&simgrid::s4u::NetZone::create_host),
           "Creates a host")
      .def("create_host",
           py::overload_cast<const std::string&, const std::vector<double>&>(&simgrid::s4u::NetZone::create_host),
           "Creates a host")
      .def("create_host",
           py::overload_cast<const std::string&, const std::vector<std::string>&>(&simgrid::s4u::NetZone::create_host),
           "Creates a host")
      .def("create_link", py::overload_cast<const std::string&, double>(&simgrid::s4u::NetZone::create_link),
           "Creates a network link")
      .def("create_link",
           py::overload_cast<const std::string&, const std::string&>(&simgrid::s4u::NetZone::create_link),
           "Creates a network link")
      .def("create_link",
           py::overload_cast<const std::string&, const std::vector<double>&>(&simgrid::s4u::NetZone::create_link),
           "Creates a network link")
      .def("create_link",
           py::overload_cast<const std::string&, const std::vector<std::string>&>(&simgrid::s4u::NetZone::create_link),
           "Creates a network link")
      .def("create_split_duplex_link",
           py::overload_cast<const std::string&, double>(&simgrid::s4u::NetZone::create_split_duplex_link),
           "Creates a split-duplex link")
      .def("create_split_duplex_link",
           py::overload_cast<const std::string&, const std::string&>(&simgrid::s4u::NetZone::create_split_duplex_link),
           "Creates a split-duplex link")
      .def("create_router", &simgrid::s4u::NetZone::create_router, "Create a router")
      .def("set_parent", &simgrid::s4u::NetZone::set_parent, "Set the parent of this zone")
      .def("set_property", &simgrid::s4u::NetZone::set_property, "Add a property to this zone")
      .def("get_netpoint", &simgrid::s4u::NetZone::get_netpoint, "Retrieve the netpoint associated to this zone")
      .def("seal", &simgrid::s4u::NetZone::seal, "Seal this NetZone")
      .def_property_readonly(
          "name", [](const simgrid::s4u::NetZone* self) { return self->get_name(); }, "The name of this network zone");

  /* Class ClusterCallbacks */
  py::class_<simgrid::s4u::ClusterCallbacks>(m, "ClusterCallbacks", "Callbacks used to create cluster zones")
      .def(py::init<const std::function<simgrid::s4u::ClusterCallbacks::ClusterNetPointCb>&,
                    const std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>&,
                    const std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>&>());

  /* Class FatTreeParams */
  py::class_<simgrid::s4u::FatTreeParams>(m, "FatTreeParams", "Parameters to create a Fat-Tree zone")
      .def(py::init<unsigned int, const std::vector<unsigned int>&, const std::vector<unsigned int>&,
                    const std::vector<unsigned int>&>());

  /* Class DragonflyParams */
  py::class_<simgrid::s4u::DragonflyParams>(m, "DragonflyParams", "Parameters to create a Dragonfly zone")
      .def(py::init<const std::pair<unsigned int, unsigned int>&, const std::pair<unsigned int, unsigned int>&,
                    const std::pair<unsigned int, unsigned int>&, unsigned int>());

  /* Class Host */
  py::class_<simgrid::s4u::Host, std::unique_ptr<Host, py::nodelete>>(m, "Host", "Simulated host")
      .def("by_name", &Host::by_name, "Retrieves a host from its name, or die")
      .def("get_pstate_count", &Host::get_pstate_count, "Retrieve the count of defined pstate levels")
      .def("get_pstate_speed", &Host::get_pstate_speed, "Retrieve the maximal speed at the given pstate")
      .def("get_netpoint", &Host::get_netpoint, "Retrieve the netpoint associated to this host")
      .def("get_disks", &Host::get_disks, "Retrieve the list of disks in this host")
      .def("set_core_count", &Host::set_core_count, "Set the number of cores in the CPU")
      .def("set_coordinates", &Host::set_coordinates, "Set the coordinates of this host")
      .def("create_disk", py::overload_cast<const std::string&, double, double>(&Host::create_disk), "Create a disk")
      .def("create_disk",
           py::overload_cast<const std::string&, const std::string&, const std::string&>(&Host::create_disk),
           "Create a disk")
      .def("seal", &Host::seal, "Seal this host")
      .def_property(
          "pstate", &Host::get_pstate,
          [](Host* h, int i) {
            GilScopedRelease gil_guard;
            h->set_pstate(i);
          },
          "The current pstate")
      .def("current", &Host::current, py::call_guard<GilScopedRelease>(),
           "Retrieves the host on which the running actor is located.")
      .def_property_readonly(
          "name",
          [](const Host* self) {
            return std::string(self->get_name().c_str()); // Convert from xbt::string because of MC
          },
          "The name of this host")
      .def_property_readonly(
          "load", &Host::get_load,
          "Returns the current computation load (in flops per second). This is the currently achieved speed.")
      .def_property_readonly(
          "speed", &Host::get_speed,
          "The peak computing speed in flops/s at the current pstate, taking the external load into account. "
          "This is the max potential speed.");

  /* Class Disk */
  py::class_<simgrid::s4u::Disk, std::unique_ptr<simgrid::s4u::Disk, py::nodelete>>(m, "Disk", "Simulated disk")
      .def("read", &simgrid::s4u::Disk::read, py::call_guard<GilScopedRelease>(), "Read data from disk")
      .def("write", &simgrid::s4u::Disk::write, py::call_guard<GilScopedRelease>(), "Write data in disk")
      .def("seal", &simgrid::s4u::Disk::seal, "Seal this disk")
      .def_property_readonly(
          "name", [](const simgrid::s4u::Disk* self) { return self->get_name(); }, "The name of this disk");

  /* Class NetPoint */
  py::class_<simgrid::kernel::routing::NetPoint, std::unique_ptr<simgrid::kernel::routing::NetPoint, py::nodelete>>(
      m, "NetPoint", "NetPoint object");

  /* Class Link */
  py::class_<simgrid::s4u::Link, std::unique_ptr<simgrid::s4u::Link, py::nodelete>> link(m, "Link", "Network link");
  link.def("set_latency", py::overload_cast<const std::string&>(&simgrid::s4u::Link::set_latency), "Set the latency");
  link.def("set_latency", py::overload_cast<double>(&simgrid::s4u::Link::set_latency), "Set the latency");
  link.def("set_sharing_policy", &simgrid::s4u::Link::set_sharing_policy, "Set sharing policy for this link");
  link.def("set_concurrency_limit", &simgrid::s4u::Link::set_concurrency_limit, "Set concurrency limit for this link");
  link.def("set_host_wifi_rate", &simgrid::s4u::Link::set_host_wifi_rate,
           "Set level of communication speed of given host on this Wi-Fi link");
  link.def("seal", &simgrid::s4u::Link::seal, "Seal this link");
  link.def_property_readonly(
      "name",
      [](const simgrid::s4u::Link* self) {
        return std::string(self->get_name().c_str()); // Convert from xbt::string because of MC
      },
      "The name of this link");
  py::enum_<simgrid::s4u::Link::SharingPolicy>(link, "SharingPolicy")
      .value("NONLINEAR", simgrid::s4u::Link::SharingPolicy::NONLINEAR)
      .value("WIFI", simgrid::s4u::Link::SharingPolicy::WIFI)
      .value("SPLITDUPLEX", simgrid::s4u::Link::SharingPolicy::SPLITDUPLEX)
      .value("SHARED", simgrid::s4u::Link::SharingPolicy::SHARED)
      .value("FATPIPE", simgrid::s4u::Link::SharingPolicy::FATPIPE)
      .export_values();

  /* Class LinkInRoute */
  py::class_<simgrid::s4u::LinkInRoute> linkinroute(m, "LinkInRoute", "Abstraction to add link in routes");
  linkinroute.def(py::init<const simgrid::s4u::Link*>());
  linkinroute.def(py::init<const simgrid::s4u::Link*, simgrid::s4u::LinkInRoute::Direction>());
  py::enum_<simgrid::s4u::LinkInRoute::Direction>(linkinroute, "Direction")
      .value("UP", simgrid::s4u::LinkInRoute::Direction::UP)
      .value("DOWN", simgrid::s4u::LinkInRoute::Direction::DOWN)
      .value("NONE", simgrid::s4u::LinkInRoute::Direction::NONE)
      .export_values();

  /* Class Split-Duplex Link */
  py::class_<simgrid::s4u::SplitDuplexLink, simgrid::s4u::Link,
             std::unique_ptr<simgrid::s4u::SplitDuplexLink, py::nodelete>>(m, "SplitDuplexLink",
                                                                           "Network split-duplex link")
      .def("get_link_up", &simgrid::s4u::SplitDuplexLink::get_link_up, "Get link direction up")
      .def("get_link_down", &simgrid::s4u::SplitDuplexLink::get_link_down, "Get link direction down");

  /* Class Mailbox */
  py::class_<simgrid::s4u::Mailbox, std::unique_ptr<Mailbox, py::nodelete>>(m, "Mailbox", "Mailbox")
      .def(
          "__str__", [](const Mailbox* self) { return std::string("Mailbox(") + self->get_cname() + ")"; },
          "Textual representation of the Mailbox`")
      .def("by_name", &Mailbox::by_name, py::call_guard<GilScopedRelease>(), "Retrieve a Mailbox from its name")
      .def_property_readonly(
          "name",
          [](const Mailbox* self) {
            return std::string(self->get_name().c_str()); // Convert from xbt::string because of MC
          },
          "The name of that mailbox")
      .def(
          "put",
          [](Mailbox* self, py::object data, int size) {
            data.inc_ref();
            self->put(data.ptr(), size);
          },
          py::call_guard<GilScopedRelease>(), "Blocking data transmission")
      .def(
          "put_async",
          [](Mailbox* self, py::object data, int size) {
            data.inc_ref();
            return self->put_async(data.ptr(), size);
          },
          py::call_guard<GilScopedRelease>(), "Non-blocking data transmission")
      .def(
          "get",
          [](Mailbox* self) {
            py::object data = pybind11::reinterpret_steal<py::object>(self->get<PyObject>());
            // data.dec_ref(); // FIXME: why does it break python-actor-create?
            return data;
          },
          py::call_guard<GilScopedRelease>(), "Blocking data reception")
      .def(
          "get_async",
          [](Mailbox* self) -> std::tuple<simgrid::s4u::CommPtr, PyGetAsync> {
            PyGetAsync wrap;
            auto comm = self->get_async(wrap.get());
            return std::make_tuple(std::move(comm), std::move(wrap));
          },
          py::call_guard<GilScopedRelease>(),
          "Non-blocking data reception. Use data.get() to get the python object after the communication has finished")
      .def(
          "set_receiver", [](Mailbox* self, ActorPtr actor) { self->set_receiver(actor); },
          py::call_guard<GilScopedRelease>(), "Sets the actor as permanent receiver");

  /* Class PyGetAsync */
  py::class_<PyGetAsync>(m, "PyGetAsync", "Wrapper for async get communications")
      .def(py::init<>())
      .def(
          "get", [](PyGetAsync* self) { return py::reinterpret_steal<py::object>(*(self->get())); },
          "Get python object after async communication in receiver side");

  /* Class Comm */
  py::class_<simgrid::s4u::Comm, simgrid::s4u::CommPtr>(m, "Comm", "Communication")
      .def("test", &simgrid::s4u::Comm::test, py::call_guard<GilScopedRelease>(),
           "Test whether the communication is terminated.")
      .def("wait", &simgrid::s4u::Comm::wait, py::call_guard<GilScopedRelease>(),
           "Block until the completion of that communication.")
      // use py::overload_cast for wait_all/wait_any, until the overload marked XBT_ATTRIB_DEPRECATED_v332 is removed
      .def_static("wait_all",
                  py::overload_cast<const std::vector<simgrid::s4u::CommPtr>&>(&simgrid::s4u::Comm::wait_all),
                  py::call_guard<GilScopedRelease>(), "Block until the completion of all communications in the list.")
      .def_static(
          "wait_any", py::overload_cast<const std::vector<simgrid::s4u::CommPtr>&>(&simgrid::s4u::Comm::wait_any),
          py::call_guard<GilScopedRelease>(),
          "Block until the completion of any communication in the list and return the index of the terminated one.");

  /* Class Exec */
  py::class_<simgrid::s4u::Exec, simgrid::s4u::ExecPtr>(m, "Exec", "Execution")
      .def_property_readonly(
          "remaining",
          [](simgrid::s4u::ExecPtr self) {
            GilScopedRelease gil_guard;
            return self->get_remaining();
          },
          "Amount of flops that remain to be computed until completion.")
      .def_property_readonly(
          "remaining_ratio",
          [](simgrid::s4u::ExecPtr self) {
            GilScopedRelease gil_guard;
            return self->get_remaining_ratio();
          },
          "Amount of work remaining until completion from 0 (completely done) to 1 (nothing done "
          "yet).")
      .def_property("host", &simgrid::s4u::Exec::get_host, &simgrid::s4u::Exec::set_host,
                    "Host on which this execution runs. Only the first host is returned for parallel executions.")
      .def("test", &simgrid::s4u::Exec::test, py::call_guard<GilScopedRelease>(),
           "Test whether the execution is terminated.")
      .def("cancel", &simgrid::s4u::Exec::cancel, py::call_guard<GilScopedRelease>(), "Cancel that execution.")
      .def("start", &simgrid::s4u::Exec::start, py::call_guard<GilScopedRelease>(), "Start that execution.")
      .def("wait", &simgrid::s4u::Exec::wait, py::call_guard<GilScopedRelease>(),
           "Block until the completion of that execution.");

  /* Class Actor */
  py::class_<simgrid::s4u::Actor, ActorPtr>(m, "Actor",
                                            "An actor is an independent stream of execution in your distributed "
                                            "application")
      .def(
          "create",
          [](py::str name, Host* host, py::object fun, py::args args) {
            fun.inc_ref();  // FIXME: why is this needed for tests like exec-async, exec-dvfs and exec-remote?
            args.inc_ref(); // FIXME: why is this needed for tests like actor-migrate?
            return simgrid::s4u::Actor::create(name, host, [fun, args]() {
              GilScopedAcquire py_context;
              try {
                fun(*args);
              } catch (const py::error_already_set& ex) {
                bool ffk = ex.matches(pyForcefulKillEx);
                py_context.reset();
                if (ffk) {
                  XBT_VERB("Actor killed");
                  simgrid::ForcefulKillException::do_throw(); // Forward that ForcefulKill exception
                }
                throw;
              }
            });
          },
          py::call_guard<GilScopedRelease>(), "Create an actor from a function or an object.")
      .def_property(
          "host", &Actor::get_host,
          [](Actor* a, Host* h) {
            GilScopedRelease gil_guard;
            a->set_host(h);
          },
          "The host on which this actor is located")
      .def_property_readonly("name", &Actor::get_cname, "The name of this actor.")
      .def_property_readonly("pid", &Actor::get_pid, "The PID (unique identifier) of this actor.")
      .def_property_readonly("ppid", &Actor::get_ppid,
                             "The PID (unique identifier) of the actor that created this one.")
      .def("by_pid", &Actor::by_pid, "Retrieve an actor by its PID")
      .def("daemonize", &Actor::daemonize, py::call_guard<GilScopedRelease>(),
           "This actor will be automatically terminated when the last non-daemon actor finishes (more info in the C++ "
           "documentation).")
      .def("is_daemon", &Actor::is_daemon,
           "Returns True if that actor is a daemon and will be terminated automatically when the last non-daemon actor "
           "terminates.")
      .def("join", py::overload_cast<double>(&Actor::join, py::const_), py::call_guard<GilScopedRelease>(),
           "Wait for the actor to finish (more info in the C++ documentation).", py::arg("timeout"))
      .def("kill", &Actor::kill, py::call_guard<GilScopedRelease>(), "Kill that actor")
      .def("kill_all", &Actor::kill_all, py::call_guard<GilScopedRelease>(), "Kill all actors but the caller.")
      .def("self", &Actor::self, "Retrieves the current actor.")
      .def("is_suspended", &Actor::is_suspended, "Returns True if that actor is currently suspended.")
      .def("suspend", &Actor::suspend, py::call_guard<GilScopedRelease>(),
           "Suspend that actor, that is blocked until resume()ed by another actor.")
      .def("resume", &Actor::resume, py::call_guard<GilScopedRelease>(),
           "Resume that actor, that was previously suspend()ed.");
}
