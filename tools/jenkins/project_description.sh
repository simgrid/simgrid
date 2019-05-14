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

get_ns3(){
  found=$(grep -c "NS-3 found" ./consoleText)
  if [ $found != 0 ]; then
    echo "✔"
  else
    echo ""
  fi
}

get_python(){
  found=$(grep -c "Compile Python bindings .....: ON" ./consoleText)
  if [ $found != 0 ]; then
    echo "✔"
  else
    echo ""
  fi
}

if [ -f consoleText ]; then
  rm consoleText
fi


if [ -z $BUILD_URL ]; then
  BUILD_URL="https://ci.inria.fr/simgrid/job/SimGrid/lastBuild"
fi

#get the list of nodes on jenkins
wget --quiet ${BUILD_URL}/consoleText >/dev/null 2>&1
nodes=($(grep -rR "Triggering SimGrid ? Debug," ./consoleText | sed "s/Triggering SimGrid ? Debug,\(.*\)/\1/g"| sort))
rm consoleText


echo "<br>Description of the nodes - Automatically updated by project_description.sh script - Don't edit here<br><br>
<script>
function compareVersion(v1, v2) {
    if (typeof v1 !== 'string') return false;
    if (typeof v2 !== 'string') return false;
    v1 = v1.split('.');
    v2 = v2.split('.');
    const k = Math.min(v1.length, v2.length);
    for (let i = 0; i < k; ++ i) {
        v1[i] = parseInt(v1[i], 10);
        v2[i] = parseInt(v2[i], 10);
        if (v1[i] > v2[i]) return 1;
        if (v1[i] < v2[i]) return -1;        
    }
    return v1.length == v2.length ? 0: (v1.length < v2.length ? -1 : 1);
}</script>
<script>
function sortTable(n, type) {
  var table, rows, switching, i, x, y, shouldSwitch, dir, switchcount = 0;
  table = document.getElementById("configuration-matrix");
  switching = true;
  //Set the sorting direction to ascending:
  dir = "asc"; 
  /*Make a loop that will continue until
  no switching has been done:*/
  while (switching) {
    //start by saying: no switching is done:
    switching = false;
    rows = table.rows;
    /*Loop through all table rows (except the
    first, which contains table headers):*/
    for (i = 1; i < (rows.length - 1); i++) {
      //start by saying there should be no switching:
      shouldSwitch = false;
      /*Get the two elements you want to compare,
      one from current row and one from the next:*/
      x = rows[i].getElementsByTagName("TD")[n];
      y = rows[i + 1].getElementsByTagName("TD")[n];
      /*check if the two rows should switch place,
      based on the direction, asc or desc:*/
      if (dir == "asc") {
        if(type == "version"){
          shouldSwitch = (compareVersion(x.innerHTML.toLowerCase(), y.innerHTML.toLowerCase()) > 0);
        }else{
          shouldSwitch = (x.innerHTML.toLowerCase() > y.innerHTML.toLowerCase());
        }
      } else if (dir == "desc") {
        if(type == "version"){
          shouldSwitch = (compareVersion(x.innerHTML.toLowerCase(), y.innerHTML.toLowerCase()) < 0);
        }else{
          shouldSwitch = (x.innerHTML.toLowerCase() < y.innerHTML.toLowerCase());
        }
      }
      if (shouldSwitch)
        break;
    }
    if (shouldSwitch) {
      /*If a switch has been marked, make the switch
      and mark that a switch has been done:*/
      rows[i].parentNode.insertBefore(rows[i + 1], rows[i]);
      switching = true;
      //Each time a switch is done, increase this count by 1:
      switchcount ++;      
    } else {
      /*If no switching has been done AND the direction is "asc",
      set the direction to "desc" and run the while loop again.*/
      if (switchcount == 0 && dir == "asc") {
        dir = "desc";
        switching = true;
      }
    }
  }
}</script>
<table id=configuration-matrix> 
<tr class=matrix-row>  <td class=matrix-header style=min-width:75px onclick='sortTable(0);'>Name of the Builder</td><td class=matrix-header style=min-width:75px onclick='sortTable(1);'>OS</td><td class=matrix-header style=min-width:75px onclick='sortTable(2);'>Compiler</td><td class=matrix-header style=min-width:75px onclick='sortTable(3, 'version');'>Boost</td><td class=matrix-header style=min-width:75px onclick='sortTable(4,'version');'>Java</td><td class=matrix-header style=min-width:75px onclick='sortTable(5,'version');'>Cmake</td><td class=matrix-header style=min-width:50px onclick='sortTable(6);'>NS3</td><td class=matrix-header style=min-width:50px onclick='sortTable(7);'>Python</td></tr>"

