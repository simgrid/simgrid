/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_PRIVATE_HPP
#define INSTR_PRIVATE_HPP

#include <xbt/base.h>

#include "instr/instr_interface.h"
#include "simgrid/instr.h"
#include "simgrid_config.h"
#include "src/internal_config.h"
#include "xbt/graph.h"
#include <iomanip> /** std::setprecision **/
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#ifdef WIN32
#include <direct.h> // _mkdir
/* Need to define function drand48 for Windows */
/* FIXME: use _drand48() defined in src/surf/random_mgr.c instead */
#define drand48() (rand() / (RAND_MAX + 1.0))
#endif

#define INSTR_DEFAULT_STR_SIZE 500

namespace simgrid {
namespace instr {

class Value;

enum e_event_type {
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
};

//--------------------------------------------------
enum e_entity_types { TYPE_VARIABLE, TYPE_LINK, TYPE_CONTAINER, TYPE_STATE, TYPE_EVENT };

class Type {
  std::string id_;
  std::string name_;
  std::string color_;
  e_entity_types kind_;
  Type* father_;

public:
  std::map<std::string, Type*> children_;
  std::map<std::string, Value*> values_; // valid for all types except variable and container

  Type(std::string name, std::string alias, std::string color, e_entity_types kind, Type* father);
  ~Type();

  std::string getName() { return name_; }
  const char* getCname() { return name_.c_str(); }
  const char* getId() { return id_.c_str(); }
  e_entity_types getKind() { return kind_; }
  bool isColored() { return not color_.empty(); }

  Type* byName(std::string name);

  Type* getOrCreateContainerType(std::string name);
  Type* getOrCreateEventType(std::string name);
  Type* getOrCreateLinkType(std::string name, Type* source, Type* dest);
  Type* getOrCreateStateType(std::string name);
  Type* getOrCreateVariableType(std::string name, std::string color);

  void logContainerTypeDefinition();
  void logVariableTypeDefinition();
  void logStateTypeDefinition();
  void logLinkTypeDefinition(simgrid::instr::Type* source, simgrid::instr::Type* dest);
  void logDefineEventType();

  static Type* createRootType();
  static Type* getRootType();
};

//--------------------------------------------------
class Value {
  std::string name_;
  std::string id_;
  std::string color_;
  Type* father_;

  explicit Value(std::string name, std::string color, Type* father);

public:
  ~Value();
  static Value* byNameOrCreate(std::string name, std::string color, Type* father);
  static Value* byName(std::string name, Type* father);
  const char* getCname() { return name_.c_str(); }
  const char* getId() { return id_.c_str(); }
  bool isColored() { return not color_.empty(); }
  void print();
};

//--------------------------------------------------
enum e_container_types {
  INSTR_HOST,
  INSTR_LINK,
  INSTR_ROUTER,
  INSTR_AS,
  INSTR_SMPI,
  INSTR_MSG_VM,
  INSTR_MSG_PROCESS,
  INSTR_MSG_TASK
};

class Container {
  e_container_types kind_; /* This container is of what kind */
  int level_ = 0;          /* Level in the hierarchy, root level is 0 */
  sg_netpoint_t netpoint_ = nullptr;

public:
  Container(std::string name, simgrid::instr::e_container_types kind, Container* father);
  virtual ~Container();

  std::string name_;       /* Unique name of this container */
  std::string id_;         /* Unique id of this container */
  Type* type_;             /* Type of this container */
  Container* father_;
  std::map<std::string, Container*> children_;
  static Container* byNameOrNull(std::string name);
  static Container* byName(std::string name);
  void removeFromParent();
  void logCreation();
  void logDestruction();
};

//--------------------------------------------------
class PajeEvent {
protected:
  Container* container;
  Type* type;

public:
  double timestamp_;
  e_event_type eventType_;
  PajeEvent(Container* container, Type* type, double timestamp, e_event_type eventType)
      : container(container), type(type), timestamp_(timestamp), eventType_(eventType){};
  virtual void print() = 0;
  virtual ~PajeEvent();
  void insertIntoBuffer();
};

//--------------------------------------------------
class SetVariableEvent : public PajeEvent {
  double value;

public:
  SetVariableEvent(double timestamp, Container* container, Type* type, double value);
  void print() override;
};

class AddVariableEvent : public PajeEvent {
  double value;

public:
  AddVariableEvent(double timestamp, Container* container, Type* type, double value);
  void print() override;
};
//--------------------------------------------------

class SubVariableEvent : public PajeEvent {
  double value;

public:
  SubVariableEvent(double timestamp, Container* container, Type* type, double value);
  void print() override;
};
//--------------------------------------------------

class SetStateEvent : public PajeEvent {
  Value* value;
  const char* filename;
  int linenumber;

public:
  SetStateEvent(double timestamp, Container* container, Type* type, Value* val);
  void print() override;
};

class PushStateEvent : public PajeEvent {
  Value* value;
  const char* filename;
  int linenumber;
  void* extra_;

public:
  PushStateEvent(double timestamp, Container* container, Type* type, Value* val);
  PushStateEvent(double timestamp, Container* container, Type* type, Value* val, void* extra);
  void print() override;
};

class PopStateEvent : public PajeEvent {
public:
  PopStateEvent(double timestamp, Container* container, Type* type);
  void print() override;
};

class ResetStateEvent : public PajeEvent {
public:
  ResetStateEvent(double timestamp, Container* container, Type* type);
  void print() override;
};

class StartLinkEvent : public PajeEvent {
  Container* sourceContainer_;
  std::string value_;
  std::string key_;
  int size_;

public:
  StartLinkEvent(double timestamp, Container* container, Type* type, Container* sourceContainer, std::string value,
                 std::string key);
  StartLinkEvent(double timestamp, Container* container, Type* type, Container* sourceContainer, std::string value,
                 std::string key, int size);
  void print() override;
};

class EndLinkEvent : public PajeEvent {
  Container* destContainer;
  std::string value;
  std::string key;

public:
  EndLinkEvent(double timestamp, Container* container, Type* type, Container* destContainer, std::string value,
               std::string key);
  ~EndLinkEvent() = default;
  void print() override;
};

class NewEvent : public PajeEvent {
  Value* val;

public:
  NewEvent(double timestamp, Container* container, Type* type, Value* val);
  void print() override;
};
}
} // namespace simgrid::instr
typedef simgrid::instr::Container* container_t;

