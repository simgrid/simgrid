#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <errno.h>
#include <getpath.h>

#ifndef MAX_PATH
#define MAX_PATH 255
#endif

int
getpath(const char* file, char** path)
{
	char buffer1[MAX_PATH + 1] = {0};
	char buffer2[MAX_PATH + 1] = {0};
	char buffer3[MAX_PATH + 1] = {0};	
	char *p1,*p2;
	size_t len = strlen(file);
	char* last_delimiter = NULL;
	struct stat buffer = {0};		
	
	strncpy(buffer1, file, len);	
	
	/* remove the /////// */
	while((p1 = strstr(buffer1, "//"))) 
	{
		if(p1[2]) 
			strcpy(p1, p1 + 1); 
		else 
			p1[1] = '\0';
	}
	
	if(*buffer1 == '~') 
	{
		for(p2 = buffer2, p1 = buffer1 + 1; *p1 && (*p1 != '/'); *p2++ = *p1++);
			*p2 = '\0';				
		
		if(buffer2[0] == '\0') 
		{
			char* home = getenv("HOME");
	
			if(home) 
			{
				strcpy(buffer2, home);		
			} 
			else 
			{
				struct passwd* pw = getpwuid(getuid());	 
				
				if(!pw) 
				{
					*path = NULL;
					return -1;	
				}
				
				strcpy(buffer2,pw->pw_dir);	
			}
			
			strcat(buffer2, p1);
		} 
		
	}
	else if (buffer1[0] != '/') 
	{
	
		getcwd(buffer2, MAX_PATH + 1);	
		
		if(buffer1[0] == '.' && buffer1[1] == '/') 
		{	/* replace */
			strcat(buffer2, &buffer1[1]);		
		} 
		else 
		{	
			strcat(buffer2, "/");			
			strcat(buffer2, buffer1);		
		}
	} 
	else 
	{	
		strcpy(buffer2, buffer1);			/* copy */
	}
		
	/*
	 * check for /..
	 */
	while((p1 = strstr( buffer2, "/.." ))) 
	{
		for( p2 = p1; --p2 > buffer2 && *p2 != '/'; );
		
		if (*(p1 + 3)) 
	                memmove(p2, p1+3, strlen(p1+3)+1);
		else 
			*p2 = '\0';
	}
	
	/*
	 * try to find links, and resolve them.
	 */
	p1 = strtok( buffer2, "/" );
	
	*buffer3 = '\0';
	
	while(p1) 
	{
		strcat( buffer3, "/" );
		strcat( buffer3, p1 );
		
		len = readlink(buffer3, buffer1, MAX_PATH);
		
		if (len != -1) 
		{
			*(buffer1 + len) = '\0';
			strcpy(buffer3, buffer1 );
		}
		
		p1 = strtok( NULL, "/" );
	}
	
	if(stat(buffer3, &buffer) || !S_ISREG(buffer.st_mode))
	{
		*path = NULL;
		errno = ENOENT;
		return -1;
	}								
		
	last_delimiter = strrchr(buffer3, '/');
	
	len = strlen(buffer3);
	
	if(last_delimiter)
		len -=strlen(last_delimiter);			 
	
	*path = (char*) calloc(len + 1, sizeof(char));
	
	if(!(*path))
	{
		*path = NULL;
		return -1;
	}
		
	strncpy(*path, buffer3, len);

	return len;
}

#include <stdio.h>

int
translatepath(const char* totranslate, char** translated)
{
	char buffer1[MAX_PATH + 1] = {0};		
	char buffer2[MAX_PATH + 1] = {0};
	char buffer3[MAX_PATH + 1] = {0};	
	char *p1,*p2;
	size_t len;	
	struct stat buffer = {0};
	
	len = strlen(totranslate);						
	
	strncpy(buffer1, totranslate, len);	
	
	if(!strcmp(buffer1,"."))
	{
		*translated = getcwd(NULL,0);
		return strlen(*translated);
	}
	else if(!strcmp(buffer1, "/.."))
	{
		*translated = strdup("/");
		return strlen(*translated);
	}
	
	while((p1 = strstr(buffer1, "//"))) 
		if(p1[2]) 
			strcpy(p1, p1 + 1); 
		else 
			p1[1] = '\0';
	
	
	
	if (buffer1[0] == '~') 
	{
		for (p2 = buffer2, p1 = buffer1 + 1; *p1 && (*p1 != '/'); *(p2++) = *(p1++));
		
		*p2 = '\0';				
		
		if(buffer2[0] == '\0') 
		{
			char* home = getenv("HOME");
	
			if(home) 
			{
				strcpy(buffer2, home);
			} 
			else 
			{
				struct passwd* pw = getpwuid(getuid());	 /* get entry */
				
				if(!pw)
				{
					*translated = NULL; 
					return -1;
				}
				
				strcpy(buffer2,pw->pw_dir);	
			}
			
			strcat(buffer2, p1);
		} 
	}
	else if (*buffer1 != '/') 
	{
	
		getcwd(buffer2, MAX_PATH + 1);
		
		if (*buffer1 == '.' && *(buffer1 + 1) == '/') 
		{
			strcat(buffer2, buffer1+1);		
		} 
		else 
		{	
			strcat(buffer2, "/");			
			strcat(buffer2, buffer1);		
		}
	} 
	else 
	{	
		strcpy(buffer2, buffer1);			
	}
	
	/*
	 * check for /..
	 */
	while((p1 = strstr( buffer2, "/.." ))) 
	{
		for( p2 = p1; --p2 > buffer2 && *p2 != '/'; );
		
		if (*(p1 + 3)) 
	                memmove(p2, p1+3, strlen(p1+3)+1);
		else 
			*p2 = '\0';
	}
	
	/*
	 * try to find links.
	 */
	p1 = strtok( buffer2, "/" );
	
	
	*buffer3 = '\0';
	
	while(p1) 
	{
		strcat( buffer3, "/" );
		strcat( buffer3, p1 );
		
		len = readlink(buffer3, buffer1, MAX_PATH);
		
		if (len != -1) 
		{
			*(buffer1 + len) = '\0';
			strcpy(buffer3, buffer1 );
		}
		
		p1 = strtok( NULL, "/" );
	}
	
	if (!(*buffer3)) 
		strcpy(buffer3, "/" );		
	
	len = strlen(buffer3);
	
	if(stat(buffer3, &buffer) || !S_ISDIR(buffer.st_mode))
	{
		*translated = NULL;
		errno = ENOTDIR;
		return -1;
	}
				 
	
	*translated = (char*) calloc(len + 1, sizeof(char));
	
	if(!(*translated))
	{
		*translated = NULL;
		return -1;
	}
	
	strncpy(*translated, buffer3, len);
	
	return len;
}


