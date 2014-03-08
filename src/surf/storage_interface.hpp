/* Copyright (c) 2004-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_interface.hpp"

#ifndef STORAGE_INTERFACE_HPP_
#define STORAGE_INTERFACE_HPP_

extern xbt_dynar_t mount_list;

/***********
 * Classes *
 ***********/

class StorageModel;
typedef StorageModel *StorageModelPtr;

class Storage;
typedef Storage *StoragePtr;

class Storage;
typedef Storage *StoragePtr;

class StorageAction;
typedef StorageAction *StorageActionPtr;

class StorageAction;
typedef StorageAction *StorageActionPtr;

/*************
 * Callbacks *
 *************/

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Storage creation *
 * @details Callback functions have the following signature: `void(StoragePtr)`
 */
extern surf_callback(void, StoragePtr) storageCreatedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Storage destruction *
 * @details Callback functions have the following signature: `void(StoragePtr)`
 */
extern surf_callback(void, StoragePtr) storageDestructedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Storage State changed *
 * @details Callback functions have the following signature: `void(StorageActionPtr)`
 */
extern surf_callback(void, StoragePtr) storageStateChangedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after StorageAction State changed *
 * @details Callback functions have the following signature: `void(StorageActionPtr)`
 */
extern surf_callback(void, StorageActionPtr) storageActionStateChangedCallbacks;

/*********
 * Model *
 *********/
/** @ingroup SURF_storage_interface
 * @brief SURF storage model interface class
 * @details A model is an object which handle the interactions between its Resources and its Actions
 */
class StorageModel : public Model {
public:
  /**
   * @brief The storage model constructor
   */
  StorageModel();

  /**
   * @brief The Storange model destructor
   */
  ~StorageModel();

  /**
   * @brief Create a Storage
   * 
   * @param id [description]
   * @param type_id [description]
   * @param content_name [description]
   * @param content_type [description]
   * @param properties [description]
   * @return The created Storage
   */
  virtual StoragePtr createResource(const char* id, const char* type_id,
		   const char* content_name, const char* content_type, xbt_dict_t properties,const char *attach)=0;

  xbt_dynar_t p_storageList;
};

/************
 * Resource *
 ************/
/** @ingroup SURF_storage_interface
 * @brief SURF storage interface class
 * @details A Storage represent a storage unit (e.g.: hard drive, usb key)
 */
class Storage : public Resource {
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
  Storage(ModelPtr model, const char *name, xbt_dict_t props,
          const char* type_id, char *content_name, char *content_type,
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
   */
  Storage(ModelPtr model, const char *name, xbt_dict_t props,
          lmm_system_t maxminSystem, double bread, double bwrite,
          double bconnection,
          const char* type_id, char *content_name, char *content_type,
          sg_size_t size, char *attach);

  /**
   * @brief Storage destructor
   */
  ~Storage();

  /**
   * @brief Check if the Storage is used
   * 
   * @return true if the current Storage is used, false otherwise
   */
  bool isUsed();

  /**
   * @brief Update the state of the current Storage
   * 
   * @param event_type [description]
   * @param value [description]
   * @param date [description]
   */
  void updateState(tmgr_trace_event_t event_type, double value, double date);

  void setState(e_surf_resource_state_t state);

  xbt_dict_t p_content;
  char* p_contentType;
  sg_size_t m_size;
  sg_size_t m_usedSize;
  char * p_typeId;
  char* p_attach;

  /**
   * @brief Open a file
   * 
   * @param mount The mount point
   * @param path The path to the file
   * 
   * @return The StorageAction corresponding to the opening
   */
  virtual StorageActionPtr open(const char* mount, const char* path)=0;

  /**
   * @brief Close a file
   * 
   * @param fd The file descriptor to close
   * @return The StorageAction corresponding to the closing
   */
  virtual StorageActionPtr close(surf_file_t fd)=0;

  /**
   * @brief List directory contents of a path
   * @details [long description]
   * 
   * @param path The path to the directory
   * @return The StorageAction corresponding to the ls action
   */
  virtual StorageActionPtr ls(const char *path)=0;

  /**
   * @brief Read a file
   * 
   * @param fd The file descriptor to read
   * @param size The size in bytes to read
   * @return The StorageAction corresponding to the reading
   */
  virtual StorageActionPtr read(surf_file_t fd, sg_size_t size)=0;

  /**
   * @brief Write a file
   * 
   * @param fd The file descriptor to write
   * @param size The size in bytes to write
   * @return The StorageAction corresponding to the writing
   */
  virtual StorageActionPtr write(surf_file_t fd, sg_size_t size)=0;

  /**
   * @brief Rename a path
   * 
   * @param src The old path
   * @param dest The new path
   */
  virtual void rename(const char *src, const char *dest)=0;

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

  xbt_dict_t parseContent(char *filename);

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
  CLOSE,  /**< Close a file */
  LS      /**< List directory contents */
} e_surf_action_storage_type_t;

/** @ingroup SURF_storage_interface
 * @brief SURF storage action interface class
 */
class StorageAction : public Action {
public:
  /**
   * @brief StorageAction constructor
   */
  StorageAction() : m_type(READ) {};//FIXME:REMOVE

  /**
   * @brief StorageAction constructor
   * 
   * @param model The StorageModel associated to this StorageAction
   * @param cost The cost of this  NetworkAction in [TODO]
   * @param failed [description]
   * @param storage The Storage associated to this StorageAction
   * @param type [description]
   */
  StorageAction(ModelPtr model, double cost, bool failed,
		            StoragePtr storage, e_surf_action_storage_type_t type);

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
  StorageAction(ModelPtr model, double cost, bool failed, lmm_variable_t var,
		            StoragePtr storage, e_surf_action_storage_type_t type);

  void setState(e_surf_action_state_t state);

  e_surf_action_storage_type_t m_type;
  StoragePtr p_storage;
  surf_file_t p_file;
  xbt_dict_t p_lsDict;
};

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
