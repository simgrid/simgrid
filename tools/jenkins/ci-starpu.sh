#!/usr/bin/env sh

# Test this script locally as follows (rerun `docker pull simgrid/unstable` to get a fresh version).
# cd (simgrid)/tools/jenkins
# docker run -it --rm --volume `pwd`:/source simgrid/unstable /source/ci-starpu.sh

set -ex

export SUDO=""

echo "XXXXXXXXXXXXXXXX Install APT dependencies"
$SUDO apt-get update
$SUDO apt-get -y install build-essential libboost-all-dev wget git xsltproc python3-setuptools

for i in starpu-1.4 master ; do
  echo "XXXXXXXXXXXXXXXX Build and test StarPU $i"
  rm -rf starpu*
  wget https://files.inria.fr/starpu/testing/$i/starpu-nightly-latest.tar.gz
  md5sum starpu-nightly-latest.tar.gz
  tar xf starpu-nightly-latest.tar.gz
  cd starpu-1*

  # NOTE: Do *not* introduce parameters to "make it work" here.
  # Things should "just work" with default parameters!
  # Users should not have to tinker to get starpu working on top of SimGrid, that is precisely why we have this CI

  if [ $i = master ]; then
    # On master, fail if we use deprecated functions, so that StarPU people know they have to stop using them, fix it, and thus make CI happy again
    CFLAGS="-Werror=deprecated-declarations"
    CXXFLAGS="-Werror=deprecated-declarations"
  else
    # Check that released versions still go fine with just a few warnings at worse.
    CFLAGS=""
    CXXFLAGS=""
  fi
  if ! ./configure CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" \
                   --enable-simgrid --disable-shared --enable-mpi --enable-mpi-check --disable-cuda \
                   --disable-build-doc --enable-quick-check
  then
    cat ./config.log
    false
  fi
  make -j$(nproc) V=1

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
