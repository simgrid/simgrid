#include <com.h>

/*#include <stdlib.h>
#include <string.h>

*/

#include <sys/types.h>
#include <sys/stat.h>

#ifndef WIN32
#include <pwd.h>
#else
#endif

#include <errno.h>
#include <getpath.h>

#ifndef PATH_MAX
#define PATH_MAX 255
#endif

#ifndef WIN32
int
getpath(const char* file, char** path)
{
	char buffer1[PATH_MAX + 1] = {0};
	char buffer2[PATH_MAX + 1] = {0};
	char buffer3[PATH_MAX + 1] = {0};	
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
	
		getcwd(buffer2, PATH_MAX + 1);	
		
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
		
		len = readlink(buffer3, buffer1, PATH_MAX);
		
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

int
translatepath(const char* totranslate, char** translated)
{
	char buffer1[PATH_MAX + 1] = {0};		
	char buffer2[PATH_MAX + 1] = {0};
	char buffer3[PATH_MAX + 1] = {0};	
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
	
		getcwd(buffer2, PATH_MAX + 1);
		
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
		
		len = readlink(buffer3, buffer1, PATH_MAX);
		
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
#else
int
getpath(const char* file, char** path)
{
	DWORD len;
	char* part = NULL;
	char buffer[PATH_MAX + 1] = {0}; 
	struct stat info = {0};
	
	len = GetFullPathName(file, PATH_MAX, buffer, &part );
	
	if(!len)
	{
		*path = NULL;
		return -1;
	}

	
	if(stat(buffer, &info) || !S_ISREG(info.st_mode))
	{
		*path = NULL;
		errno = ENOENT;
		return -1;
	}		
	
	*path = (char*)calloc(strlen(buffer) - strlen(part), sizeof(char));


	*path = strncpy(*path, buffer, strlen(buffer) - strlen(part) - 1);

	return (int)(strlen(buffer) - strlen(part) -1);
}

int
translatepath(const char* totranslate, char** translated)
{
	char buffer1[PATH_MAX + 1] = {0};		
	char buffer2[PATH_MAX + 1] = {0};
	char *p1;
	int i, j, len;
	
	struct stat stat_buf = {0};
	
	/* return the current directory */
	if(!strcmp(totranslate,".") || !strcmp(totranslate,"./"))
	{
		*translated = getcwd(NULL,0);
		return (int)strlen(*translated);
	}
	/* return the previous directory */
	else if(!strcmp(totranslate,"..") || !strcmp(totranslate,"../"))
	{
		getcwd(buffer1, PATH_MAX + 1);
		p1 = strrchr(buffer1, '\\');
		*translated = (char*) calloc((p1 - buffer1) + 1, sizeof(char));
		strncpy(*translated, buffer1, (p1 - buffer1));
		
		return (int)strlen(*translated);
	}
	/* return the root directory */
	else if(!strcmp(totranslate, "/"))
	{
		*translated = getcwd(NULL,0);
		(*translated)[2] = '\0';
		return (int)strlen(*translated);
	}
	/* it's a relative directory name build the full directory name */
	else if(!strchr(totranslate, '/') && !strchr(totranslate, '\\') && !stat(totranslate, &stat_buf) && S_ISDIR(stat_buf.st_mode))
	{
		getcwd(buffer1, PATH_MAX + 1);
		strcat(buffer1,"\\");
		strcat(buffer1,totranslate);
		
		*translated = (char*) calloc(strlen(buffer1) + 1, sizeof(char));
		strcpy(*translated, buffer1);

		return (int)strlen(*translated);
	}
	
	len = (int)strlen(totranslate);						
	
	strncpy(buffer1, totranslate, len);	
	
	if(buffer1[strlen(buffer1) - 1] == '/' || buffer1[strlen(buffer1) - 1] == '\\')
		buffer1[strlen(buffer1) - 1] = '\0';
	
	while((p1 = strstr(buffer1, "//"))) 
		if(p1[2]) 
			strcpy(p1, p1 + 1); 
		else 
			p1[1] = '\0';
	
	for(i = 0, j = 0; buffer1[i] !='\0'; i++)
	{
		if(buffer1[i] == '/')
		{
			j++;
			
			if(j > 1)
				break;
		}
	}
	
	if(j == 1 && buffer1[i - 1] == '/')
	{
		/* perhaps it's a relative directory : `dir/' */
		strncpy(buffer2, buffer1, strlen(buffer1) - 1);
		
		if(!stat(buffer2, &stat_buf) && S_ISDIR(stat_buf.st_mode))
		{
			getcwd(buffer1, PATH_MAX + 1);
			strcat(buffer1,"\\");
			strcat(buffer1,buffer2);
			
			*translated = (char*) calloc(strlen(buffer1) + 1, sizeof(char));
			strcpy(*translated, buffer1);

			return (int)strlen(*translated);
		}
		else
			memset(buffer2, 0, PATH_MAX + 1);
		
	}
	
	if(buffer1[0] == '~') 
	{
		/* TODO */
		*translated = NULL;
		errno = ENOSYS;
		return -1;
	}
	else if (*buffer1 == '.') 
	{
		_fullpath(buffer2, buffer1, sizeof(buffer1));	
	} 
	else 
		strcpy(buffer2, buffer1);
	
	if(stat(buffer2, &stat_buf) || !S_ISDIR(stat_buf.st_mode))
	{
		*translated = NULL;
		errno = ENOTDIR;
		return -1;
	}
				 
	len = (int)strlen(buffer2);
	
	*translated = (char*) calloc(len + 1, sizeof(char));
	
	if(!(*translated))
	{
		*translated = NULL;
		return -1;
	}

	return len;
}
#endif


