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
- The generators are discovered in a sorted order, so that the cases are numbered the same way on every machine.

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

Categorization of the 162 cases still failing at level 2 (November 2026), by where the fix probably belongs.

### Bugs in MBB (12 cases)

These cases are wrong: SimGrid rightfully reports an error on a correct program, or the program is not erroneous.

- `Correct-mpi_gather-002` (1 case, reported as `MPI_ERR_BUFFER`): `errors/coll/ParamMatchingType.py` passes
  `MPI_IN_PLACE` as the send buffer on every rank, while MPI only allows it at the root of rooted collectives. The
  generator should only use `MPI_IN_PLACE` at the root.
- `Correct-lock-*_localbuf*` (8 cases, reported as `MPI_ERR_WIN`): in the `lock` synchronization of
  `errors/rma/GlobalConcurrency.py`, rank 1 locks the window of rank 1, but the `localbufread` and `localbufwrite`
  accesses are implemented as RMA calls (`MPI_Put` or `MPI_Get` from the local window buffer) targeting rank 0,
  where no epoch is open. The generator should also lock rank 0, or use a plain load or store for these accesses.
- `InvalidParam-Request-mpi_waitany-001` and `InvalidParam-Request-mpi_waitsome-001` (2 cases, not detected): the
  request appears twice in the array, but `count` is 1, so only the first entry is used and the program is correct.
  The generator should pass a count of 2.
- The `InvalidParam-*` case calling `MPI_Win_fence` on an uninitialized `MPI_Win` (1 case, crashing): uninitialized
  handles or pointers are undefined behavior that no runtime can reliably detect. The same goes for
  `InvalidParam-array_of_indices-mpi_waitsome-002`, which passes only because its uninitialized pointer happens to be
  `NULL`. Such cases should rather be dropped from MBB.

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

### Limitations of simgrid-mc (57 cases)

- Matched probes (57 cases, reported as `UNIMPLEMENTED`): `MPI_Mprobe` and `MPI_Improbe` (including the cases using
  `MPI_Mrecv` and `MPI_Imrecv`) are not implemented in SMPI. They need `MPI_Message` handles, as a matched message
  must be removed from the mailbox and kept for a later reception, and a model-checker transition removing it, built
  on the one of `MPI_Probe`.

### Out of reach for now (25 cases)

- `LocalParameterMissmatch-*` (25 cases, not detected): the C type of the buffer differs from the MPI datatype, such as
  an `int*` buffer sent as `MPI_FLOAT`. SimGrid detects the issue when the sizes differ (110 other cases of this
  family pass), but not when they match. Detecting this requires knowing the type of each allocation, as TypeART does
  for MUST. That could perhaps be done one day through `mclang` instrumentation, but it is way beyond our scope for now.
