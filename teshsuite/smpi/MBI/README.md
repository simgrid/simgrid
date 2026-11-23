# MPI Bugs Initiative (MBI) test suite

This directory holds the test generators of the MPI Bugs Initiative (https://gitlab.com/MpiBugsInitiative/), and the
glue code to run the generated cases with SimGrid's model-checker. It is enabled with `-Denable_testsuite_smpi_MBI=ON`,
which requires SMPI and the model-checker.

| File                                   | Origin                                                             |
| -------------------------------------- | ------------------------------------------------------------------ |
| `*Generator.py`, `generator_utils.py`  | MBI generators, with local changes listed below                    |
| `MBIutils.py`, `simgrid.py`            | MBI infrastructure to run the cases and classify SimGrid's output  |
| `MBI.py`                               | SimGrid runner, executing one compiled case as a ctest test        |
| `CMakeLists.txt`                       | SimGrid integration                                                |

At configure time, each generator writes its C cases in `build/MBI/tmp`, and some of them are removed (see below). Each
remaining case is compiled with `smpicc` as `mbi_<case>`, and run by the `mbi-<case>` test, under `simgrid-mc` with
`-analyze`. A case passes if SimGrid reports an error if and only if the case expects one.

MBI is superseded by MPI-BugBench (see `../MBB/README.md`), but MBB does not cover all of MBI yet. The section
"Before dropping MBI" of MBB's README lists what must be added to MBB before removing this directory.

## Local changes

- `MissingWaitandStartGenerator.py` does not generate the point-to-point `ReqLifecycle_MissingWait_*` cases: they
  free the active request with `MPI_Request_free`, which is legal MPI. The missing waits on collectives are kept.
- `simgrid.py` classifies the data races reported by the model-checker as concurrency errors.

## Removed and commented-out cases

These generated cases are removed at configure time:

- `LocalConcurrency*` (63 cases), where a process touches a buffer used by one of its own pending operations. The data
  race detection only compares the accesses of distinct actors, so it cannot see these errors.
- `GlobalConcurrency*` (38 cases), data races between processes on RMA windows, only when SimGrid is built without
  smemory. Otherwise, they are compiled with `smpicc -smemory -O0`, so that the memory accesses of the application
  are instrumented (`-O0` keeps the accesses whose result is unused), and the model-checker reports their data races.

The point-to-point `ReqLifecycle_MissingWait_*` cases are not generated, as listed in the local changes above.

The following cases were already commented out in the generators when they were imported from MBI, so they are never
generated. MBI gives no reason, except for the second one, marked "TO CHECK":

- `CollP2PMatchingGenerator.py`: `CollP2PBuffering_*_nok`, a deadlock depending on the buffering of a point-to-point
  communication matched with a collective.
- `P2PComGenerator.py`: `InvalidParam_Com_*_nok`, a send and a receive using different communicators.
- `RMAInvalidArgGenerator.py`: `InvalidParam_BufferNull_*_nok`, a one-sided operation with a `NULL` origin buffer.

## Known failures

None: all the cases pass (November 2026), including the `GlobalConcurrency*` ones when SimGrid is built with smemory.
