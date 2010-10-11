#include <units.h>
#include <unit.h>
#include <fstreams.h>
 XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);
units_t  units_new(runner_t runner, fstreams_t fstreams) 
{
  fstream_t fstream;
  unsigned int i;
  unit_t unit;
  units_t units = xbt_new0(s_units_t, 1);
  units->items =
      xbt_dynar_new(sizeof(unit_t), (void_f_pvoid_t) unit_free);
  xbt_dynar_foreach(fstreams->items, i, fstream)  {
    unit = unit_new(runner, NULL, NULL, fstream);
    xbt_dynar_push(units->items, &unit);
  }
  return units;
}

int  units_is_empty(units_t units) 
{
  if (!units)
     {
    errno = EINVAL;
    return 0;
    }
  return (0 == xbt_dynar_length(units->items));
}

int  units_get_size(units_t units) 
{
  if (!units)
     {
    errno = EINVAL;
    return -1;
    }
  return xbt_dynar_length(units->items);
}

int  units_run_all(units_t units, xbt_os_mutex_t mutex) 
{
  unit_t unit;
  unsigned int i;
  if (!units)
    return EINVAL;
  if (!xbt_dynar_length(units->items))
    return EAGAIN;
  xbt_dynar_foreach(units->items, i, unit)  {
    xbt_os_sem_acquire(jobs_sem);
    unit_run(unit, mutex);
  }
  return 0;
}

int  units_join_all(units_t units) 
{
  unit_t unit;
  unsigned int i;
  if (!units)
    return EINVAL;
  if (!xbt_dynar_length(units->items))
    return EAGAIN;
  xbt_dynar_foreach(units->items, i, unit)  {
    if (unit->thread)
      xbt_os_thread_join(unit->thread, NULL);
  }
  return 0;
}

int  units_interrupt_all(units_t units) 
{
  unit_t unit;
  unsigned int i;
  if (!units)
    return EINVAL;
  if (!xbt_dynar_length(units->items))
    return EAGAIN;
  xbt_dynar_foreach(units->items, i, unit)  {
    if (!(unit->successeded) && !(unit->interrupted))
      unit_interrupt(unit);
    
    else
       {
      if (!unit->released && unit->sem)
         {
        unit->released = 1;
        xbt_os_sem_release(unit->sem);
        }
      }
  }
  return 0;
}

int  units_summuarize(units_t units) 
{
  unit_t unit;
  unsigned int i;
  if (!units)
    return EINVAL;
  if (!xbt_dynar_length(units->items))
    return EAGAIN;
  xbt_dynar_foreach(units->items, i, unit)  {
    unit_summuarize(unit);
  }
  return 0;
}

int  units_reset_all(units_t units) 
{
  unit_t unit;
  unsigned int i;
  if (!units)
    return EINVAL;
  if (!xbt_dynar_length(units->items))
    return EAGAIN;
  xbt_dynar_foreach(units->items, i, unit)  {
    unit_reset(unit);
  }
  return 0;
}

int  units_free(void **unitsptr) 
{
  if (!(*unitsptr))
    return EINVAL;
  if ((*((units_t *) unitsptr))->items)
    xbt_dynar_free(&((*((units_t *) unitsptr))->items));
  free(*unitsptr);
  *unitsptr = NULL;
  return 0;
}


