#! /bin/sh

# Copyright (c) 2007-2014. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.


#usage to print the way this script should be called
usage () {
    cat <<EOF
Usage: $0 [OPTIONS] -platform <xmldesc> -hostfile <hostfile> replay-file output-file
EOF
}

#check if we have at least one parameter
if [ $# -eq 0 ]
then
    usage
    exit
fi

EXTOPT=""
WRAPPER=""
HOSTFILE=""

while true; do
    case "$1" in
        "-platform")
	    PLATFORM="$2"
            if [ ! -f "${PLATFORM}" ]; then
		echo "[`basename $0`] ** error: the file '${PLATFORM}' does not exist. Aborting."
		exit 1
            fi
	    shift 2
            ;;
        "-hostfile")
	    HOSTFILE="$2"
            if [ ! -f "${HOSTFILE}" ]; then
		echo "[`basename $0`] ** error: the file '${HOSTFILE}' does not exist. Aborting."
		exit 1
            fi
	    shift 2
            ;;
        "-machinefile")
	    HOSTFILE="$2"
            if [ ! -f "${HOSTFILE}" ]; then
		echo "[`basename $0`] ** error: the file '${HOSTFILE}' does not exist. Aborting."
		exit 1
            fi
	    shift 2
            ;;
        *)
            break
            ;;
    esac
done

# steel --cfg and --logs options
while [ $# -gt 0 ]; do
    case "$1" in
        "--cfg="*|"--log="*)
            for OPT in ${1#*=}
            do
                SIMOPTS="$SIMOPTS ${1%%=*}=$OPT"
            done
            shift 1
            ;;
        *)
            PROC_ARGS="${PROC_ARGS:+$PROC_ARGS }$1"
            shift      
            ;;
    esac
done


##-----------------------------------


if [ -z "${HOSTFILE}" ] && [ -z "${PLATFORM}" ] ; then
    echo "No hostfile nor platform specified."
    usage
    exit 1
fi

HOSTFILETMP=0
if [ -z "${HOSTFILE}" ] ; then
    HOSTFILETMP=1
    HOSTFILE="$(mktemp tmphostXXXXXX)"
    perl -ne 'print "$1\n" if /.*<host.*?id="(.*?)".*?\/>.*/' ${PLATFORM} > ${HOSTFILE}
fi
UNROLLEDHOSTFILETMP=0

#parse if our lines are terminated by :num_process
multiple_processes=`grep -c ":" $HOSTFILE`
if [ "${multiple_processes}" -gt 0 ] ; then
    UNROLLEDHOSTFILETMP=1
    UNROLLEDHOSTFILE="$(mktemp tmphostXXXXXX)"
    perl -ne ' do{ for ( 1 .. $2 ) { print "$1\n" } } if /(.*?):(\d+).*/'  ${HOSTFILE}  > ${UNROLLEDHOSTFILE}
    if [ ${HOSTFILETMP} = 1 ] ; then
        rm ${HOSTFILE}
        HOSTFILETMP=0
    fi
    HOSTFILE=$UNROLLEDHOSTFILE
fi


# Don't use wc -l to compute it to avoid issues with trailing \n at EOF
hostfile_procs=`grep -c "[a-zA-Z0-9]" $HOSTFILE`
if [ ${hostfile_procs} = 0 ] ; then
   echo "[`basename $0`] ** error: the hostfile '${HOSTFILE}' is empty. Aborting." >&2
   exit 1
fi

APP_TRACES=($PROC_ARGS)
##-------------------------------- DEFAULT APPLICATION --------------------------------------

APPLICATIONTMP=${APP_TRACES[1]}

cat > ${APPLICATIONTMP} <<APPLICATIONHEAD
<?xml version='1.0'?>
<!DOCTYPE platform SYSTEM "http://simgrid.gforge.inria.fr/simgrid.dtd">
<platform version="3">
APPLICATIONHEAD

##---- cache hostnames of hostfile---------------
if [ -n "${HOSTFILE}" ] && [ -f ${HOSTFILE} ]; then
    hostnames=$(cat ${HOSTFILE} | tr '\n\r' '  ')
    NUMHOSTS=$(cat ${HOSTFILE} | wc -l)
