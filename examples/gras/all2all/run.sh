#! /bin/bash

#
# USAGE: run.sh  plaform  nb_host (broadcast source?)
#
# This script takes a platform file and a number of hosts as argument.
#  if a third argument is passed, this is the source of the broadcast 
#  (given as a number between 0 and nb_host-1).
#
# It generates the right deployment platform and run the experiment, 
#  only showing the last line of the run, showing the resulting time.

plat=$1
nb_host=$2
bcast=$3
set -e

if [ -z $plat -o -z $nb_host ] ; then
  # invalid argument. Display the comment at the script begining & exit
  grep '^#\(\([^!]\)\|$\)' $0 | sed 's/# *//' >&2
  exit 1
fi
if ! [ -e $plat ] ; then
  echo "Platform file not found" >&2
  exit 1
fi

echo "Generating the deployment"
./make_deployment.pl $plat $nb_host $bcast > tmp_deployment_$nb_host
echo "Running the experiment"
./all2all_simulator $plat tmp_deployment_$nb_host 2>&1 |tee run.log|grep "Congrat"
rm tmp_deployment_$nb_host
