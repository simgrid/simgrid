#include <is_cmd.h>

#include <explode.h>

#ifdef WIN32
static int is_w32_binary(const char* cmd)
{
	DWORD binary_type;

	GetBinaryType(cmd, &binary_type);

	if(SCS_32BIT_BINARY == binary_type || SCS_64BIT_BINARY == binary_type || SCS_64BIT_BINARY == binary_type)
		return 1;

	return 0;
}
#endif

int
is_cmd(char** path, char** builtin, const char* p)
{
	size_t i = 0;
	size_t j = 0;
	int yes = 0;

	struct stat stat_buff = {0};
	char command[PATH_MAX + 1] = {0};
	char buff[PATH_MAX + 1] = {0};
	
	size_t len = strlen(p);
	
	if(!p)
		return EINVAL;
	
	while(i < len)
	{
		if(p[i] != ' ' && p[i] != '\t' && p[i] != '>')
			command[j++] = p[i];
		else
			break;
			
		i++;
	}
	
	/* check first if it's a shell buitin */
	
	if(builtin)
	{
		for(i = 0; builtin[i] != NULL; i++)
		{
			if(!strcmp(builtin[i], command))
				return 0;
		}
	}
	
	if(stat(command, &stat_buff) || !S_ISREG(stat_buff.st_mode))
	{
		if(path)
		{
			for (i = 0; path[i] != NULL; i++)
			{
				sprintf(buff,"%s/%s",path[i], command);
				
				if(!stat(buff, &stat_buff) && S_ISREG(stat_buff.st_mode))
				{
					#ifdef WIN32
					if(is_w32_binary(buff))
						yes = 1;
						break;
					#else
					if(!access(buff, X_OK))
					{
						yes = 1;
						break;
					}
					#endif
				}
			}
		}
	}
	else
	{
		#ifdef WIN32
		if(is_w32_binary(command))
			yes = 1;			
		#else
		if(!access(command, X_OK))
			yes = 1;
		#endif
	}
		
	return yes ? 0 : ECMDNOTFOUND;	
}

