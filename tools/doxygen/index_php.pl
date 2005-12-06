#!/usr/bin/perl -w

($#ARGV >= 2) or die "Usage: index_php.pl <input-php.in-file> <input-html-file> <output-php-file>";

my(@toc);
my($level,$label,$name);

$inputphp  = $ARGV[0];
$inputhtml  = $ARGV[1];
$output = $ARGV[2];

my($onglets) = '
<div class="tabs">
  <ul>
    <li><a href="doc/index.html"><span>Overview</span></a></li>
    <li><a href="doc/faq.html"><span>FAQ</span></a></li>
    <li><a href="doc/modules.html"><span>Modules API</span></a></li>
    <li><a href="doc/publis.html"><span>Publications</span></a></li>
    
    <li><a href="doc/pages.html"><span>Related&nbsp;Pages</span></a></li>
  </ul></div>
<center>  
 </center><p><br>
';

my($body) = '
The specific goal of the project is to facilitate research in the area of
distributed and parallel application scheduling on distributed computing
platforms ranging from simple network of workstations to Computational
Grids.

<h2><a class="anchor" name="overview">
Overview of the toolkit components</a></h2>
As depicted by the following diagram, the SimGrid toolkit is basically three-layered (click on the picture to jump to a specific component).<p>
 
<center>
 <IMG style="border:0px "SRC="doc/simgrid_modules.png" USEMAP="#simgrid_modules">
<MAP NAME="simgrid_modules">
<AREA SHAPE="poly" COORDS="418,171,418,139,343,139,343,89,239,89,239,171" href="doc/group__GRAS__API.html" ALT="GRAS">
<AREA COORDS="428,89,607,171" href="doc/group__SMPI__API.html" ALT="SMPI">
<AREA COORDS="50,89,229,171" href="doc/group__MSG__API.html" ALT="MSG">
<AREA COORDS="50,180,607,240" href="doc/group__SURF__API.html" ALT="SMPI">
<AREA SHAPE="poly" COORDS="43,89,12,89,12,313,607,313,607,250,43,250" href="doc/group__XBT__API.html" ALT="XBT">
<AREA COORDS="347,89,417,133" href="doc/group__AMOK__API.html" ALT="AMOK">
</MAP>
  
<br><b>Relationships between the SimGrid components</b>
</center>
<h3><a class="anchor" name="overview_fondation">
Base layer</a></h3>
The base of the whole toolkit is constituted by the <b><a class="el" href="doc/group__XBT__API.html">XBT</a> (eXtended Bundle of Tools)</b>.<p>
It is a portable library providing some grounding features such as <a class="el" href="doc/group__XBT__log.html">Logging support</a>, <a class="el" href="doc/group__XBT__ex.html">Exception support</a> and <a class="el" href="doc/group__XBT__config.html">Configuration support</a>. XBT also encompass the following convenient datastructures: <a class="el" href="doc/group__XBT__dynar.html">Dynar: generic dynamic array</a>, <a class="el" href="doc/group__XBT__fifo.html">Fifo: generic workqueue</a>, <a class="el" href="doc/group__XBT__dict.html">Dict: generic dictionnary</a>, <a class="el" href="doc/group__XBT__heap.html">Heap: generic heap data structure</a>, <a class="el" href="doc/group__XBT__set.html">Set: generic set datatype</a> and <a class="el" href="doc/group__XBT__swag.html">Swag: O(1) set datatype</a>.<p>
See the <a class="el" href="doc/group__XBT__API.html">XBT</a> section for more details.<h3><a class="anchor" name="overview_kernel">
Simulation kernel layer</a></h3>
The core functionnalities to simulate a virtual platform are provided by a module called <b><a class="el" href="doc/group__SURF__API.html">SURF</a></b> ("that\'s historical, my friend"). It is very low-level and is not intended to be used as such by end-users. Instead, it serve as a basis for the higher level layer.<p>
SURF main features are a fast max-min linear solver and the ability to change transparently the model used to describe the platform. This greatly eases the comparison of the several models existing in the litterature.<p>
See the <a class="el" href="doc/group__SURF__API.html">SURF</a> section for more details.<h3><a class="anchor" name="overview_envs">
Programmation environments layer</a></h3>
This simulation kernel is used to build several programmation environments. Each of them target a specific audiance and constitute a different paradigm. To choose which of them you want to use, you have to think about what you want to do and what would be the result of your work.<p>
<ul>
<li>If you want to study a theoritical problem and compare several heuristics, you probably want to try <b><a class="el" href="doc/group__MSG__API.html">MSG</a></b> (yet another historical name). It was designed exactly to that extend and should allow you to build easily rather realistic multi-agents simulation. Yet, realism is not the main goal of this environment and the most annoying technical issues of real platforms are masked here. Check the <a class="el" href="doc/group__MSG__API.html">MSG</a> section for more information.</li></ul>
<p>
<ul>
<li>If you want to study the behaviour of a MPI application using emulation technics, you should have a look at the <b><a class="el" href="doc/group__SMPI__API.html">SMPI</a></b> (Simulated MPI) programming environment. Unfortunately, this work is still underway. Check the <a class="el" href="doc/group__SMPI__API.html">SMPI</a> section for more information.</li></ul>
<p>
<ul>
<li>If you want to develop a real distributed application, then you may find <b><a class="el" href="doc/group__GRAS__API.html">GRAS</a></b> (Grid Reality And Simulation) useful. This is an API for the realization of distributed applications. <br>
<br>
 Moreover, there is two implementations of this API: one on top of the SURF (allowing to develop and test your application within the comfort of the simulator) and another suited for deployment on real platforms (allowing the resulting application to be highly portable and extremely efficient). <br>
<br>
 Even if you do not plan to run your code for real, you may want to switch to GRAS if you intend to use MSG in a very intensive way (e.g. for simulating a peer-to-peer environment). <br>
<br>
 See the <a class="el" href="doc/group__GRAS__API.html">GRAS</a> section for more details.</li></ul>
<p>
If your favorite programming environment/model is not there (BSP, components, etc.) is not represented in the SimGrid toolkit yet, you may consider adding it. You should contact us first on the <a href="doc/http://lists.gforge.inria.fr/mailman/listinfo/simgrid-devel>">SimGrid developers mailing list</a>, though.<p>
Any question, remark or suggestion are welcome on the <a href="doc/http://lists.gforge.inria.fr/mailman/listinfo/simgrid-user>">SimGrid users mailing list</a>.<p>
<hr>
';

open FILE,$inputphp;
open OUTPUT,"> $output";

while($line=<FILE>) {
    chomp $line;
    if($line =~/______ONGLETS______/) {
	$line =~ s/______ONGLETS______/$onglets/g;
    } elsif($line =~/______BODY______/) {
	$line =~ s/______BODY______/$body/g;
    }
    print OUTPUT "$line\n";
}

close(FILE);
close(OUTPUT);
