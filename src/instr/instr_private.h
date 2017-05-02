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
class PajeEvent;
typedef PajeEvent *paje_event_t;

class PajeEvent {
  public:
  double timestamp;
  e_event_type event_type;
  virtual void print(){};
  void *data;
  virtual ~PajeEvent();
};
typedef PajeEvent s_paje_event_t;

class DefineContainerEvent : public PajeEvent
{
  public:
  type_t type;
  void print() override;
  DefineContainerEvent(type_t);
};

//--------------------------------------------------


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
  void print();// override;
};

class DefineEventTypeEvent : public PajeEvent  {
  type_t type;
  public: 
  DefineEventTypeEvent(type_t type);
  void print() override;
};

class DefineLinkTypeEvent : public PajeEvent  {
  type_t type;
  type_t source;
  type_t dest;
  public:
  DefineLinkTypeEvent(type_t type, type_t source, type_t dest);
  void print() override;
};

class DefineEntityValueEvent : public PajeEvent  {
  val_t value;
  public:
  DefineEntityValueEvent (val_t type);
  void print() override;
};

class CreateContainerEvent : public PajeEvent  {
  public:
  container_t container;
  explicit CreateContainerEvent (container_t container);
  void print() override;
};

class DestroyContainerEvent : public PajeEvent  {
  public:
  container_t container;
  DestroyContainerEvent (container_t container);
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
  void* extra;
  public:
  PushStateEvent (double timestamp, container_t container, type_t type, val_t value);
  PushStateEvent (double timestamp, container_t container, type_t type, val_t value,
                                             void* extra);
  void print() override;
};

class PopStateEvent : public PajeEvent  {
  container_t container;
  type_t type;
  xbt_dynar_t extra;
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
  StartLinkEvent (double timestamp, container_t container, type_t type, container_t sourceContainer,
                                    const char *value, const char *key);
  StartLinkEvent (double timestamp, container_t container, type_t type,
                                            container_t sourceContainer, const char *value, const char *key, int size);
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
XBT_PRIVATE void PJ_type_alloc ();
XBT_PRIVATE void PJ_type_release ();
XBT_PUBLIC(type_t)  PJ_type_get_root ();
XBT_PRIVATE type_t PJ_type_container_new (const char *name, type_t father);
XBT_PRIVATE type_t PJ_type_event_new (const char *name, type_t father);
type_t PJ_type_link_new (const char *name, type_t father, type_t source, type_t dest);
XBT_PRIVATE XBT_PRIVATE type_t PJ_type_variable_new (const char *name, const char *color, type_t father);
XBT_PRIVATE type_t PJ_type_state_new (const char *name, type_t father);
XBT_PUBLIC(type_t)  PJ_type_get (const char *name, const type_t father);
XBT_PUBLIC(type_t)  PJ_type_get_or_null (const char *name, type_t father);
void PJ_type_free_all ();
XBT_PRIVATE XBT_PRIVATE void PJ_type_free (type_t type);

/* instr_paje_values.c */
XBT_PUBLIC(val_t)  PJ_value_new (const char *name, const char *color, type_t father);
XBT_PUBLIC(val_t)  PJ_value_get_or_new (const char *name, const char *color, type_t father);
XBT_PUBLIC(val_t)  PJ_value_get (const char *name, const type_t father);
XBT_PRIVATE void PJ_value_free (val_t value);

XBT_PRIVATE void print_pajeDefineContainerType(paje_event_t event);
XBT_PRIVATE void print_pajeDefineVariableType(paje_event_t event);
XBT_PRIVATE void print_pajeDefineStateType(paje_event_t event);
XBT_PRIVATE void print_pajeDefineEventType(paje_event_t event);
XBT_PRIVATE void print_pajeDefineLinkType(paje_event_t event);
XBT_PRIVATE void print_pajeDefineEntityValue (paje_event_t event);
XBT_PRIVATE void print_pajeCreateContainer(paje_event_t event);
XBT_PRIVATE void print_pajeDestroyContainer(paje_event_t event);
XBT_PRIVATE void print_pajeSetVariable(paje_event_t event);
XBT_PRIVATE void print_pajeAddVariable(paje_event_t event);
XBT_PRIVATE void print_pajeSubVariable(paje_event_t event);
XBT_PRIVATE void print_pajeSetState(paje_event_t event);
XBT_PRIVATE void print_pajePushState(paje_event_t event);
XBT_PRIVATE void print_pajePopState(paje_event_t event);
XBT_PRIVATE void print_pajeResetState(paje_event_t event);
XBT_PRIVATE void print_pajeStartLink(paje_event_t event);
XBT_PRIVATE void print_pajeEndLink(paje_event_t event);
XBT_PRIVATE void print_pajeNewEvent (paje_event_t event);

XBT_PRIVATE void print_TIPushState(paje_event_t event);
XBT_PRIVATE void print_TICreateContainer(paje_event_t event);
XBT_PRIVATE void print_TIDestroyContainer(paje_event_t event);
XBT_PRIVATE void TRACE_TI_start();
XBT_PRIVATE void TRACE_TI_end();
XBT_PRIVATE void TRACE_TI_init();

XBT_PRIVATE void print_NULL (paje_event_t event);
XBT_PRIVATE void TRACE_paje_dump_buffer (int force);
XBT_PRIVATE void dump_comment_file (const char *filename);
XBT_PRIVATE void dump_comment (const char *comment);

typedef struct instr_trace_writer {
  void (*print_DefineContainerType) (paje_event_t event);
  void (*print_DefineVariableType)(paje_event_t event);
  void (*print_DefineStateType)(paje_event_t event);
  void (*print_DefineEventType)(paje_event_t event);
  void (*print_DefineLinkType)(paje_event_t event);
  void (*print_DefineEntityValue)(paje_event_t event);
  void (*print_CreateContainer)(paje_event_t event);
  void (*print_DestroyContainer)(paje_event_t event);
  void (*print_SetVariable)(paje_event_t event);
  void (*print_AddVariable)(paje_event_t event);
  void (*print_SubVariable)(paje_event_t event);
  void (*print_SetState)(paje_event_t event);
  void (*print_PushState)(paje_event_t event);
  void (*print_PopState)(paje_event_t event);
  void (*print_ResetState)(paje_event_t event);
  void (*print_StartLink)(paje_event_t event);
  void (*print_EndLink)(paje_event_t event);
  void (*print_NewEvent) (paje_event_t event);
} s_instr_trace_writer_t;

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
  TRACING_EXSCAN
} e_caller_type ;

typedef struct s_instr_extra_data {
  e_caller_type type;
  int send_size;
  int recv_size;
  double comp_size;
  double sleep_duration;
  int src;
  int dst;
  int root;
  const char* datatype1;
  const char* datatype2;
  int * sendcounts;
  int * recvcounts;
  int num_processes;
} s_instr_extra_data_t;

SG_END_DECL()

#endif
