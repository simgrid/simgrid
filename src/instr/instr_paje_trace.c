/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"

#ifdef HAVE_TRACING

#define INSTR_PAJE_ASSERT(str) {xbt_assert1(str!=NULL&&strlen(str)>0, "'%s' is NULL or length is zero", #str);}

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_paje_trace, instr, "Paje tracing event system");

typedef struct paje_event *paje_event_t;
typedef struct paje_event {
  unsigned int id;
  double timestamp;
} s_paje_event_t;

typedef struct s_defineContainerType *defineContainerType_t;
typedef struct s_defineContainerType {
  s_paje_event_t event;
  type_t type;
  void (*print) (defineContainerType_t event);
}s_defineContainerType_t;

typedef struct s_defineVariableType *defineVariableType_t;
typedef struct s_defineVariableType {
  s_paje_event_t event;
  type_t type;
  void (*print) (defineVariableType_t event);
}s_defineVariableType_t;

typedef struct s_defineStateType *defineStateType_t;
typedef struct s_defineStateType {
  s_paje_event_t event;
  type_t type;
  void (*print) (defineStateType_t event);
}s_defineStateType_t;

typedef struct s_defineEventType *defineEventType_t;
typedef struct s_defineEventType {
  s_paje_event_t event;
  type_t type;
  void (*print) (defineEventType_t event);
}s_defineEventType_t;

typedef struct s_defineLinkType *defineLinkType_t;
typedef struct s_defineLinkType {
  s_paje_event_t event;
  type_t type;
  type_t source;
  type_t dest;
  void (*print) (defineLinkType_t event);
}s_defineLinkType_t;

typedef struct s_createContainer *createContainer_t;
typedef struct s_createContainer {
  s_paje_event_t event;
  container_t container;
  void (*print) (createContainer_t event);
}s_createContainer_t;

typedef struct s_destroyContainer *destroyContainer_t;
typedef struct s_destroyContainer {
  s_paje_event_t event;
  container_t container;
  void (*print) (destroyContainer_t event);
}s_destroyContainer_t;

typedef struct s_setVariable *setVariable_t;
typedef struct s_setVariable {
  s_paje_event_t event;
  container_t container;
  type_t type;
  double value;
  void (*print) (setVariable_t event);
}s_setVariable_t;

typedef struct s_addVariable *addVariable_t;
typedef struct s_addVariable {
  s_paje_event_t event;
  container_t container;
  type_t type;
  double value;
  void (*print) (addVariable_t event);
}s_addVariable_t;

typedef struct s_subVariable *subVariable_t;
typedef struct s_subVariable {
  s_paje_event_t event;
  container_t container;
  type_t type;
  double value;
  void (*print) (subVariable_t event);
}s_subVariable_t;

typedef struct s_setState *setState_t;
typedef struct s_setState {
  s_paje_event_t event;
  container_t container;
  type_t type;
  char *value;
  void (*print) (setState_t event);
}s_setState_t;

typedef struct s_pushState *pushState_t;
typedef struct s_pushState {
  s_paje_event_t event;
  container_t container;
  type_t type;
  char *value;
  void (*print) (pushState_t event);
}s_pushState_t;

typedef struct s_popState *popState_t;
typedef struct s_popState {
  s_paje_event_t event;
  container_t container;
  type_t type;
  char *value;
  void (*print) (popState_t event);
}s_popState_t;

typedef struct s_startLink *startLink_t;
typedef struct s_startLink {
  s_paje_event_t event;
  type_t type;
  container_t container;
  container_t sourceContainer;
  char *value;
  char *key;
  void (*print) (startLink_t event);
}s_startLink_t;

typedef struct s_endLink *endLink_t;
typedef struct s_endLink {
  s_paje_event_t event;
  type_t type;
  container_t container;
  container_t destContainer;
  char *value;
  char *key;
  void (*print) (endLink_t event);
}s_endLink_t;

