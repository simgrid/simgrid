#! /bin/sh

pipol-sub esn i386-linux-debian-testing.dd.gz none 01:00 ~/simgrid-svn/buildtools/scripts/make_dist.sh
sleep 300

pipol-sub esn i386-linux-ubuntu-intrepid.dd.gz none 01:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh
pipol-sub esn i386-linux-ubuntu-jaunty.dd.gz none 01:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh
pipol-sub esn i386-linux-ubuntu-karmic.dd.gz none 01:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh
pipol-sub esn i386-linux-debian-testing.dd.gz none 01:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh
pipol-sub esn i386-linux-fedora-core11.dd.gz none 01:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh

pipol-sub esn ia64-linux-debian-lenny.dd.gz none 01:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh

pipol-sub esn amd64-linux-ubuntu-intrepid.dd.gz none 01:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh
pipol-sub esn amd64-linux-ubuntu-jaunty.dd.gz none 01:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh
pipol-sub esn amd64-linux-ubuntu-karmic.dd.gz none 01:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh
pipol-sub esn amd64-linux-debian-testing.dd.gz none 01:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh
pipol-sub esn amd64-linux-fedora-core11.dd.gz none 01:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh

pipol-sub esn i386_mac-mac-osx-server-leopard.dd.gz none 02:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh

