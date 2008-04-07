
#include <dictionary.h>
#include <htable.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define __DEFAULT_BLOCK_CAPACITY	((int)512)
#define __DEFAULT_TABLE_SIZE		((int)256)

static unsigned int 
hfunc(const char* key) 
{
  unsigned int hval = 5381;
  int ch;
  
  while ( (ch = *key++) ) 
    hval = ((hval << 5) + hval) + ch;
  
  return hval;
}

static int 
cmp_key(const char* key1, const char* key2)
{
	return !strcmp(key1,key2);
}


dictionary_t
dictionary_new(fn_finalize_t fn_finalize)
{
	dictionary_t dictionary;
		
	if(!(dictionary = (dictionary_t)calloc(1,sizeof(s_dictionary_t))))
		return NULL;
	
	if(!(dictionary->htable = htable_new(
							__DEFAULT_BLOCK_CAPACITY,
							__DEFAULT_TABLE_SIZE,
							(fn_hfunc_t)hfunc,
							(fn_cmp_key_t)cmp_key,
							fn_finalize)))
	{
		free(dictionary);
		return NULL;
	}
	
	return dictionary;
}

int
dictionary_set(dictionary_t dictionary,const char* key, const void* val)
{
	if(!dictionary || !key || !val)
		return EINVAL;
	
	return htable_set(dictionary->htable,(const void*)key,val);
}

void*
dictionary_get(dictionary_t dictionary, const char* key)
{
	
	if(!dictionary || !key)
	{
		errno = EINVAL;
		return NULL;
	}
	
	return htable_lookup(dictionary->htable,(const void*)key);
}


int
dictionary_free(dictionary_t* dictionaryptr)
{
	int rv;
	
	if(!(*dictionaryptr))
		return EINVAL;
	
	if((rv = htable_free(&((*dictionaryptr)->htable))))
		return rv;
	
	free(*dictionaryptr);
	*dictionaryptr = NULL;
	
	return 0;
}

int
dictionary_clear(dictionary_t dictionary)
{
	if(!dictionary)
		return EINVAL;
	
	return htable_clear(dictionary->htable);
}

int
dictionary_get_size(dictionary_t dictionary)
{
	if(!dictionary)
	{
		errno = EINVAL;
		return -1;
	}
	
	return htable_get_size(dictionary->htable);
}

int
dictionary_is_empty(dictionary_t dictionary)
{
	if(!dictionary)
	{
		errno = EINVAL;
		return 0;
	}
	
	return htable_is_empty(dictionary->htable);	
}

void*
dictionary_remove(dictionary_t dictionary,const char* key)
{
	if(!dictionary)
	{
		errno = EINVAL;
		return NULL;
	}
	
	return htable_remove(dictionary->htable,key);	
}


