/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _MC_MC_H
#define _MC_MC_H

#include <src/internal_config.h>
#include <simgrid/simix.h>
#include <simgrid/modelchecker.h> /* our public interface (and definition of HAVE_MC) */
#if HAVE_UCONTEXT_H
#include <ucontext.h>           /* context relative declarations */
#endif

/* Maximum size of the application heap.
 *
 * The model-checker heap is placed at this offset from the beginning of the application heap.
 *
 * In the current implementation, if the application uses more than this for the application heap the application heap
 * will smash the beginning of the model-checker heap and bad things will happen.
 *
 * For 64 bits systems, we have a lot of virtual memory available so we wan use a much bigger value in order to avoid
 * bad things from happening.
 * */

#define STD_HEAP_SIZE   (sizeof(void*)<=4 ? (100*1024*1024) : (1ll*1024*1024*1024*1024))

SG_BEGIN_DECL()

/********************************** Configuration of MC **************************************/  
extern XBT_PUBLIC(int) _sg_do_model_check;
extern XBT_PRIVATE int _sg_do_model_check_record;
extern XBT_PRIVATE int _sg_mc_checkpoint;
extern XBT_PUBLIC(int) _sg_mc_sparse_checkpoint;
extern XBT_PUBLIC(int) _sg_mc_ksm;
extern XBT_PUBLIC(int) _sg_mc_soft_dirty;
extern XBT_PUBLIC(char*) _sg_mc_property_file;
extern XBT_PRIVATE int _sg_mc_timeout;
extern XBT_PRIVATE int _sg_mc_hash;
extern XBT_PRIVATE int _sg_mc_max_depth;
extern XBT_PUBLIC(int) _sg_mc_visited;
extern XBT_PRIVATE char* _sg_mc_dot_output_file;
extern XBT_PUBLIC(int) _sg_mc_comms_determinism;
extern XBT_PUBLIC(int) _sg_mc_send_determinism;
extern XBT_PRIVATE int _sg_mc_safety;
extern XBT_PRIVATE int _sg_mc_liveness;
extern XBT_PRIVATE int _sg_mc_snapshot_fds;
extern XBT_PRIVATE int _sg_mc_termination;

/********************************* Global *************************************/
XBT_PRIVATE void _mc_cfg_cb_reduce(const char *name, int pos);
XBT_PRIVATE void _mc_cfg_cb_checkpoint(const char *name, int pos);
XBT_PRIVATE void _mc_cfg_cb_sparse_checkpoint(const char *name, int pos);
XBT_PRIVATE void _mc_cfg_cb_ksm(const char *name, int pos);
XBT_PRIVATE void _mc_cfg_cb_soft_dirty(const char *name, int pos);
XBT_PRIVATE void _mc_cfg_cb_property(const char *name, int pos);
XBT_PRIVATE void _mc_cfg_cb_timeout(const char *name, int pos);
XBT_PRIVATE void _mc_cfg_cb_snapshot_fds(const char *name, int pos);
XBT_PRIVATE void _mc_cfg_cb_hash(const char *name, int pos);
XBT_PRIVATE void _mc_cfg_cb_max_depth(const char *name, int pos);
XBT_PRIVATE void _mc_cfg_cb_visited(const char *name, int pos);
XBT_PRIVATE void _mc_cfg_cb_dot_output(const char *name, int pos);
XBT_PRIVATE void _mc_cfg_cb_comms_determinism(const char *name, int pos);
XBT_PRIVATE void _mc_cfg_cb_send_determinism(const char *name, int pos);
XBT_PRIVATE void _mc_cfg_cb_termination(const char *name, int pos);

XBT_PUBLIC(void) MC_run(void);
XBT_PUBLIC(void) MC_init(void);
XBT_PUBLIC(void) MC_exit(void);
XBT_PUBLIC(void) MC_process_clock_add(smx_process_t, double);
XBT_PUBLIC(double) MC_process_clock_get(smx_process_t);
XBT_PRIVATE void MC_automaton_load(const char *file);

/****************************** MC ignore **********************************/
XBT_PUBLIC(void) MC_ignore_heap(void *address, size_t size);
XBT_PUBLIC(void) MC_remove_ignore_heap(void *address, size_t size);
XBT_PUBLIC(void) MC_ignore_local_variable(const char *var_name, const char *frame);
XBT_PUBLIC(void) MC_ignore_global_variable(const char *var_name);
#if HAVE_UCONTEXT_H
XBT_PUBLIC(void) MC_register_stack_area(void *stack, smx_process_t process, ucontext_t* context, size_t size);
#endif

/********************************* Memory *************************************/
XBT_PUBLIC(void) MC_memory_init(void);  /* Initialize the memory subsystem */
XBT_PUBLIC(void) MC_memory_exit(void);
XBT_PUBLIC(void) MC_memory_init_server(void);

SG_END_DECL()

#endif                          /* _MC_MC_H */
