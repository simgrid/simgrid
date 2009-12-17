#! /bin/sh

ssh pipol pipol-sub esn i386-linux-ubuntu-jaunty.dd.gz none 02:00 /home/mescal/navarro/script/verif.sh
ssh pipol pipol-sub esn i386-linux-ubuntu-karmic.dd.gz none 02:00 /home/mescal/navarro/script/verif.sh
ssh pipol pipol-sub esn amd64-linux-ubuntu-jaunty.dd.gz none 02:00 /home/mescal/navarro/script/verif.sh
ssh pipol pipol-sub esn amd64-linux-ubuntu-karmic.dd.gz none 02:00 /home/mescal/navarro/script/verif.sh

ssh pipol pipol-sub esn i386-linux-debian-etch.dd.gz none 02:00 /home/mescal/navarro/script/verif.sh
ssh pipol pipol-sub esn i386-linux-debian-lenny.dd.gz none 02:00 /home/mescal/navarro/script/verif.sh
ssh pipol pipol-sub esn amd64-linux-debian-etch.dd.gz none 02:00 /home/mescal/navarro/script/verif.sh
ssh pipol pipol-sub esn amd64-linux-debian-lenny.dd.gz none 02:00 /home/mescal/navarro/script/verif.sh

ssh pipol pipol-sub esn i386-linux-fedora-core11.dd.gz none 02:00 /home/mescal/navarro/script/verif.sh
ssh pipol pipol-sub esn amd64-linux-fedora-core11.dd.gz none 02:00 /home/mescal/navarro/script/verif.sh
ssh pipol pipol-sub esn i386-linux-mandriva-2009_powerpack.dd.gz none 02:00 /home/mescal/navarro/script/verif.sh
ssh pipol pipol-sub esn amd64-linux-mandriva-2009_powerpack.dd.gz none 02:00 /home/mescal/navarro/script/verif.sh

ssh pipol pipol-sub esn i386_mac-mac-osx-server-leopard.dd.gz none 02:00 /home/mescal/navarro/script/verif.sh
