/* Copyright (c) 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "StorageImpl.hpp"

#include "surf_private.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <fstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_storage, surf, "Logging specific to the SURF storage module");

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

StorageImpl* StorageImpl::byName(std::string name)
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

StorageImpl::StorageImpl(Model* model, std::string name, lmm_system_t maxminSystem, double bread, double bwrite,
                         std::string type_id, std::string content_name, sg_size_t size, std::string attach)
    : Resource(model, name.c_str(), lmm_constraint_new(maxminSystem, this, MAX(bread, bwrite)))
    , piface_(this)
    , typeId_(type_id)
    , size_(size)
    , attach_(attach)
{
  content_ = parseContent(content_name);
  turnOn();
  XBT_DEBUG("Create resource with Bread '%f' Bwrite '%f' and Size '%llu'", bread, bwrite, size);
  constraintRead_  = lmm_constraint_new(maxminSystem, this, bread);
  constraintWrite_ = lmm_constraint_new(maxminSystem, this, bwrite);
  storages->insert({name, this});
}

StorageImpl::~StorageImpl()
{
  storageDestructedCallbacks(this);
  if (content_ != nullptr)
    delete content_;
}

std::map<std::string, sg_size_t>* StorageImpl::parseContent(std::string filename)
{
  usedSize_ = 0;
  if (filename.empty())
    return nullptr;

  std::map<std::string, sg_size_t>* parse_content = new std::map<std::string, sg_size_t>();

  std::ifstream* fs = surf_ifsopen(filename);

  std::string line;
  std::vector<std::string> tokens;
  do {
    std::getline(*fs, line);
    boost::trim(line);
    if (line.length() > 0) {
      boost::split(tokens, line, boost::is_any_of(" \t"), boost::token_compress_on);
      xbt_assert(tokens.size() == 2, "Parse error in %s: %s", filename.c_str(), line.c_str());
      sg_size_t size = std::stoull(tokens.at(1));

      usedSize_ += size;
      parse_content->insert({tokens.front(), size});
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

std::map<std::string, sg_size_t>* StorageImpl::getContent()
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
void StorageAction::setState(Action::State state)
{
  Action::State old = getState();
  Action::setState(state);
  storageActionStateChangedCallbacks(this, old, state);
}
}
}
