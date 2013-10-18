#include "storage.hpp"
#include "surf_private.h"

extern "C" {
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_storage, surf,
                                "Logging specific to the SURF storage module");
}

xbt_lib_t storage_lib;
int ROUTING_STORAGE_LEVEL;      //Routing for storagelevel
int ROUTING_STORAGE_HOST_LEVEL;
int SURF_STORAGE_LEVEL;
xbt_lib_t storage_type_lib;
int ROUTING_STORAGE_TYPE_LEVEL; //Routing for storage_type level

static xbt_dynar_t storage_list;

xbt_dynar_t mount_list = NULL;  /* temporary store of current mount storage */
StorageModelPtr surf_storage_model = NULL;

lmm_system_t storage_maxmin_system = NULL;
static int storage_selective_update = 0;
static xbt_swag_t storage_running_action_set_that_does_not_need_being_checked = NULL;

/*************
 * CallBacks *
 *************/

static XBT_INLINE void routing_storage_type_free(void *r)
{
  storage_type_t stype = (storage_type_t) r;
  free(stype->model);
  free(stype->type_id);
  free(stype->content);
  xbt_dict_free(&(stype->properties));
  free(stype);
}

static XBT_INLINE void surf_storage_resource_free(void *r)
{
  // specific to storage
  StoragePtr storage = (StoragePtr) r;
  xbt_dict_free(&storage->p_content);
  xbt_dynar_free(&storage->p_writeActions);
  // generic resource
  delete storage;
}

static XBT_INLINE void routing_storage_host_free(void *r)
{
  xbt_dynar_t dyn = (xbt_dynar_t) r;
  xbt_dynar_free(&dyn);
}

static void parse_storage_init(sg_platf_storage_cbarg_t storage)
{
  void* stype = xbt_lib_get_or_null(storage_type_lib,
                                    storage->type_id,
                                    ROUTING_STORAGE_TYPE_LEVEL);
  if(!stype) xbt_die("No storage type '%s'",storage->type_id);

  // if storage content is not specified use the content of storage_type if exist
  if(!strcmp(storage->content,"") && strcmp(((storage_type_t) stype)->content,"")){
    storage->content = ((storage_type_t) stype)->content;
    XBT_DEBUG("For disk '%s' content is empty, use the content of storage type '%s'",storage->id,((storage_type_t) stype)->type_id);
  }

  XBT_DEBUG("SURF storage create resource\n\t\tid '%s'\n\t\ttype '%s' \n\t\tmodel '%s' \n\t\tcontent '%s'\n\t\tproperties '%p'\n",
      storage->id,
      ((storage_type_t) stype)->model,
      ((storage_type_t) stype)->type_id,
      storage->content,
      ((storage_type_t) stype)->properties);

  surf_storage_model->createResource(storage->id, ((storage_type_t) stype)->model,
                                     ((storage_type_t) stype)->type_id,
                                     storage->content);
}

static void parse_mstorage_init(sg_platf_mstorage_cbarg_t mstorage)
{
  XBT_DEBUG("parse_mstorage_init");
}

static void parse_storage_type_init(sg_platf_storage_type_cbarg_t storagetype_)
{
  XBT_DEBUG("parse_storage_type_init");
}

static void parse_mount_init(sg_platf_mount_cbarg_t mount)
{
  XBT_DEBUG("parse_mount_init");
}

static void storage_parse_storage(sg_platf_storage_cbarg_t storage)
{
  xbt_assert(!xbt_lib_get_or_null(storage_lib, storage->id,ROUTING_STORAGE_LEVEL),
               "Reading a storage, processing unit \"%s\" already exists", storage->id);

  // Verification of an existing type_id
#ifndef NDEBUG
  void* storage_type = xbt_lib_get_or_null(storage_type_lib, storage->type_id,ROUTING_STORAGE_TYPE_LEVEL);
#endif
  xbt_assert(storage_type,"Reading a storage, type id \"%s\" does not exists", storage->type_id);

  XBT_DEBUG("ROUTING Create a storage name '%s' with type_id '%s' and content '%s'",
      storage->id,
      storage->type_id,
      storage->content);

  xbt_lib_set(storage_lib,
      storage->id,
      ROUTING_STORAGE_LEVEL,
      (void *) xbt_strdup(storage->type_id));
}

