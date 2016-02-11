/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>
#include <xbt/signal.hpp>

#include "surf_interface.hpp"
#include "src/surf/PropertyHolder.hpp"

#ifndef STORAGE_INTERFACE_HPP_
#define STORAGE_INTERFACE_HPP_

namespace simgrid {
namespace surf {

/***********
 * Classes *
 ***********/

class StorageModel;
class Storage;
class StorageAction;

/*************
 * Callbacks *
 *************/

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Storage creation *
 * @details Callback functions have the following signature: `void(Storage*)`
 */
XBT_PUBLIC_DATA(simgrid::xbt::signal<void(simgrid::surf::Storage*)>) storageCreatedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Storage destruction *
 * @details Callback functions have the following signature: `void(StoragePtr)`
 */
XBT_PUBLIC_DATA(simgrid::xbt::signal<void(simgrid::surf::Storage*)>) storageDestructedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Storage State changed *
 * @details Callback functions have the following signature: `void(StorageAction *action, int previouslyOn, int currentlyOn)`
 */
XBT_PUBLIC_DATA(simgrid::xbt::signal<void(simgrid::surf::Storage*, int, int)>) storageStateChangedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after StorageAction State changed *
 * @details Callback functions have the following signature: `void(StorageAction *action, e_surf_action_state_t old, e_surf_action_state_t current)`
 */
XBT_PUBLIC_DATA(simgrid::xbt::signal<void(simgrid::surf::StorageAction*, e_surf_action_state_t, e_surf_action_state_t)>) storageActionStateChangedCallbacks;

/*********
 * Model *
 *********/
/** @ingroup SURF_storage_interface
 * @brief SURF storage model interface class
 * @details A model is an object which handle the interactions between its Resources and its Actions
 */
class StorageModel : public Model {
public:
  StorageModel();
  ~StorageModel();

  /** @brief Create a Storage
   *
   * @param id [description]
   * @param type_id [description]
   * @param content_name [description]
   * @param content_type [description]
   * @param properties [description]
   * @param attach [description]
   * @return The created Storage
   */
  virtual Storage *createStorage(const char* id,
                                    const char* type_id,
                                    const char* content_name,
                                    const char* content_type,
                                    xbt_dict_t properties,
                                    const char *attach) = 0;

  bool next_occuring_event_isIdempotent() {return true;}

