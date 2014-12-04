#include <stddef.h>

#include <xbt/dynar.h>

#include "mc_object_info.h"
#include "mc_private.h"

dw_frame_t MC_file_object_info_find_function(mc_object_info_t info, void *ip)
{
  xbt_dynar_t dynar = info->functions_index;
  mc_function_index_item_t base =
      (mc_function_index_item_t) xbt_dynar_get_ptr(dynar, 0);
  int i = 0;
  int j = xbt_dynar_length(dynar) - 1;
  while (j >= i) {
    int k = i + ((j - i) / 2);
    if (ip < base[k].low_pc) {
      j = k - 1;
    } else if (ip >= base[k].high_pc) {
      i = k + 1;
    } else {
      return base[k].function;
    }
  }
  return NULL;
}