static xbt_dict_t parse_storage_content(char *filename, size_t *used_size)
{
  *used_size = 0;
  if ((!filename) || (strcmp(filename, "") == 0))
    return NULL;

  xbt_dict_t parse_content = xbt_dict_new_homogeneous(NULL);
  FILE *file = NULL;

  file = surf_fopen(filename, "r");
  xbt_assert(file != NULL, "Cannot open file '%s' (path=%s)", filename,
              xbt_str_join(surf_path, ":"));

  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  char path[1024];
  size_t size;


  while ((read = xbt_getline(&line, &len, file)) != -1) {
    if (read){
    if(sscanf(line,"%s %zu",path, &size)==2) {
        *used_size += size;
        xbt_dict_set(parse_content,path,(void*) size,NULL);
      } else {
        xbt_die("Be sure of passing a good format for content file.\n");
      }
    }
  }
  free(line);
  fclose(file);
  return parse_content;
}

static void storage_parse_storage_type(sg_platf_storage_type_cbarg_t storage_type)
{
  xbt_assert(!xbt_lib_get_or_null(storage_type_lib, storage_type->id,ROUTING_STORAGE_TYPE_LEVEL),
               "Reading a storage type, processing unit \"%s\" already exists", storage_type->id);

  storage_type_t stype = xbt_new0(s_storage_type_t, 1);
  stype->model = xbt_strdup(storage_type->model);
  stype->properties = storage_type->properties;
  stype->content = xbt_strdup(storage_type->content);
  stype->type_id = xbt_strdup(storage_type->id);
  stype->size = storage_type->size * 1000000000; /* storage_type->size is in Gbytes and stype->sizeis in bytes */

  XBT_DEBUG("ROUTING Create a storage type id '%s' with model '%s' content '%s'",
      stype->type_id,
      stype->model,
      storage_type->content);

  xbt_lib_set(storage_type_lib,
      stype->type_id,
      ROUTING_STORAGE_TYPE_LEVEL,
      (void *) stype);
}
static void storage_parse_mstorage(sg_platf_mstorage_cbarg_t mstorage)
{
  THROW_UNIMPLEMENTED;
//  mount_t mnt = xbt_new0(s_mount_t, 1);
//  mnt->id = xbt_strdup(mstorage->type_id);
//  mnt->name = xbt_strdup(mstorage->name);
//
//  if(!mount_list){
//    XBT_DEBUG("Creata a Mount list for %s",A_surfxml_host_id);
//    mount_list = xbt_dynar_new(sizeof(char *), NULL);
//  }
//  xbt_dynar_push(mount_list,(void *) mnt);
//  free(mnt->id);
//  free(mnt->name);
//  xbt_free(mnt);
//  XBT_DEBUG("ROUTING Mount a storage name '%s' with type_id '%s'",mstorage->name, mstorage->id);
}

static void mount_free(void *p)
{
  mount_t mnt = (mount_t) p;
  xbt_free(mnt->name);
}

static void storage_parse_mount(sg_platf_mount_cbarg_t mount)
{
  // Verification of an existing storage
#ifndef NDEBUG
  void* storage = xbt_lib_get_or_null(storage_lib, mount->id,ROUTING_STORAGE_LEVEL);
#endif
  xbt_assert(storage,"Disk id \"%s\" does not exists", mount->id);

  XBT_DEBUG("ROUTING Mount '%s' on '%s'",mount->id, mount->name);

  s_mount_t mnt;
  mnt.id = surf_storage_resource_priv(surf_storage_resource_by_name(mount->id));
  mnt.name = xbt_strdup(mount->name);

  if(!mount_list){
    //FIXME:XBT_DEBUG("Create a Mount list for %s",A_surfxml_host_id);
    mount_list = xbt_dynar_new(sizeof(s_mount_t), mount_free);
  }
  xbt_dynar_push(mount_list,&mnt);
}

static void storage_define_callbacks()
{
  sg_platf_storage_add_cb(parse_storage_init);
  sg_platf_storage_type_add_cb(parse_storage_type_init);
  sg_platf_mstorage_add_cb(parse_mstorage_init);
  sg_platf_mount_add_cb(parse_mount_init);
}

void storage_register_callbacks() {

  ROUTING_STORAGE_LEVEL = xbt_lib_add_level(storage_lib,xbt_free);
  ROUTING_STORAGE_HOST_LEVEL = xbt_lib_add_level(storage_lib, routing_storage_host_free);
  ROUTING_STORAGE_TYPE_LEVEL = xbt_lib_add_level(storage_type_lib, routing_storage_type_free);
  SURF_STORAGE_LEVEL = xbt_lib_add_level(storage_lib, surf_storage_resource_free);

  sg_platf_storage_add_cb(storage_parse_storage);
  sg_platf_mstorage_add_cb(storage_parse_mstorage);
  sg_platf_storage_type_add_cb(storage_parse_storage_type);
  sg_platf_mount_add_cb(storage_parse_mount);
}

