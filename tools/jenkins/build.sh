#!/usr/bin/env sh

# This script is used by various build projects on Jenkins

# See https://ci.inria.fr/simgrid/job/SimGrid/configure
# See https://ci.inria.fr/simgrid/job/Simgrid-Windows/configure

# ensure that the locales are set, so that perl keeps its nerves
export LC_ALL=C

echo "XXXX Cleanup previous attempts. Remaining content of /tmp:"
rm -f /tmp/cc*
rm -f /tmp/*.so
rm -f /tmp/*.so.*
rm -rf /tmp/simgrid-java*
rm -rf /var/tmp/simgrid-java*
rm -rf /tmp/jvm-*
find $WORKSPACE -name "hs_err_pid*.log" -exec rm -f {} +
ls /tmp
df -h
echo "XXXX Let's go"

set -e

# Help older cmakes
if [ -e /usr/lib/jvm/java-7-openjdk-amd64 ] ;
then
  export JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64
fi

# usage: die status message...
die () {
  status=${1:-1}
  shift
  [ $# -gt 0 ] || set -- "Error - Halting"
  echo "$@" >&2
  exit "$status"
}

# Get an ON/OFF string from a command:
onoff() {
  if "$@" > /dev/null ; then
    echo ON
  else
    echo OFF
  fi
}

if type lsb_release >/dev/null 2>&1; then # recent versions of Debian/Ubuntu
    # linuxbase.org
    os=$(lsb_release -si)
    ver="$(lsb_release -sr) ($(lsb_release -sc))"
elif [ -f /etc/lsb-release ]; then # For some versions of Debian/Ubuntu without lsb_release command
    . /etc/lsb-release
    os=$DISTRIB_ID
    ver=$DISTRIB_RELEASE
elif [ -f /etc/debian_version ]; then # Older Debian/Ubuntu/etc.
    os=Debian
    ver=$(cat /etc/debian_version)
elif [ -f /etc/redhat-release ]; then #RH, Fedora, Centos
    read -r os ver < /etc/redhat-release
elif [ -f /usr/bin/sw_vers ]; then #osx
    os=$(sw_vers -productName)
    ver=$(sw_vers -productVersion)
elif [ -f /bin/freebsd-version ]; then #freebsd
    os=$(uname -s)
    ver=$(freebsd-version -u)
elif [ -f /etc/release ]; then #openindiana
    read -r os ver < /etc/release
elif [ -f /etc/os-release ]; then # freedesktop.org and systemd, put last as usually missing useful info
    . /etc/os-release
    os=$NAME
    ver=$VERSION_ID
else
    # Fall back to uname, e.g. "Linux <version>", also works for BSD, etc.
    echo "fallback as OS name not found"
    os=$(uname -s)
    ver=$(uname -r)
fi

# Are we running on wsl ?
if [ -f /mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe ]; then
    #To identify the windows underneath the linux
    PATH="/mnt/c/Windows/System32/WindowsPowerShell/v1.0/:$PATH"
    major=$(powershell.exe -command "[environment]::OSVersion.Version.Major" | sed 's/\r//g')
    build=$(powershell.exe -command "[environment]::OSVersion.Version.Build"| sed 's/\r//g')
    ver="$major v$build - WSL $os $ver"
    os=Windows
fi

case $(uname -m) in
x86_64)
    bits="64 bits"
    ;;
i*86)
    bits="32 bits"
    ;;
*)
    bits=""
    ;;
esac
echo "OS Version : $os $ver $bits"


build_mode="$1"
echo "Build mode $build_mode on $(uname -np)" >&2
case "$build_mode" in
  "Debug")
      INSTALL="$HOME/simgrid_install"
  ;;

  "ModelChecker")
      INSTALL="$HOME/mc_simgrid_install"
  ;;

  "DynamicAnalysis")
      INSTALL=""
  ;;

  *)
    die 1 "Unknown build_mode $build_mode"
  ;;
esac

if [ "$2" = "" ]; then
  branch_name="unknown"
else
  branch_name="$2"
fi
echo "Branch built is $branch_name"

NUMBER_OF_PROCESSORS="$(nproc)" || NUMBER_OF_PROCESSORS=1
GENERATOR="Unix Makefiles"

ulimit -c 0 || true

echo "XX"
echo "XX Get out of the tree"
echo "XX"
if [ -d "$WORKSPACE"/build ]
then
  # Windows cannot remove the directory if it's still used by the previous build
  rm -rf "$WORKSPACE"/build || sleep 10 && rm -rf "$WORKSPACE"/build || sleep 10 && rm -rf "$WORKSPACE"/build
fi
mkdir "$WORKSPACE"/build
cd "$WORKSPACE"/build

have_NS3="no"
if [ "$os" = "Debian" ] || [ "$os" = "Ubuntu" ] ; then
  if dpkg --compare-versions "$(dpkg-query -f '${Version}' -W libns3-dev)" ge 3.28; then
    have_NS3="yes"
  fi
fi
if [ "$os" = "NixOS" ] ; then
  have_NS3="yes"
fi
echo "XX have_NS3: ${have_NS3}"

# This is for Windows:
PATH="$WORKSPACE/build/lib:$PATH"

echo "XX"
echo "XX Build the archive out of the tree"
echo "XX   pwd: $(pwd)"
echo "XX"

cmake -G"$GENERATOR" -Denable_documentation=OFF "$WORKSPACE"
make dist -j $NUMBER_OF_PROCESSORS
SIMGRID_VERSION=$(cat VERSION)

echo "XX"
echo "XX Open the resulting archive"
echo "XX"
gunzip "${SIMGRID_VERSION}".tar.gz
tar xf "${SIMGRID_VERSION}".tar
mkdir "${WORKSPACE}"/build/"${SIMGRID_VERSION}"/build
cd "${WORKSPACE}"/build/"${SIMGRID_VERSION}"/build
SRCFOLDER="${WORKSPACE}/build/${SIMGRID_VERSION}"

echo "XX"
echo "XX Configure and build SimGrid"
echo "XX   pwd: $(pwd)"
echo "XX"
set -x

if cmake --version | grep -q 3\.11 ; then
  # -DCMAKE_DISABLE_SOURCE_CHANGES=ON is broken with java on CMake 3.11
  # https://gitlab.kitware.com/cmake/cmake/issues/17933
  MAY_DISABLE_SOURCE_CHANGE=""
else
  MAY_DISABLE_SOURCE_CHANGE="-DCMAKE_DISABLE_SOURCE_CHANGES=ON"
fi

if [ "$os" = "CentOS" ] && [ "$(ld -v | cut -d\  -f4 | cut -c1-4)" = "2.30" ]; then
  echo "Temporary disable LTO, believed to be broken on this system."
  MAY_DISABLE_LTO=-Denable_lto=OFF
else
  MAY_DISABLE_LTO=
fi

cmake -G"$GENERATOR" ${INSTALL:+-DCMAKE_INSTALL_PREFIX=$INSTALL} \
  -Denable_debug=ON -Denable_documentation=OFF -Denable_coverage=OFF \
  -Denable_model-checking=$(onoff test "$build_mode" = "ModelChecker") \
  -Denable_smpi_ISP_testsuite=$(onoff test "$build_mode" = "ModelChecker") \
  -Denable_compile_optimizations=$(onoff test "$build_mode" != "DynamicAnalysis") \
  -Denable_smpi_MPICH3_testsuite=$(onoff test "$build_mode" = "Debug") \
  -Denable_mallocators=$(onoff test "$build_mode" != "DynamicAnalysis") \
  -Denable_memcheck=$(onoff test "$build_mode" = "DynamicAnalysis") \
  -Denable_compile_warnings=$(onoff test "$GENERATOR" != "MSYS Makefiles") -Denable_smpi=ON \
  -Denable_ns3=$(onoff test "$have_NS3" = "yes" -a "$build_mode" = "Debug") \
  ${MAY_DISABLE_SOURCE_CHANGE} ${MAY_DISABLE_LTO} \
  -Denable_java=$(onoff test "$build_mode" = "ModelChecker") \
  -Denable_msg=$(onoff test "$build_mode" = "ModelChecker") \
  -DLTO_EXTRA_FLAG="auto" \
  "$SRCFOLDER"
set +x

make -j $NUMBER_OF_PROCESSORS VERBOSE=1 tests

echo "XX"
echo "XX Run the tests"
echo "XX   pwd: "$(pwd)
echo "XX"

ctest -T test --output-on-failure --no-compress-output -j $NUMBER_OF_PROCESSORS || true

if test -n "$INSTALL" && [ "${branch_name}" = "origin/master" ] ; then
  echo "XX"
  echo "XX Test done. Install everything since it's a regular build, not on a Windows."
  echo "XX"

  rm -rf "$INSTALL"

  make install
fi

echo "XX"
echo "XX Done. Return the results to cmake"
echo "XX"
