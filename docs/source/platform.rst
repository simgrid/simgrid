.. _platform:

.. raw:: html

   <object id="TOC" data="graphical-toc.svg" width="100%" type="image/svg+xml"></object>
   <script>
   window.onload=function() { // Wait for the SVG to be loaded before changing it
     var elem=document.querySelector("#TOC").contentDocument.getElementById("PlatformBox")
     elem.style="opacity:0.93999999;fill:#ff0000;fill-opacity:0.1;stroke:#000000;stroke-width:0.35277778;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1";
   }
   </script>
   <br/>
   <br/>

Describing your Simulated Platform
##################################

The goal of SimGrid is to run an application on a simulated platform.
For that, you have to describe **each element of your platform**, such
as computing hosts, clusters, each disks, links, etc. You must also
define the **routing on your platform**, ie which path is taken
between two hosts. Finally, you may also describe an **experimental
scenario**, with qualitative changes (e.g., bandwidth changes
representing an external load) and qualitative changes (representing
how some elements fail and restart over time).

You should really separate your application from the platform
description, as it will ease your experimental campain afterward.
Mixing them is seen as a really bad experimental practice. The easiest
to enforce this split is to put the platform description in a XML
file. Many example platforms are provided in the archive, and this
page gives all needed details to write such files, as well as some
hints and tricks about describing your platform.

On the other side, XML is sometimes not expressive enough, in
particular for large platforms exhibiting repetitive patterns that are
not simply expressed in XML.  In practice, many users end up
generating their XML platform files from some sort of scripts. It is
probably preferable to rewrite your XML :ref:`platform using the lua
scripting language <platform_lua>` instead.  In the future, it should
be possible to describe the platform in python or directly in C++, but
this is not possible yet.

As usual, SimGrid is a versatile framework, and you should find the
way of describing your platform that best fits your experimental
practice. 




..  LocalWords:  SimGrid