typedef struct s_newEvent *newEvent_t;
typedef struct s_newEvent {
  s_paje_event_t event;
  type_t type;
  container_t container;
  char *value;
  void (*print) (newEvent_t event);
}s_newEvent_t;

static FILE *tracing_file = NULL;

static int pajeDefineContainerTypeId = 0;
static int pajeDefineStateTypeId = 1;
static int pajeDefineEntityValueId = 2;
static int pajeDefineEventTypeId = 3;
static int pajeDefineLinkTypeId = 4;
static int pajeCreateContainerId = 5;
static int pajeSetStateId = 6;
#define UNUSED007 7
static int pajePopStateId = 8;
static int pajeDestroyContainerId = 9;
#define UNUSED006 10
#define UNUSED003 11
static int pajeStartLinkId = 12;
static int pajeEndLinkId = 13;
#define UNUSED000 14
#define UNUSED004 15
#define UNUSED008 16
#define UNUSED009 17
#define UNUSED005 18
static int pajePushStateId = 19;
static int pajeDefineEventTypeWithColorId = 20;
static int pajeDefineVariableTypeWithColorId = 21;
static int pajeSetVariableId = 22;
static int pajeAddVariableId = 23;
static int pajeSubVariableId = 24;
static int pajeDefineVariableTypeId = 25;
#define UNUSED001 26
static int pajeNewEventId = 27;

#define TRACE_LINE_SIZE 1000

void TRACE_paje_start(void)
{
  char *filename = TRACE_get_filename();
  tracing_file = fopen(filename, "w");
  xbt_assert1 (tracing_file != NULL, "Tracefile %s could not be opened for writing.", filename);

  DEBUG1("Filename %s is open for writing", filename);

  /* output header */
  TRACE_paje_create_header();
}

void TRACE_paje_end(void)
{
  fclose(tracing_file);
  char *filename = TRACE_get_filename();
  DEBUG1("Filename %s is closed", filename);
}