/*********
 * Model *
 *********/

void surf_storage_model_init_default(void)
{
  surf_storage_model = new StorageModel();
  storage_define_callbacks();
  xbt_dynar_push(model_list, &surf_storage_model);
}

StorageModel::StorageModel() : Model("Storage"){
  StorageActionLmm action;

  XBT_DEBUG("surf_storage_model_init_internal");

  storage_running_action_set_that_does_not_need_being_checked =
      xbt_swag_new(xbt_swag_offset(action, p_stateHookup));

  if (!storage_maxmin_system) {
    storage_maxmin_system = lmm_system_new(storage_selective_update);
  }
}


StorageModel::~StorageModel(){
  lmm_system_free(storage_maxmin_system);
  storage_maxmin_system = NULL;

  surf_storage_model = NULL;

  xbt_dynar_free(&storage_list);

  xbt_swag_free(storage_running_action_set_that_does_not_need_being_checked);
  storage_running_action_set_that_does_not_need_being_checked = NULL;
}

StoragePtr StorageModel::createResource(const char* id, const char* model, const char* type_id, const char* content_name)
{

  xbt_assert(!surf_storage_resource_priv(surf_storage_resource_by_name(id)),
              "Storage '%s' declared several times in the platform file",
              id);

  storage_type_t storage_type = (storage_type_t) xbt_lib_get_or_null(storage_type_lib, type_id,ROUTING_STORAGE_TYPE_LEVEL);

  double Bread  = atof((char*)xbt_dict_get(storage_type->properties, "Bread"));
  double Bwrite = atof((char*)xbt_dict_get(storage_type->properties, "Bwrite"));
  double Bconnection   = atof((char*)xbt_dict_get(storage_type->properties, "Bconnection"));

  StoragePtr storage = new StorageLmm(this, NULL, NULL, p_maxminSystem,
		  Bread, Bwrite, Bconnection,
		  parseContent((char*)content_name, &(storage->m_usedSize)),
		  storage_type->size);

  xbt_lib_set(storage_lib, id, SURF_STORAGE_LEVEL, storage);

  XBT_DEBUG("SURF storage create resource\n\t\tid '%s'\n\t\ttype '%s' \n\t\tmodel '%s' \n\t\tproperties '%p'\n\t\tBread '%f'\n",
      id,
      model,
      type_id,
      storage_type->properties,
      Bread);

  if(!storage_list) storage_list=xbt_dynar_new(sizeof(char *),NULL);
  xbt_dynar_push(storage_list, &storage);

  return storage;
}

double StorageModel::shareResources(double now)
{
  XBT_DEBUG("storage_share_resources %f", now);
  unsigned int i, j;
  StoragePtr storage;
  StorageActionLmmPtr write_action;

  double min_completion = shareResourcesMaxMin(p_runningActionSet,
      storage_maxmin_system, lmm_solve);

  double rate;
  // Foreach disk
  xbt_dynar_foreach(storage_list,i,storage)
  {
    rate = 0;
    // Foreach write action on disk
    xbt_dynar_foreach(storage->p_writeActions, j, write_action)
    {
      rate += lmm_variable_getvalue(write_action->p_variable);
    }
    if(rate > 0)
      min_completion = MIN(min_completion, (storage->m_size-storage->m_usedSize)/rate);
  }

  return min_completion;
}

