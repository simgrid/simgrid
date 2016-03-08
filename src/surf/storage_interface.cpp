/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "storage_interface.hpp"
#include "surf_private.h"
#include "xbt/file.h" /* xbt_getline */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_storage, surf,
                                "Logging specific to the SURF storage module");

xbt_lib_t file_lib;
xbt_lib_t storage_lib;
int ROUTING_STORAGE_LEVEL;      //Routing for storagelevel
int ROUTING_STORAGE_HOST_LEVEL;
int SURF_STORAGE_LEVEL;
xbt_lib_t storage_type_lib;
int ROUTING_STORAGE_TYPE_LEVEL; //Routing for storage_type level
simgrid::surf::StorageModel *surf_storage_model = NULL;

namespace simgrid {
namespace surf {

/*************
 * Callbacks *
 *************/

simgrid::xbt::signal<void(simgrid::surf::Storage*)> storageCreatedCallbacks;
simgrid::xbt::signal<void(simgrid::surf::Storage*)> storageDestructedCallbacks;
simgrid::xbt::signal<void(simgrid::surf::Storage*, int, int)> storageStateChangedCallbacks; // signature: wasOn, isOn
simgrid::xbt::signal<void(simgrid::surf::StorageAction*, e_surf_action_state_t, e_surf_action_state_t)> storageActionStateChangedCallbacks;

/*********
 * Model *
 *********/

StorageModel::StorageModel()
  : Model()
{
  p_storageList = NULL;
}

StorageModel::~StorageModel(){
  lmm_system_free(maxminSystem_);

  surf_storage_model = NULL;

  xbt_dynar_free(&p_storageList);
}

/************
 * Resource *
 ************/

Storage::Storage(Model *model, const char *name, xbt_dict_t props,
                 const char* type_id, const char *content_name, const char *content_type,
                 sg_size_t size)
 : Resource(model, name)
 , PropertyHolder(props)
 , p_contentType(xbt_strdup(content_type))
 , m_size(size), m_usedSize(0)
 , p_typeId(xbt_strdup(type_id))
 , p_writeActions(xbt_dynar_new(sizeof(Action*),NULL))
{
  p_content = parseContent(content_name);
  turnOn();
}

Storage::Storage(Model *model, const char *name, xbt_dict_t props,
                 lmm_system_t maxminSystem, double bread, double bwrite,
                 double bconnection, const char* type_id, const char *content_name,
                 const char *content_type, sg_size_t size, const char *attach)
 : Resource(model, name, lmm_constraint_new(maxminSystem, this, bconnection))
 , PropertyHolder(props)
 , p_contentType(xbt_strdup(content_type))
 , m_size(size), m_usedSize(0)
 , p_typeId(xbt_strdup(type_id))
 , p_writeActions(xbt_dynar_new(sizeof(Action*),NULL))
{
  p_content = parseContent(content_name);
  p_attach = xbt_strdup(attach);
  turnOn();
  XBT_DEBUG("Create resource with Bconnection '%f' Bread '%f' Bwrite '%f' and Size '%llu'", bconnection, bread, bwrite, size);
  p_constraintRead  = lmm_constraint_new(maxminSystem, this, bread);
  p_constraintWrite = lmm_constraint_new(maxminSystem, this, bwrite);
}

Storage::~Storage(){
  storageDestructedCallbacks(this);
  xbt_dict_free(&p_content);
  xbt_dynar_free(&p_writeActions);
  free(p_typeId);
  free(p_contentType);
  free(p_attach);
}

xbt_dict_t Storage::parseContent(const char *filename)
{
  m_usedSize = 0;
  if ((!filename) || (strcmp(filename, "") == 0))
    return NULL;

  xbt_dict_t parse_content = xbt_dict_new_homogeneous(xbt_free_f);

  FILE *file =  surf_fopen(filename, "r");
  xbt_assert(file, "Cannot open file '%s' (path=%s)", filename, xbt_str_join(surf_path, ":"));

  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  char path[1024];
  sg_size_t size;

  while ((read = xbt_getline(&line, &len, file)) != -1) {
    if (read){
      xbt_assert(sscanf(line,"%s %llu", path, &size) == 2, "Parse error in %s: %s",filename,line);

      m_usedSize += size;
      sg_size_t *psize = xbt_new(sg_size_t, 1);
      *psize = size;
      xbt_dict_set(parse_content,path,psize,NULL);
    }
  }
  free(line);
  fclose(file);
  return parse_content;
}

bool Storage::isUsed()
{
  THROW_UNIMPLEMENTED;
  return false;
}

void Storage::apply_event(tmgr_trace_iterator_t /*event*/, double /*value*/)
{
  THROW_UNIMPLEMENTED;
}

void Storage::turnOn() {
  if (isOff()) {
    Resource::turnOn();
    storageStateChangedCallbacks(this, 0, 1);
  }
}
void Storage::turnOff() {
  if (isOn()) {
    Resource::turnOff();
    storageStateChangedCallbacks(this, 1, 0);
  }
}

xbt_dict_t Storage::getContent()
{
  /* For the moment this action has no cost, but in the future we could take in account access latency of the disk */

  xbt_dict_t content_dict = xbt_dict_new_homogeneous(NULL);
  xbt_dict_cursor_t cursor = NULL;
  char *file;
  sg_size_t *psize;

  xbt_dict_foreach(p_content, cursor, file, psize){
    xbt_dict_set(content_dict,file,psize,NULL);
  }
  return content_dict;
}

sg_size_t Storage::getSize(){
  return m_size;
}

sg_size_t Storage::getFreeSize(){
  return m_size - m_usedSize;
}

sg_size_t Storage::getUsedSize(){
  return m_usedSize;
}

/**********
 * Action *
 **********/
StorageAction::StorageAction(Model *model, double cost, bool failed,
                             Storage *storage, e_surf_action_storage_type_t type)
: Action(model, cost, failed)
, m_type(type), p_storage(storage), p_file(NULL){
  progress = 0;
};

StorageAction::StorageAction(Model *model, double cost, bool failed, lmm_variable_t var,
                             Storage *storage, e_surf_action_storage_type_t type)
  : Action(model, cost, failed, var)
  , m_type(type), p_storage(storage), p_file(NULL){
  progress = 0;
}

void StorageAction::setState(e_surf_action_state_t state){
  e_surf_action_state_t old = getState();
  Action::setState(state);
  storageActionStateChangedCallbacks(this, old, state);
}

}
}
