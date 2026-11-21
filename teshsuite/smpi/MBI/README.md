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

At configure time, each generator writes its C cases in `build/MBI/tmp`. The cases about concurrency between the
application and pending MPI operations (`LocalConcurrency*` and `GlobalConcurrency*`) are then removed, as SimGrid does
not intercept the memory accesses of the application. Each remaining case is compiled with `smpicc` as `mbi_<case>`, and
run by the `mbi-<case>` test, under `simgrid-mc` with `-analyze`. A case passes if SimGrid reports an error if and only
if the case expects one.

MBI is superseded by MPI-BugBench (see `../MBB/README.md`), but MBB does not cover all of MBI yet. The section
"Before dropping MBI" of MBB's README lists what must be added to MBB before removing this directory.

## Local changes

- `MissingWaitandStartGenerator.py` does not generate the point-to-point `ReqLifecycle_MissingWait_*` cases: they
  free the active request with `MPI_Request_free`, which is legal MPI. The missing waits on collectives are kept.

## Known failures

None: all the cases pass (November 2026).
