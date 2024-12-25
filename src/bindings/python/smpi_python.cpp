/* Copyright (c) 2018-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include <pybind11/pybind11.h> // Must come before our own stuff

#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include <string>
#include <vector>

#include <smpi/smpi.h>
#include <smpi_comm.hpp>
#include <smpi_datatype.hpp>
#include <smpi_op.hpp>

#include <simgrid/Exception.hpp>
#include <simgrid/s4u/Host.hpp>

#define BIND_ALL_TYPES(module, name, function, ...)                                                                    \
  module.def(name, function<uint8_t>, ##__VA_ARGS__);                                                                  \
  module.def(name, function<uint16_t>, ##__VA_ARGS__);                                                                 \
  module.def(name, function<int>, ##__VA_ARGS__);                                                                      \
  module.def(name, function<double>, ##__VA_ARGS__);                                                                   \
  module.def(name, function<float>, ##__VA_ARGS__);                                                                    \
  module.def(name, function<long>, ##__VA_ARGS__);                                                                     \
  module.def(name, function<long long>, ##__VA_ARGS__);

namespace py = pybind11;
using simgrid::s4u::Host;
XBT_LOG_NEW_DEFAULT_CATEGORY(smpi_python, "python");

namespace {
template <typename T>
std::vector<T>* wrap_returned_mpi_alltoall(std::vector<T> data, int sendcount, MPI_Datatype sendtype, int recvcount,
                                           MPI_Datatype recvtype, MPI_Comm comm)
{
  auto new_list = new std::vector<T>();
  new_list->resize(recvcount * comm->size());
  MPI_Alltoall(data.data(), sendcount, sendtype, new_list->data(), recvcount, recvtype, comm);
  return new_list;
}
template <typename T>
void wrap_mpi_alltoall(py::array_t<T> data, int sendcount, MPI_Datatype sendtype, py::array_t<T>& output, int recvcount,
                       MPI_Datatype recvtype, MPI_Comm comm)
{
  py::buffer_info out_buffer = output.request();
  auto* output_ptr           = static_cast<T*>(out_buffer.ptr);
  MPI_Alltoall(data.data(), sendcount, sendtype, output_ptr, recvcount, recvtype, comm);
}
template <typename T> void wrap_mpi_bcast(py::array_t<T> output, int count, MPI_Datatype type, int root, MPI_Comm comm)
{
  py::buffer_info out_buffer = output.request();
  auto* output_ptr           = static_cast<T*>(out_buffer.ptr);
  MPI_Bcast(output_ptr, count, type, root, comm);
}
template <typename T>
void wrap_mpi_reduce_scatter(py::array_t<T> input, py::array_t<T> output, std::vector<int> count, MPI_Datatype type,
                             MPI_Op op, MPI_Comm comm)
{
  py::buffer_info out_buffer = output.request();
  auto* output_ptr           = static_cast<T*>(out_buffer.ptr);
  MPI_Reduce_scatter(input.data(), output_ptr, count.data(), type, op, comm);
}
template <typename T>
void wrap_mpi_reduce_scatter_block(py::array_t<T> input, py::array_t<T> output, int count, MPI_Datatype type, MPI_Op op,
                                   MPI_Comm comm)
{
  py::buffer_info out_buffer = output.request();
  auto* output_ptr           = static_cast<T*>(out_buffer.ptr);
  MPI_Reduce_scatter_block(input.data(), output_ptr, count, type, op, comm);
}
template <typename T>
void wrap_mpi_reduce(py::array_t<T> input, py::array_t<T> output, int count, MPI_Datatype type, MPI_Op op, int root,
                     MPI_Comm comm)
{
  py::buffer_info out_buffer = output.request();
  auto* output_ptr           = static_cast<T*>(out_buffer.ptr);
  MPI_Reduce(input.data(), output_ptr, count, type, op, root, comm);
}
template <typename T>
void wrap_mpi_allreduce(py::array_t<T> input, py::array_t<T> output, int count, MPI_Datatype type, MPI_Op op,
                        MPI_Comm comm)
{
  py::buffer_info out_buffer = output.request();
  auto* output_ptr           = static_cast<T*>(out_buffer.ptr);
  MPI_Allreduce(input.data(), output_ptr, count, type, op, comm);
}
template <typename T>
void wrap_mpi_allgather(py::array_t<T> input, int send_count, MPI_Datatype send_type, py::array_t<T> output,
                        int output_count, MPI_Datatype output_type, MPI_Comm comm)
{
  py::buffer_info out_buffer = output.request();
  auto* output_ptr           = static_cast<T*>(out_buffer.ptr);
  MPI_Allgather(input.data(), send_count, send_type, output_ptr, output_count, output_type, comm);
}
} // namespace

void SMPI_bindings(py::module& m)
{

  static py::object pyForcefulKillEx(py::register_exception<simgrid::ForcefulKillException>(m, "ActorKilled"));

  /* smpi namespace */
  auto smpi_m =
      m.def_submodule("smpi", "Bindings of the s4u::smpi namespace. See the C++ documentation for details.")
          .def("replay_init", &smpi_replay_init, py::call_guard<py::gil_scoped_release>(), "Initialize the SMPI replay")
          .def("replay_run", &smpi_replay_run, py::call_guard<py::gil_scoped_release>(), "Run the SMPI replay")
          .def("replay_main", &smpi_replay_main, py::call_guard<py::gil_scoped_release>(), "Run the SMPI replay main")
          .def("init", &SMPI_init, py::call_guard<py::gil_scoped_release>(), "Initialize the SMPI interface")
          .def("finalize", &SMPI_finalize, py::call_guard<py::gil_scoped_release>(), "Finalize the SMPI interface")
          .def("thread_create", &SMPI_thread_create, py::call_guard<py::gil_scoped_release>(),
               "Initialize the SMPI interface with threads")
          .def(
              "main",
              [](const std::string& name, std::vector<std::string> args) {
                auto argc = static_cast<int>(args.size());
                std::vector<char*> argv(args.size() + 1); // argv[argc] is nullptr
                std::transform(begin(args), end(args), begin(argv), [](std::string& s) { return &s.front(); });
                // Currently this can be dangling, we should wrap this somehow.
                return smpi_main(name.c_str(), argc, argv.data());
              },
              "The constructor should take the parameters from the command line, as is ")
          .def(
              "app_instance_register",
              [](const std::string& name, std::vector<Host*> const hosts, py::object fun, py::args args) {
                fun.inc_ref();  // keep alive after return
                args.inc_ref(); // keep alive after return
                const py::gil_scoped_release gil_release;
                return SMPI_app_instance_start(
                    name.c_str(),
                    [fun_p = fun.ptr(), args_p = args.ptr()]() {
                      const py::gil_scoped_acquire py_context;
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
                    },
                    hosts);
              },
              "Create an actor from a function or an object. See the :ref:`example <s4u_ex_actors_create>`.")
          .def(
              "app_instance_start",
              [](const std::string& name, py::object fun, std::vector<Host*> const hosts) {
                fun.inc_ref(); // keep alive after return
                const py::gil_scoped_release gil_release;
                return SMPI_app_instance_start(
                    name.c_str(),
                    [fun_p = fun.ptr()]() {
                      const py::gil_scoped_acquire py_context;
                      try {
                        const auto fun = py::reinterpret_borrow<py::object>(fun_p);
                        fun();
                      } catch (const py::error_already_set& ex) {
                        if (ex.matches(pyForcefulKillEx)) {
                          XBT_VERB("Actor killed");
                          simgrid::ForcefulKillException::do_throw(); // Forward that ForcefulKill exception
                        }
                        throw;
                      }
                    },
                    hosts);
              },
              "Create an actor from a function or an object. See the :ref:`example <s4u_ex_actors_create>`.");
  auto mpi_m =
      smpi_m.def_submodule("MPI", "Bindings of MPI functions.")
          .def(
              "Init", []() { return MPI_Init(); }, py::call_guard<py::gil_scoped_release>(),
              "Initialize the MPI interface")
          .def(
              "Init",
              [](std::vector<std::string> args) {
                auto argc = static_cast<int>(args.size());
                std::vector<char*> argv(args.size() + 1); // argv[argc] is nullptr
                std::transform(begin(args), end(args), begin(argv), [](std::string& s) { return &s.front(); });
                // Currently this can be dangling, we should wrap this somehow.
                char** argv_data = argv.data();
                return MPI_Init(&argc, &argv_data);
              },
              "Initialize the MPI interface")
          .def("Finalize", &MPI_Finalize, py::call_guard<py::gil_scoped_release>(), "Finalize the MPI interface");
  py::class_<simgrid::smpi::Comm, std::unique_ptr<simgrid::smpi::Comm, py::nodelete>> mpi_comm(
      mpi_m, "Comm", "Simulated host. See the C++ documentation for details.");

  mpi_comm
      .def(
          "size",
          [](simgrid::smpi::Comm* comm) {
            int size;
            MPI_Comm_size(comm, &size);
            return size;
          },
          "Get the size of the communicator")
      .def(
          "rank",
          [](simgrid::smpi::Comm* comm) {
            int rank;
            MPI_Comm_rank(comm, &rank);
            return rank;
          },
          "Get the rank within the communicator");
  mpi_comm.def_property_readonly_static("WORLD",
                                        [](py::object /* self */) { return (simgrid::smpi::Comm*)MPI_COMM_WORLD; });
  mpi_comm.def_property_readonly_static("SELF",
                                        [](py::object /* self */) { return (simgrid::smpi::Comm*)MPI_COMM_SELF; });

  BIND_ALL_TYPES(mpi_m, "Alltoall", wrap_mpi_alltoall, py::arg("data"), py::arg("sendcount"), py::arg("sendtype"),
                 py::arg("output"), py::arg("recvcount"), py::arg("recvtype"), py::arg("comm"),
                 py::call_guard<py::gil_scoped_release>(), "Alltoall operation");

  BIND_ALL_TYPES(mpi_m, "Bcast", wrap_mpi_bcast, py::arg("output"), py::arg("count"), py::arg("type"), py::arg("root"),
                 py::arg("comm"), py::call_guard<py::gil_scoped_release>(), "Bcast operation");

  BIND_ALL_TYPES(mpi_m, "Allreduce", wrap_mpi_allreduce, py::arg("input"), py::arg("output"), py::arg("count"),
                 py::arg("type"), py::arg("op"), py::arg("comm"), py::call_guard<py::gil_scoped_release>(),
                 "Allreduce operation");

  BIND_ALL_TYPES(mpi_m, "Allgather", wrap_mpi_allgather, py::arg("input"), py::arg("send_count"), py::arg("send_type"),
                 py::arg("output"), py::arg("output_count"), py::arg("output_type"), py::arg("comm"),
                 py::call_guard<py::gil_scoped_release>(), "Allgather operation");

  BIND_ALL_TYPES(mpi_m, "Reduce", wrap_mpi_reduce, py::arg("input"), py::arg("output"), py::arg("count"),
                 py::arg("type"), py::arg("op"), py::arg("root"), py::arg("comm"),
                 py::call_guard<py::gil_scoped_release>(), "Reduce operation");

  BIND_ALL_TYPES(mpi_m, "Reduce_scatter", wrap_mpi_reduce_scatter, py::arg("input"), py::arg("output"),
                 py::arg("count"), py::arg("type"), py::arg("op"), py::arg("comm"),
                 py::call_guard<py::gil_scoped_release>(), "Reduce scatter operation");

  BIND_ALL_TYPES(mpi_m, "Reduce_scatter_block", wrap_mpi_reduce_scatter_block, py::arg("input"), py::arg("output"),
                 py::arg("count"), py::arg("type"), py::arg("op"), py::arg("comm"),
                 py::call_guard<py::gil_scoped_release>(), "Reduce scatter block operation");

  py::class_<simgrid::smpi::Op, std::unique_ptr<simgrid::smpi::Op, py::nodelete>> mpi_op(
      mpi_m, "Op", "Simulated host. See the C++ documentation for details.");
  mpi_op.def_readonly_static("MAX", MPI_MAX)
      .def_readonly_static("MIN", MPI_MIN)
      .def_readonly_static("MAX", MPI_MAXLOC)
      .def_readonly_static("MIN", MPI_MINLOC)
      .def_readonly_static("SUM", MPI_SUM)
      .def_readonly_static("PROD", MPI_PROD)
      .def_readonly_static("LAND", MPI_LAND)
      .def_readonly_static("LOR", MPI_LOR)
      .def_readonly_static("LXOR", MPI_LXOR)
      .def_readonly_static("BAND", MPI_BAND)
      .def_readonly_static("BOR", MPI_BOR)
      .def_readonly_static("BXOR", MPI_BXOR)
      .def_readonly_static("REPLACE", MPI_REPLACE)
      .def_readonly_static("NO_OP", MPI_NO_OP);

  py::class_<MPI_Status>(mpi_m, "MPI_Status")
      .def(py::init<>())
      .def_readonly("MPI_SOURCE", &MPI_Status::MPI_SOURCE)
      .def_readonly("MPI_TAG", &MPI_Status::MPI_TAG)
      .def_readonly("MPI_ERROR", &MPI_Status::MPI_ERROR)
      .def_readonly("count", &MPI_Status::count)
      .def_readonly("cancelled", &MPI_Status::cancelled);

  py::class_<simgrid::smpi::Datatype, std::unique_ptr<simgrid::smpi::Datatype, py::nodelete>> mpi_datatype(
      mpi_m, "Datatype", "Simulated host. See the C++ documentation for details.");
  mpi_datatype.def_property_readonly_static("DOUBLE", [](py::object /* self */) { return MPI_DOUBLE; })
      .def_property_readonly_static("INT", [](py::object /* self */) { return MPI_INT; });
}