extern "C" {

extern XBT_PRIVATE std::set<std::string> created_categories;
extern XBT_PRIVATE std::set<std::string> declared_marks;
extern XBT_PRIVATE std::set<std::string> user_host_variables;
extern XBT_PRIVATE std::set<std::string> user_vm_variables;
extern XBT_PRIVATE std::set<std::string> user_link_variables;
extern XBT_PRIVATE double TRACE_last_timestamp_to_dump;

/* instr_paje_header.c */
XBT_PRIVATE void TRACE_header(bool basic, int size);

/* from paje.c */
XBT_PRIVATE void TRACE_paje_start();
XBT_PRIVATE void TRACE_paje_end();

/* from instr_config.c */
XBT_PRIVATE bool TRACE_needs_platform();
XBT_PRIVATE bool TRACE_is_enabled();
XBT_PRIVATE bool TRACE_platform();
XBT_PRIVATE bool TRACE_platform_topology();
XBT_PRIVATE bool TRACE_is_configured();
XBT_PRIVATE bool TRACE_categorized();
XBT_PRIVATE bool TRACE_uncategorized();
XBT_PRIVATE bool TRACE_msg_process_is_enabled();
XBT_PRIVATE bool TRACE_msg_vm_is_enabled();
XBT_PRIVATE bool TRACE_buffer();
XBT_PRIVATE bool TRACE_disable_link();
XBT_PRIVATE bool TRACE_disable_speed();
XBT_PRIVATE bool TRACE_onelink_only();
XBT_PRIVATE bool TRACE_disable_destroy();
XBT_PRIVATE bool TRACE_basic();
XBT_PRIVATE bool TRACE_display_sizes();
XBT_PRIVATE char* TRACE_get_comment();
XBT_PRIVATE char* TRACE_get_comment_file();
XBT_PRIVATE int TRACE_precision();
XBT_PRIVATE char* TRACE_get_filename();
XBT_PRIVATE char* TRACE_get_viva_uncat_conf();
XBT_PRIVATE char* TRACE_get_viva_cat_conf();
XBT_PRIVATE void TRACE_generate_viva_uncat_conf();
XBT_PRIVATE void TRACE_generate_viva_cat_conf();
XBT_PRIVATE void instr_pause_tracing();
XBT_PRIVATE void instr_resume_tracing();

/* Public functions used in SMPI */
XBT_PUBLIC(bool) TRACE_smpi_is_enabled();
XBT_PUBLIC(bool) TRACE_smpi_is_grouped();
XBT_PUBLIC(bool) TRACE_smpi_is_computing();
XBT_PUBLIC(bool) TRACE_smpi_is_sleeping();
XBT_PUBLIC(bool) TRACE_smpi_view_internals();

/* from resource_utilization.c */
XBT_PRIVATE void TRACE_surf_host_set_utilization(const char* resource, const char* category, double value, double now,
                                                 double delta);
XBT_PRIVATE void TRACE_surf_link_set_utilization(const char* resource, const char* category, double value, double now,
                                                 double delta);
XBT_PUBLIC(void) TRACE_surf_resource_utilization_alloc();

/* instr_paje.c */
extern XBT_PRIVATE std::set<std::string> trivaNodeTypes;
extern XBT_PRIVATE std::set<std::string> trivaEdgeTypes;
XBT_PRIVATE long long int instr_new_paje_id();
XBT_PUBLIC(container_t) PJ_container_get_root ();
XBT_PUBLIC(void) PJ_container_set_root (container_t root);
void instr_new_variable_type(std::string new_typename, const char* color);
void instr_new_user_variable_type(std::string father_type, std::string new_typename, const char* color);
void instr_new_user_state_type(std::string father_type, std::string new_typename);
void instr_new_value_for_user_state_type(std::string new_typename, const char* value, const char* color);

/* instr_config.c */
XBT_PRIVATE void TRACE_TI_start();
XBT_PRIVATE void TRACE_TI_end();

XBT_PRIVATE void TRACE_paje_dump_buffer(bool force);
XBT_PRIVATE void dump_comment_file(std::string filename);
XBT_PRIVATE void dump_comment(std::string comment);

enum e_caller_type {
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
};

struct s_instr_extra_data_t {
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
  int* sendcounts;
  int* recvcounts;
  int num_processes;
};
typedef s_instr_extra_data_t* instr_extra_data;

/* Format of TRACING output.
 *   - paje is the regular format, that we all know
 *   - TI is a trick to reuse the tracing functions to generate a time independent trace during the execution. Such
 *     trace can easily be replayed with smpi_replay afterward. This trick should be removed and replaced by some code
 *     using the signal that we will create to cleanup the TRACING
 */
enum instr_fmt_type_t { instr_fmt_paje, instr_fmt_TI };
extern instr_fmt_type_t instr_fmt_type;
}

#endif
