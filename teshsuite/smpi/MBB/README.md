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
- In the lock mode of the RMA GlobalConcurrency cases, the local buffer accesses (RMA calls to rank 0) also lock the
  window of rank 0, as an RMA call needs an access epoch on its target.
- The generators are discovered in a sorted order, so that the cases are numbered the same way on every machine.

To update the copy from an MBB checkout in `$MBB`:

```
cd teshsuite/smpi/MBB/upstream
rm -rf Infrastructure errors
cp -r $MBB/scripts/Infrastructure $MBB/scripts/errors $MBB/scripts/generate.py .
find . -name __pycache__ -prune -exec rm -rf {} +
```

Then check that the local changes listed above are still needed, reapply them, and update the commit above.

## Removed and commented-out cases

The generator only produces the cases using MPI 3.1 (`--mpi_version 3.1`), the version implemented by SMPI. At levels 2
and 4, it only produces the cases that are common in real-world codes (see the scoring table below).

Some generated cases are then removed at configure time (counts at level 2):

- `LocalConcurrency-*` (67 cases), where a process touches a buffer used by one of its own pending operations. The data
  race detection only compares the accesses of distinct actors, so it cannot see these errors.
- `GlobalConcurrency-*` (58 cases), data races between processes on RMA windows, only when SimGrid is built without
  smemory. Otherwise, they are compiled with `smpicc -smemory -O0`, so that the memory accesses of the application
  are instrumented (`-O0` keeps the accesses whose result is unused). SMPI records the accesses that the RMA calls do
  on behalf of the application, the model-checker reports the data races, and `MBB.py` classifies them as concurrency
  errors.
- The cases using an uninitialized pointer (30 cases, named `not_allocated` by the generators). This is undefined
  behavior that no runtime can reliably detect, so their outcome depends on the garbage left on the stack. The
  `InvalidParam-*` case calling `MPI_Win_fence` on an uninitialized `MPI_Win` is not removed, and currently passes only
  by chance.

The following cases are commented out in the upstream generators, so they are never generated:

- `errors/pt2pt/InvalidRequest.py`: `InvalidParam-Request-mpi_request_free`, freeing an active persistent request. The
  comment notes that MPI permits it.
- `errors/pt2pt/InvalidTag.py`: the invalid tags of `MPI_Sendrecv` and `MPI_Sendrecv_replace`. No reason is given.

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

Categorization of the 154 cases still failing at level 2 (November 2026), by where the fix probably belongs.

### Bugs in MBB (9 cases)

These cases are wrong: SimGrid rightfully reports an error on a correct program, or the program is not erroneous.

- `Correct-mpi_gather-002` (1 case, reported as `MPI_ERR_BUFFER`): `errors/coll/ParamMatchingType.py` passes
  `MPI_IN_PLACE` as the send buffer on every rank, while MPI only allows it at the root of rooted collectives. The
  generator should only use `MPI_IN_PLACE` at the root.
- `Correct-rmarequest-*` (4 cases, reported as data races): in the `rmarequest` synchronization of
  `errors/rma/GlobalConcurrency.py`, rank 0 waits for the local completion of its request-based RMA call on the window
  of rank 1, while another rank accesses the same location: rank 1 gets data into it (`raccumulate_localbufwrite` and
  `rget_accumulate_localbufwrite`), or rank 2 puts data into it (`rget_put` and `rget_rput`). This wait does not order
  anything with the other rank, so the program has a data race. The generator should synchronize both ranks, for
  instance with a barrier after the wait.
- `GlobalConcurrency-lock-compare_and_swap_bufread-001` and `GlobalConcurrency-lock-fetch_and_op_bufread-001` (2
  cases, not detected): the `bufread` and `bufwrite` instructions of `errors/rma/GlobalConcurrency.py` access
  `winbuf[1]`, while `MPI_Compare_and_swap` and `MPI_Fetch_and_op` only update `winbuf[0]`. MPI only forbids
  conflicting accesses to the same location, so these programs have no data race. The other RMA calls of these cases
  update 10 elements, which include `winbuf[1]`. The generator should access `winbuf[0]` instead.
- `InvalidParam-Request-mpi_waitany-001` and `InvalidParam-Request-mpi_waitsome-001` (2 cases, not detected): the
  request appears twice in the array, but `count` is 1, so only the first entry is used and the program is correct.
  The generator should pass a count of 2.

### Missing checks in SMPI (56 cases)

- Invalid statuses (16 cases, not detected): `InvalidParam-Status-*` pass `NULL` as status, or `MPI_STATUSES_IGNORE`
  where a single status is expected, to `MPI_Probe`, `MPI_Recv`, `MPI_Sendrecv`, `MPI_Sendrecv_replace` and `MPI_Wait*`.
  In C, `smpi.h` defines `MPI_STATUS_IGNORE`, `MPI_STATUSES_IGNORE` and `NULL` all as `NULL`, so SMPI cannot tell them
  apart. They should become distinct sentinel values, as in MPICH.
- Buffers used after `free()` (8 cases, not detected): `InvalidParam-Buffer-*` of the point-to-point calls free the
  buffer before using it. The interception of `malloc` and `free` should remember the freed blocks, so that
  `CHECK_BUFFER` rejects them.
- Mismatched datatypes between sender and receiver (32 cases, not detected): `GlobalParameterMissmatch-Dtype-*` and
  `GlobalParameterMissmatch-Type-mpi_bcast-*`. `Request::match_types()` accepts `MPI_BYTE` and `MPI_PACKED` with any
  other datatype, and never compares derived datatypes, so a `MPI_BYTE` received as `MPI_INT`, or a
  `contiguous(2, MPI_INT)` received as `MPI_DOUBLE`, go unnoticed. Comparing the type signatures (the flattened
  sequences of basic types) would fix it, at least in pedantic mode.

### Missing features in SMPI (12 cases)

- `MPI_Intercomm_create` is not implemented (12 cases, reported as `UNIMPLEMENTED`), as SMPI has no intercommunicators
  at all. This is a significant work.

### Limitations of simgrid-mc (52 cases)

- Matched probes (52 cases, reported as `UNIMPLEMENTED`): `MPI_Mprobe` and `MPI_Improbe` (including the cases using
  `MPI_Mrecv` and `MPI_Imrecv`) are not implemented in SMPI. They need `MPI_Message` handles, as a matched message
  must be removed from the mailbox and kept for a later reception, and a model-checker transition removing it, built
  on the one of `MPI_Probe`.

### Out of reach for now (25 cases)

- `LocalParameterMissmatch-*` (25 cases, not detected): the C type of the buffer differs from the MPI datatype, such as
  an `int*` buffer sent as `MPI_FLOAT`. SimGrid detects the issue when the sizes differ (110 other cases of this
  family pass), but not when they match. Detecting this requires knowing the type of each allocation, as TypeART does
  for MUST. That could perhaps be done one day through `mclang` instrumentation, but it is way beyond our scope for now.
