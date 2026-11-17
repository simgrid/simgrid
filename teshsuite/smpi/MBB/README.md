# MPI-BugBench (MBB) test suite

The `upstream/` directory is a copy of the test generator of MPI-BugBench
(https://git-ce.rwth-aachen.de/hpc-public/mpi-bugbench), taken from its `main` branch at commit `TODO`:

| Upstream path             | Local path                 |
|---------------------------|----------------------------|
| `scripts/Infrastructure/` | `upstream/Infrastructure/` |
| `scripts/errors/`         | `upstream/errors/`         |
| `scripts/generate.py`     | `upstream/generate.py`     |

These files are modified only by patches meant to be sent upstream, so `upstream/` can be diffed against an MBB
checkout to extract them. Local changes not merged upstream yet:

- clang-format is not applied to the generated files unless `generate.py --clang_format` is given.
- `generate.py --score_table` loads a precomputed scoring table, and `--save_score_table` saves the computed one.
- Allocations no longer cast to the allocated type (invalid C for anonymous structs such as the MPI_2INT buffer),
  and `mpi_intercomm_create_from_groups` builds its groups with `MPI_Group_range_incl` and a valid last rank.

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
