/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_PRIVATE_H_
#define INSTR_PRIVATE_H_

#include <xbt/base.h>

#include "simgrid/instr.h"
#include "instr/instr_interface.h"
#include "src/internal_config.h"
#include "simgrid_config.h"

SG_BEGIN_DECL()

/* Need to define function drand48 for Windows */
/* FIXME: use _drand48() defined in src/surf/random_mgr.c instead */
#ifdef _WIN32
#  define drand48() (rand()/(RAND_MAX + 1.0))
#endif

#define INSTR_DEFAULT_STR_SIZE 500

#include "xbt/graph.h"
#include "xbt/dict.h"

typedef enum {
  PAJE_DefineContainerType,
  PAJE_DefineVariableType,
  PAJE_DefineStateType,
  PAJE_DefineEventType,
  PAJE_DefineLinkType,
  PAJE_DefineEntityValue,
  PAJE_CreateContainer,
  PAJE_DestroyContainer,
  PAJE_SetVariable,
  PAJE_AddVariable,
  PAJE_SubVariable,
  PAJE_SetState,
  PAJE_PushState,
  PAJE_PopState,
  PAJE_ResetState,
  PAJE_StartLink,
  PAJE_EndLink,
  PAJE_NewEvent
} e_event_type;

typedef enum {
  TYPE_VARIABLE,
  TYPE_LINK,
  TYPE_CONTAINER,
  TYPE_STATE,
  TYPE_EVENT
} e_entity_types;

//--------------------------------------------------
class s_type;
typedef s_type *type_t;
class s_type {
  public:
  char *id;
  char *name;
  char *color;
  e_entity_types kind;
  s_type *father;
  xbt_dict_t children;
  xbt_dict_t values; //valid for all types except variable and container
};

typedef s_type s_type_t;

//--------------------------------------------------
class s_val;
typedef s_val *val_t;

class s_val {
  public:
  char *id;
  char *name;
  char *color;
  type_t father;
};
typedef s_val s_val_t;

//--------------------------------------------------
typedef enum {
  INSTR_HOST,
  INSTR_LINK,
  INSTR_ROUTER,
  INSTR_AS,
  INSTR_SMPI,
  INSTR_MSG_VM,
  INSTR_MSG_PROCESS,
  INSTR_MSG_TASK
} e_container_types;

//--------------------------------------------------
class s_container;
typedef s_container *container_t;

class s_container {
  public:
  sg_netpoint_t netpoint;
  char *name;     /* Unique name of this container */
  char *id;       /* Unique id of this container */
  type_t type;    /* Type of this container */
  int level;      /* Level in the hierarchy, root level is 0 */
  e_container_types kind; /* This container is of what kind */
  s_container *father;
  xbt_dict_t children;
};
typedef s_container s_container_t;

//--------------------------------------------------
class PajeEvent {
  public:
  double timestamp;
  e_event_type event_type;
  virtual void print() = 0;
  void *data;
  virtual ~PajeEvent();
};

//--------------------------------------------------

class DefineVariableTypeEvent : public PajeEvent
{
  public:
  type_t type;
   DefineVariableTypeEvent(type_t type);
   void print() override;
};
//--------------------------------------------------

class DefineStateTypeEvent : public PajeEvent  {
  type_t type;
  public:
  DefineStateTypeEvent(type_t type);
  void print() override;
};


class SetVariableEvent : public PajeEvent  {
  container_t container;
  type_t type;
  double value;
  public:
  SetVariableEvent (double timestamp, container_t container, type_t type, double value);
  void print() override;
};


class AddVariableEvent:public PajeEvent {
  container_t container;
  type_t type;
  double value;
  public:
  AddVariableEvent (double timestamp, container_t container, type_t type, double value);
  void print() override;
};

//--------------------------------------------------


class SubVariableEvent : public PajeEvent  {
  public:
  container_t container;
  type_t type;
  double value;
  public:
  SubVariableEvent(double timestamp, container_t container, type_t type, double value);
  void print() override;
};
//--------------------------------------------------

class SetStateEvent : public PajeEvent  {
  public:
  container_t container;
  type_t type;
  val_t value;
  const char* filename;
  int linenumber;
  public:
  SetStateEvent (double timestamp, container_t container, type_t type, val_t value);
  void print() override;
};


class PushStateEvent : public PajeEvent  {
  public:
  container_t container;
  type_t type;
  val_t value;
  int size;
  const char* filename;
  int linenumber;
  void* extra_;
  public:
  PushStateEvent (double timestamp, container_t container, type_t type, val_t value);
  PushStateEvent (double timestamp, container_t container, type_t type, val_t value,
                                             void* extra);
  void print() override;
};

class PopStateEvent : public PajeEvent  {
  container_t container;
  type_t type;
  public:
  PopStateEvent (double timestamp, container_t container, type_t type);
  void print() override;
};

