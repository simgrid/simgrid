.. _MSG_doc:

====================================
The MSG Interface (legacy interface)
====================================

.. raw:: html

   <object id="TOC" data="graphical-toc.svg" width="100%" type="image/svg+xml"></object>
   <script>
   window.onload=function() { // Wait for the SVG to be loaded before changing it
     //var elem=document.querySelector("#TOC").contentDocument.getElementById("S4UBox")
     //elem.style="opacity:0.93999999;fill:#ff0000;fill-opacity:0.1";
   }
   </script>
   <br/>
   <br/>

MSG is a simple API to write algorithms organized with Concurrent
Sequential Processes (CSP) that interact by exchanging messages. It
constitutes a convenient simplification of the reality of distributed
systems. It can be used to build rather realistic simulations, but
remains simple to use: most unpleasant technical elements can be
abstracted away rather easily.

This page describes the C version of this API while the :ref:`Java
bindings of MSG <Java_doc>` are described in another section.

.. warning::

   MSG used to be the main API of SimGrid 3, but we are currently in
   the process of releasing SimGrid 4. So MSG is frozen and will
   probably never evolve. If you are starting a new project, you
   should consider S4U instead. Note that the support for MSG will not
   be removed from SimGrid before 2020 at least.

Main MSG Functions
------------------

The basic workflow is the following:

 - Initialize the library with :c:macro:`MSG_init`
 - Create a platform (usually by parsing a file with :cpp:func:`MSG_create_environment`)
 - Register the functions that your processes are supposed to run with
   :cpp:func:`MSG_function_register` (and maybe :cpp:func:`MSG_function_register_default`) 
 - Launch your processes from a deployment file with :cpp:func:`MSG_launch_application`
 - Run the simulation with :cpp:func:`MSG_main`

.. doxygenenum:: msg_error_t

.. doxygenfunction:: MSG_config
.. doxygenfunction:: MSG_create_environment
.. doxygenfunction:: MSG_function_register
.. doxygenfunction:: MSG_function_register_default
.. doxygenfunction:: MSG_get_clock
.. doxygenfunction:: MSG_get_sent_msg
.. doxygendefine:: MSG_init
.. doxygenfunction:: MSG_launch_application
.. doxygenfunction:: MSG_main
.. doxygenfunction:: MSG_set_function

Process Management Functions
----------------------------

This describes the process structure :cpp:type:`msg_process_t` and the functions for managing it.

.. doxygentypedef:: msg_process_t
.. doxygenfunction:: MSG_process_attach
.. doxygenfunction:: MSG_process_auto_restart_set
.. doxygenfunction:: MSG_process_create
.. doxygenfunction:: MSG_process_create_with_arguments
.. doxygenfunction:: MSG_process_create_with_environment
.. doxygenfunction:: MSG_process_create_from_stdfunc
.. doxygenfunction:: MSG_process_daemonize
.. doxygenfunction:: MSG_process_detach
.. doxygenfunction:: MSG_processes_as_dynar
.. doxygenfunction:: MSG_process_from_PID
.. doxygenfunction:: MSG_process_get_data
.. doxygenfunction:: MSG_process_get_host
.. doxygenfunction:: MSG_process_get_name
.. doxygenfunction:: MSG_process_get_number
.. doxygenfunction:: MSG_process_get_PID
.. doxygenfunction:: MSG_process_get_PPID
.. doxygenfunction:: MSG_process_get_properties
.. doxygenfunction:: MSG_process_get_property_value
.. doxygenfunction:: MSG_process_get_smx_ctx
.. doxygenfunction:: MSG_process_is_suspended
.. doxygenfunction:: MSG_process_join
.. doxygenfunction:: MSG_process_kill
.. doxygenfunction:: MSG_process_killall
.. doxygenfunction:: MSG_process_migrate
.. doxygenfunction:: MSG_process_on_exit
.. doxygenfunction:: MSG_process_ref
.. doxygenfunction:: MSG_process_restart
.. doxygenfunction:: MSG_process_resume
.. doxygenfunction:: MSG_process_self
.. doxygenfunction:: MSG_process_self_name
.. doxygenfunction:: MSG_process_self_PID
.. doxygenfunction:: MSG_process_self_PPID
.. doxygenfunction:: MSG_process_set_data
.. doxygenfunction:: MSG_process_set_data_cleanup
.. doxygenfunction:: MSG_process_set_kill_time
.. doxygenfunction:: MSG_process_sleep
.. doxygenfunction:: MSG_process_suspend
.. doxygenfunction:: MSG_process_unref
.. doxygenfunction:: MSG_process_yield

