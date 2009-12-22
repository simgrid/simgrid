#! /bin/sh

ssh pipol pipol-sub esn i386-linux-ubuntu-karmic.dd.gz none 02:00 /home/mescal/navarro/script/DISTRIB.sh

ssh pipol pipol-sub esn i386-linux-ubuntu-jaunty.dd.gz none 02:00 /home/mescal/navarro/script/WAIT.sh
ssh pipol pipol-sub esn i386-linux-ubuntu-karmic.dd.gz none 02:00 /home/mescal/navarro/script/WAIT.sh
