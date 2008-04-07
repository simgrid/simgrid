#include <fstreams.h>
#include <excludes.h>
#include <fstream.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

fstreams_t
fstreams_new(int capacity, fn_finalize_t fn_finalize)
{
	fstreams_t fstreams;
	
	if(!(fstreams = (fstreams_t) calloc(1, sizeof(s_fstreams_t))))
		return NULL;
	
	if(!(fstreams->items = vector_new(capacity, fn_finalize)))
	{
		free(fstreams);
		return NULL;
	}
	
	return fstreams;
}

int
fstreams_exclude(fstreams_t fstreams, excludes_t excludes)
{
	vector_t to_erase;
	fstream_t fstream;
	
	if(!fstreams || !excludes)
		return EINVAL;
		
	if(excludes_is_empty(excludes))
		return 0;
	
	if(!(to_erase = vector_new(8, NULL)))
		return errno;
	
	/* collecte the file streams to exclude */
	vector_rewind(fstreams->items);
	
	while((fstream = vector_get(fstreams->items)))
	{
		if(excludes_contains(excludes, fstream))
			vector_push_back(to_erase, fstream);
		
		
		vector_move_next(fstreams->items);
	}
	
	if(!vector_is_empty(to_erase))
	{
	
		/* erase the file streams to exclude from the vector of file streams to run */
		vector_rewind(to_erase);
		
		while((fstream = vector_get(to_erase)))
		{
			vector_erase(fstreams->items, fstream);
			
			vector_move_next(to_erase);
		}
	}
	
	return vector_free(&to_erase);
}

int 
fstreams_contains(fstreams_t fstreams, fstream_t fstream)
{
	register fstream_t cur;
	
	if(!fstreams || !fstream)
	{
		errno = EINVAL;
		return 0;
	}
	
	vector_rewind(fstreams->items);
	
	while((cur = vector_get(fstreams->items)))
	{
		if(!strcmp(cur->name, fstream->name) && !strcmp(cur->directory, fstream->directory))
			return 1;
		
		vector_move_next(fstreams->items);
	}
	
	return 0;
}

int
fstreams_load(fstreams_t fstreams)
{
	register fstream_t fstream;
	
	if(!fstreams )
		return EINVAL;
	
	vector_rewind(fstreams->items);
	
	while((fstream = vector_get(fstreams->items)))
	{
		fstream_open(fstream);
		vector_move_next(fstreams->items);
	}
	
	
	return 0;
}

int
fstreams_add(fstreams_t fstreams, fstream_t fstream)
{
	if(!fstreams)
		return EINVAL;
	
	if(vector_push_back(fstreams->items, fstream))
		return errno;
	
	return 0;
	
}

int
fstreams_free(void** fstreamsptr)
{
	int rv;
	
	if(!(* fstreamsptr))
		return EINVAL;
	
	if(EAGAIN != (rv = vector_free(&((*((fstreams_t*)fstreamsptr))->items))))
		return rv;
		
	free(*fstreamsptr);
	
	*fstreamsptr = NULL;
	return 0;
}

int
fstreams_get_size(fstreams_t fstreams)
{
	if(!fstreams)
	{
		errno = EINVAL;
		return -1;
	}
	
	return vector_get_size(fstreams->items);
}

int
fstreams_is_empty(fstreams_t fstreams)
{
	if(!fstreams)
	{
		errno = EINVAL;
		return -1;
	}
	
	return vector_is_empty(fstreams->items);
}
