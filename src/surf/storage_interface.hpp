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

class StorageLmm;
typedef StorageLmm *StorageLmmPtr;

class StorageAction;
typedef StorageAction *StorageActionPtr;

class StorageActionLmm;
typedef StorageActionLmm *StorageActionLmmPtr;

/*********
 * Model *
 *********/
class StorageModel : public Model {
public:
  StorageModel();
  ~StorageModel();
  virtual StoragePtr createResource(const char* id, const char* type_id,
		   const char* content_name, const char* content_type, xbt_dict_t properties)=0;

  xbt_dynar_t p_storageList;
};

/************
 * Resource *
 ************/

class Storage : virtual public Resource {
public:
  Storage(const char* type_id, char *content_name, char *content_type, sg_size_t size);
  ~Storage();

  bool isUsed();
  void updateState(tmgr_trace_event_t event_type, double value, double date);

  xbt_dict_t p_content;
  char* p_contentType;
  sg_size_t m_size;
  sg_size_t m_usedSize;
  char * p_typeId;

  virtual StorageActionPtr open(const char* mount, const char* path)=0;
  virtual StorageActionPtr close(surf_file_t fd)=0;
  //virtual StorageActionPtr unlink(surf_file_t fd)=0;
  virtual StorageActionPtr ls(const char *path)=0;
  virtual StorageActionPtr read(surf_file_t fd, sg_size_t size)=0;
  virtual StorageActionPtr write(surf_file_t fd, sg_size_t size)=0;
  virtual void rename(const char *src, const char *dest)=0;

  virtual xbt_dict_t getContent();
  virtual sg_size_t getSize();

  xbt_dict_t parseContent(char *filename);

  xbt_dynar_t p_writeActions;
};

class StorageLmm : public ResourceLmm, public Storage {
public:
  StorageLmm(lmm_system_t maxminSystem, double bread, double bwrite, double bconnection,
		     const char* type_id, char *content_name, char *content_type, sg_size_t size);

  lmm_constraint_t p_constraintWrite;    /* Constraint for maximum write bandwidth*/
  lmm_constraint_t p_constraintRead;     /* Constraint for maximum write bandwidth*/
};

/**********
 * Action *
 **********/

typedef enum {
  READ=0, WRITE, STAT, OPEN, CLOSE, LS
} e_surf_action_storage_type_t;


class StorageAction : virtual public Action {
public:
  StorageAction() : m_type(READ) {};//FIXME:REMOVE
  StorageAction(StoragePtr storage, e_surf_action_storage_type_t type);

  e_surf_action_storage_type_t m_type;
  StoragePtr p_storage;
  surf_file_t p_file;
  xbt_dict_t p_lsDict;
};

class StorageActionLmm : public ActionLmm, public StorageAction {
public:
  StorageActionLmm() {};//FIXME:REMOVE

  StorageActionLmm(StorageLmmPtr storage, e_surf_action_storage_type_t type);
};

typedef struct s_storage_type {
  char *model;
  char *content;
  char *content_type;
  char *type_id;
  xbt_dict_t properties;
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