void TRACE_paje_create_header(void)
{
  DEBUG0 ("Define paje header");
  fprintf(tracing_file, "\
%%EventDef PajeDefineContainerType %d \n\
%%       Alias string \n\
%%       ContainerType string \n\
%%       Name string \n\
%%EndEventDef \n\
%%EventDef PajeDefineStateType %d \n\
%%       Alias string \n\
%%       ContainerType string \n\
%%       Name string \n\
%%EndEventDef \n\
%%EventDef PajeDefineEntityValue %d \n\
%%       Alias string \n\
%%       EntityType string \n\
%%       Name string \n\
%%EndEventDef \n\
%%EventDef PajeDefineEventType %d \n\
%%       Alias string \n\
%%       EntityType string \n\
%%       Name string \n\
%%       Color color \n\
%%EndEventDef \n\
%%EventDef PajeDefineEventType %d \n\
%%       Alias string \n\
%%       EntityType string \n\
%%       Name string \n\
%%EndEventDef \n\
%%EventDef PajeDefineLinkType %d \n\
%%       Alias string \n\
%%       ContainerType string \n\
%%       SourceContainerType string \n\
%%       DestContainerType string \n\
%%       Name string \n\
%%EndEventDef \n\
%%EventDef PajeCreateContainer %d \n\
%%       Time date \n\
%%       Alias string \n\
%%       Type string \n\
%%       Container string \n\
%%       Name string \n\
%%EndEventDef \n\
%%EventDef PajeDestroyContainer %d \n\
%%       Time date \n\
%%       Type string \n\
%%       Container string \n\
%%EndEventDef \n\
%%EventDef PajeSetState %d \n\
%%       Time date \n\
%%       EntityType string \n\
%%       Container string \n\
%%       Value string \n\
%%EndEventDef\n\
%%EventDef PajePopState %d \n\
%%       Time date \n\
%%       EntityType string \n\
%%       Container string \n\
%%EndEventDef\n\
%%EventDef PajeStartLink %d \n\
%%       Time date \n\
%%       EntityType string \n\
%%       Container string \n\
%%       Value string \n\
%%       SourceContainer string \n\
%%       Key string \n\
%%EndEventDef\n\
%%EventDef PajeEndLink %d \n\
%%       Time date \n\
%%       EntityType string \n\
%%       Container string \n\
%%       Value string \n\
%%       DestContainer string \n\
%%       Key string \n\
%%EndEventDef\n\
%%EventDef PajePushState %d \n\
%%       Time date \n\
%%       EntityType string \n\
%%       Container string \n\
%%       Value string \n\
%%EndEventDef\n\
%%EventDef PajeSetVariable %d \n\
%%       Time date \n\
%%       EntityType string \n\
%%       Container string \n\
%%       Value string \n\
%%EndEventDef\n\
%%EventDef PajeAddVariable %d \n\
%%       Time date \n\
%%       EntityType string \n\
%%       Container string \n\
%%       Value string \n\
%%EndEventDef\n\
%%EventDef PajeSubVariable %d \n\
%%       Time date \n\
%%       EntityType string \n\
%%       Container string \n\
%%       Value string \n\
%%EndEventDef\n\
%%EventDef PajeDefineVariableType %d \n\
%%       Alias string \n\
%%       ContainerType string \n\
%%       Name string \n\
%%EndEventDef \n\
%%EventDef PajeDefineVariableType %d \n\
%%       Alias string \n\
%%       ContainerType string \n\
%%       Name string \n\
%%       Color color \n\
%%EndEventDef \n\
%%EventDef PajeNewEvent %d \n\
%%       Time date \n\
%%       EntityType string \n\
%%       Container string \n\
%%       Value string \n\
%%EndEventDef\n", pajeDefineContainerTypeId, pajeDefineStateTypeId, pajeDefineEntityValueId, pajeDefineEventTypeWithColorId, pajeDefineEventTypeId, pajeDefineLinkTypeId, pajeCreateContainerId, pajeDestroyContainerId, pajeSetStateId, pajePopStateId, pajeStartLinkId, pajeEndLinkId, pajePushStateId, pajeSetVariableId, pajeAddVariableId, pajeSubVariableId, pajeDefineVariableTypeId, pajeDefineVariableTypeWithColorId, pajeNewEventId);
}

/* internal do the instrumentation module */
static void print_pajeDefineContainerType(defineContainerType_t event)
{
  fprintf(tracing_file, "%d %s %s %s\n",
      event->event.id,
      event->type->id,
      event->type->father->id,
      event->type->name);
}

static void print_pajeDefineVariableType(defineVariableType_t event)
{
  fprintf(tracing_file, "%d %s %s %s \"%s\"\n",
      event->event.id,
      event->type->id,
      event->type->father->id,
      event->type->name,
      event->type->color);
}

static void print_pajeDefineStateType(defineStateType_t event)
{
  fprintf(tracing_file, "%d %s %s %s\n",
      event->event.id,
      event->type->id,
      event->type->father->id,
      event->type->name);
}

static void print_pajeDefineEventType(defineEventType_t event)
{
  fprintf(tracing_file, "%d %s %s %s \"%s\"\n",
      event->event.id,
      event->type->id,
      event->type->father->id,
      event->type->name,
      event->type->color);
}

static void print_pajeDefineLinkType(defineLinkType_t event)
{
  fprintf(tracing_file, "%d %s %s %s %s %s\n",
      event->event.id,
      event->type->id,
      event->type->father->id,
      event->source->id,
      event->dest->id,
      event->type->name);
}

static void print_pajeCreateContainer(createContainer_t event)
{
  if (event->event.timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %s %s\n",
        event->event.id,
        event->container->id,
        event->container->type->id,
        event->container->father->id,
        event->container->name);
  }else{
    fprintf(tracing_file, "%d %lf %s %s %s %s\n",
        event->event.id,
        event->event.timestamp,
        event->container->id,
        event->container->type->id,
        event->container->father->id,
        event->container->name);
  }
}

