#include "surf.hpp"

#ifndef STORAGE_HPP_
#define STORAGE_HPP_

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
class StorageModel : Model {
public:
  StorageModel();
  ~StorageModel();
  StoragePtr createResource(const char* id, const char* model, const char* type_id, const char* content_name);
  double shareResources(double now);
  void updateActionsState(double now, double delta);

  xbt_dict_t parseContent(char *filename, size_t *used_size);
};

/************
 * Resource *
 ************/

class Storage : virtual public Resource {
public:
  Storage(StorageModelPtr model, const char* name, xbt_dict_t properties);

  bool isUsed();
  void updateState(tmgr_trace_event_t event_type, double value, double date);

  xbt_dict_t p_content; /* char * -> s_surf_file_t */

  virtual StorageActionPtr open(const char* mount, const char* path)=0;
  virtual StorageActionPtr close(surf_file_t fd)=0;
  //virtual StorageActionPtr unlink(surf_file_t fd)=0;
  virtual StorageActionPtr ls(const char *path)=0;
  //virtual size_t getSize(surf_file_t fd);
  virtual StorageActionPtr read(void* ptr, size_t size, surf_file_t fd)=0;//FIXME:why we have a useless param ptr ??
  virtual StorageActionPtr write(const void* ptr, size_t size, surf_file_t fd)=0;//FIXME:why we have a useless param ptr ??

  size_t m_size;
  size_t m_usedSize;
  xbt_dynar_t p_writeActions;
};

class StorageLmm : public ResourceLmm, public Storage {
public:
  StorageLmm(StorageModelPtr model, const char* name, xbt_dict_t properties,
		     lmm_system_t maxminSystem, double bread, double bwrite, double bconnection,
		     xbt_dict_t content, size_t size);

  StorageActionPtr open(const char* mount, const char* path);
  StorageActionPtr close(surf_file_t fd);
  //StorageActionPtr unlink(surf_file_t fd);
  StorageActionPtr ls(const char *path);
  //size_t getSize(surf_file_t fd);
  StorageActionPtr read(void* ptr, size_t size, surf_file_t fd);//FIXME:why we have a useless param ptr ??
  StorageActionPtr write(const void* ptr, size_t size, surf_file_t fd);//FIXME:why we have a useless param ptr ??

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
  StorageAction(){};
  StorageAction(ModelPtr model, double cost, bool failed, StoragePtr storage, e_surf_action_storage_type_t type){};



  e_surf_action_storage_type_t m_type;
  StoragePtr p_storage;
  surf_file_t p_file;
  xbt_dict_t p_lsDict;
};

class StorageActionLmm : public ActionLmm, public StorageAction {
public:
  StorageActionLmm(){};
  StorageActionLmm(ModelPtr model, double cost, bool failed, StorageLmmPtr storage, e_surf_action_storage_type_t type);
  void suspend();
  int unref();
  void cancel();
  //FIXME:??void recycle();
  void resume();
  bool isSuspended();
  void setMaxDuration(double duration);
  void setPriority(double priority);

};


typedef struct s_storage_type {
  char *model;
  char *content;
  char *type_id;
  xbt_dict_t properties;
  size_t size;
} s_storage_type_t, *storage_type_t;

typedef struct s_mount {
  void *id;
  char *name;
} s_mount_t, *mount_t;

typedef struct surf_file {
  char *name;
  char *storage;
  size_t size;
} s_surf_file_t;


#endif /* STORAGE_HPP_ */
