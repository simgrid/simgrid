#include "private.h"

void MC_take_snapshot(mc_snapshot_t snapshot)
{  
/* Save the heap! */
	snapshot->heap_size = MC_save_heap(&(snapshot->heap));    

/* Save data and bss that */
  snapshot->data_size = MC_save_dataseg(&(snapshot->data));
}

void MC_restore_snapshot(mc_snapshot_t snapshot)
{ 
  MC_restore_heap(snapshot->heap, snapshot->heap_size);
  MC_restore_dataseg(snapshot->data, snapshot->data_size);  
}

void MC_free_snapshot(mc_snapshot_t snapshot)
{
  xbt_free(snapshot->heap);
  xbt_free(snapshot->data);
  xbt_free(snapshot);   
}
