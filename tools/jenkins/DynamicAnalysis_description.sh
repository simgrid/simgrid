#!/usr/bin/env sh

set -e

#get and store valgrind graph for once, as its generation can take a long time
wget https://ci.inria.fr/simgrid/job/SimGrid-DynamicAnalysis-Valgrind/label=simgrid-debian8-64-dynamic-analysis/valgrindResult/graph -O ./valgrind_graph.png

#find string containing errors and leaked size for valgrind run
wget https://ci.inria.fr/simgrid/job/SimGrid-DynamicAnalysis-Valgrind/label=simgrid-debian8-64-dynamic-analysis/lastCompletedBuild/
VALGRIND_RES=$(grep href=\"valgrindResult\" ./index.html | sed -e "s/.*\"valgrindResult\">\(.*lost\)<\/a.*/\1/g")
rm index.html

#set html description and write it in a file

echo "
Testing with valgrind and gcov. Click on the graphs for details.
<br><br>

<table id=\"configuration-matrix\">
 <tr class=\"matrix-row\">
  <td class=\"matrix-header\">
    Valgrind Results
  </td>
  <td class=\"matrix-header\">
    Coverage Results
  </td>
  <td class=\"matrix-header\">
    Test Results
  </td>
 </tr>
 <tr class=\"matrix-row\">
  <td class=\"matrix-cell\">
    <a href=\"https://ci.inria.fr/simgrid/job/SimGrid-DynamicAnalysis-Valgrind/label=simgrid-debian8-64-dynamic-analysis/valgrindResult/\">
  	 <img src=\"https://ci.inria.fr/simgrid/job/SimGrid-DynamicAnalysis/lastCompletedBuild/label=simgrid-debian8-64-dynamic-analysis/artifact/valgrind_graph.png\" title=\"$VALGRIND_RES\">
	</a>
  </td>
  <td class=\"matrix-cell\">
    <a href=\"https://ci.inria.fr/simgrid/job/SimGrid-DynamicAnalysis-Coverage/label=simgrid-debian8-64-dynamic-analysis/cobertura\">
  	 <img src=\"https://ci.inria.fr/simgrid/job/SimGrid-DynamicAnalysis-Coverage/label=simgrid-debian8-64-dynamic-analysis/cobertura/graph\">
	</a>
  </td>
  <td class=\"matrix-cell\">
   <a href=\"https://ci.inria.fr/simgrid/view/Tous/job/SimGrid-DynamicAnalysis-Sanitizers/label=simgrid-debian8-64-dynamic-analysis/lastCompletedBuild/testReport/AddressSanitizer/junit/\">
    <img src=\"https://ci.inria.fr/simgrid/job/SimGrid-DynamicAnalysis-Coverage/label=simgrid-debian8-64-dynamic-analysis/groupedTests/trendGraph/unit/png?width=500&height=250\">
   </a>
  </td>
 </tr>
 <tr class=\"matrix-row\">
  <td class=\"matrix-header\">
    Sloccount Results
  </td>
  <td class=\"matrix-header\">
    Sloccount Variation
  </td>
  <td class=\"matrix-header\">
    Test Results : Address Sanitizer (<a href=\"https://github.com/google/sanitizers/wiki/AddressSanitizer\">info</a>)
  </td>
 </tr>
 <tr class=\"matrix-row\">
  <td class=\"matrix-cell\">
    <a href=\"https://ci.inria.fr/simgrid/job/SimGrid-DynamicAnalysis-Coverage/label=simgrid-debian8-64-dynamic-analysis/sloccountResult\">
  	 <img src=\"https://ci.inria.fr/simgrid/job/SimGrid-DynamicAnalysis-Coverage/label=simgrid-debian8-64-dynamic-analysis/sloccountResult/trend\">
	</a>
  </td>
  <td class=\"matrix-cell\">
	<a href=\"https://ci.inria.fr/simgrid/job/SimGrid-DynamicAnalysis-Coverage/label=simgrid-debian8-64-dynamic-analysis/sloccountResult\">
  	 <img src=\"https://ci.inria.fr/simgrid/job/SimGrid-DynamicAnalysis-Coverage/label=simgrid-debian8-64-dynamic-analysis/sloccountResult/trendDelta\">
	</a>
  </td>
  <td class=\"matrix-cell\">
   <a href=\"https://ci.inria.fr/simgrid/job/SimGrid-DynamicAnalysis-Coverage/label=simgrid-debian8-64-dynamic-analysis-2/lastCompletedBuild/testReport/unit/junit/(root)/TestSuite/\">
    <img src=\"https://ci.inria.fr/simgrid/view/Tous/job/SimGrid-DynamicAnalysis-Sanitizers/label=simgrid-debian8-64-dynamic-analysis-2/groupedTests/trendGraph/AddressSanitizer/png?width=500&height=250\">
   </a>
  </td>
 </tr>
 <tr class=\"matrix-row\">
  <td class=\"matrix-header\">
   Test Results : Thread Sanitizer (<a href=\"https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual\">info</a>)
  </td>
  <td class=\"matrix-header\">
   Test Results : Undefined Sanitizer (<a href=\"http://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html\">info</a>)
  </td>
 </tr>
 <tr class=\"matrix-row\">
  <td class=\"matrix-cell\">
   <a href=\"https://ci.inria.fr/simgrid/view/Tous/job/SimGrid-DynamicAnalysis-Sanitizers/label=simgrid-debian8-64-dynamic-analysis-2/lastCompletedBuild/testReport/ThreadSanitizer/junit/\">
    <img src=\"https://ci.inria.fr/simgrid/view/Tous/job/SimGrid-DynamicAnalysis-Sanitizers/label=simgrid-debian8-64-dynamic-analysis-2/groupedTests/trendGraph/ThreadSanitizer/png?width=500&height=250\">
   </a>
  </td>
  <td class=\"matrix-cell\">
   <a href=\"https://ci.inria.fr/simgrid/view/Tous/job/SimGrid-DynamicAnalysis-Sanitizers/label=simgrid-debian8-64-dynamic-analysis-2/lastCompletedBuild/testReport/UndefinedSanitizer/junit/\">
    <img src=\"https://ci.inria.fr/simgrid/view/Tous/job/SimGrid-DynamicAnalysis-Sanitizers/label=simgrid-debian8-64-dynamic-analysis-2/groupedTests/trendGraph/UndefinedSanitizer/png?width=500&height=250\">
   </a>
  </td>
 </tr>
</table>" > ./description.html
