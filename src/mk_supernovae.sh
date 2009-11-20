#! /bin/sh

file=$1
shift

echo "#define SUPERNOVAE_MODE 1" > $file
echo '#include <ctype.h>' >> $file
echo '#include "portable.h"' >> $file
echo '#include "xbt.h"' >> $file
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