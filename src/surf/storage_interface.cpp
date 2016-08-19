/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "storage_interface.hpp"
#include "surf_private.h"
#include "xbt/file.h" /* xbt_getline */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_storage, surf, "Logging specific to the SURF storage module");

xbt_lib_t file_lib;
xbt_lib_t storage_lib;
int ROUTING_STORAGE_LEVEL = -1;      //Routing for storagelevel
int ROUTING_STORAGE_HOST_LEVEL = -1;
int SURF_STORAGE_LEVEL = -1;
xbt_lib_t storage_type_lib;
int ROUTING_STORAGE_TYPE_LEVEL = -1; //Routing for storage_type level
simgrid::surf::StorageModel *surf_storage_model = nullptr;

namespace simgrid {
namespace surf {

/*************
 * Callbacks *
 *************/

simgrid::xbt::signal<void(Storage*)> storageCreatedCallbacks;
simgrid::xbt::signal<void(Storage*)> storageDestructedCallbacks;
simgrid::xbt::signal<void(Storage*, int, int)> storageStateChangedCallbacks; // signature: wasOn, isOn
simgrid::xbt::signal<void(StorageAction*, Action::State, Action::State)> storageActionStateChangedCallbacks;

/*********
 * Model *
 *********/

StorageModel::StorageModel(): Model()
{
}

StorageModel::~StorageModel(){
  lmm_system_free(maxminSystem_);
  surf_storage_model = nullptr;
}

/************
 * Resource *
 ************/

Storage::Storage(Model *model, const char *name, xbt_dict_t props,
                 const char* type_id, const char *content_name, const char *content_type,
                 sg_size_t size)
 : Resource(model, name)
 , PropertyHolder(props)
 , contentType_(xbt_strdup(content_type))
 , size_(size), usedSize_(0)
 , typeId_(xbt_strdup(type_id))
 , writeActions_(std::vector<StorageAction*>())
{
  content_ = parseContent(content_name);
  turnOn();
}

Storage::Storage(Model *model, const char *name, xbt_dict_t props,
                 lmm_system_t maxminSystem, double bread, double bwrite,
                 double bconnection, const char* type_id, const char *content_name,
                 const char *content_type, sg_size_t size, const char *attach)
 : Resource(model, name, lmm_constraint_new(maxminSystem, this, bconnection))
 , PropertyHolder(props)
 , contentType_(xbt_strdup(content_type))
 , size_(size), usedSize_(0)
 , typeId_(xbt_strdup(type_id))
 , writeActions_(std::vector<StorageAction*>())
{
  content_ = parseContent(content_name);
  attach_ = xbt_strdup(attach);
  turnOn();
  XBT_DEBUG("Create resource with Bconnection '%f' Bread '%f' Bwrite '%f' and Size '%llu'", bconnection, bread, bwrite, size);
  constraintRead_  = lmm_constraint_new(maxminSystem, this, bread);
  constraintWrite_ = lmm_constraint_new(maxminSystem, this, bwrite);
}

Storage::~Storage(){
  storageDestructedCallbacks(this);
  xbt_dict_free(&content_);
  free(typeId_);
  free(contentType_);
  free(attach_);
}

xbt_dict_t Storage::parseContent(const char *filename)
{
  usedSize_ = 0;
  if ((!filename) || (strcmp(filename, "") == 0))
    return nullptr;

  xbt_dict_t parse_content = xbt_dict_new_homogeneous(xbt_free_f);

  FILE *file =  surf_fopen(filename, "r");
  xbt_assert(file, "Cannot open file '%s' (path=%s)", filename, xbt_str_join(surf_path, ":"));

  char *line = nullptr;
  size_t len = 0;
  ssize_t read;
  char path[1024];
  sg_size_t size;

  while ((read = xbt_getline(&line, &len, file)) != -1) {
    if (read){
      xbt_assert(sscanf(line,"%s %llu", path, &size) == 2, "Parse error in %s: %s",filename,line);

      usedSize_ += size;
      sg_size_t *psize = xbt_new(sg_size_t, 1);
      *psize = size;
      xbt_dict_set(parse_content,path,psize,nullptr);
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

  xbt_dict_t content_dict = xbt_dict_new_homogeneous(nullptr);
  xbt_dict_cursor_t cursor = nullptr;
  char *file;
  sg_size_t *psize;

  xbt_dict_foreach(content_, cursor, file, psize){
    xbt_dict_set(content_dict,file,psize,nullptr);
  }
  return content_dict;
}

sg_size_t Storage::getSize(){
  return size_;
}

sg_size_t Storage::getFreeSize(){
  return size_ - usedSize_;
}

sg_size_t Storage::getUsedSize(){
  return usedSize_;
}

/**********
 * Action *
 **********/
StorageAction::StorageAction(Model *model, double cost, bool failed,
                             Storage *storage, e_surf_action_storage_type_t type)
: Action(model, cost, failed)
, m_type(type), p_storage(storage), p_file(nullptr){
  progress = 0;
};

StorageAction::StorageAction(Model *model, double cost, bool failed, lmm_variable_t var,
                             Storage *storage, e_surf_action_storage_type_t type)
  : Action(model, cost, failed, var)
  , m_type(type), p_storage(storage), p_file(nullptr){
  progress = 0;
}

void StorageAction::setState(Action::State state){
  Action::State old = getState();
  Action::setState(state);
  storageActionStateChangedCallbacks(this, old, state);
}

}
}