void StorageModel::updateActionsState(double now, double delta)
{
  void *_action, *_next_action;
  StorageActionLmmPtr action = NULL;

  // Update the disk usage
  // Update the file size
  // For each action of type write
  xbt_swag_foreach_safe(_action, _next_action, p_runningActionSet) {
	action = (StorageActionLmmPtr) _action;
    if(action->m_type == WRITE)
    {
      double rate = lmm_variable_getvalue(action->p_variable);
      /* Hack to avoid rounding differences between x86 and x86_64
       * (note that the next sizes are of type size_t). */
      long incr = delta * rate + MAXMIN_PRECISION;
      action->p_storage->m_usedSize += incr; // disk usage
      action->p_file->size += incr; // file size
    }
  }

  xbt_swag_foreach_safe(_action, _next_action, p_runningActionSet) {
	action = (StorageActionLmmPtr) _action;

    double_update(&action->m_remains,
                  lmm_variable_getvalue(action->p_variable) * delta);

    if (action->m_maxDuration != NO_MAX_DURATION)
      double_update(&action->m_maxDuration, delta);

    if(action->m_remains > 0 &&
        lmm_get_variable_weight(action->p_variable) > 0 &&
        action->p_storage->m_usedSize == action->p_storage->m_size)
    {
      action->m_finish = surf_get_clock();
      action->setState(SURF_ACTION_FAILED);
    } else if ((action->m_remains <= 0) &&
        (lmm_get_variable_weight(action->p_variable) > 0))
    {
      action->m_finish = surf_get_clock();
      action->setState(SURF_ACTION_DONE);
    } else if ((action->m_maxDuration != NO_MAX_DURATION) &&
               (action->m_maxDuration <= 0))
    {
      action->m_finish = surf_get_clock();
      action->setState(SURF_ACTION_DONE);
    }
  }

  return;
}

xbt_dict_t StorageModel::parseContent(char *filename, size_t *used_size)
{
  *used_size = 0;
  if ((!filename) || (strcmp(filename, "") == 0))
    return NULL;

  xbt_dict_t parse_content = xbt_dict_new_homogeneous(NULL);
  FILE *file = NULL;

  file = surf_fopen(filename, "r");
  xbt_assert(file != NULL, "Cannot open file '%s' (path=%s)", filename,
              xbt_str_join(surf_path, ":"));

  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  char path[1024];
  size_t size;


  while ((read = xbt_getline(&line, &len, file)) != -1) {
    if (read){
    if(sscanf(line,"%s %zu",path, &size)==2) {
        *used_size += size;
        xbt_dict_set(parse_content,path,(void*) size,NULL);
      } else {
        xbt_die("Be sure of passing a good format for content file.\n");
      }
    }
  }
  free(line);
  fclose(file);
  return parse_content;
}

/************
 * Resource *
 ************/

Storage::Storage(StorageModelPtr model, const char* name, xbt_dict_t properties) {
  p_writeActions = xbt_dynar_new(sizeof(char *),NULL);
}

StorageLmm::StorageLmm(StorageModelPtr model, const char* name, xbt_dict_t properties,
	     lmm_system_t maxminSystem, double bread, double bwrite, double bconnection,
	     xbt_dict_t content, size_t size)
 :    ResourceLmm(), Storage(model, name, properties) {
  XBT_DEBUG("Create resource with Bconnection '%f' Bread '%f' Bwrite '%f' and Size '%lu'", bconnection, bread, bwrite, ((unsigned long)size));

  p_constraint = lmm_constraint_new(maxminSystem, this, bconnection);
  p_constraintRead  = lmm_constraint_new(maxminSystem, this, bread);
  p_constraintWrite = lmm_constraint_new(maxminSystem, this, bwrite);
  p_content = content;
  m_size = size;
}

bool Storage::isUsed()
{
  THROW_UNIMPLEMENTED;
  return false;
}

void Storage::updateState(tmgr_trace_event_t event_type, double value, double date)
{
  THROW_UNIMPLEMENTED;
}

StorageActionPtr StorageLmm::ls(const char* path)
{
  StorageActionLmmPtr action = new StorageActionLmm(p_model, 0, p_stateCurrent != SURF_RESOURCE_ON, this, LS);

  action->p_lsDict = NULL;
  xbt_dict_t ls_dict = xbt_dict_new();

  char* key;
  size_t size = 0;
  xbt_dict_cursor_t cursor = NULL;

  xbt_dynar_t dyn = NULL;
  char* file = NULL;

  // for each file in the storage content
  xbt_dict_foreach(p_content,cursor,key,size){
    // Search if file start with the prefix 'path'
    if(xbt_str_start_with(key,path)){
      file = &key[strlen(path)];

      // Split file with '/'
      dyn = xbt_str_split(file,"/");
      file = xbt_dynar_get_as(dyn,0,char*);

      // file
      if(xbt_dynar_length(dyn) == 1){
        xbt_dict_set(ls_dict,file,&size,NULL);
      }
      // Directory
      else
      {
        // if directory does not exist yet in the dictionary
        if(!xbt_dict_get_or_null(ls_dict,file))
          xbt_dict_set(ls_dict,file,NULL,NULL);
      }
      xbt_dynar_free(&dyn);
    }
  }

  action->p_lsDict = ls_dict;
  return action;
}