class ResetStateEvent : public PajeEvent  {
  container_t container;
  type_t type;
  public:
  ResetStateEvent (double timestamp, container_t container, type_t type);
  void print() override;
};

class StartLinkEvent : public PajeEvent  {
  public:
  container_t container;
  type_t type;
  container_t sourceContainer;
  char *value;
  char *key;
  int size;
  public:
    ~StartLinkEvent();
    StartLinkEvent(double timestamp, container_t container, type_t type, container_t sourceContainer, const char* value,
                   const char* key);
    StartLinkEvent(double timestamp, container_t container, type_t type, container_t sourceContainer, const char* value,
                   const char* key, int size);
    void print() override;
};

class EndLinkEvent : public PajeEvent  {
  container_t container;
  type_t type;
  container_t destContainer;
  char *value;
  char *key;
  public:
  EndLinkEvent (double timestamp, container_t container, type_t type, container_t destContainer,
                                  const char *value, const char *key);
  ~EndLinkEvent();
  void print() override;
};


class NewEvent : public PajeEvent  {
  public:
  container_t container;
  type_t type;
  val_t value;
  public:
  NewEvent (double timestamp, container_t container, type_t type, val_t value);
  void print() override;

};


extern XBT_PRIVATE xbt_dict_t created_categories;
extern XBT_PRIVATE xbt_dict_t declared_marks;
extern XBT_PRIVATE xbt_dict_t user_host_variables;
extern XBT_PRIVATE xbt_dict_t user_vm_variables;
extern XBT_PRIVATE xbt_dict_t user_link_variables;
extern XBT_PRIVATE double TRACE_last_timestamp_to_dump;

/* instr_paje_header.c */
XBT_PRIVATE void TRACE_header(int basic, int size);

/* from paje.c */
XBT_PRIVATE void TRACE_paje_start();
XBT_PRIVATE void TRACE_paje_end();
XBT_PRIVATE void TRACE_paje_dump_buffer (int force);


/* from instr_config.c */
XBT_PRIVATE bool TRACE_needs_platform ();
XBT_PRIVATE bool TRACE_is_enabled();
XBT_PRIVATE bool TRACE_platform();
XBT_PRIVATE bool TRACE_platform_topology();
XBT_PRIVATE bool TRACE_is_configured();
XBT_PRIVATE bool TRACE_categorized ();
XBT_PRIVATE bool TRACE_uncategorized ();
XBT_PRIVATE bool TRACE_msg_process_is_enabled();
XBT_PRIVATE bool TRACE_msg_vm_is_enabled();
XBT_PRIVATE bool TRACE_buffer ();
XBT_PRIVATE bool TRACE_disable_link();
XBT_PRIVATE bool TRACE_disable_speed();
XBT_PRIVATE bool TRACE_onelink_only ();
XBT_PRIVATE bool TRACE_disable_destroy ();
XBT_PRIVATE bool TRACE_basic ();
XBT_PRIVATE bool TRACE_display_sizes ();
XBT_PRIVATE char *TRACE_get_comment ();
XBT_PRIVATE char *TRACE_get_comment_file ();
XBT_PRIVATE int TRACE_precision ();
XBT_PRIVATE char *TRACE_get_filename();
XBT_PRIVATE char *TRACE_get_viva_uncat_conf ();
XBT_PRIVATE char *TRACE_get_viva_cat_conf ();
XBT_PRIVATE void TRACE_generate_viva_uncat_conf ();
XBT_PRIVATE void TRACE_generate_viva_cat_conf ();
XBT_PRIVATE void instr_pause_tracing ();
XBT_PRIVATE void instr_resume_tracing ();

/* Public functions used in SMPI */
XBT_PUBLIC(bool) TRACE_smpi_is_enabled();
XBT_PUBLIC(bool) TRACE_smpi_is_grouped();
XBT_PUBLIC(bool) TRACE_smpi_is_computing();
XBT_PUBLIC(bool) TRACE_smpi_is_sleeping();
XBT_PUBLIC(bool) TRACE_smpi_view_internals();

/* from resource_utilization.c */
XBT_PRIVATE void TRACE_surf_host_set_utilization(const char *resource, const char *category, double value, double now,
                                     double delta);
XBT_PRIVATE void TRACE_surf_link_set_utilization(const char *resource,const char *category, double value, double now,
                                     double delta);
XBT_PUBLIC(void) TRACE_surf_resource_utilization_alloc();

/* instr_paje.c */
extern XBT_PRIVATE xbt_dict_t trivaNodeTypes;
extern XBT_PRIVATE xbt_dict_t trivaEdgeTypes;
XBT_PRIVATE long long int instr_new_paje_id ();
XBT_PRIVATE void PJ_container_alloc ();
XBT_PRIVATE void PJ_container_release ();
XBT_PUBLIC(container_t) PJ_container_new (const char *name, e_container_types kind, container_t father);
XBT_PUBLIC(container_t) PJ_container_get (const char *name);
XBT_PUBLIC(container_t) PJ_container_get_or_null (const char *name);
XBT_PUBLIC(container_t) PJ_container_get_root ();
XBT_PUBLIC(void) PJ_container_set_root (container_t root);
XBT_PUBLIC(void) PJ_container_free (container_t container);
XBT_PUBLIC(void) PJ_container_free_all (void);
XBT_PUBLIC(void) PJ_container_remove_from_parent (container_t container);