Host Management Functions
-------------------------

.. doxygentypedef:: msg_host_t
.. doxygenfunction:: MSG_host_by_name
.. doxygenfunction:: MSG_get_host_by_name
.. doxygenfunction:: MSG_get_host_number
.. doxygenfunction:: MSG_host_get_attached_storage_lists
.. doxygenfunction:: MSG_host_get_core_number
.. doxygenfunction:: MSG_host_get_data
.. doxygenfunction:: MSG_host_get_mounted_storage_list
.. doxygenfunction:: MSG_host_get_name
.. doxygenfunction:: MSG_host_get_nb_pstates
.. doxygenfunction:: MSG_host_get_power_peak_at
.. doxygenfunction:: MSG_host_get_process_list
.. doxygenfunction:: MSG_host_get_properties
.. doxygenfunction:: MSG_host_get_property_value
.. doxygenfunction:: MSG_host_get_pstate
.. doxygenfunction:: MSG_host_get_speed
.. doxygenfunction:: MSG_host_is_off
.. doxygenfunction:: MSG_host_is_on
.. doxygenfunction:: MSG_host_off
.. doxygenfunction:: MSG_host_on
.. doxygenfunction:: MSG_hosts_as_dynar
.. doxygenfunction:: MSG_host_self
.. doxygenfunction:: MSG_host_set_data
.. doxygenfunction:: MSG_host_set_property_value
.. doxygenfunction:: MSG_host_set_pstate

Task Management Functions
-------------------------

Task structure of MSG :cpp:type:`msg_task_t` and associated functions.

.. doxygentypedef:: msg_task_t
.. doxygendefine:: MSG_TASK_UNINITIALIZED

.. doxygenfunction:: MSG_parallel_task_create
.. doxygenfunction:: MSG_parallel_task_execute
.. doxygenfunction:: MSG_parallel_task_execute_with_timeout
.. doxygenfunction:: MSG_task_cancel
.. doxygenfunction:: MSG_task_create
.. doxygenfunction:: MSG_task_destroy
.. doxygenfunction:: MSG_task_dsend
.. doxygenfunction:: MSG_task_dsend_bounded
.. doxygenfunction:: MSG_task_execute
.. doxygenfunction:: MSG_task_get_bytes_amount
.. doxygenfunction:: MSG_task_get_category
.. doxygenfunction:: MSG_task_get_data
.. doxygenfunction:: MSG_task_get_flops_amount
.. doxygenfunction:: MSG_task_get_name
.. doxygenfunction:: MSG_task_get_remaining_communication
.. doxygenfunction:: MSG_task_get_remaining_work_ratio
.. doxygenfunction:: MSG_task_get_sender
.. doxygenfunction:: MSG_task_get_source
.. doxygenfunction:: MSG_task_irecv
.. doxygenfunction:: MSG_task_irecv_bounded
.. doxygenfunction:: MSG_task_isend
.. doxygenfunction:: MSG_task_isend_bounded
.. doxygenfunction:: MSG_task_listen
.. doxygenfunction:: MSG_task_listen_from
.. doxygenfunction:: MSG_task_receive
.. doxygenfunction:: MSG_task_receive_bounded
.. doxygenfunction:: MSG_task_receive_ext
.. doxygenfunction:: MSG_task_receive_ext_bounded
.. doxygenfunction:: MSG_task_receive_with_timeout
.. doxygenfunction:: MSG_task_receive_with_timeout_bounded
.. doxygenfunction:: MSG_task_recv
.. doxygenfunction:: MSG_task_recv_bounded
.. doxygenfunction:: MSG_task_send
.. doxygenfunction:: MSG_task_send_bounded
.. doxygenfunction:: MSG_task_send_with_timeout
.. doxygenfunction:: MSG_task_send_with_timeout_bounded
.. doxygenfunction:: MSG_task_set_bound
.. doxygenfunction:: MSG_task_set_bytes_amount
.. doxygenfunction:: MSG_task_set_category
.. doxygenfunction:: MSG_task_set_copy_callback
.. doxygenfunction:: MSG_task_set_data
.. doxygenfunction:: MSG_task_set_flops_amount
.. doxygenfunction:: MSG_task_set_name
.. doxygenfunction:: MSG_task_set_priority

		   
Mailbox Management Functions
----------------------------

