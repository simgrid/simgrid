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
cd $MBB && tar xzf real_world_data.csv.tar.gz && cd scripts
python3 -c "import pandas as pd
from Infrastructure.ScoingModule.ScoringTable import get_scoring_table
get_scoring_table(pd.read_csv('../output.csv', header=0, low_memory=False), c_only=True).to_csv('score_table.csv', index=False)"
mv score_table.csv $SIMGRID/teshsuite/smpi/MBB/
```

Reload it with `pd.read_csv('score_table.csv', header=0, dtype=str).astype({'score': float})` to get a table that
generates exactly the same cases as the one computed from the dataset.