fi


    if [ -n "${APP_TRACES[0]}" ] && [ -f "${APP_TRACES[0]}" ]; then
        NUMINSTANCES=$(cat ${APP_TRACES[0]} | wc -l)
        replayinstances=$(cat ${APP_TRACES[0]})
        IFS_OLD=$IFS
        IFS=$'\n'
        NUMPROCS=0
        for line in $replayinstances; do
        # return IFS back if you need to split new line by spaces:
        IFS=$IFS_OLD
        IFS_OLD=
        array=( $line )
        # generate three lists, one with instance id, ont with instance size, one with files
        instance=${array[0]}
        hosttraces="$hosttraces$(cat ${array[1]}| tr '\n\r' '  ' )"
        NUMPROCSMINE=$(cat ${array[1]} | wc -l)
        
        if [ $NUMPROCSMINE != ${array[2]} ];
        then
          echo "declared num of processes for instance $instance : ${array[2]} is not the same as the one in the replay files : $NUMPROCSMINE. Please check consistency of these information"
          exit 1
        fi
        
        sleeptime=${array[3]}
        HAVE_SEQ="`which seq 2>/dev/null`"

        if [ -n "${HAVE_SEQ}" ]; then
            SEQ1=`${HAVE_SEQ} 0 $((${NUMPROCSMINE}-1))`
        else
            cnt=0
            while (( $cnt < ${NUMPROCSMINE} )) ; do
            SEQ1="$SEQ1 $cnt"
            cnt=$((cnt + 1));
            done
        fi
        NUMPROCS=$((${NUMPROCS}+${NUMPROCSMINE}));
        for i in $SEQ1
        do
          instances="$instances$instance "
          ranks="$ranks$SEQ1 "
          sleeptimes="$sleeptimes$sleeptime "
        done
        # return IFS back to newline for "for" loop
        IFS_OLD=$IFS
        IFS=$'\n'
        done 

        # return delimiter to previous value
        IFS=$IFS_OLD
        IFS_OLD=
    else
        printf "File not found: %s\n", "${APP_TRACES[0]:-\${APP_TRACES[0]\}}" >&2
        exit 1
    fi


##----------------------------------------------------------
##  generate application.xml with hostnames from hostfile:
##  the name of host_i (1<=i<=p, where -np p) is the line i
##  in hostfile (where -hostfile hostfile), or "host$i" if
##  hostfile has less than i lines.
##----------------------------------------------------------

HAVE_SEQ="`which seq 2>/dev/null`"

if [ -n "${HAVE_SEQ}" ]; then
    SEQ=`${HAVE_SEQ} 0 $((${NUMPROCS}-1))`
else
    cnt=0
    while (( $cnt < ${NUMPROCS} )) ; do
	SEQ="$SEQ $cnt"
	cnt=$((cnt + 1));
    done
fi

##---- generate <process> tags------------------------------

hostnames=($hostnames)
instances=($instances)
hosttraces=($hosttraces)
sleeptimes=($sleeptimes)
ranks=($ranks)
for i in ${SEQ}
do
    if [ -n "${HOSTFILE}" ]; then
	j=$(( $i % ${NUMHOSTS} ))
    fi
    ##---- optional display of ranks to process mapping
    if [ -n "${MAPOPT}" ]; then
	echo "[rank $i] -> ${hostnames[j]}"
    fi

    if [ -z "${hostnames[j]}" ]; then
	host="host"$($j)
    else
	host="${hostnames[j]}"
    fi
    echo "  <process host=\"${host}\" function=\"${instances[i]}\"> <!-- function name used only for logging -->" >> ${APPLICATIONTMP}
        echo "    <argument value=\"${instances[i]}\"/> <!-- instance -->" >> ${APPLICATIONTMP}
    echo "    <argument value=\"${ranks[i]}\"/> <!-- rank -->" >> ${APPLICATIONTMP}
        if  [ ${NUMPROCS} -gt 1 ]; then
            echo "    <argument value=\"${hosttraces[i]}\"/>" >> ${APPLICATIONTMP}
        else
            echo "    <argument value=\"${hosttraces[i]}\"/>" >> ${APPLICATIONTMP}
        fi
    echo "    <argument value=\"${sleeptimes[i]}\"/> <!-- delay -->" >> ${APPLICATIONTMP}
    echo "  </process>" >> ${APPLICATIONTMP}
done

cat >> ${APPLICATIONTMP} <<APPLICATIONFOOT
</platform>
APPLICATIONFOOT
##-------------------------------- end DEFAULT APPLICATION --------------------------------------


    if [ ${HOSTFILETMP} = 1 ] ; then
        rm ${HOSTFILE}
    fi
    if [ ${UNROLLEDHOSTFILETMP} = 1 ] ; then
        rm ${UNROLLEDHOSTFILE}
    fi


exit 0
