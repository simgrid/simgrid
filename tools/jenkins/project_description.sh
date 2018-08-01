#!/bin/bash

get_boost(){
    grep -m 1 "Boost version:" ./consoleText | sed  "s/.*-- Boost version: \([a-zA-Z0-9\.]*\)/\1/g"
}

get_compiler(){
    grep -m 1 "The C compiler identification" ./consoleText | sed  "s/.*-- The C compiler identification is \([a-zA-Z0-9\.]*\)/\1/g"
}

get_java(){
    grep -m 1 "Found Java:" ./consoleText | sed "s/.*-- Found Java.*found suitable version \"\([a-zA-Z0-9\.]*\)\",.*/\1/g"
}

get_cmake(){
    grep -m 1 "Cmake version" ./consoleText| sed "s/.*-- Cmake version \([a-zA-Z0-9\.]*\)/\1/g"
}

if [ -f consoleText ]; then
  rm consoleText
fi

#get the list of nodes on jenkins
wget –quiet https://ci.inria.fr/simgrid/job/SimGrid/lastBuild/consoleText >/dev/null 2>&1
nodes=($(grep -rR "Triggering SimGrid ? Debug," ./consoleText | sed "s/Triggering SimGrid ? Debug,\(.*\)/\1/g"| sort))
rm consoleText


echo "<br>Description of the nodes - Automatically updated by project_description.sh script - Don't edit here<br><br>
<table id="configuration-matrix"> 
<tr class="matrix-row">  <td class="matrix-header">Name of the Builder</td><td class="matrix-header">OS version</td><td class="matrix-header">Compiler name and version</td><td class="matrix-header">Boost version</td><td class="matrix-header">Java version</td><td class="matrix-header">Cmake version</td></tr>"

for node in "${nodes[@]}"
do
    wget –quiet https://ci.inria.fr/simgrid/job/SimGrid/lastBuild/build_mode=Debug,node=${node}/consoleText >/dev/null 2>&1
    if [ ! -f consoleText ]; then
      echo "file not existing for node ${node}"
      exit 1
    fi
    boost=$(get_boost)
    compiler=$(get_compiler)
    java=$(get_java)
    cmake=$(get_cmake)
    os=$(grep -m 1 "OS Version" ./consoleText| sed "s/OS Version : \(.*\)/\1/g")
    echo "<tr> <td class=\"matrix-leftcolumn\">$node</td><td class=\"matrix-cell\" style=\"text-align:left\">$os</td><td class=\"matrix-cell\" style=\"text-align:left\">$compiler</td><td class=\"matrix-cell\" style=\"text-align:left\">$boost</td><td class=\"matrix-cell\" style=\"text-align:left\">$java</td><td class=\"matrix-cell\" style=\"text-align:left\">$cmake</td></tr>"
    rm consoleText
done


#Travis - get ID of the last jobs with the API
BUILD_NUM=$(curl -s 'https://api.travis-ci.org/repos/simgrid/simgrid/builds?limit=1' | grep -o '^\[{"id":[0-9]*,' | grep -o '[0-9]' | tr -d '\n')
BUILDS=($(curl -s https://api.travis-ci.org/repos/simgrid/simgrid/builds/${BUILD_NUM} | grep -o '{"id":[0-9]*,' | grep -o '[0-9]*'| tail -n 2))

for id in "${!BUILDS[@]}"
do
    wget –quiet https://api.travis-ci.org/v3/job/${BUILDS[$id]}/log.txt -O ./consoleText >/dev/null 2>&1 
    sed -i -e "s/\r//g" ./consoleText
    if [ $id == 0 ]; then
      node="<a href=\"https://travis-ci.org/simgrid/simgrid\">travis-linux</a>"
      os="Ubuntu 14.04 (<a href=\"https://docs.travis-ci.com/user/reference/trusty/\">Trusty</a>) 64 bits"
    else
      node="<a href=\"https://travis-ci.org/simgrid/simgrid\">travis-mac</a>"
      os="Mac OSX Sierra (kernel: 16.7.0)"
    fi
    boost=$(get_boost)
    compiler=$(get_compiler)
    java=$(get_java)
    cmake=$(get_cmake)
    
    echo "<tr> <td class=\"matrix-leftcolumn\">$node</td><td class=\"matrix-cell\" style=\"text-align:left\">$os</td><td class=\"matrix-cell\" style=\"text-align:left\">$compiler</td><td class=\"matrix-cell\" style=\"text-align:left\">$boost</td><td class=\"matrix-cell\" style=\"text-align:left\">$java</td><td class=\"matrix-cell\" style=\"text-align:left\">$cmake</td></tr>"
    rm consoleText
done

#Appveyor - get ID of the last job with the API
BUILD_ID=$(curl -s "https://ci.appveyor.com/api/projects/mquinson/simgrid" | grep -o '\[{"jobId":"[a-zA-Z0-9]*",' | sed "s/\[{\"jobId\":\"//" | sed "s/\",//")
wget –quiet https://ci.appveyor.com/api/buildjobs/$BUILD_ID/log -O ./consoleText >/dev/null 2>&1 
sed -i -e "s/\r//g" ./consoleText
node="<a href="https://ci.appveyor.com/project/mquinson/simgrid">appveyor</a>"
os="Windows Server 2012 - VS2015 + mingw64 5.3.0"
boost=$(get_boost)
compiler=$(get_compiler)
java=$(get_java)
cmake=$(get_cmake)

echo "<tr> <td class=\"matrix-leftcolumn\">$node</td><td class=\"matrix-cell\" style=\"text-align:left\">$os</td><td class=\"matrix-cell\" style=\"text-align:left\">$compiler</td><td class=\"matrix-cell\" style=\"text-align:left\">$boost</td><td class=\"matrix-cell\" style=\"text-align:left\">$java</td><td class=\"matrix-cell\" style=\"text-align:left\">$cmake</td></tr>"
rm consoleText

echo "</table>"
