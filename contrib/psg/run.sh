#!/bin/bash

if [ $(uname -m) = "i686" ]; then
	eval ulimit -s 64
else 
	eval ulimit -s 128
fi
echo '------------- Start execution..';
java -Xmx1024m -cp lib.jar:classes peersim.Simulator $1
echo '------------- done -------------';
exit 0




