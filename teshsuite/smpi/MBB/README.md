# MPI-BugBench (MBB) test suite

The `upstream/` directory is a copy of the test generator of MPI-BugBench
(https://git-ce.rwth-aachen.de/hpc-public/mpi-bugbench), taken from its `main` branch at commit `TODO`:

| Upstream path                 | Local path                            |
|-------------------------------|---------------------------------------|
| `scripts/Infrastructure/`     | `upstream/Infrastructure/`            |
| `scripts/errors/`             | `upstream/errors/`                    |
| `scripts/generate.py`         | `upstream/generate.py`                |
| `real_world_data.csv.tar.gz`  | `upstream/real_world_data.csv.tar.gz` |

These files are modified only by patches meant to be sent upstream, so `upstream/` can be diffed against an MBB
checkout to extract them. Local changes not merged upstream yet:

- clang-format is not applied to the generated files unless `generate.py --clang_format` is given.

To update the copy from an MBB checkout in `$MBB`:

```
cd teshsuite/smpi/MBB/upstream
rm -rf Infrastructure errors
cp -r $MBB/scripts/Infrastructure $MBB/scripts/errors $MBB/scripts/generate.py $MBB/real_world_data.csv.tar.gz .
find . -name __pycache__ -prune -exec rm -rf {} +
```

Then check that the local changes listed above are still needed, reapply them, and update the commit above.
