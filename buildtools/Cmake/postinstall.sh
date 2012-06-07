#! /bin/sh
echo "export SIMGRID_ROOT=/Applications/SimGrid" >> ~/.profile
echo "export DYLD_LYBRARY_PATH=$DYLD_LYBRARY_PATH:$SIMGRID_ROOT/lib" >> ~/.profile