/* instr_paje_types.c */
XBT_PRIVATE void PJ_type_release ();
XBT_PUBLIC(type_t)  PJ_type_get_root ();
XBT_PRIVATE type_t PJ_type_container_new (const char *name, type_t father);
XBT_PRIVATE type_t PJ_type_event_new (const char *name, type_t father);
type_t PJ_type_link_new (const char *name, type_t father, type_t source, type_t dest);
XBT_PRIVATE XBT_PRIVATE type_t PJ_type_variable_new (const char *name, const char *color, type_t father);
XBT_PRIVATE type_t PJ_type_state_new (const char *name, type_t father);
XBT_PUBLIC(type_t)  PJ_type_get (const char *name, const type_t father);
XBT_PUBLIC(type_t)  PJ_type_get_or_null (const char *name, type_t father);
XBT_PRIVATE XBT_PRIVATE void PJ_type_free (type_t type); 

/* instr_config.c */
XBT_PRIVATE void recursiveDestroyType (type_t type);

/* instr_paje_values.c */
XBT_PUBLIC(val_t)  PJ_value_new (const char *name, const char *color, type_t father);
XBT_PUBLIC(val_t)  PJ_value_get_or_new (const char *name, const char *color, type_t father);
XBT_PUBLIC(val_t)  PJ_value_get (const char *name, const type_t father);

XBT_PRIVATE void TRACE_TI_start();
XBT_PRIVATE void TRACE_TI_end();

XBT_PRIVATE void TRACE_paje_dump_buffer (int force);
XBT_PRIVATE void dump_comment_file (const char *filename);
XBT_PRIVATE void dump_comment (const char *comment);

struct s_instr_extra_data;
typedef struct s_instr_extra_data *instr_extra_data;

typedef enum{
  TRACING_INIT,
  TRACING_FINALIZE,
  TRACING_COMM_SIZE,
  TRACING_COMM_SPLIT,
  TRACING_COMM_DUP,
  TRACING_SEND,
  TRACING_ISEND,
  TRACING_SSEND,
  TRACING_ISSEND,
  TRACING_RECV,
  TRACING_IRECV,
  TRACING_SENDRECV,
  TRACING_TEST,
  TRACING_WAIT,
  TRACING_WAITALL,
  TRACING_WAITANY,
  TRACING_BARRIER,
  TRACING_BCAST,
  TRACING_REDUCE,
  TRACING_ALLREDUCE,
  TRACING_ALLTOALL,
  TRACING_ALLTOALLV,
  TRACING_GATHER,
  TRACING_GATHERV,
  TRACING_SCATTER,
  TRACING_SCATTERV,
  TRACING_ALLGATHER,
  TRACING_ALLGATHERV,
  TRACING_REDUCE_SCATTER,
  TRACING_COMPUTING,
  TRACING_SLEEPING,
  TRACING_SCAN,
  TRACING_EXSCAN,
  TRACING_CUSTOM
} e_caller_type ;

typedef struct s_instr_extra_data {
  e_caller_type type;
  int send_size;
  int recv_size;
  double comp_size;
  double sleep_duration;
  int src;
  int dst;
  int tag; // Needed for matching waits to requests.
  int root;
  const char* datatype1;
  const char* datatype2;
  int * sendcounts;
  int * recvcounts;
  int num_processes;
  int (*print_push)(FILE *trace_file, char *process_id, instr_extra_data extra);
  int (*print_pop)(FILE *trace_file, char *process_id, instr_extra_data extra);
} s_instr_extra_data_t;

/* Format of TRACING output.
 *   - paje is the regular format, that we all know
 *   - TI is a trick to reuse the tracing functions to generate a time independent trace during the execution. Such trace can easily be replayed with smpi_replay afterward.
 *     This trick should be removed and replaced by some code using the signal that we will create to cleanup the TRACING
 */
typedef enum { instr_fmt_paje, instr_fmt_TI } instr_fmt_type_t;
extern instr_fmt_type_t instr_fmt_type;

SG_END_DECL()

void DefineContainerEvent(type_t type);
void LogVariableTypeDefinition(type_t type);
void LogStateTypeDefinition(type_t type);
void LogLinkTypeDefinition(type_t type, type_t source, type_t dest);
void LogEntityValue (val_t value);
void LogContainerCreation (container_t container);
void LogContainerDestruction (container_t container);
void LogDefineEventType(type_t type);

#endif