  xbt_dynar_t p_storageList;
};

/************
 * Resource *
 ************/
/** @ingroup SURF_storage_interface
 * @brief SURF storage interface class
 * @details A Storage represent a storage unit (e.g.: hard drive, usb key)
 */
class Storage : public simgrid::surf::Resource,
        public simgrid::surf::PropertyHolder {
public:
  /**
   * @brief Storage constructor
   *
   * @param model StorageModel associated to this Storage
   * @param name The name of the Storage
   * @param props Dictionary of properties associated to this Storage
   * @param type_id [description]
   * @param content_name [description]
   * @param content_type [description]
   * @param size [description]
   */
  Storage(Model *model, const char *name, xbt_dict_t props,
          const char* type_id, const char *content_name, const char *content_type,
          sg_size_t size);

  /**
   * @brief Storage constructor
   *
   * @param model StorageModel associated to this Storage
   * @param name The name of the Storage
   * @param props Dictionary of properties associated to this Storage
   * @param maxminSystem [description]
   * @param bread [description]
   * @param bwrite [description]
   * @param bconnection [description]
   * @param type_id [description]
   * @param content_name [description]
   * @param content_type [description]
   * @param size [description]
   * @param attach [description]
   */
  Storage(Model *model, const char *name, xbt_dict_t props,
          lmm_system_t maxminSystem, double bread, double bwrite,
          double bconnection,
          const char* type_id, const char *content_name, const char *content_type,
          sg_size_t size, const char *attach);

  ~Storage();

  /** @brief Check if the Storage is used (if an action currently uses its resources) */
  bool isUsed() override;

  void apply_event(tmgr_trace_iterator_t event, double value) override;

  void turnOn() override;
  void turnOff() override;

  xbt_dict_t p_content;
  char* p_contentType;
  sg_size_t m_size;
  sg_size_t m_usedSize;
  char * p_typeId;
  char* p_attach; //FIXME: this is the name of the host. Use the host directly

  /**
   * @brief Open a file
   *
   * @param mount The mount point
   * @param path The path to the file
   *
   * @return The StorageAction corresponding to the opening
   */
  virtual StorageAction *open(const char* mount, const char* path)=0;

  /**
   * @brief Close a file
   *
   * @param fd The file descriptor to close
   * @return The StorageAction corresponding to the closing
   */
  virtual StorageAction *close(surf_file_t fd)=0;

  /**
   * @brief Read a file
   *
   * @param fd The file descriptor to read
   * @param size The size in bytes to read
   * @return The StorageAction corresponding to the reading
   */
  virtual StorageAction *read(surf_file_t fd, sg_size_t size)=0;

  /**
   * @brief Write a file
   *
   * @param fd The file descriptor to write
   * @param size The size in bytes to write
   * @return The StorageAction corresponding to the writing
   */
  virtual StorageAction *write(surf_file_t fd, sg_size_t size)=0;

  /**
   * @brief Get the content of the current Storage
   *
   * @return A xbt_dict_t with path as keys and size in bytes as values
   */
  virtual xbt_dict_t getContent();

  /**
   * @brief Get the size in bytes of the current Storage
   *
   * @return The size in bytes of the current Storage
   */
  virtual sg_size_t getSize();

  /**
   * @brief Get the available size in bytes of the current Storage
   *
   * @return The available size in bytes of the current Storage
   */
  virtual sg_size_t getFreeSize();

  /**
   * @brief Get the used size in bytes of the current Storage
   *
   * @return The used size in bytes of the current Storage
   */
  virtual sg_size_t getUsedSize();


  xbt_dict_t parseContent(const char *filename);

  xbt_dynar_t p_writeActions;

  lmm_constraint_t p_constraintWrite;    /* Constraint for maximum write bandwidth*/
  lmm_constraint_t p_constraintRead;     /* Constraint for maximum write bandwidth*/
};

/**********
 * Action *
 **********/

/** @ingroup SURF_storage_interface
 * @brief The possible type of action for the storage component
 */
typedef enum {
  READ=0, /**< Read a file */
  WRITE,  /**< Write in a file */
  STAT,   /**< Stat a file */
  OPEN,   /**< Open a file */
  CLOSE  /**< Close a file */
} e_surf_action_storage_type_t;

/** @ingroup SURF_storage_interface
 * @brief SURF storage action interface class
 */
class StorageAction : public Action {
public:
  /**
   * @brief StorageAction constructor
   *
   * @param model The StorageModel associated to this StorageAction
   * @param cost The cost of this  NetworkAction in [TODO]
   * @param failed [description]
   * @param storage The Storage associated to this StorageAction
   * @param type [description]
   */
  StorageAction(Model *model, double cost, bool failed, Storage *storage,
      e_surf_action_storage_type_t type);

    /**
   * @brief StorageAction constructor
   *
   * @param model The StorageModel associated to this StorageAction
   * @param cost The cost of this  StorageAction in [TODO]
   * @param failed [description]
   * @param var The lmm variable associated to this StorageAction if it is part of a LMM component
   * @param storage The Storage associated to this StorageAction
   * @param type [description]
   */
  StorageAction(Model *model, double cost, bool failed, lmm_variable_t var,
      Storage *storage, e_surf_action_storage_type_t type);

  void setState(e_surf_action_state_t state);

  e_surf_action_storage_type_t m_type;
  Storage *p_storage;
  surf_file_t p_file;
  double progress;
};

}
}

typedef struct s_storage_type {
  char *model;
  char *content;
  char *content_type;
  char *type_id;
  xbt_dict_t properties;
  xbt_dict_t model_properties;
  sg_size_t size;
} s_storage_type_t, *storage_type_t;

typedef struct s_mount {
  void *storage;
  char *name;
} s_mount_t, *mount_t;

typedef struct surf_file {
  char *name;
  char *mount;
  sg_size_t size;
  sg_size_t current_position;
} s_surf_file_t;


#endif /* STORAGE_INTERFACE_HPP_ */
