#!/bin/bash

if [ $(uname -m) = "i686" ]; then
	eval ulimit -s 64
else 
	eval ulimit -s 128
fi

echo -e "\n";
echo '------------- Execute the edaggregation example under PSG -------------';
echo -e "\n";
java -Xmx1024m -cp lib.jar:classes:../../simgrid.jar peersim.Simulator configs/edaggregationPSG.txt
echo -e "\n";
echo '------------- Execute the edaggregation example under PS -------------';
echo -e "\n";
java -Xmx1024m -cp lib.jar:classes:../../simgrid.jar peersim.Simulator configs/edaggregation.txt
echo -e "\n";
echo '------------- Execute the chord example under PSG -------------';
echo -e "\n";
java -Xmx1024m -cp lib.jar:classes:../../simgrid.jar peersim.Simulator configs/chordPSG.txt
echo -e "\n";
echo '------------- Execute the chord example under PS -------------';
echo -e "\n";
java -Xmx1024m -cp lib.jar:classes:../../simgrid.jar peersim.Simulator configs/chord.txt
echo -e "\n";
echo '------------- Compare the 2 results PS and PSG -------------';
echo -e "\n";

cd outputs

ListeRep="$(find * -type d -prune)"   # liste des repertoires
for Rep in ${ListeRep}; do	
	cd $Rep
	VAR=$(diff ps.txt psg.txt)
	if [ "${VAR}"1 = 1 ]
		then
  		 echo The results of diff "for" the $Rep example is '.............:)';
	else
  		 echo The results of diff "for" the $Rep example is '.............:(';
	fi
	cd ..
done
echo -e "\n";
exit 0

