#!/bin/bash

rm consoleText
wget –quiet https://ci.inria.fr/simgrid/job/SimGrid/lastBuild/consoleText >/dev/null 2>&1
nodes=($(grep -rR "Triggering SimGrid ? Debug," ./consoleText | sed "s/Triggering SimGrid ? Debug,\(.*\)/\1/g"))
nodes=($(for i in "${nodes[@]}"; do echo "$i"; done | sort))
rm consoleText
echo "<br>Info on configurations<br><br>
<table id="configuration-matrix"> 
<tr class="matrix-row">  <td class="matrix-header">Name of the Builder</td><td class="matrix-header">OS version</td><td class="matrix-header">Compiler name and version</td><td class="matrix-header">Boost version</td><td class="matrix-header">Java version</td><td class="matrix-header">Cmake version</td></tr>"

for node in "${nodes[@]}"
do
    wget –quiet https://ci.inria.fr/simgrid/job/SimGrid/lastBuild/build_mode=Debug,node=${node}/consoleText >/dev/null 2>&1
    if [ ! -f consoleText ]; then
	  echo "file not existing for node ${node}"
	  exit 1
    fi
    boost=$(grep -m 1 "Boost version:" ./consoleText | sed  "s/-- Boost version: \(.*\)/\1/g")
    compiler=$(grep -m 1 "The C compiler identification" ./consoleText | sed  "s/-- The C compiler identification is \(.*\)/\1/g")
	java=$(grep -m 1 "Found Java:" ./consoleText | sed "s/-- Found Java.*found suitable version \"\(.*\)\",.*/\1/g")
	cmake=$(grep -m 1 "Cmake version" ./consoleText| sed "s/-- Cmake version \(.*\)/\1/g")
	os=$(grep -m 1 "OS Version" ./consoleText| sed "s/OS Version : \(.*\)/\1/g")
    echo "<tr> <td class=\"matrix-leftcolumn\">$node</td><td class=\"matrix-cell\" style=\"text-align:left\">$os</td><td class=\"matrix-cell\" style=\"text-align:left\">$compiler</td><td class=\"matrix-cell\" style=\"text-align:left\">$boost</td><td class=\"matrix-cell\" style=\"text-align:left\">$java</td><td class=\"matrix-cell\" style=\"text-align:left\">$cmake</td></tr>"
    rm consoleText
done
echo "<tr> <td class="matrix-leftcolumn"><a href="https://travis-ci.org/simgrid/simgrid">travis-linux</a></td><td class="matrix-cell" style="text-align:left">Ubuntu 14.04 (<a href="https://docs.travis-ci.com/user/reference/trusty/">Trusty</a>) 64 bits</td><td class="matrix-cell" style="text-align:left">GNU 4.8.4</td><td class="matrix-cell" style="text-align:left">1.60.0</td><td class="matrix-cell" style="text-align:left">1.8.0.151</td><td class="matrix-cell" style="text-align:left">3.9.2</td></tr>
<tr> <td class="matrix-leftcolumn"><a href="https://travis-ci.org/simgrid/simgrid">travis-mac</a></td><td class="matrix-cell" style="text-align:left">Mac OSX Sierra (kernel: 16.7.0)</td><td class="matrix-cell" style="text-align:left">AppleClang 8.1.0.8020042</td><td class="matrix-cell" style="text-align:left">1.65.1</td><td class="matrix-cell" style="text-align:left">1.8.0.112</td><td class="matrix-cell" style="text-align:left">3.9.4</td></tr>
<tr> <td class="matrix-leftcolumn"><a href="https://ci.appveyor.com/project/mquinson/simgrid">appveyor</a></td><td class="matrix-cell" style="text-align:left">Windows Server 2012 - VS2015 + mingw64 5.3.0</td><td class="matrix-cell" style="text-align:left">GNU 5.3.0</td><td class="matrix-cell" style="text-align:left">1.60.0</td><td class="matrix-cell" style="text-align:left">1.8.0.162</td><td class="matrix-cell" style="text-align:left">3.11.3</td></tr>
</table>"
