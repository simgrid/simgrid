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

directory_t
directories_search_fstream_directory(directories_t directories, const char* name)
{
	
	struct stat buffer = {0};
	char* prev;
	directory_t directory;
	
	if(!directories)
	{
		errno = EINVAL;
		return NULL;
	}
	
	prev = getcwd(NULL, 0);
	
	vector_rewind(directories->items);
	
	while((directory = vector_get(directories->items)))
	{
		chdir(directory->name);
		
		if(!stat(name, &buffer) || S_ISREG(buffer.st_mode))
		{
			chdir(prev);
			free(prev);
			return directory;
		}
		
		vector_move_next(directories->items);
	}
	
	chdir(prev);
	free(prev);
	errno = ESRCH;
	return NULL;	
}

int
directories_load(directories_t directories, fstreams_t fstreams, lstrings_t suffixes)
{
	directory_t directory;
	
	if(!directories || !fstreams || !suffixes)
		return EINVAL;
	
	vector_rewind(directories->items);
	
	while((directory = vector_get(directories->items)))
	{
		if(directory->load)
		{
			if((errno = directory_open(directory)))
				return errno;
			
			chdir(directory->name);
			
			if((errno = directory_load(directory, fstreams, suffixes)))
				return errno;
				
			if((errno = directory_close(directory)))
				return errno;
			
			chdir(root_directory->name);
		}
			
		vector_move_next(directories->items);
		
		
	}
	
	return 0;
}

int
directories_has_directories_to_load(directories_t directories)
{
	directory_t directory;
	
	if(!directories)
	{
		errno = EINVAL;
		return 0;
	}
	
	vector_rewind(directories->items);
	
	while((directory = vector_get(directories->items)))
	{
		if(directory->load)
			return 1;
			
		vector_move_next(directories->items);
		
	}
	
	return 0;
}

directory_t
directories_get_back(directories_t directories)
{
	if(!directories)
	{
		errno = EINVAL;
		return NULL;
	}
	
	return vector_get_back(directories->items);	
}

int
directories_free(void** directoriesptr)
{
	if(!(*directoriesptr))
		return EINVAL;
		
	if((errno = vector_free(&((*((directories_t*)directoriesptr))->items))))
		return errno;
	
	free(*directoriesptr);
	*directoriesptr = NULL;
	
	return 0;	
	
}