.. doxygenfunction:: MSG_mailbox_set_async

Communications
--------------

.. doxygentypedef:: msg_comm_t

.. doxygenfunction:: MSG_comm_destroy
.. doxygenfunction:: MSG_comm_get_status
.. doxygenfunction:: MSG_comm_get_task
.. doxygenfunction:: MSG_comm_test
.. doxygenfunction:: MSG_comm_testany
.. doxygenfunction:: MSG_comm_wait
.. doxygenfunction:: MSG_comm_waitall
.. doxygenfunction:: MSG_comm_waitany

Explicit Synchronization Functions
----------------------------------

Explicit synchronization mechanisms: semaphores (:cpp:type:`msg_sem_t`) and friends.

In some situations, these things are very helpful to synchronize processes without message exchanges.

Barriers
........

.. doxygentypedef:: msg_bar_t
.. doxygenfunction:: MSG_barrier_destroy
.. doxygenfunction:: MSG_barrier_init
.. doxygenfunction:: MSG_barrier_wait

Semaphores
..........
		     
.. doxygentypedef:: msg_sem_t
.. doxygenfunction:: MSG_sem_acquire
.. doxygenfunction:: MSG_sem_acquire_timeout
.. doxygenfunction:: MSG_sem_destroy
.. doxygenfunction:: MSG_sem_get_capacity
.. doxygenfunction:: MSG_sem_init
.. doxygenfunction:: MSG_sem_release
.. doxygenfunction:: MSG_sem_would_block

Virtual Machines
----------------

This interface mimics IaaS clouds.
With it, you can create virtual machines to put your processes
into, and interact directly with the VMs to manage groups of
processes.

.. doxygentypedef:: msg_vm_t
.. doxygenfunction:: MSG_vm_create
.. doxygenfunction:: MSG_vm_create_core
.. doxygenfunction:: MSG_vm_create_multicore
.. doxygenfunction:: MSG_vm_destroy
.. doxygenfunction:: MSG_vm_get_name
.. doxygenfunction:: MSG_vm_get_pm
.. doxygenfunction:: MSG_vm_get_ramsize
.. doxygenfunction:: MSG_vm_is_created
.. doxygenfunction:: MSG_vm_is_running
.. doxygenfunction:: MSG_vm_is_suspended
.. doxygenfunction:: MSG_vm_resume
.. doxygenfunction:: MSG_vm_set_bound
.. doxygenfunction:: MSG_vm_set_ramsize
.. doxygenfunction:: MSG_vm_shutdown
.. doxygenfunction:: MSG_vm_start
.. doxygenfunction:: MSG_vm_suspend

Storage Management Functions
----------------------------
Storage structure of MSG (:cpp:type:`msg_storage_t`) and associated functions, inspired from POSIX.

.. doxygentypedef:: msg_storage_t
.. doxygenfunction:: MSG_storage_get_by_name
.. doxygenfunction:: MSG_storage_get_data
.. doxygenfunction:: MSG_storage_get_host
.. doxygenfunction:: MSG_storage_get_name
.. doxygenfunction:: MSG_storage_get_properties
.. doxygenfunction:: MSG_storage_get_property_value
.. doxygenfunction:: MSG_storage_read
.. doxygenfunction:: MSG_storages_as_dynar
.. doxygenfunction:: MSG_storage_set_data
.. doxygenfunction:: MSG_storage_set_property_value
.. doxygenfunction:: MSG_storage_write

Zone Management Functions
-------------------------
Network Zone (:cpp:class:`msg_file_t`) and associated functions.

.. doxygentypedef:: msg_netzone_t
.. doxygenfunction:: MSG_zone_get_by_name
.. doxygenfunction:: MSG_zone_get_hosts
.. doxygenfunction:: MSG_zone_get_name
.. doxygenfunction:: MSG_zone_get_property_value
.. doxygenfunction:: MSG_zone_get_root
.. doxygenfunction:: MSG_zone_get_sons
.. doxygenfunction:: MSG_zone_set_property_value
