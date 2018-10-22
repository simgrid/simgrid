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

In SimGrid, platforms are usually described in XML. This formalism has
some drawbacks, but using a specific format ensures that the platform
is not mixed with the tested application. This separation of concern
:ref:`is a must <howto_science>` for your Modeling and Simulation (M&S)
work. When XML is too limiting, you may describe your platforms using
the :ref:`lua bindings <platform_lua>` (it is not yet possible to do so in
python or directly in C++).

Since we know that writing platform description files is not trivial,
we included :ref:`many examples <platform_examples>` in the archive. This
documentation also contains some :ref:`hints and howtos <howto>`, as well
as the full :ref:`XML reference guide <platform_reference>`.


Any simulated platform must contain **basic elements**, such as hosts,
links, storages, etc.  SimGrid gives you a great liberty when defining
**routing of your platform**, ie the path taken between each pair of
hosts.  Finally, you may also describe an **experimental scenario**,
with qualitative changes (e.g., bandwidth changes representing an
external load) and qualitative changes (representing how some elements
fail and restart over time).

Defining Basic Elements
***********************

There is not much to say about the definition of basic elements. Just
use the appropriate tags: :ref:`pf_tag_host`, :ref:`pf_tag_link` and
:ref:`pf_tag_storage`.

Defining a Routing
******************

Performance Profiles and Churn
******************************

..  LocalWords:  SimGrid
