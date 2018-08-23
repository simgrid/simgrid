.. _application:

.. raw:: html

   <object id="TOC" data="graphical-toc.svg" width="100%" type="image/svg+xml"></object>
   <script>
   window.onload=function() { // Wait for the SVG to be loaded before changing it
     var elem=document.querySelector("#TOC").contentDocument.getElementById("ApplicationBox")
     elem.style="opacity:0.93999999;fill:#ff0000;fill-opacity:0.1;stroke:#000000;stroke-width:0.35277778;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1";
   }
   </script>
   <br/>
   <br/>

Describing your Application
***************************

Every SimGrid simulation entails a distributed application, that
virtually executes on the simulated platform. This application can
be either an existing MPI program (if you use the SMPI interface), or
a program specifically written to execute within SimGrid, using one of
the dedicated APIs.

.. include:: app_s4u.rst

.. include:: app_smpi.rst

.. include:: app_legacy.rst
