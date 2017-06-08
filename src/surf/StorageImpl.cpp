/* Copyright (c) 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "StorageImpl.hpp"

#include "surf_private.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <fstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_storage, surf, "Logging specific to the SURF storage module");

xbt_lib_t storage_lib;
int MSG_STORAGE_LEVEL                           = -1; // Msg storage level
int SURF_STORAGE_LEVEL                          = -1;
simgrid::surf::StorageModel* surf_storage_model = nullptr;

namespace simgrid {
namespace surf {

/*************
 * Callbacks *
 *************/

simgrid::xbt::signal<void(StorageImpl*)> storageCreatedCallbacks;
simgrid::xbt::signal<void(StorageImpl*)> storageDestructedCallbacks;
simgrid::xbt::signal<void(StorageImpl*, int, int)> storageStateChangedCallbacks; // signature: wasOn, isOn
simgrid::xbt::signal<void(StorageAction*, Action::State, Action::State)> storageActionStateChangedCallbacks;

/* List of storages */
std::unordered_map<std::string, StorageImpl*>* StorageImpl::storages =
    new std::unordered_map<std::string, StorageImpl*>();

StorageImpl* StorageImpl::byName(const char* name)
{
  if (storages->find(name) == storages->end())
    return nullptr;
  return storages->at(name);
}

/*********
 * Model *
 *********/

StorageModel::StorageModel() : Model()
{
  maxminSystem_ = lmm_system_new(true /* lazy update */);
}

StorageModel::~StorageModel()
{
  lmm_system_free(maxminSystem_);
  surf_storage_model = nullptr;
}

/************
 * Resource *
 ************/

StorageImpl::StorageImpl(Model* model, const char* name, lmm_system_t maxminSystem, double bread, double bwrite,
                         const char* type_id, const char* content_name, sg_size_t size, const char* attach)
    : Resource(model, name, lmm_constraint_new(maxminSystem, this, MAX(bread, bwrite)))
    , piface_(this)
    , size_(size)
    , usedSize_(0)
    , typeId_(xbt_strdup(type_id))
    , writeActions_(std::vector<StorageAction*>())
{
  content_ = parseContent(content_name);
  attach_  = xbt_strdup(attach);
  turnOn();
  XBT_DEBUG("Create resource with Bread '%f' Bwrite '%f' and Size '%llu'", bread, bwrite, size);
  constraintRead_  = lmm_constraint_new(maxminSystem, this, bread);
  constraintWrite_ = lmm_constraint_new(maxminSystem, this, bwrite);
  storages->insert({name, this});
}

StorageImpl::~StorageImpl()
{
  storageDestructedCallbacks(this);
  if (content_ != nullptr) {
    for (auto entry : *content_)
      delete entry.second;
    delete content_;
  }
  free(typeId_);
  free(attach_);
}

std::map<std::string, sg_size_t*>* StorageImpl::parseContent(const char* filename)
{
  usedSize_ = 0;
  if ((not filename) || (strcmp(filename, "") == 0))
    return nullptr;

  std::map<std::string, sg_size_t*>* parse_content = new std::map<std::string, sg_size_t*>();

  std::ifstream* fs = surf_ifsopen(filename);

  std::string line;
  std::vector<std::string> tokens;
  do {
    std::getline(*fs, line);
    boost::trim(line);
    if (line.length() > 0) {
      boost::split(tokens, line, boost::is_any_of(" \t"), boost::token_compress_on);
      xbt_assert(tokens.size() == 2, "Parse error in %s: %s", filename, line.c_str());
      sg_size_t size = std::stoull(tokens.at(1));

      usedSize_ += size;
      sg_size_t* psize = new sg_size_t;
      *psize           = size;
      parse_content->insert({tokens.front(), psize});
    }
  } while (not fs->eof());
  delete fs;
  return parse_content;
}

bool StorageImpl::isUsed()
{
  THROW_UNIMPLEMENTED;
  return false;
}

void StorageImpl::apply_event(tmgr_trace_event_t /*event*/, double /*value*/)
{
  THROW_UNIMPLEMENTED;
}

void StorageImpl::turnOn()
{
  if (isOff()) {
    Resource::turnOn();
    storageStateChangedCallbacks(this, 0, 1);
  }
}
void StorageImpl::turnOff()
{
  if (isOn()) {
    Resource::turnOff();
    storageStateChangedCallbacks(this, 1, 0);
  }
}

std::map<std::string, sg_size_t*>* StorageImpl::getContent()
{
  /* For the moment this action has no cost, but in the future we could take in account access latency of the disk */
  return content_;
}

sg_size_t StorageImpl::getFreeSize()
{
  return size_ - usedSize_;
}

sg_size_t StorageImpl::getUsedSize()
{
  return usedSize_;
}

/**********
 * Action *
 **********/
StorageAction::StorageAction(Model* model, double cost, bool failed, StorageImpl* storage,
                             e_surf_action_storage_type_t type)
    : Action(model, cost, failed), type_(type), storage_(storage), file_(nullptr)
{
  progress_ = 0;
};

StorageAction::StorageAction(Model* model, double cost, bool failed, lmm_variable_t var, StorageImpl* storage,
                             e_surf_action_storage_type_t type)
    : Action(model, cost, failed, var), type_(type), storage_(storage), file_(nullptr)
{
  progress_ = 0;
}

void StorageAction::setState(Action::State state)
{
  Action::State old = getState();
  Action::setState(state);
  storageActionStateChangedCallbacks(this, old, state);
}
}
}
