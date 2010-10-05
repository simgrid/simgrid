/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/private.h"

#ifdef HAVE_TRACING

static FILE *tracing_file = NULL;
int tracing_active = 0;

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
#define UNUSED001 20
#define UNUSED002 21
static int pajeSetVariableId = 22;
static int pajeAddVariableId = 23;
static int pajeSubVariableId = 24;
static int pajeDefineVariableTypeId = 25;
static int pajeStartLinkWithVolumeId = 26;
static int pajeNewEventId = 27;

#define TRACE_LINE_SIZE 1000

void TRACE_paje_start (FILE *file)
{
  tracing_file = file;
  tracing_active = 1;
}

FILE *TRACE_paje_end (void)
{
  tracing_active = 0;
  return tracing_file;
}


void TRACE_paje_create_header(void) {
  if (!tracing_active)
    return;
  fprintf(
      tracing_file,
      "\
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
%%EventDef PajeStartLink %d \n\
%%       Time date \n\
%%       EntityType string \n\
%%       Container string \n\
%%       Value string \n\
%%       SourceContainer string \n\
%%       Key string \n\
%%       Volume string \n\
%%EndEventDef\n\
%%EventDef PajeNewEvent %d \n\
%%       Time date \n\
%%       EntityType string \n\
%%       Container string \n\
%%       Value string \n\
%%EndEventDef\n",
      pajeDefineContainerTypeId, pajeDefineStateTypeId, pajeDefineEntityValueId,
      pajeDefineEventTypeId, pajeDefineLinkTypeId, pajeCreateContainerId,
      pajeDestroyContainerId, pajeSetStateId,
      pajePopStateId,
      pajeStartLinkId, pajeEndLinkId,
      pajePushStateId,
      pajeSetVariableId,
      pajeAddVariableId,
      pajeSubVariableId,
      pajeDefineVariableTypeId,
      pajeStartLinkWithVolumeId,
      pajeNewEventId);
}

/* internal to this file */
static void __pajeCreateContainer (char *output, int len, int eventid, double time, const char *alias, const char *type,
    const char *container, const char *name)
{
  snprintf (output, len, "%d %lf %s %s %s %s", eventid, time,
      alias, type, container, name);
}

static void __pajeSetState (char *output, int len, int eventid, double time, const char *entityType, const char *container, const char *value)
{
  snprintf (output, len, "%d %lf %s %s %s", eventid, time, entityType, container, value);
}

static void __pajeSetVariable (char *output, int len, int eventid, double time, const char *entityType, const char *container, const char *value)
{
  snprintf (output, len, "%d %lf %s %s %s", eventid, time, entityType, container, value);
}

static void __pajeStartLink (char *output, int len, int eventid,  double time, const char *entityType, const char *container, const char *value,
const char *sourceContainer, const char *key)
{
  snprintf(output, len, "%d %lf %s %s %s %s %s", eventid, time, entityType, container, value, sourceContainer, key);
}

/* internal do the instrumentation module */
void pajeDefineContainerType(const char *alias, const char *containerType,
    const char *name) {
  fprintf(tracing_file, "%d %s %s %s\n", pajeDefineContainerTypeId, alias,
      containerType, name);
}

void pajeDefineStateType(const char *alias, const char *containerType,
    const char *name) {
  fprintf(tracing_file, "%d %s %s %s\n", pajeDefineStateTypeId, alias,
      containerType, name);
}

void pajeDefineEventType(const char *alias, const char *containerType,
    const char *name) {
  fprintf(tracing_file, "%d %s %s %s\n", pajeDefineEventTypeId, alias,
      containerType, name);
}

void pajeDefineLinkType(const char *alias, const char *containerType,
    const char *sourceContainerType, const char *destContainerType,
    const char *name) {
  fprintf(tracing_file, "%d %s %s %s %s %s\n", pajeDefineLinkTypeId, alias,
      containerType, sourceContainerType, destContainerType, name);
}

void pajeCreateContainer(double time, const char *alias, const char *type, const char *container, const char *name) {
  char line[TRACE_LINE_SIZE];
  __pajeCreateContainer (line, TRACE_LINE_SIZE, pajeCreateContainerId, time, alias, type, container, name);
  fprintf (tracing_file, "%s\n", line);
}

void pajeDestroyContainer (double time, const char *type, const char *container)
{
  fprintf(tracing_file, "%d %lf %s %s\n", pajeDestroyContainerId, time, type, container);
}

void pajeSetState (double time, const char *entityType, const char *container, const char *value)
{
  char line[TRACE_LINE_SIZE];
  __pajeSetState (line, TRACE_LINE_SIZE, pajeSetStateId, time, entityType, container, value);
  fprintf(tracing_file, "%s\n", line);
}

void pajePushState (double time, const char *entityType, const char *container, const char *value)
{
  char line[TRACE_LINE_SIZE];
  __pajeSetState (line, TRACE_LINE_SIZE, pajePushStateId, time, entityType, container, value);
  fprintf(tracing_file, "%s\n", line);
}

void pajePopState (double time, const char *entityType, const char *container)
{
  fprintf(tracing_file, "%d %lf %s %s\n", pajePopStateId, time, entityType, container);
}

void pajeStartLink (double time, const char *entityType, const char *container, const char *value,
    const char *sourceContainer, const char *key)
{
  char line[TRACE_LINE_SIZE];
  __pajeStartLink (line, TRACE_LINE_SIZE, pajeStartLinkId, time, entityType, container, value, sourceContainer, key);
  fprintf (tracing_file, "%s\n", line);
}

void pajeStartLinkWithVolume (double time, const char *entityType, const char *container, const char *value,
    const char *sourceContainer, const char *key, double volume)
{
  char line[TRACE_LINE_SIZE];
  __pajeStartLink (line, TRACE_LINE_SIZE, pajeStartLinkWithVolumeId, time, entityType, container, value, sourceContainer, key);
  fprintf (tracing_file, "%s %f\n", line, volume);
}

void pajeEndLink (double time, const char *entityType, const char *container, const char *value,
    const char *destContainer, const char *key)
{
  fprintf(tracing_file, "%d %lf %s %s %s %s %s\n", pajeEndLinkId, time, entityType, container, value, destContainer, key);
}

void pajeDefineVariableType(const char *alias, const char *containerType, const char *name) {
  fprintf(tracing_file, "%d %s %s %s\n", pajeDefineVariableTypeId, alias, containerType, name);
}

void pajeSetVariable (double time, const char *entityType, const char *container, const char *value)
{
  char line[TRACE_LINE_SIZE];
  __pajeSetVariable (line, TRACE_LINE_SIZE, pajeSetVariableId, time, entityType, container, value);
  fprintf(tracing_file, "%s\n", line);
}

void pajeAddVariable (double time, const char *entityType, const char *container, const char *value)
{
  char line[TRACE_LINE_SIZE];
  __pajeSetVariable (line, TRACE_LINE_SIZE, pajeAddVariableId, time, entityType, container, value);
  fprintf(tracing_file, "%s\n", line);
}

void pajeSubVariable (double time, const char *entityType, const char *container, const char *value)
{
  char line[TRACE_LINE_SIZE];
  __pajeSetVariable (line, TRACE_LINE_SIZE, pajeSubVariableId, time, entityType, container, value);
  fprintf(tracing_file, "%s\n", line);
}

void pajeNewEvent (double time, const char *entityType, const char *container, const char *value)
{
  fprintf(tracing_file, "%d %lf %s %s %s\n", pajeNewEventId, time, entityType, container, value);
}

#endif
