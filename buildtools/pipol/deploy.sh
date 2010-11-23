#!/bin/bash

ssh pipol pipol-sub esn i386-linux-ubuntu-intrepid.dd.gz none 02:00 "~/Experimental_all_simgrid_gt.sh"
ssh pipol pipol-sub esn amd64-linux-ubuntu-intrepid.dd.gz none 02:00 "~/Experimental_all_simgrid_gt.sh"

ssh pipol pipol-sub esn i386-linux-ubuntu-jaunty.dd.gz none 02:00 "~/Experimental_all_simgrid_gt.sh"
ssh pipol pipol-sub esn amd64-linux-ubuntu-jaunty.dd.gz none 02:00 "~/Experimental_all_simgrid_gt.sh"

ssh pipol pipol-sub esn i386-linux-ubuntu-karmic.dd.gz none 02:00 "~/Experimental_all_simgrid_gt.sh"
ssh pipol pipol-sub esn amd64-linux-ubuntu-karmic.dd.gz none 02:00 "~/Experimental_all_simgrid_gt.sh"

ssh pipol pipol-sub esn i386-linux-debian-lenny.dd none 02:00 "~/Experimental_all_simgrid_gt.sh"
ssh pipol pipol-sub esn amd64-linux-debian-lenny.dd none 02:00 "~/Experimental_all_simgrid_gt.sh"

ssh pipol pipol-sub esn i386-linux-fedora-core11.dd.gz none 02:00 "~/Experimental_all_simgrid_gt.sh"
ssh pipol pipol-sub esn amd64-linux-fedora-core11.dd.gz none 02:00 "~/Experimental_all_simgrid_gt.sh"