StorageActionPtr StorageLmm::open(const char* mount, const char* path)
{
  XBT_DEBUG("\tOpen file '%s'",path);
  size_t size = (size_t) xbt_dict_get_or_null(p_content, path);
  // if file does not exist create an empty file
  if(!size){
    xbt_dict_set(p_content, path, &size, NULL);
    XBT_DEBUG("File '%s' was not found, file created.",path);
  }
  surf_file_t file = xbt_new0(s_surf_file_t,1);
  file->name = xbt_strdup(path);
  file->size = size;
  file->storage = xbt_strdup(mount);

  StorageActionLmmPtr action = new StorageActionLmm(p_model, 0, p_stateCurrent != SURF_RESOURCE_ON, this, OPEN);
  action->p_file = file;
  return action;
}

StorageActionPtr StorageLmm::close(surf_file_t fd)
{
  char *filename = fd->name;
  XBT_DEBUG("\tClose file '%s' size '%zu'", filename, fd->size);
  // unref write actions from storage
  StorageActionLmmPtr write_action;
  unsigned int i;
  xbt_dynar_foreach(p_writeActions, i, write_action) {
    if ((write_action->p_file) == fd) {
      xbt_dynar_cursor_rm(p_writeActions, &i);
      write_action->unref();
    }
  }
  free(fd->name);
  free(fd->storage);
  xbt_free(fd);
  StorageActionLmmPtr action = new StorageActionLmm(p_model, 0, p_stateCurrent != SURF_RESOURCE_ON, this, CLOSE);
  return action;
}

StorageActionPtr StorageLmm::read(void* ptr, size_t size, surf_file_t fd)
{
  if(size > fd->size)
    size = fd->size;
  StorageActionLmmPtr action = new StorageActionLmm(p_model, 0, p_stateCurrent != SURF_RESOURCE_ON, this, READ);
  return action;
}

StorageActionPtr StorageLmm::write(const void* ptr, size_t size, surf_file_t fd)
{
  char *filename = fd->name;
  XBT_DEBUG("\tWrite file '%s' size '%zu/%zu'",filename,size,fd->size);

  StorageActionLmmPtr action = new StorageActionLmm(p_model, 0, p_stateCurrent != SURF_RESOURCE_ON, this, WRITE);
  action->p_file = fd;

  // If the storage is full
  if(m_usedSize==m_size) {
    action->setState(SURF_ACTION_FAILED);
  }
  return action;
}

/**********
 * Action *
 **********/

StorageActionLmm::StorageActionLmm(ModelPtr model, double cost, bool failed, StorageLmmPtr storage, e_surf_action_storage_type_t type)
  : ActionLmm(model, cost, failed), StorageAction(model, cost, failed, storage, type) {
  XBT_IN("(%s,%zu", storage->m_name, cost);
  // Must be less than the max bandwidth for all actions
  lmm_expand(storage_maxmin_system, storage->p_constraint, p_variable, 1.0);
  switch(type) {
  case OPEN:
  case CLOSE:
  case STAT:
  case LS:
    break;
  case READ:
    lmm_expand(storage_maxmin_system, storage->p_constraintRead,
               p_variable, 1.0);
    break;
  case WRITE:
    lmm_expand(storage_maxmin_system, storage->p_constraintWrite,
               p_variable, 1.0);
    xbt_dynar_push(storage->p_writeActions,this);
    break;
  }
  XBT_OUT();
}

int StorageActionLmm::unref()
{
  m_refcount--;
  if (!m_refcount) {
    xbt_swag_remove(static_cast<ActionPtr>(this), p_stateSet);
    if (p_variable)
      lmm_variable_free(storage_maxmin_system, p_variable);
#ifdef HAVE_TRACING
    xbt_free(p_category);
#endif
    delete this;
    return 1;
  }
  return 0;
}

void StorageActionLmm::cancel()
{
  setState(SURF_ACTION_FAILED);
  return;
}

void StorageActionLmm::suspend()
{
  XBT_IN("(%p)", this);
  if (m_suspended != 2) {
    lmm_update_variable_weight(storage_maxmin_system,
                               p_variable,
                               0.0);
    m_suspended = 1;
  }
  XBT_OUT();
}

void StorageActionLmm::resume()
{
  THROW_UNIMPLEMENTED;
}

bool StorageActionLmm::isSuspended()
{
  return m_suspended == 1;
}

void StorageActionLmm::setMaxDuration(double duration)
{
  THROW_UNIMPLEMENTED;
}

void StorageActionLmm::setPriority(double priority)
{
  THROW_UNIMPLEMENTED;
}