for node in "${nodes[@]}"
do
    wget --quiet ${BUILD_URL}/build_mode=Debug,node=${node}/consoleText >/dev/null 2>&1
    if [ ! -f consoleText ]; then
      echo "file not existing for node ${node}"
      exit 1
    fi
    boost=$(get_boost)
    compiler=$(get_compiler)
    java=$(get_java)
    cmake=$(get_cmake)
    ns3=$(get_ns3)
    py=$(get_python)
    os=$(grep -m 1 "OS Version" ./consoleText| sed "s/OS Version : \(.*\)/\1/g")
    echo "<tr> <td class=\"matrix-leftcolumn\">$node</td><td class=\"matrix-cell\" style=\"text-align:left\">$os</td><td class=\"matrix-cell\" style=\"text-align:left\">$compiler</td><td class=\"matrix-cell\" style=\"text-align:left\">$boost</td><td class=\"matrix-cell\" style=\"text-align:left\">$java</td><td class=\"matrix-cell\" style=\"text-align:left\">$cmake</td><td class=\"matrix-cell\" style=\"text-align:center\">$ns3</td><td class=\"matrix-cell\" style=\"text-align:center\">$py</td></tr>"
    rm consoleText
done


#Travis - get ID of the last jobs with the API
BUILD_NUM=$(curl -s 'https://api.travis-ci.org/repos/simgrid/simgrid/builds?limit=1' | grep -o '^\[{"id":[0-9]*,' | grep -o '[0-9]' | tr -d '\n')
BUILDS=($(curl -s https://api.travis-ci.org/repos/simgrid/simgrid/builds/${BUILD_NUM} | grep -o '{"id":[0-9]*,' | grep -o '[0-9]*'| tail -n 2))

for id in "${!BUILDS[@]}"
do
    wget --quiet https://api.travis-ci.org/v3/job/${BUILDS[$id]}/log.txt -O ./consoleText >/dev/null 2>&1
    sed -i -e "s/\r//g" ./consoleText
    if [ $id == 0 ]; then
      node="<a href=\"https://travis-ci.org/simgrid/simgrid\">travis-linux</a>"
      os="Ubuntu 16.04 (<a href=\"https://docs.travis-ci.com/user/reference/xenial/\">Xenial</a>) 64 bits"
    else
      node="<a href=\"https://travis-ci.org/simgrid/simgrid\">travis-mac</a>"
      os="Mac OSX High Sierra (kernel: 17.4.0)"
    fi
    boost=$(get_boost)
    compiler=$(get_compiler)
    java=$(get_java)
    cmake=$(get_cmake)
    ns3=$(get_ns3)
    py=$(get_python)
    echo "<tr> <td class=\"matrix-leftcolumn\">$node</td><td class=\"matrix-cell\" style=\"text-align:left\">$os</td><td class=\"matrix-cell\" style=\"text-align:left\">$compiler</td><td class=\"matrix-cell\" style=\"text-align:left\">$boost</td><td class=\"matrix-cell\" style=\"text-align:left\">$java</td><td class=\"matrix-cell\" style=\"text-align:left\">$cmake</td><td class=\"matrix-cell\" style=\"text-align:center\">$ns3</td><td class=\"matrix-cell\" style=\"text-align:center\">$py</td></tr>"
    rm consoleText
done

#Appveyor - get ID of the last job with the API
BUILD_ID=$(curl -s "https://ci.appveyor.com/api/projects/mquinson/simgrid" | grep -o '\[{"jobId":"[a-zA-Z0-9]*",' | sed "s/\[{\"jobId\":\"//" | sed "s/\",//")
wget --quiet https://ci.appveyor.com/api/buildjobs/$BUILD_ID/log -O ./consoleText >/dev/null 2>&1
sed -i -e "s/\r//g" ./consoleText
node="<a href="https://ci.appveyor.com/project/mquinson/simgrid">appveyor</a>"
os="Windows Server 2012 - VS2015 + mingw64 5.3.0"
boost=$(get_boost)
compiler=$(get_compiler)
java=$(get_java)
cmake=$(get_cmake)
ns3=$(get_ns3)
py=$(get_python)
echo "<tr> <td class=\"matrix-leftcolumn\">$node</td><td class=\"matrix-cell\" style=\"text-align:left\">$os</td><td class=\"matrix-cell\" style=\"text-align:left\">$compiler</td><td class=\"matrix-cell\" style=\"text-align:left\">$boost</td><td class=\"matrix-cell\" style=\"text-align:left\">$java</td><td class=\"matrix-cell\" style=\"text-align:left\">$cmake</td><td class=\"matrix-cell\" style=\"text-align:center\">$ns3</td><td class=\"matrix-cell\" style=\"text-align:center\">$py</td></tr>"
rm consoleText

echo "</table>"
