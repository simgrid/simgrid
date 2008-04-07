#include <excludes.h>
#include <vector.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

excludes_t
excludes_new(void)
{
	excludes_t excludes = xbt_new0(s_excludes_t, 1);
	
	if(!(excludes->items = vector_new(8,NULL)))
	{
		free(excludes);
		return NULL;
	}
	
	return excludes;
}

int
excludes_is_empty(excludes_t excludes)
{
	if(!excludes)
	{
		errno = EINVAL;
		return 0;
	}
	
	return vector_is_empty(excludes->items);
}

int
excludes_contains(excludes_t excludes, fstream_t fstream)
{
	fstream_t cur;
	
	if(!excludes || !fstream)
	{
		errno = EINVAL;
		return 0;
	}
		
	vector_rewind(excludes->items);
		
	while((cur = vector_get(excludes->items)))
	{
		if(!strcmp(fstream->name, cur->name) && !strcmp(fstream->directory, cur->directory))
			return 1;
			
		vector_move_next(excludes->items);
	}
	
	return 0;
}

int
excludes_add(excludes_t excludes, fstream_t fstream)
{
	if(!excludes)
		return EINVAL;
		
	if(excludes_contains(excludes, fstream))
		return EEXIST;
		
	return vector_push_back(excludes->items, fstream);
}

int
excludes_check(excludes_t excludes, fstreams_t fstreams)
{
	fstream_t exclude;
	fstream_t fstream;
	int success = 1;
	int exists;
	
	if(!excludes || !fstreams)
		return EINVAL;
		
	vector_rewind(excludes->items);
		
	while((exclude = vector_get(excludes->items)))
	{
		vector_rewind(fstreams->items);
			
		while((fstream = vector_get(fstreams->items)))
		{
			exists = 0;
			
			if(!strcmp(fstream->name, exclude->name) && !strcmp(fstream->directory, exclude->directory))
			{
				exists = 1;
				break;
			}
				
			vector_move_next(fstreams->items);
		}
			
		if(!exists)
		{
			success = 0;
			WARN1("cannot exclude the file %s",exclude->name);	
		}
			
		vector_move_next(excludes->items);
	}
		
	return success;
}

int
excludes_free(void** excludesptr)
{
	int rv;
	
	if(!(*excludesptr))
		return EINVAL;
		
	if((rv =vector_free((&(*((excludes_t*)excludesptr))->items))))
		return rv;
		
	free(*excludesptr);
	*excludesptr = NULL;	
	
	return 0;
}
