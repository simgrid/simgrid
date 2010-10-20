#! /bin/sh

file=$1
shift

cat > $file <<EOF
#define SUPERNOVAE_MODE 1
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE   /* for getline() with older libc */
#endif
#include <ctype.h>
#include "portable.h"
#include "xbt.h"
EOF
for n in $@ ; do
#    echo "File $n"
    if [ "X$n" = 'Xxbt/log.c' ] ; then 
      echo "#define _simgrid_log_category__default &_simgrid_log_category__log">> $file;
    else 
      if grep -q 'XBT_LOG_[^ ]*DEFAULT_[^ ]*CATEGORY' $n ; then 
        default=`grep 'XBT_LOG_[^ ]*DEFAULT_[^ ]*CATEGORY' $n|
                 sed 's/^[^(]*(\([^,)]*\).*$/\1/'|head -n1`; 
        echo "#define _simgrid_log_category__default &_simgrid_log_category__$default">> $file;
      else
        echo "/* no default category in file $n */" >> $file; 
      fi;
    fi;
    echo '  #include "'$n'"' >> $file ; 
    echo "#undef _simgrid_log_category__default">> $file; 
    echo >> $file; 
done

exit 0