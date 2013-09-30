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

class Storage : public ResourceLmm {
public:
  Storage(StorageModelPtr model, const char* name, xbt_dict_t properties);

  bool isUsed();
  void updateState(tmgr_trace_event_t event_type, double value, double date);

  lmm_constraint_t p_constraintWrite;    /* Constraint for maximum write bandwidth*/
  lmm_constraint_t p_constraintRead;     /* Constraint for maximum write bandwidth*/
  xbt_dict_t p_content; /* char * -> s_surf_file_t */

  StorageActionLmmPtr open(const char* mount, const char* path);
  StorageActionLmmPtr close(surf_file_t fd);
  StorageActionLmmPtr unlink(surf_file_t fd);
  StorageActionLmmPtr ls(const char *path);
  size_t getSize(surf_file_t fd);
  StorageActionLmmPtr read(void* ptr, size_t size, surf_file_t fd);//FIXME:why we have a useless param ptr ??
  StorageActionLmmPtr write(const void* ptr, size_t size, surf_file_t fd);//FIXME:why we have a useless param ptr ??

  size_t m_size;
  size_t m_usedSize;
  xbt_dynar_t p_writeActions;
};

/**********
 * Action *
 **********/

typedef enum {
  READ=0, WRITE, STAT, OPEN, CLOSE, LS
} e_surf_action_storage_type_t;

class StorageActionLmm : public ActionLmm {
public:
  StorageActionLmm(){};
  StorageActionLmm(ModelPtr model, double cost, bool failed, StoragePtr storage, e_surf_action_storage_type_t type);

  int unref();
  void cancel();
  //FIXME:??void recycle();
  void suspend();
  void resume();
  bool isSuspended();
  void setMaxDuration(double duration);
  void setPriority(double priority);

  e_surf_action_storage_type_t m_type;
  StoragePtr p_storage;
  surf_file_t p_file;
  xbt_dict_t p_lsDict;
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
