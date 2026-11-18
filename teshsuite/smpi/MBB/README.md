# MPI-BugBench (MBB) test suite

The `upstream/` directory is a copy of the test generator of MPI-BugBench
(https://git-ce.rwth-aachen.de/hpc-public/mpi-bugbench), taken from its `main` branch at commit `TODO`:

| Upstream path             | Local path                 |
| ------------------------- | -------------------------- |
| `scripts/Infrastructure/` | `upstream/Infrastructure/` |
| `scripts/errors/`         | `upstream/errors/`         |
| `scripts/generate.py`     | `upstream/generate.py`     |

These files are modified only by patches meant to be sent upstream, so `upstream/` can be diffed against an MBB
checkout to extract them. Local changes not merged upstream yet:

- clang-format is not applied to the generated files unless `generate.py --clang_format` is given.
- `generate.py --score_table` loads a precomputed scoring table, and `--save_score_table` saves the computed one.
- Allocations no longer cast to the allocated type (invalid C for anonymous structs such as the MPI_2INT buffer),
  and `mpi_intercomm_create_from_groups` builds its groups with `MPI_Group_range_incl` and a valid last rank.
- The RMA GlobalConcurrency cases only use a third rank when both accesses are remote ones.

To update the copy from an MBB checkout in `$MBB`:

```
cd teshsuite/smpi/MBB/upstream
rm -rf Infrastructure errors
cp -r $MBB/scripts/Infrastructure $MBB/scripts/errors $MBB/scripts/generate.py .
find . -name __pycache__ -prune -exec rm -rf {} +
```

Then check that the local changes listed above are still needed, reapply them, and update the commit above.

## The scoring table

Generation levels 2 and 4 only produce the cases that are common in real-world codes. MBB scores them with a scoring
table that it computes from `real_world_data.csv.tar.gz`, a dataset of the MPI calls found in real-world applications
(1.3 MB compressed, 24 MB once extracted). Only the resulting table is needed, so we ship it as `score_table.csv`
(114 kB) instead of the dataset. Regenerate it when the dataset or `upstream/Infrastructure/ScoingModule/` changes:

```
cd $MBB && tar xzf real_world_data.csv.tar.gz
cd $SIMGRID/teshsuite/smpi/MBB
python3 upstream/generate.py --generator_dir upstream/errors --level 2 --count_only \
        --real_world_data $MBB/output.csv --save_score_table score_table.csv
```

`generate.py --score_table score_table.csv` then generates exactly the same cases as from the dataset.

## Before dropping MBI

MBB does not cover everything that `../MBI/` tests yet, at any generation level. The following should be added to MBB
(preferably upstream) before removing MBI:

- RMA epochs mixing `MPI_Win_lock` and `MPI_Win_lock_all` in both orders. MBB only nests epochs of the same kind or
  after a fence, while these mixed cases exercise distinct checks in `Win::lock()` and `Win::lock_all()`.
- Scan and Exscan with at least 3 processes, racing with an `MPI_ANY_SOURCE` reception. MBB only runs them with 2
  processes, so no rank waits on several receptions at once (MBI's `MessageRace_{Scan,Exscan}_*_ok` cases).
- Calls to `MPI_Allgatherv`, `MPI_Alltoallv`, `MPI_Iallgatherv`, `MPI_Ialltoallv`, `MPI_Iexscan`, `MPI_Group_excl`,
  `MPI_Op_create` and `MPI_Op_free`, that MBB never uses.
- Resource leaks (MBI's `ResLeak_*` cases) and missing starts of persistent requests (`ReqLifecycle_MissingStart_*`).
- Missing waits on non-blocking collectives. MBB only has `MPI_Ibarrier`, while MBI covers 11 other ones
  (`ReqLifecycle_MissingWait_I*`).
- Cases depending on the buffering of point-to-point communications, run with both zero and infinite buffering
  (MBI's `P2PBuffering_*` cases, using `--cfg=smpi/buffering`).
- Call orderings depending on the input of the program (MBI's `InputHazardCallOrdering_*` cases).

## TODO

- Free the buffers allocated by the generated cases (`Infrastructure/AllocCall.py` provides `get_free()`). SMPI reports
  the unfreed buffers at the end of each explored trace, which bloats the logs of the failing cases.

## Known failures

Categorization of the 259 failing cases at level 2 (November 2026), by where the fix belongs.

| Fix in     | Cases | Problem |
| ---------- | ----- | --- |
| MBB        |   1   | `Correct-mpi_gather`: `ParamMatchingType.py` passes `MPI_IN_PLACE` as sendbuf on non-root ranks too |
| MBB        |   8   | `Correct-lock-*_localbuf*`: the local access is an RMA call to rank 0, but only rank 1 is locked |
| MBB        |   2   | `InvalidParam-Request-mpi_wait{any,some}-001`: the duplicated request is not within `count` |
| MBB        |   1   | `InvalidParam-array_of_indices-mpi_waitsome-002`: an uninitialized pointer cannot be detected |
| SMPI       |   31  | `MPI_IN_PLACE`, or `MPI_BOTTOM` with a basic datatype, given as point-to-point buffer |
| SMPI       |   9   | `NULL` arrays of requests or indices, `MPI_Initialized(NULL)`, `MPI_Start` on an active request |
| SMPI       |   3   | `InvalidParam-001`, `-011` and `-013` (RMA): not understood yet |
| SMPI       |   14  | Invalid status: `MPI_STATUS_IGNORE`, `MPI_STATUSES_IGNORE` and `NULL` are all `NULL` in `smpi.h` |
| SMPI       |   8   | Buffer used after `free()`: the malloc interception should remember the freed blocks |
| SMPI       |   42  | Mismatched datatypes: `Request::match_types` accepts `MPI_BYTE` with anything, ignores derived types |
| SMPI       |   6   | `MPI_Comm_idup` is not implemented |
| SMPI       |   12  | `MPI_Intercomm_create` is not implemented (no intercommunicators in SMPI) |
| simgrid-mc |   59  | `MPI_Probe` unsupported by the model-checker, `MPI_Mprobe` and `MPI_Improbe` not implemented |
| simgrid-mc |   38  | Timeouts of `Correct-lockall-*` and `Correct-rmarequest-*`, using `MPI_Win_lock_all` on 3 ranks |
| none yet | 25 | `LocalParameterMissmatch`: the C type of the buffer is not known at runtime |

The first two SMPI lines are addressed by the argument checks added to `smpi_pmpi_request.cpp` and
`smpi_pmpi.cpp`. 8 of the timeouts should be gone with 2 ranks instead of 3 (see the local changes above).
