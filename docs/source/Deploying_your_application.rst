.. _deploy:

Deploying your Application
==========================

.. raw:: html

   <object id="TOC" data="graphical-toc.svg" type="image/svg+xml"></object>
   <script>
   window.onload=function() { // Wait for the SVG to be loaded before changing it
     var elem=document.querySelector("#TOC").contentDocument.getElementById("DeployBox")
     elem.style="opacity:0.93999999;fill:#ff0000;fill-opacity:0.1;stroke:#000000;stroke-width:0.35277778;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:4;stroke-dasharray:none;stroke-dashoffset:0;stroke-opacity:1";
   }
   </script>
   <br/>
   <br/>

There is several ways to deploy the :ref:`application <application>` you want to
study on your :ref:`simulated platform <platform>`, i.e. to specify which actor
should be started on which host. You can do so directly in your program (as
shown in :ref:`these examples <s4u_ex_actors>`), or using an XML deployment
file. Unless you have a good reason, you should keep your application apart
from the deployment as it will :ref:`ease your experimental campaign afterward
<howto_science>`.

Deploying actors from XML is easy: it only involves 3 tags: :ref:`pf_tag_actor`,
:ref:`pf_tag_argument`, and :ref:`pf_tag_prop`. They must be placed in an
encompassing :ref:`pf_tag_platform`. Here is a first example (search in the
archive for files named ``???_d.xml`` for more):

.. code-block:: xml

   <?xml version='1.0'?>
   <!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">
   <platform version="4.1">
     <!-- The following starts an actor that runs the function `alice()` on the given host.
       -- It is not given any parameter, so its args is empty.
       -->
     <actor host="host1" function="alice" />

     <!-- The following starts another actor that runs `bob()` on host2.
       -- The args of this actor contains "3" and "3000" on creation.
       -->
     <actor host="host2" function="bob" />
       <argument value="3"/>
       <argument value="3000"/>
     </actor>

     <!-- Carole runs on 'host3', has 1 parameter "42" in its argv and one property.
       -- Use simgrid::s4u::Actor::get_property() to retrieve it.-->
     <actor host="host3" function="carol">
       <argument value="42"/>
       <prop id="SomeProp" value="SomeValue"/>
     </actor>
   </platform>


-------------------------------------------------------------------------------

.. _pf_tag_actor:

<actor>
--------

This tag starts a new actor executing the given function on a given host.


**Parent tags:** :ref:`pf_tag_platform` (only in deployment files) |br|
**Children tags:** :ref:`pf_tag_argument`, :ref:`pf_tag_prop` |br|
**Attributes:**

:``host``: Host on which this actor should be started (mandatory).
:``function``: Code to execute.

   That function must be registered beforehand
   with :cpp:func:`simgrid::s4u::Engine::register_actor` or
   with :cpp:func:`simgrid::s4u::Engine::register_function`.

   If you are stuck with MSG, use :cpp:func:`MSG_process_create`,
   :cpp:func:`MSG_process_create_with_arguments` or
   :cpp:func:`MSG_process_create_with_environment`.

   There is nothing to do in Java, as SimGrid uses introspection abilities to
   retrieve the classes from their names. You must then use the full class name
   (including the package name) in your XML file.

:``start_time``: Useful to delay the start of your actor.

	 -1 starts the actor immediately.
:``kill_time``:  Time at which the actor should be killed.

   -1 means that the actor should not be killed automatically.
:``on_failure``: What to do when the actor's host is turned off and back on.

   Either ``DIE`` (default -- don't restart the actor) or ``RESTART``

-------------------------------------------------------------------------------

.. _pf_tag_argument:

<argument>
----------

Add a parameter to the actor, to its args vector. Naturally, the semantic of
these parameters completely depend on your program.


**Parent tags:** :ref:`pf_tag_actor`  |br|
**Children tags:** none |br|
**Attributes:**

:``value``: The string to add to the actor's args vector.

.. |br| raw:: html

   <br />
