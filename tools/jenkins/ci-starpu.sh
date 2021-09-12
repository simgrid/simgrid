#!/usr/bin/env sh
set -e

export SUDO=""

# Update refs, just in case
$SUDO apt-get update

# Install basic tools
$SUDO apt-get -y install build-essential
$SUDO apt-get -y install libboost-all-dev
$SUDO apt-get -y install wget
$SUDO apt-get -y install git

for i in master 1.3 ; do
  rm -rf starpu*
  wget https://files.inria.fr/starpu/simgrid/starpu-simgrid-$i.tar.gz
  md5sum starpu-simgrid-$i.tar.gz
  tar xf starpu-simgrid-$i.tar.gz
  cd starpu-1*

  # NOTE: Do *not* introduce parameters to "make it work" here.
  # Things should "just work" with default parameters!
  # Users should not have to tinker to get starpu working on top of simgrid, that is precisely why we have this CI

  if [ $i = master ]; then
    # On master, fail if we use deprecated functions, so that StarPU people know they have to stop using them, fix it, and thus make CI happy again
    CFLAGS="-Werror=deprecated-declarations"
    CXXFLAGS="-Werror=deprecated-declarations"
  else
    CFLAGS=""
    CXXFLAGS=""
  fi
  if ! ./configure CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" \
                   --enable-simgrid --disable-shared --enable-mpi-check --disable-cuda \
                   --disable-build-doc --enable-quick-check
  then
    cat ./config.log
    false
  fi
  make -j 2 V=1

  for STARPU_SCHED in eager dmdas ; do
    export STARPU_SCHED
    if ! make check -k
    then
      make showsuite
      make showfailed
      false
    fi
  done
  cd ..
done
