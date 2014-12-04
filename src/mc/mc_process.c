#include <stddef.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h> // PROT_*

#include <libgen.h>

#include "mc_process.h"
#include "mc_object_info.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_process, mc,
                                "MC process information");

static void MC_init_memory_map_info(mc_process_t process);
static void MC_init_debug_info(mc_process_t process);

void MC_process_init(mc_process_t process, pid_t pid)
{
  process->process_flags = MC_PROCESS_NO_FLAG;
  process->pid = pid;
  if (pid==getpid())
    process->process_flags |= MC_PROCESS_SELF_FLAG;
  process->memory_map = MC_get_memory_map(pid);
  MC_init_memory_map_info(process);
  MC_init_debug_info(process);
}

void MC_process_clear(mc_process_t process)
{
  process->process_flags = MC_PROCESS_NO_FLAG;
  process->pid = 0;
  MC_free_memory_map(process->memory_map);
  process->memory_map = NULL;
  process->maestro_stack_start = NULL;
  process->maestro_stack_end = NULL;
  free(process->libsimgrid_path);
  process->libsimgrid_path = NULL;
  MC_free_object_info(&process->binary_info);
  MC_free_object_info(&process->libsimgrid_info);
}

/** @brief Finds the range of the different memory segments and binary paths */
static void MC_init_memory_map_info(mc_process_t process)
{
  process->maestro_stack_start = NULL;
  process->maestro_stack_end = NULL;
  process->libsimgrid_path = NULL;

  memory_map_t maps = process->memory_map;

  unsigned int i = 0;
  while (i < maps->mapsize) {
    map_region_t reg = &(maps->regions[i]);
    if (maps->regions[i].pathname == NULL) {
      // Nothing to do
    } else if ((reg->prot & PROT_WRITE)
               && !memcmp(maps->regions[i].pathname, "[stack]", 7)) {
      process->maestro_stack_start = reg->start_addr;
      process->maestro_stack_end = reg->end_addr;
    } else if ((reg->prot & PROT_READ) && (reg->prot & PROT_EXEC)
               && !memcmp(basename(maps->regions[i].pathname), "libsimgrid",
                          10)) {
      if (process->libsimgrid_path == NULL)
        process->libsimgrid_path = strdup(maps->regions[i].pathname);
    }
    i++;
  }

  xbt_assert(process->maestro_stack_start, "maestro_stack_start");
  xbt_assert(process->maestro_stack_end, "maestro_stack_end");
  xbt_assert(process->libsimgrid_path, "libsimgrid_path&");
}

static void MC_init_debug_info(mc_process_t process)
{
  XBT_INFO("Get debug information ...");

  memory_map_t maps = process->memory_map;

  // TODO, fix binary name

  /* Get local variables for state equality detection */
  process->binary_info = MC_find_object_info(maps, xbt_binary_name, 1);
  process->object_infos[0] = process->binary_info;

  process->libsimgrid_info = MC_find_object_info(maps, process->libsimgrid_path, 0);
  process->object_infos[1] = process->libsimgrid_info;

  process->object_infos_size = 2;

  // Use information of the other objects:
  MC_post_process_object_info(process, process->libsimgrid_info);
  MC_post_process_object_info(process, process->binary_info);

  XBT_INFO("Get debug information done !");
}

mc_object_info_t MC_process_find_object_info(mc_process_t process, void *ip)
{
  size_t i;
  for (i = 0; i != process->object_infos_size; ++i) {
    if (ip >= (void *) process->object_infos[i]->start_exec
        && ip <= (void *) process->object_infos[i]->end_exec) {
      return process->object_infos[i];
    }
  }
  return NULL;
}

dw_frame_t MC_process_find_function(mc_process_t process, void *ip)
{
  mc_object_info_t info = MC_process_find_object_info(process, ip);
  if (info == NULL)
    return NULL;
  else
    return MC_file_object_info_find_function(info, ip);
}