static void print_pajeDestroyContainer(destroyContainer_t event)
{
  if (event->event.timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s\n",
        event->event.id,
        event->container->type->id,
        event->container->id);
  }else{
    fprintf(tracing_file, "%d %lf %s %s\n",
        event->event.id,
        event->event.timestamp,
        event->container->type->id,
        event->container->id);
  }
}

static void print_pajeSetVariable(setVariable_t event)
{
  if (event->event.timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %f\n",
        event->event.id,
        event->type->id,
        event->container->id,
        event->value);
  }else{
    fprintf(tracing_file, "%d %lf %s %s %f\n",
        event->event.id,
        event->event.timestamp,
        event->type->id,
        event->container->id,
        event->value);
  }
}

static void print_pajeAddVariable(addVariable_t event)
{
  if (event->event.timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %f\n",
        event->event.id,
        event->type->id,
        event->container->id,
        event->value);
  }else{
    fprintf(tracing_file, "%d %lf %s %s %f\n",
        event->event.id,
        event->event.timestamp,
        event->type->id,
        event->container->id,
        event->value);
  }
}

static void print_pajeSubVariable(subVariable_t event)
{
  if (event->event.timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %f\n",
        event->event.id,
        event->type->id,
        event->container->id,
        event->value);
  }else{
    fprintf(tracing_file, "%d %lf %s %s %f\n",
        event->event.id,
        event->event.timestamp,
        event->type->id,
        event->container->id,
        event->value);
  }
}

static void print_pajeSetState(setState_t event)
{
  if (event->event.timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %s\n",
        event->event.id,
        event->type->id,
        event->container->id,
        event->value);
  }else{
    fprintf(tracing_file, "%d %lf %s %s %s\n",
        event->event.id,
        event->event.timestamp,
        event->type->id,
        event->container->id,
        event->value);
  }
}

static void print_pajePushState(pushState_t event)
{
  if (event->event.timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %s\n",
        event->event.id,
        event->type->id,
        event->container->id,
        event->value);
  }else{
    fprintf(tracing_file, "%d %lf %s %s %s\n",
        event->event.id,
        event->event.timestamp,
        event->type->id,
        event->container->id,
        event->value);
  }
}

static void print_pajePopState(popState_t event)
{
  if (event->event.timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s\n",
        event->event.id,
        event->type->id,
        event->container->id);
  }else{
    fprintf(tracing_file, "%d %lf %s %s\n",
        event->event.id,
        event->event.timestamp,
        event->type->id,
        event->container->id);
  }
}

static void print_pajeStartLink(startLink_t event)
{
  if (event->event.timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %s %s %s\n",
        event->event.id,
        event->type->id,
        event->container->id,
        event->value,
        event->sourceContainer->id,
        event->key);
  }else {
    fprintf(tracing_file, "%d %lf %s %s %s %s %s\n",
        event->event.id,
        event->event.timestamp,
        event->type->id,
        event->container->id,
        event->value,
        event->sourceContainer->id,
        event->key);
  }
}

static void print_pajeEndLink(endLink_t event)
{
  if (event->event.timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %s %s %s\n",
        event->event.id,
        event->type->id,
        event->container->id,
        event->value,
        event->destContainer->id,
        event->key);
  }else {
    fprintf(tracing_file, "%d %lf %s %s %s %s %s\n",
        event->event.id,
        event->event.timestamp,
        event->type->id,
        event->container->id,
        event->value,
        event->destContainer->id,
        event->key);
  }
}

static void print_pajeNewEvent (newEvent_t event)
{
  if (event->event.timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %s\n",
        event->event.id,
        event->type->id,
        event->container->id,
        event->value);
  }else{
    fprintf(tracing_file, "%d %lf %s %s %s\n",
        event->event.id,
        event->event.timestamp,
        event->type->id,
        event->container->id,
        event->value);
  }
}

