/* Copyright (c) 2018-2022. The SimGrid Team. All rights reserved.          */

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

#include <pybind11/pybind11.h> // Must come before our own stuff

#include <pybind11/functional.h>
#include <pybind11/stl.h>

#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif

#include "simgrid/kernel/ProfileBuilder.hpp"
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
using simgrid::s4u::Link;
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

} // namespace

PYBIND11_DECLARE_HOLDER_TYPE(T, boost::intrusive_ptr<T>)

PYBIND11_MODULE(simgrid, m)
{
  m.doc() = "SimGrid userspace API";

  m.attr("simgrid_version") = get_simgrid_version();

  // Swapped contexts are broken, starting from pybind11 v2.8.0.  Use thread contexts by default.
  simgrid::s4u::Engine::set_config("contexts/factory:thread");

  // Internal exception used to kill actors and sweep the RAII chimney (free objects living on the stack)
  static py::object pyForcefulKillEx(py::register_exception<simgrid::ForcefulKillException>(m, "ActorKilled"));

  py::register_exception<simgrid::NetworkFailureException>(m, "NetworkFailureException");
  py::register_exception<simgrid::TimeoutException>(m, "TimeoutException");
  py::register_exception<simgrid::HostFailureException>(m, "HostFailureException");
  py::register_exception<simgrid::StorageFailureException>(m, "StorageFailureException");
  py::register_exception<simgrid::VmFailureException>(m, "VmFailureException");
  py::register_exception<simgrid::CancelException>(m, "CancelException");

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
           py::call_guard<py::gil_scoped_release>(),
           "Block the current actor, computing the given amount of flops at the given priority.", py::arg("flops"),
           py::arg("priority") = 1)
      .def("exec_init", py::overload_cast<double>(&simgrid::s4u::this_actor::exec_init),
           py::call_guard<py::gil_scoped_release>())
      .def("get_host", &simgrid::s4u::this_actor::get_host, "Retrieves host on which the current actor is located")
      .def("set_host", &simgrid::s4u::this_actor::set_host, py::call_guard<py::gil_scoped_release>(),
           "Moves the current actor to another host.", py::arg("dest"))
      .def("sleep_for", static_cast<void (*)(double)>(&simgrid::s4u::this_actor::sleep_for),
           py::call_guard<py::gil_scoped_release>(), "Block the actor sleeping for that amount of seconds.",
           py::arg("duration"))
      .def("sleep_until", static_cast<void (*)(double)>(&simgrid::s4u::this_actor::sleep_until),
           py::call_guard<py::gil_scoped_release>(), "Block the actor sleeping until the specified timestamp.",
           py::arg("duration"))
      .def("suspend", &simgrid::s4u::this_actor::suspend, py::call_guard<py::gil_scoped_release>(),
           "Suspend the current actor, that is blocked until resume()ed by another actor.")
      .def("yield_", &simgrid::s4u::this_actor::yield, py::call_guard<py::gil_scoped_release>(), "Yield the actor")
      .def("exit", &simgrid::s4u::this_actor::exit, py::call_guard<py::gil_scoped_release>(), "kill the current actor")
      .def(
          "on_exit",
          [](py::object cb) {
            py::function fun = py::reinterpret_borrow<py::function>(cb);
            fun.inc_ref(); // FIXME: why is this needed for tests like actor-kill and actor-lifetime?
            simgrid::s4u::this_actor::on_exit([fun](bool failed) {
              py::gil_scoped_acquire py_context; // need a new context for callback
              try {
                fun(failed);
              } catch (const py::error_already_set& e) {
                xbt_die("Error while executing the on_exit lambda: %s", e.what());
              }
            });
          },
          py::call_guard<py::gil_scoped_release>(),
          "Define a lambda to be called when the actor ends. It takes a bool parameter indicating whether the actor "
          "was killed. If False, the actor finished peacefully.")
      .def("get_pid", &simgrid::s4u::this_actor::get_pid, "Retrieves PID of the current actor")
      .def("get_ppid", &simgrid::s4u::this_actor::get_ppid,
           "Retrieves PPID of the current actor (i.e., the PID of its parent).");

  /* Class Engine */
  py::class_<Engine>(m, "Engine", "Simulation Engine")
      .def(py::init([](std::vector<std::string> args) {
             auto argc = static_cast<int>(args.size());
             std::vector<char*> argv(args.size() + 1); // argv[argc] is nullptr
             std::transform(begin(args), end(args), begin(argv), [](std::string& s) { return &s.front(); });
             // Currently this can be dangling, we should wrap this somehow.
             return new simgrid::s4u::Engine(&argc, argv.data());
           }),
           "The constructor should take the parameters from the command line, as is ")
      .def_static("get_clock",
                  []() // XBT_ATTRIB_DEPRECATED_v334
                  {
                    PyErr_WarnEx(
                        PyExc_DeprecationWarning,
                        "get_clock() is deprecated and  will be dropped after v3.33, use `Engine.clock` instead.", 1);
                    return Engine::get_clock();
                  })
      .def_property_readonly_static(
          "clock", [](py::object /* self */) { return Engine::get_clock(); },
          "The simulation time, ie the amount of simulated seconds since the simulation start.")
      .def_property_readonly_static(
          "instance", [](py::object /* self */) { return Engine::get_instance(); }, "Retrieve the simulation engine")
      .def("get_all_hosts",
           [](py::object self) // XBT_ATTRIB_DEPRECATED_v334
           {
             PyErr_WarnEx(PyExc_DeprecationWarning,
                          "get_all_hosts() is deprecated and  will be dropped after v3.33, use all_hosts instead.", 1);
             return self.attr("all_hosts");
           })
      .def_property_readonly("all_hosts", &Engine::get_all_hosts, "Returns the list of all hosts found in the platform")
      .def("get_all_links",
           [](py::object self) // XBT_ATTRIB_DEPRECATED_v334
           {
             PyErr_WarnEx(PyExc_DeprecationWarning,
                          "get_all_links() is deprecated and  will be dropped after v3.33, use all_links instead.", 1);
             return self.attr("all_links");
           })
      .def_property_readonly("all_links", &Engine::get_all_links, "Returns the list of all links found in the platform")
      .def("get_all_netpoints",
           [](py::object self) // XBT_ATTRIB_DEPRECATED_v334
           {
             PyErr_WarnEx(
                 PyExc_DeprecationWarning,
                 "get_all_netpoints() is deprecated and  will be dropped after v3.33, use all_netpoints instead.", 1);
             return self.attr("all_netpoints");
           })
      .def_property_readonly("all_netpoints", &Engine::get_all_netpoints)
      .def("get_netzone_root",
           [](py::object self) // XBT_ATTRIB_DEPRECATED_v334
           {
             PyErr_WarnEx(PyExc_DeprecationWarning,
                          "get_netzone_root() is deprecated and  will be dropped after v3.3, use netzone_root instead.",
                          1);
             return self.attr("netzone_root");
           })
      .def_property_readonly("netzone_root", &Engine::get_netzone_root,
                             "Retrieve the root netzone, containing all others.")
      .def("netpoint_by_name", &Engine::netpoint_by_name_or_null)
      .def("netzone_by_name", &Engine::netzone_by_name_or_null)
      .def("load_platform", &Engine::load_platform, "Load a platform file describing the environment")
      .def("load_deployment", &Engine::load_deployment, "Load a deployment file and launch the actors that it contains")
      .def("run", &Engine::run, py::call_guard<py::gil_scoped_release>(), "Run the simulation until its end")
      .def("run_until", py::overload_cast<double>(&Engine::run_until, py::const_),
           py::call_guard<py::gil_scoped_release>(), "Run the simulation until the given date",
           py::arg("max_date") = -1)
      .def(
          "register_actor",
          [](Engine* e, const std::string& name, py::object fun_or_class) {
            e->register_actor(name, [fun_or_class](std::vector<std::string> args) {
              py::gil_scoped_acquire py_context;
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
                  simgrid::ForcefulKillException::do_throw(); // Forward that ForcefulKill exception
                }
                throw;
              }
            });
          },
          "Registers the main function of an actor");

  /* Class Netzone */
  py::class_<simgrid::s4u::NetZone, std::unique_ptr<simgrid::s4u::NetZone, py::nodelete>> netzone(
      m, "NetZone", "Networking Zones. See the C++ documentation for details.");
  netzone.def_static("create_full_zone", &simgrid::s4u::create_full_zone, "Creates a zone of type FullZone")
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
      .def("get_netpoint",
           [](py::object self) // XBT_ATTRIB_DEPRECATED_v334
           {
             PyErr_WarnEx(PyExc_DeprecationWarning,
                          "get_netpoint() is deprecated and  will be dropped after v3.33, use netpoint instead.", 1);
             return self.attr("netpoint");
           })
      .def_property_readonly("netpoint", &simgrid::s4u::NetZone::get_netpoint,
                             "Retrieve the netpoint associated to this zone")
      .def("seal", &simgrid::s4u::NetZone::seal, "Seal this NetZone")
      .def_property_readonly(
          "name", [](const simgrid::s4u::NetZone* self) { return self->get_name(); },
          "The name of this network zone (read-only property).");

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
  py::class_<simgrid::s4u::Host, std::unique_ptr<Host, py::nodelete>> host(
      m, "Host", "Simulated host. See the C++ documentation for details.");
  host.def_static("by_name", &Host::by_name, py::arg("name"), "Retrieves a host from its name, or die")
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
          py::call_guard<py::gil_scoped_release>(),
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
          py::call_guard<py::gil_scoped_release>(),
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
      .def("get_pstate_count",
           [](py::object self) // XBT_ATTRIB_DEPRECATED_v334
           {
             PyErr_WarnEx(
                 PyExc_DeprecationWarning,
                 "get_pstate_count() is deprecated and  will be dropped after v3.33, use pstate_count instead.", 1);
             return self.attr("pstate_count");
           })
      .def_property_readonly("pstate_count", &Host::get_pstate_count, "Retrieve the count of defined pstate levels")
      .def("get_pstate_speed",
           [](py::object self, int state) // XBT_ATTRIB_DEPRECATED_v334
           {
             PyErr_WarnEx(
                 PyExc_DeprecationWarning,
                 "get_pstate_speed() is deprecated and  will be dropped after v3.33, use pstate_speed instead.", 1);
             return self.attr("pstate_speed")(state);
           })
      .def("pstate_speed", &Host::get_pstate_speed, py::call_guard<py::gil_scoped_release>(),
           "Retrieve the maximal speed at the given pstate")
      .def("get_netpoint",
           [](py::object self) // XBT_ATTRIB_DEPRECATED_v334
           {
             PyErr_WarnEx(PyExc_DeprecationWarning,
                          "get_netpoint() is deprecated and  will be dropped after v3.33, use netpoint instead.", 1);
             return self.attr("netpoint");
           })
      .def_property_readonly("netpoint", &Host::get_netpoint, "Retrieve the netpoint associated to this zone")
      .def("get_disks", &Host::get_disks, "Retrieve the list of disks in this host")
      .def("set_core_count",
           [](py::object self, double count) // XBT_ATTRIB_DEPRECATED_v334
           {
             PyErr_WarnEx(PyExc_DeprecationWarning,
                          "set_core_count() is deprecated and  will be dropped after v3.33, use core_count instead.",
                          1);
             self.attr("core_count")(count);
           })
      .def_property(
          "core_count", &Host::get_core_count,
          [](Host* h, int count) {
            py::gil_scoped_release gil_guard;
            return h->set_core_count(count);
          },
          "Manage the number of cores in the CPU")
      .def("set_coordinates", &Host::set_coordinates, py::call_guard<py::gil_scoped_release>(),
           "Set the coordinates of this host")
      .def("set_sharing_policy", &simgrid::s4u::Host::set_sharing_policy, py::call_guard<py::gil_scoped_release>(),
           "Describe how the CPU is shared", py::arg("policy"), py::arg("cb") = simgrid::s4u::NonLinearResourceCb())
      .def("create_disk", py::overload_cast<const std::string&, double, double>(&Host::create_disk),
           py::call_guard<py::gil_scoped_release>(), "Create a disk")
      .def("create_disk",
           py::overload_cast<const std::string&, const std::string&, const std::string&>(&Host::create_disk),
           py::call_guard<py::gil_scoped_release>(), "Create a disk")
      .def("seal", &Host::seal, py::call_guard<py::gil_scoped_release>(), "Seal this host")
      .def_property(
          "pstate", &Host::get_pstate,
          [](Host* h, int i) {
            py::gil_scoped_release gil_guard;
            h->set_pstate(i);
          },
          "The current pstate (read/write property).")
      .def_static("current", &Host::current, py::call_guard<py::gil_scoped_release>(),
           "Retrieves the host on which the running actor is located.")
      .def_property_readonly(
          "name",
          [](const Host* self) {
            return std::string(self->get_name().c_str()); // Convert from xbt::string because of MC
          },
          "The name of this host (read-only property).")
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
            Host::on_creation_cb([cb](Host& h) {
              py::function fun = py::reinterpret_borrow<py::function>(cb);
              py::gil_scoped_acquire py_context; // need a new context for callback
              try {
                fun(&h);
              } catch (const py::error_already_set& e) {
                xbt_die("Error while executing the on_creation lambda : %s", e.what());
              }
            });
          },
          py::call_guard<py::gil_scoped_release>(), "");

  py::enum_<simgrid::s4u::Host::SharingPolicy>(host, "SharingPolicy")
      .value("NONLINEAR", simgrid::s4u::Host::SharingPolicy::NONLINEAR)
      .value("LINEAR", simgrid::s4u::Host::SharingPolicy::LINEAR)
      .export_values();

  /* Class Disk */
  py::class_<simgrid::s4u::Disk, std::unique_ptr<simgrid::s4u::Disk, py::nodelete>> disk(
      m, "Disk", "Simulated disk. See the C++ documentation for details.");
  disk.def("read", py::overload_cast<sg_size_t, double>(&simgrid::s4u::Disk::read, py::const_),
           py::call_guard<py::gil_scoped_release>(), "Read data from disk", py::arg("size"), py::arg("priority") = 1)
      .def("write", py::overload_cast<sg_size_t, double>(&simgrid::s4u::Disk::write, py::const_),
           py::call_guard<py::gil_scoped_release>(), "Write data in disk", py::arg("size"), py::arg("priority") = 1)
      .def("read_async", &simgrid::s4u::Disk::read_async, py::call_guard<py::gil_scoped_release>(),
           "Non-blocking read data from disk")
      .def("write_async", &simgrid::s4u::Disk::write_async, py::call_guard<py::gil_scoped_release>(),
           "Non-blocking write data in disk")
      .def("set_sharing_policy", &simgrid::s4u::Disk::set_sharing_policy, py::call_guard<py::gil_scoped_release>(),
           "Set sharing policy for this disk", py::arg("op"), py::arg("policy"),
           py::arg("cb") = simgrid::s4u::NonLinearResourceCb())
      .def("seal", &simgrid::s4u::Disk::seal, py::call_guard<py::gil_scoped_release>(), "Seal this disk")
      .def_property_readonly(
          "name", [](const simgrid::s4u::Disk* self) { return self->get_name(); },
          "The name of this disk (read-only property).");
  py::enum_<simgrid::s4u::Disk::SharingPolicy>(disk, "SharingPolicy")
      .value("NONLINEAR", simgrid::s4u::Disk::SharingPolicy::NONLINEAR)
      .value("LINEAR", simgrid::s4u::Disk::SharingPolicy::LINEAR)
      .export_values();
  py::enum_<simgrid::s4u::Disk::Operation>(disk, "Operation")
      .value("READ", simgrid::s4u::Disk::Operation::READ)
      .value("WRITE", simgrid::s4u::Disk::Operation::WRITE)
      .value("READWRITE", simgrid::s4u::Disk::Operation::READWRITE)
      .export_values();

  /* Class NetPoint */
  py::class_<simgrid::kernel::routing::NetPoint, std::unique_ptr<simgrid::kernel::routing::NetPoint, py::nodelete>>
      netpoint(m, "NetPoint", "NetPoint object");

  /* Class Link */
  py::class_<Link, std::unique_ptr<Link, py::nodelete>> link(m, "Link",
                                                             "Network link. See the C++ documentation for details.");
  link.def("set_latency", py::overload_cast<const std::string&>(&Link::set_latency),
           py::call_guard<py::gil_scoped_release>(),
           "Set the latency as a string. Accepts values with units, such as ‘1s’ or ‘7ms’.\nFull list of accepted "
           "units: w (week), d (day), h, s, ms, us, ns, ps.")
      .def("set_latency", py::overload_cast<double>(&Link::set_latency), py::call_guard<py::gil_scoped_release>(),
           "Set the latency as a float (in seconds).")
      .def("set_bandwidth", &Link::set_bandwidth, py::call_guard<py::gil_scoped_release>(),
           "Set the bandwidth (in byte per second).")
      .def(
          "set_bandwidth_profile",
          [](Link* l, const std::string& profile, double period) {
            l->set_bandwidth_profile(simgrid::kernel::profile::ProfileBuilder::from_string("", profile, period));
          },
          py::call_guard<py::gil_scoped_release>(),
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
          py::call_guard<py::gil_scoped_release>(),
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

      .def("turn_on", &Link::turn_on, py::call_guard<py::gil_scoped_release>(), "Turns the link on.")
      .def("turn_off", &Link::turn_off, py::call_guard<py::gil_scoped_release>(), "Turns the link off.")
      .def("is_on", &Link::is_on, "Check whether the link is on.")

      .def("set_sharing_policy", &Link::set_sharing_policy, py::call_guard<py::gil_scoped_release>(),
           "Set sharing policy for this link")
      .def("set_concurrency_limit", &Link::set_concurrency_limit, py::call_guard<py::gil_scoped_release>(),
           "Set concurrency limit for this link")
      .def("set_host_wifi_rate", &Link::set_host_wifi_rate, py::call_guard<py::gil_scoped_release>(),
           "Set level of communication speed of given host on this Wi-Fi link")
      .def_static("by_name", &Link::by_name, "Retrieves a Link from its name, or dies")
      .def("seal", &Link::seal, py::call_guard<py::gil_scoped_release>(), "Seal this link")
      .def_property_readonly(
          "name",
          [](const Link* self) {
            return std::string(self->get_name().c_str()); // Convert from xbt::string because of MC
          },
          "The name of this link")
      .def_property_readonly("bandwidth", &Link::get_bandwidth,
                             "The bandwidth (in bytes per second) (read-only property).")
      .def_property_readonly("latency", &Link::get_latency, "The latency (in seconds) (read-only property).");

  py::enum_<Link::SharingPolicy>(link, "SharingPolicy")
      .value("NONLINEAR", Link::SharingPolicy::NONLINEAR)
      .value("WIFI", Link::SharingPolicy::WIFI)
      .value("SPLITDUPLEX", Link::SharingPolicy::SPLITDUPLEX)
      .value("SHARED", Link::SharingPolicy::SHARED)
      .value("FATPIPE", Link::SharingPolicy::FATPIPE)
      .export_values();

  /* Class LinkInRoute */
  py::class_<simgrid::s4u::LinkInRoute> linkinroute(m, "LinkInRoute", "Abstraction to add link in routes");
  linkinroute.def(py::init<const Link*>());
  linkinroute.def(py::init<const Link*, simgrid::s4u::LinkInRoute::Direction>());
  py::enum_<simgrid::s4u::LinkInRoute::Direction>(linkinroute, "Direction")
      .value("UP", simgrid::s4u::LinkInRoute::Direction::UP)
      .value("DOWN", simgrid::s4u::LinkInRoute::Direction::DOWN)
      .value("NONE", simgrid::s4u::LinkInRoute::Direction::NONE)
      .export_values();

  /* Class Split-Duplex Link */
  py::class_<simgrid::s4u::SplitDuplexLink, Link, std::unique_ptr<simgrid::s4u::SplitDuplexLink, py::nodelete>>(
      m, "SplitDuplexLink", "Network split-duplex link")
      .def("get_link_up",
           [](py::object self) // XBT_ATTRIB_DEPRECATED_v334
           {
             PyErr_WarnEx(PyExc_DeprecationWarning,
                          "get_link_up() is deprecated and  will be dropped after v3.33, use link_up instead.", 1);
             return self.attr("link_up");
           })
      .def_property_readonly("link_up", &simgrid::s4u::SplitDuplexLink::get_link_up, "Get link direction up")
      .def("get_link_down",
           [](py::object self) // XBT_ATTRIB_DEPRECATED_v334
           {
             PyErr_WarnEx(PyExc_DeprecationWarning,
                          "get_link_down() is deprecated and  will be dropped after v3.33, use link_down instead.", 1);
             return self.attr("link_down");
           })
      .def_property_readonly("link_down", &simgrid::s4u::SplitDuplexLink::get_link_down, "Get link direction down");

  /* Class Mailbox */
  py::class_<simgrid::s4u::Mailbox, std::unique_ptr<Mailbox, py::nodelete>>(
      m, "Mailbox", "Mailbox. See the C++ documentation for details.")
      .def(
          "__str__", [](const Mailbox* self) { return std::string("Mailbox(") + self->get_cname() + ")"; },
          "Textual representation of the Mailbox`")
      .def_static("by_name", &Mailbox::by_name,
                  py::call_guard<py::gil_scoped_release>(),
                  py::arg("name"),
                  "Retrieve a Mailbox from its name")
      .def_property_readonly(
          "name",
          [](const Mailbox* self) {
            return std::string(self->get_name().c_str()); // Convert from xbt::string because of MC
          },
          "The name of that mailbox (read-only property).")
      .def(
          "put",
          [](Mailbox* self, py::object data, int size, double timeout) {
            data.inc_ref();
            self->put(data.ptr(), size, timeout);
          },
          py::call_guard<py::gil_scoped_release>(), "Blocking data transmission with a timeout")
      .def(
          "put",
          [](Mailbox* self, py::object data, int size) {
            data.inc_ref();
            self->put(data.ptr(), size);
          },
          py::call_guard<py::gil_scoped_release>(), "Blocking data transmission")
      .def(
          "put_async",
          [](Mailbox* self, py::object data, int size) {
            data.inc_ref();
            return self->put_async(data.ptr(), size);
          },
          py::call_guard<py::gil_scoped_release>(), "Non-blocking data transmission")
      .def(
          "put_init",
          [](Mailbox* self, py::object data, int size) {
            data.inc_ref();
            return self->put_init(data.ptr(), size);
          },
          py::call_guard<py::gil_scoped_release>(),
          "Creates (but don’t start) a data transmission to that mailbox.")
      .def(
          "get",
          [](Mailbox* self) {
            py::object data = py::reinterpret_steal<py::object>(self->get<PyObject>());
            // data.dec_ref(); // FIXME: why does it break python-actor-create?
            return data;
          },
          py::call_guard<py::gil_scoped_release>(), "Blocking data reception")
      .def(
          "get_async",
          [](Mailbox* self) -> std::tuple<simgrid::s4u::CommPtr, PyGetAsync> {
            PyGetAsync wrap;
            auto comm = self->get_async(wrap.get());
            return std::make_tuple(std::move(comm), std::move(wrap));
          },
          py::call_guard<py::gil_scoped_release>(),
          "Non-blocking data reception. Use data.get() to get the python object after the communication has finished")
      .def(
          "set_receiver", [](Mailbox* self, ActorPtr actor) { self->set_receiver(actor); },
          py::call_guard<py::gil_scoped_release>(), "Sets the actor as permanent receiver");

  /* Class PyGetAsync */
  py::class_<PyGetAsync>(m, "PyGetAsync", "Wrapper for async get communications")
      .def(py::init<>())
      .def(
          "get", [](const PyGetAsync* self) { return py::reinterpret_steal<py::object>(*(self->get())); },
          "Get python object after async communication in receiver side");

  /* Class Comm */
  py::class_<simgrid::s4u::Comm, simgrid::s4u::CommPtr>(m, "Comm",
                                                        "Communication. See the C++ documentation for details.")
      .def("test", &simgrid::s4u::Comm::test, py::call_guard<py::gil_scoped_release>(),
           "Test whether the communication is terminated.")
      .def("wait", &simgrid::s4u::Comm::wait, py::call_guard<py::gil_scoped_release>(),
           "Block until the completion of that communication.")
      .def("wait_for", &simgrid::s4u::Comm::wait_for,
           py::arg("timeout"),
           py::call_guard<py::gil_scoped_release>(),
           "Block until the completion of that communication, or raises TimeoutException after the specified timeout.")
      .def("detach", [](simgrid::s4u::Comm* self) {
              return self->detach();
           },
           py::return_value_policy::reference_internal,
           py::call_guard<py::gil_scoped_release>(),
           "Start the comm, and ignore its result. It can be completely forgotten after that.")
      // use py::overload_cast for wait_all/wait_any, until the overload marked XBT_ATTRIB_DEPRECATED_v332 is removed
      .def_static(
          "wait_all", py::overload_cast<const std::vector<simgrid::s4u::CommPtr>&>(&simgrid::s4u::Comm::wait_all),
          py::arg("comms"),
          py::call_guard<py::gil_scoped_release>(),
          "Block until the completion of all communications in the list.")
      .def_static("wait_all_for", &simgrid::s4u::Comm::wait_all_for,
                  py::arg("comms"), py::arg("timeout"),
                  py::call_guard<py::gil_scoped_release>(),
                  "Block until the completion of all communications in the list, or raises TimeoutException after "
                  "the specified timeout.")
      .def_static(
          "wait_any", py::overload_cast<const std::vector<simgrid::s4u::CommPtr>&>(&simgrid::s4u::Comm::wait_any),
          py::arg("comms"),
          py::call_guard<py::gil_scoped_release>(),
          "Block until the completion of any communication in the list and return the index of the terminated one.")
      .def_static(
          "wait_any_for",
          py::overload_cast<const std::vector<simgrid::s4u::CommPtr>&, double>(&simgrid::s4u::Comm::wait_any_for),
          py::arg("comms"), py::arg("timeout"),
          py::call_guard<py::gil_scoped_release>(),
          "Block until the completion of any communication in the list and return the index of the terminated "
          "one, or -1 if a timeout occurred."
      );

  /* Class Io */
  py::class_<simgrid::s4u::Io, simgrid::s4u::IoPtr>(m, "Io", "I/O activities. See the C++ documentation for details.")
      .def("test", &simgrid::s4u::Io::test, py::call_guard<py::gil_scoped_release>(),
           "Test whether the I/O is terminated.")
      .def("wait", &simgrid::s4u::Io::wait, py::call_guard<py::gil_scoped_release>(),
           "Block until the completion of that I/O operation")
      .def_static(
          "wait_any_for", &simgrid::s4u::Io::wait_any_for, py::call_guard<py::gil_scoped_release>(),
          "Block until the completion of any I/O in the list (or timeout) and return the index of the terminated one.")
      .def_static("wait_any", &simgrid::s4u::Io::wait_any, py::call_guard<py::gil_scoped_release>(),
                  "Block until the completion of any I/O in the list and return the index of the terminated one.");

  /* Class Exec */
  py::class_<simgrid::s4u::Exec, simgrid::s4u::ExecPtr>(m, "Exec", "Execution. See the C++ documentation for details.")
      .def_property_readonly(
          "remaining",
          [](simgrid::s4u::ExecPtr self) {
            py::gil_scoped_release gil_guard;
            return self->get_remaining();
          },
          "Amount of flops that remain to be computed until completion (read-only property).")
      .def_property_readonly(
          "remaining_ratio",
          [](simgrid::s4u::ExecPtr self) {
            py::gil_scoped_release gil_guard;
            return self->get_remaining_ratio();
          },
          "Amount of work remaining until completion from 0 (completely done) to 1 (nothing done "
          "yet) (read-only property).")
      .def_property("host", &simgrid::s4u::Exec::get_host, &simgrid::s4u::Exec::set_host,
                    "Host on which this execution runs. Only the first host is returned for parallel executions. "
                    "Changing this value migrates the execution.")
      .def("test", &simgrid::s4u::Exec::test, py::call_guard<py::gil_scoped_release>(),
           "Test whether the execution is terminated.")
      .def("cancel", &simgrid::s4u::Exec::cancel, py::call_guard<py::gil_scoped_release>(), "Cancel that execution.")
      .def("start", &simgrid::s4u::Exec::start, py::call_guard<py::gil_scoped_release>(), "Start that execution.")
      .def("wait", &simgrid::s4u::Exec::wait, py::call_guard<py::gil_scoped_release>(),
           "Block until the completion of that execution.");

  /* Class Actor */
  py::class_<simgrid::s4u::Actor, ActorPtr>(m, "Actor",
                                            "An actor is an independent stream of execution in your distributed "
                                            "application. See the C++ documentation for details.")
      .def(
          "create",
          [](py::str name, Host* h, py::object fun, py::args args) {
            fun.inc_ref();  // FIXME: why is this needed for tests like exec-async, exec-dvfs and exec-remote?
            args.inc_ref(); // FIXME: why is this needed for tests like actor-migrate?
            return simgrid::s4u::Actor::create(name, h, [fun, args]() {
              py::gil_scoped_acquire py_context;
              try {
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
          py::call_guard<py::gil_scoped_release>(),
          "Create an actor from a function or an object. See the :ref:`example <s4u_ex_actors_create>`.")
      .def_property(
          "host", &Actor::get_host,
          [](Actor* a, Host* h) {
            py::gil_scoped_release gil_guard;
            a->set_host(h);
          },
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
      .def("set_auto_restart", &Actor::set_auto_restart, py::call_guard<py::gil_scoped_release>(),
           "Specify whether the actor shall restart when its host reboots.")
      .def("daemonize", &Actor::daemonize, py::call_guard<py::gil_scoped_release>(),
           "This actor will be automatically terminated when the last non-daemon actor finishes (more info in the C++ "
           "documentation).")
      .def("is_daemon", &Actor::is_daemon,
           "Returns True if that actor is a daemon and will be terminated automatically when the last non-daemon actor "
           "terminates.")
      .def("join", py::overload_cast<double>(&Actor::join, py::const_), py::call_guard<py::gil_scoped_release>(),
           "Wait for the actor to finish (more info in the C++ documentation).", py::arg("timeout"))
      .def("kill", &Actor::kill, py::call_guard<py::gil_scoped_release>(), "Kill that actor")
      .def("self", &Actor::self, "Retrieves the current actor.")
      .def("is_suspended", &Actor::is_suspended, "Returns True if that actor is currently suspended.")
      .def("suspend", &Actor::suspend, py::call_guard<py::gil_scoped_release>(),
           "Suspend that actor, that is blocked until resume()ed by another actor.")
      .def("resume", &Actor::resume, py::call_guard<py::gil_scoped_release>(),
           "Resume that actor, that was previously suspend()ed.")
      .def_static("kill_all", &Actor::kill_all, py::call_guard<py::gil_scoped_release>(), "Kill all actors but the caller.");
}
