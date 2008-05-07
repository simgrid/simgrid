#include <str_replace.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <stdio.h>

int
str_replace(char** str, const char* what, const char* with)
{
	size_t pos, i;
	char* begin;
	char* buf;
	 
	if(!(begin = strstr(*str, what)))
	{
		errno = ESRCH;
		return -1;
	}
	
	pos = begin - *str;
	
	i = 0;
	
	pos += strlen(what);
	
	if(begin == *str)
	{
		if(!(buf = (char*) calloc(strlen(with) + ((pos < strlen(*str)) ? strlen(*str + pos) : 0) + 1, sizeof(char))))
			return -1;
			
		strcpy(buf, with);
		
		if(pos < strlen(*str))
			strcpy(buf + strlen(with), *str + pos);
	}
	else
	{
		if(!(buf = (char*) calloc((begin - *str) + strlen(with) + ((pos < strlen(*str)) ? strlen(*str + pos) : 0) + 1, sizeof(char))))
			return -1;
		
		strncpy(buf, *str,  (begin - *str));
		strcpy(buf + (begin - *str) , with);
		

		if(pos < strlen(*str))
			strcpy(buf + (begin - *str) + strlen(with), *str + pos);
	}	
	
	free(*str);;
	*str = buf;
	
	return 0;
 
} 

int
str_replace_all(char** str, const char* what, const char* with)
{
	int rv;
	
	while(!(rv = str_replace(str, what, with)));
	
	return (errno == ESRCH) ? 0 : -1;
}