void new_pajeDefineContainerType(type_t type)
{
  defineContainerType_t event = xbt_new0(s_defineContainerType_t, 1);
  event->type = type;
  event->print = print_pajeDefineContainerType;
  event->event.id = pajeDefineContainerTypeId;
  event->event.timestamp = 0;

  //print it
  event->print (event);

  //destroy it
  xbt_free (event);
  event = NULL;
}

void new_pajeDefineVariableType(type_t type)
{
  defineVariableType_t event = xbt_new0(s_defineVariableType_t, 1);
  event->type = type;
  event->print = print_pajeDefineVariableType;
  event->event.id = pajeDefineVariableTypeWithColorId;
  event->event.timestamp = 0;

  //print it
  event->print (event);

  //destroy it
  xbt_free (event);
  event = NULL;
}

void new_pajeDefineStateType(type_t type)
{
  defineStateType_t event = xbt_new0(s_defineStateType_t, 1);
  event->type = type;
  event->print = print_pajeDefineStateType;
  event->event.id = pajeDefineStateTypeId;
  event->event.timestamp = 0;

  //print it
  event->print (event);

  //destroy it
  xbt_free (event);
  event = NULL;
}

void new_pajeDefineEventType(type_t type)
{
  defineEventType_t event = xbt_new0(s_defineEventType_t, 1);
  event->type = type;
  event->print = print_pajeDefineEventType;
  event->event.id = pajeDefineEventTypeWithColorId;
  event->event.timestamp = 0;

  //print it
  event->print (event);

  //destroy it
  xbt_free (event);
  event = NULL;
}

void new_pajeDefineLinkType(type_t type, type_t source, type_t dest)
{
  defineLinkType_t event = xbt_new0(s_defineLinkType_t, 1);
  event->type = type;
  event->source = source;
  event->dest = dest;
  event->print = print_pajeDefineLinkType;
  event->event.id = pajeDefineLinkTypeId;
  event->event.timestamp = 0;

  //print it
  event->print (event);

  //destroy it
  xbt_free (event);
  event = NULL;
}

void new_pajeCreateContainer (container_t container)
{
  createContainer_t event = xbt_new0(s_createContainer_t, 1);
  event->container = container;
  event->print = print_pajeCreateContainer;
  event->event.id = pajeCreateContainerId;
  event->event.timestamp = SIMIX_get_clock();

  //print it
  event->print (event);

  //destroy it
  xbt_free (event);
  event = NULL;
}

void new_pajeDestroyContainer (container_t container)
{
  destroyContainer_t event = xbt_new0(s_destroyContainer_t, 1);
  event->container = container;
  event->print = print_pajeDestroyContainer;
  event->event.id = pajeDestroyContainerId;
  event->event.timestamp = SIMIX_get_clock();

  //print it
  event->print (event);

  //destroy it
  xbt_free (event);
  event = NULL;

  fflush (tracing_file);
}

void new_pajeSetVariable (double timestamp, container_t container, type_t type, double value)
{
  setVariable_t event = xbt_new0(s_setVariable_t, 1);
  event->type = type;
  event->container = container;
  event->value = value;
  event->print = print_pajeSetVariable;
  event->event.id = pajeSetVariableId;
  event->event.timestamp = timestamp;

  //print it
  event->print (event);

  //destroy it
  xbt_free (event);
  event = NULL;
}


void new_pajeAddVariable (double timestamp, container_t container, type_t type, double value)
{
  addVariable_t event = xbt_new0(s_addVariable_t, 1);
  event->type = type;
  event->container = container;
  event->value = value;
  event->print = print_pajeAddVariable;
  event->event.id = pajeAddVariableId;
  event->event.timestamp = timestamp;

  //print it
  event->print (event);

  //destroy it
  xbt_free (event);
  event = NULL;
}

void new_pajeSubVariable (double timestamp, container_t container, type_t type, double value)
{
  subVariable_t event = xbt_new0(s_subVariable_t, 1);
  event->type = type;
  event->container = container;
  event->value = value;
  event->print = print_pajeSubVariable;
  event->event.id = pajeSubVariableId;
  event->event.timestamp = timestamp;

  //print it
  event->print (event);

  //destroy it
  xbt_free (event);
  event = NULL;
}

