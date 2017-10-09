/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_PRIVATE_H_
#define INSTR_PRIVATE_H_

#include <xbt/base.h>

#include "instr/instr_interface.h"
#include "simgrid/instr.h"
#include "simgrid_config.h"
#include "src/internal_config.h"
#include <map>
#include <set>
#include <string>

/* Need to define function drand48 for Windows */
/* FIXME: use _drand48() defined in src/surf/random_mgr.c instead */
#ifdef _WIN32
#  define drand48() (rand()/(RAND_MAX + 1.0))
#endif

#define INSTR_DEFAULT_STR_SIZE 500

#include "xbt/graph.h"
#include "xbt/dict.h"

namespace simgrid {
namespace instr {
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

class Type {
public:
  char* id_;
  char* name_;
  char* color_;

  e_entity_types kind_;
  Type* father_;
  xbt_dict_t children_;
  xbt_dict_t values_; // valid for all types except variable and container
  Type(const char* typeNameBuff, const char* key, const char* color, e_entity_types kind, Type* father);
  ~Type();
  Type* getChild(const char* name);
  Type* getChildOrNull(const char* name);

  static Type* containerNew(const char* name, Type* father);
  static Type* eventNew(const char* name, Type* father);
  static Type* variableNew(const char* name, const char* color, Type* father);
  static Type* linkNew(const char* name, Type* father, Type* source, Type* dest);
  static Type* stateNew(const char* name, Type* father);
};

//--------------------------------------------------
class Value {
public:
  char* id_;
  char* name_;
  char* color_;

  Type* father_;
  Value(const char* name, const char* color, Type* father);
  ~Value();
  static Value* get_or_new(const char* name, const char* color, Type* father);
  static Value* get(const char* name, Type* father);
};


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

class Container {
public:
  Container(std::string name, simgrid::instr::e_container_types kind, Container* father);
  virtual ~Container();

  sg_netpoint_t netpoint_;
  std::string name_;       /* Unique name of this container */
  std::string id_;         /* Unique id of this container */
  Type* type_;             /* Type of this container */
  int level_ = 0;          /* Level in the hierarchy, root level is 0 */
  e_container_types kind_; /* This container is of what kind */
  Container* father_;
  std::map<std::string, Container*> children_;
};

//--------------------------------------------------
class PajeEvent {
  public:
    double timestamp_;
    e_event_type eventType_;
    virtual void print() = 0;
    virtual ~PajeEvent();
};

//--------------------------------------------------
class SetVariableEvent : public PajeEvent  {
  private:
    Container* container;
    Type* type;
    double value;

  public:
    SetVariableEvent(double timestamp, Container* container, Type* type, double value);
    void print() override;
};

class AddVariableEvent:public PajeEvent {
  private:
    Container* container;
    Type* type;
    double value;

  public:
    AddVariableEvent(double timestamp, Container* container, Type* type, double value);
    void print() override;
};
//--------------------------------------------------


class SubVariableEvent : public PajeEvent  {
  private:
    Container* container;
    Type* type;
    double value;

  public:
    SubVariableEvent(double timestamp, Container* container, Type* type, double value);
    void print() override;
};
//--------------------------------------------------

class SetStateEvent : public PajeEvent  {
  private:
    Container* container;
    Type* type;
    Value* value;
    const char* filename;
    int linenumber;

  public:
    SetStateEvent(double timestamp, Container* container, Type* type, Value* val);
    void print() override;
};


class PushStateEvent : public PajeEvent  {
  public:
    Container* container;
    Type* type;
    Value* value;
    int size;
    const char* filename;
    int linenumber;
    void* extra_;

  public:
    PushStateEvent(double timestamp, Container* container, Type* type, Value* val);
    PushStateEvent(double timestamp, Container* container, Type* type, Value* val, void* extra);
    void print() override;
};

class PopStateEvent : public PajeEvent  {
  Container* container;
  Type* type;

public:
  PopStateEvent(double timestamp, Container* container, Type* type);
  void print() override;
};

class ResetStateEvent : public PajeEvent  {
  Container* container;
  Type* type;

public:
  ResetStateEvent(double timestamp, Container* container, Type* type);
  void print() override;
};

class StartLinkEvent : public PajeEvent  {
  Container* container_;
  Type* type_;
  Container* sourceContainer_;
  std::string value_;
  std::string key_;
  int size_;

public:
  StartLinkEvent(double timestamp, Container* container, Type* type, Container* sourceContainer, const char* value,
                 const char* key);
  StartLinkEvent(double timestamp, Container* container, Type* type, Container* sourceContainer, const char* value,
                 const char* key, int size);
  void print() override;
};

class EndLinkEvent : public PajeEvent  {
  Container* container;
  Type* type;
  Container* destContainer;
  std::string value;
  std::string key;

public:
  EndLinkEvent(double timestamp, Container* container, Type* type, Container* destContainer, std::string value,
               std::string key);
  ~EndLinkEvent() = default;
  void print() override;
};


class NewEvent : public PajeEvent  {
  public:
    Container* container;
    Type* type;
    Value* val;

  public:
    NewEvent(double timestamp, Container* container, Type* type, Value* val);
    void print() override;
};
}
} // namespace simgrid::instr
typedef simgrid::instr::Container* container_t;

SG_BEGIN_DECL()

extern XBT_PRIVATE std::set<std::string> created_categories;
extern XBT_PRIVATE std::set<std::string> declared_marks;
extern XBT_PRIVATE std::set<std::string> user_host_variables;
extern XBT_PRIVATE std::set<std::string> user_vm_variables;
extern XBT_PRIVATE std::set<std::string> user_link_variables;
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
extern XBT_PRIVATE std::set<std::string> trivaNodeTypes;
extern XBT_PRIVATE std::set<std::string> trivaEdgeTypes;
XBT_PRIVATE long long int instr_new_paje_id ();
XBT_PUBLIC(container_t) PJ_container_get (const char *name);
XBT_PUBLIC(simgrid::instr::Container*) PJ_container_get_or_null(const char* name);
XBT_PUBLIC(container_t) PJ_container_get_root ();
XBT_PUBLIC(void) PJ_container_set_root (container_t root);
XBT_PUBLIC(void) PJ_container_free_all (void);
XBT_PUBLIC(void) PJ_container_remove_from_parent (container_t container);

/* instr_paje_types.c */
XBT_PUBLIC(simgrid::instr::Type*) PJ_type_get_root();

/* instr_config.c */
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

/* Format of TRACING output.
 *   - paje is the regular format, that we all know
 *   - TI is a trick to reuse the tracing functions to generate a time independent trace during the execution. Such trace can easily be replayed with smpi_replay afterward.
 *     This trick should be removed and replaced by some code using the signal that we will create to cleanup the TRACING
 */
typedef enum { instr_fmt_paje, instr_fmt_TI } instr_fmt_type_t;
extern instr_fmt_type_t instr_fmt_type;

SG_END_DECL()

void LogContainerTypeDefinition(simgrid::instr::Type* type);
void LogVariableTypeDefinition(simgrid::instr::Type* type);
void LogStateTypeDefinition(simgrid::instr::Type* type);
void LogLinkTypeDefinition(simgrid::instr::Type* type, simgrid::instr::Type* source, simgrid::instr::Type* dest);
void LogEntityValue(simgrid::instr::Value* val);
void LogContainerCreation (container_t container);
void LogContainerDestruction (container_t container);
void LogDefineEventType(simgrid::instr::Type* type);

#endif
