#include <directories.h>
#include <directory.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

directories_t
directories_new(void)
{
	directories_t directories = xbt_new0(s_directories_t, 1);
	
	if(!(directories->items = vector_new(8, directory_free)))
	{
		free(directories);
		return NULL;
	}
	
	return directories;
}

int
directories_get_size(directories_t directories)
{
	if(!directories)
	{
		errno = EINVAL;
		return -1;
	}
	
	return vector_get_size(directories->items);
}

int
directories_is_empty(directories_t directories)
{
	if(!directories)
	{
		errno = EINVAL;
		return -1;
	}
	
	return vector_is_empty(directories->items);
}

int
directories_add(directories_t directories, directory_t directory)
{
	directory_t cur;
	
	if(!directories)
		return EINVAL;
	
	vector_rewind(directories->items);
	
	while((cur = vector_get(directories->items)))
	{
		if(!strcmp(cur->name, directory->name))
			return EEXIST;
		
		vector_move_next(directories->items);
	}
	
	return vector_push_back(directories->items, directory);
}

int
directories_contains(directories_t directories, directory_t directory)
{
	directory_t cur;
	
	if(!directories)
		return EINVAL;
	
	vector_rewind(directories->items);
	
	while((cur = vector_get(directories->items)))
	{
		if(!strcmp(cur->name, directory->name))
			return 1;
		
		vector_move_next(directories->items);
	}
	
	return 0;
}

int
directories_load(directories_t directories, fstreams_t fstreams, lstrings_t suffixes)
{
	directory_t directory;
	int rv;
	
	if(!directories || !fstreams || !suffixes)
		return EINVAL;
	
	vector_rewind(directories->items);
	
	while((directory = vector_get(directories->items)))
	{
		if((rv = directory_open(directory)))
			return rv;
		
		if((rv = directory_load(directory, fstreams, suffixes)))
			return rv;
			
		if((rv = directory_close(directory)))
			return rv;
			
		vector_move_next(directories->items);
		
		
	}
	
	return 0;
}

int
directories_free(void** directoriesptr)
{
	int rv;
	
	if(!(*directoriesptr))
		return EINVAL;
		
	if((rv = vector_free(&((*((directories_t*)directoriesptr))->items))))
		return rv;
	
	free(*directoriesptr);
	*directoriesptr = NULL;
	
	return 0;	
	
}
