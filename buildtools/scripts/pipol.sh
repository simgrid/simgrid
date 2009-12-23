#! /bin/sh

pipol-sub esn i386-linux-debian-testing.dd.gz none 02:00 ~/simgrid-svn/buildtools/scripts/make_dist.sh
sleep 200

pipol-sub esn i386-linux-ubuntu-intrepid.dd.gz none 02:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh
pipol-sub esn i386-linux-ubuntu-jaunty.dd.gz none 02:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh
pipol-sub esn i386-linux-ubuntu-karmic.dd.gz none 02:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh
pipol-sub esn i386-linux-debian-testing.dd.gz none 02:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh

pipol-sub esn amd64-linux-ubuntu-intrepid.dd.gz none 02:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh
pipol-sub esn amd64-linux-ubuntu-jaunty.dd.gz none 02:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh
pipol-sub esn amd64-linux-ubuntu-karmic.dd.gz none 02:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh
pipol-sub esn amd64-linux-debian-testing.dd.gz none 02:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh

pipol-sub esn 386_mac-mac-osx-server-leopard.dd.gz none 02:00 ~/simgrid-svn/buildtools/scripts/test_dist_with_cmake.sh