void new_pajeSetState (double timestamp, container_t container, type_t type, const char *value)
{
  setState_t event = xbt_new0(s_setState_t, 1);
  event->type = type;
  event->container = container;
  event->value = xbt_strdup(value);
  event->print = print_pajeSetState;
  event->event.id = pajeSetStateId;
  event->event.timestamp = timestamp;

  //print it
  event->print (event);

  //destroy it
  xbt_free (event->value);
  xbt_free (event);
  event = NULL;
}


void new_pajePushState (double timestamp, container_t container, type_t type, const char *value)
{
  pushState_t event = xbt_new0(s_pushState_t, 1);
  event->type = type;
  event->container = container;
  event->value = xbt_strdup(value);
  event->print = print_pajePushState;
  event->event.id = pajePushStateId;
  event->event.timestamp = timestamp;

  //print it
  event->print (event);

  //destroy it
  xbt_free (event->value);
  xbt_free (event);
  event = NULL;
}


void new_pajePopState (double timestamp, container_t container, type_t type)
{
  popState_t event = xbt_new0(s_popState_t, 1);
  event->type = type;
  event->container = container;
  event->print = print_pajePopState;
  event->event.id = pajePopStateId;
  event->event.timestamp = timestamp;

  //print it
  event->print (event);

  //destroy it
  xbt_free (event);
  event = NULL;
}

void new_pajeStartLink (double timestamp, container_t container, type_t type, container_t sourceContainer, const char *value, const char *key)
{
  startLink_t event = xbt_new0(s_startLink_t, 1);
  event->type = type;
  event->container = container;
  event->sourceContainer = sourceContainer;
  event->value = xbt_strdup (value);
  event->key = xbt_strdup (key);
  event->print = print_pajeStartLink;
  event->event.id = pajeStartLinkId;
  event->event.timestamp = timestamp;

  //print it
  event->print (event);

  //destroy it
  xbt_free (event->value);
  xbt_free (event->key);
  xbt_free (event);
  event = NULL;
}

void new_pajeEndLink (double timestamp, container_t container, type_t type, container_t destContainer, const char *value, const char *key)
{
  endLink_t event = xbt_new0(s_endLink_t, 1);
  event->type = type;
  event->container = container;
  event->destContainer = destContainer;
  event->value = xbt_strdup (value);
  event->key = xbt_strdup (key);
  event->print = print_pajeEndLink;
  event->event.id = pajeEndLinkId;
  event->event.timestamp = timestamp;

  //print it
  event->print (event);

  //destroy it
  xbt_free (event->value);
  xbt_free (event->key);
  xbt_free (event);
  event = NULL;
}

void new_pajeNewEvent (double timestamp, container_t container, type_t type, const char *value)
{
  newEvent_t event = xbt_new0(s_newEvent_t, 1);
  event->type = type;
  event->container = container;
  event->value = xbt_strdup (value);
  event->print = print_pajeNewEvent;
  event->event.id = pajeNewEventId;
  event->event.timestamp = timestamp;

  //print it
  event->print (event);

  //destroy it
  xbt_free (event->value);
  xbt_free (event);
  event = NULL;
}
//
//void pajeNewEvent(double time, const char *entityType,
//                  const char *container, const char *value)
//{
//  INSTR_PAJE_ASSERT(entityType);
//  INSTR_PAJE_ASSERT(container);
//  INSTR_PAJE_ASSERT(value);
//
//  if (time == 0){
//    fprintf(tracing_file, "%d 0 %s %s %s\n", pajeNewEventId,
//          entityType, container, value);
//  }else{
//    fprintf(tracing_file, "%d %lf %s %s %s\n", pajeNewEventId, time,
//          entityType, container, value);
//  }
//}

#endif /* HAVE_TRACING */
