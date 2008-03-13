#include <units.h>
#include <unit.h>
#include <fstreams.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

units_t
units_new(runner_t runner, fstreams_t fstreams)
{
	register fstream_t fstream;
	
	units_t units =  xbt_new0(s_units_t, 1);
	
	if(!(units->items = vector_new(fstreams_get_size(fstreams), unit_free)))
	{
		free(units);
		return NULL;
	}
	
	
	vector_rewind(fstreams->items);
	
	while((fstream = vector_get(fstreams->items)))
	{
		if((errno = vector_push_back(units->items, unit_new(runner, NULL, fstream))))
		{
			vector_free(&(units->items));
			free(units);
			return NULL;	
		}
		
		vector_move_next(fstreams->items);
	}
	
	return units;
}


int
units_is_empty(units_t units)
{
	if(!units)
	{
		errno = EINVAL;
		return 0;
	}
	
	return vector_is_empty(units->items);
}

int
units_get_size(units_t units)
{
	if(!units)
	{
		errno = EINVAL;
		return -1;
	}
	
	return vector_get_size(units->items);
}


int
units_run_all(units_t units, xbt_os_mutex_t mutex)
{
	register unit_t unit;
	
	if(!units)
		return EINVAL;
		
	if(vector_is_empty(units->items))
		return EAGAIN;
		
	/* move the cursor at the begining of the vector */
	vector_rewind(units->items);
	
	/* run all the units */
	while((unit = vector_get(units->items)))
	{
		unit_run(unit, mutex);
		vector_move_next(units->items);
		
	}
	
	return 0;
	
}

int
units_join_all(units_t units)
{
	register unit_t unit;
	
	if(!units)
		return EINVAL;
		
	if(vector_is_empty(units->items))
		return EAGAIN;
		
	/* move the cursor at the begining of the vector */
	vector_rewind(units->items);
	
	/* run all the units */
	while((unit = vector_get(units->items)))
	{
		if(unit->thread)
			xbt_os_thread_join(unit->thread, NULL);
			
		vector_move_next(units->items);
	}
	
	return 0;
}

int
units_interrupt_all(units_t units)
{
	register unit_t unit;
	
	if(!units)
		return EINVAL;
		
	if(vector_is_empty(units->items))
		return EAGAIN;
		
	/* move the cursor at the begining of the vector */
	vector_rewind(units->items);
	
	/* run all the units */
	while((unit = vector_get(units->items)))
	{
		if(!(unit->successeded) && !(unit->interrupted))
			unit_interrupt(unit);
			
		vector_move_next(units->items);
	}
	
	return 0;
}

int
units_verbose(units_t units)
{
	register unit_t unit;
	
	if(!units)
		return EINVAL;
		
	if(vector_is_empty(units->items))
		return EAGAIN;
		
	/* move the cursor at the begining of the vector */
	vector_rewind(units->items);
	
	/* run all the units */
	while((unit = vector_get(units->items)))
	{
		unit_verbose(unit);
		vector_move_next(units->items);
	}
	
	return 0;
	
}

int
units_reset_all(units_t units)
{
	register unit_t unit;
	
	if(!units)
		return EINVAL;
		
	if(vector_is_empty(units->items))
		return EAGAIN;
		
	/* move the cursor at the begining of the vector */
	vector_rewind(units->items);
	
	/* run all the units */
	while((unit = vector_get(units->items)))
	{
		unit_reset(unit);
		vector_move_next(units->items);
	}
	
	return 0;
}

int
units_free(void** unitsptr)
{
	if(!(*unitsptr))
		return EINVAL;
	
	if((errno = vector_free(&((*((units_t*)unitsptr))->items))))
		return errno;
		
	free(*unitsptr);
	*unitsptr = NULL;
	
	return 0;
}

