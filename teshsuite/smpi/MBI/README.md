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

Categorization of the 12 cases still failing (November 2026), by where the fix probably belongs.

### Limitations of simgrid-mc (12 cases)

- Probes (8 cases, crashing): `CallOrdering_Probe_{Irecv,Recv}_{Isend,Send}_{ok,nok}`. `MPI_Probe` aborts under the
  model-checker (`smpi_request.cpp`: "MPI_Probe is not supported yet in MC mode"), as SMPI implements it as a busy loop
  on `MPI_Iprobe`, which would never end under the model-checker. It needs a dedicated blocking transition, only
  enabled when a matching message is in the mailbox. The same issue breaks 4 MBB cases.
- Scan and Exscan (4 cases, crashing): `MessageRace_{Scan,Exscan}_{Isend,Send}_Irecv_ok`. The application aborts with
  "Please, model-checker, don't execute disabled transitions. You tried to execute BARRIER_WAIT(barrier_id:0) which
  is disabled". This is a bug of the exploration: when reversing a race between two `WaitAny` of the same actor, ODPOR
  builds a sequence in which a `BARRIER_WAIT` runs before all the actors locked the barrier. Only Scan and Exscan
  trigger it because their default implementation (`scan__default` and `exscan__default`) waits for several receptions
  with `Request::waitany()`, on 4 processes here. The suspects are the dependencies of `BarrierTransition` with the
  `BARRIER_ASYNC_LOCK` of the other actors in `src/mc/transition/TransitionSynchro.cpp`, and the handling of `WaitAny`
  transitions whose `times_considered` changes in `src/mc/explo/odpor/`. The replay sequence printed in this case is
  truncated, which should be fixed first to make this bug easier to investigate.
