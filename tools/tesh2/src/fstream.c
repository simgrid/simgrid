#include <fstream.h>
#include <errno.h>
#include <context.h>
#include <command.h>
#include <unit.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

fstream_t
fstream_new(const char* directory, const char* name)
{
	fstream_t fstream;
	
	if(!name)
	{
		errno = EINVAL;
		return NULL;
	}
	
	if(!directory && !strcmp("stdin", name))
	{
		fstream = xbt_new0(s_fstream_t, 1);
		fstream->name = strdup("stdin");
		return fstream;
	}
	else if(!directory)
	{
		errno = EINVAL;
		return NULL;
	}
	
	fstream = xbt_new0(s_fstream_t, 1);
	
	fstream->name = strdup(name);
	
	fstream->directory = strdup(directory);
	fstream->stream = NULL;
	
	
	return fstream;
}

int
fstream_open(fstream_t fstream)
{
	char path[MAX_PATH] = {0};
	
	if(!fstream || fstream->stream)
		return EINVAL;
		
	if(!strcmp(fstream->name, "stdin"))
	{
		fstream->stream = stdin;
		return 0;
	}
	
	sprintf(path,"%s/%s",fstream->directory, fstream->name);
	
	if(!(fstream->stream = fopen(path, "r")))
		return errno;
	
	return 0;
}

int
fstream_close(fstream_t fstream)
{
	if(!fstream || !strcmp(fstream->name, "stdin"))
		return EINVAL;
		
	if(!fstream->stream)
		return EBADF;	
	
	fclose(fstream->stream);
	fstream->stream = NULL;
	return errno;
}

int
fstream_free(void** fstreamptr)
{
	if(!(*fstreamptr))
		return EINVAL;
		
	if((*((fstream_t*)fstreamptr))->stream)
		fclose((*((fstream_t*)fstreamptr))->stream);
	
	free((*((fstream_t*)fstreamptr))->name);
	
	if((*((fstream_t*)fstreamptr))->directory)
		free((*((fstream_t*)fstreamptr))->directory);
		
	free(*fstreamptr);
	
	*fstreamptr = NULL;
	
	return 0;
		
}

void
fstream_parse(fstream_t fstream, unit_t unit, xbt_os_mutex_t mutex)
{
	size_t len;
	char * line = NULL;
	int line_num = 0;
	char file_pos[256];
	xbt_strbuff_t buff;
	int buffbegin = 0; 
	context_t context;
	
	/* Count the line length while checking wheather it's blank */
	int blankline;
	int linelen;    
	/* Deal with \ at the end of the line, and call handle_line on result */
	int to_be_continued;
	
	buff=xbt_strbuff_new();
	context = context_new();
	
	while(!(unit->root->interrupted)  && getline(&line, &len, fstream->stream) != -1)
	{
		
		blankline=1;
		linelen = 0;    
		to_be_continued = 0;

		line_num++;
		
		while(line[linelen] != '\0') 
		{
			if (line[linelen] != ' ' && line[linelen] != '\t' && line[linelen]!='\n' && line[linelen]!='\r')
				blankline = 0;
			
			linelen++;
		}
	
		if(blankline) 
		{
			if(!context->command_line && (context->input->used || context->output->used))
			{
				ERROR1("[%d] Error: no command found in this chunk of lines.",buffbegin);
				ERROR1("Unit `%s': NOK (syntax error)", fstream->name);
			
				
				exit_code = ESYNTAX;
				unit_handle_failure(unit);
				break;
			}
			else if(unit->running_suite)
				unit->running_suite = 0;
				
			
			if(context->command_line)
			{
				if(!want_dry_run)
				{
					command_t command = command_new(unit, context, mutex);
					command_run(command);
					
				}
				
				context_reset(context);
			}
		
			continue;
		}
		
		if(linelen>1 && line[linelen-2]=='\\') 
		{
			if(linelen>2 && line[linelen-3] == '\\') 
			{
				/* Damn. Escaped \ */
				line[linelen-2] = '\n';
				line[linelen-1] = '\0';
			} 
			else 
			{
				to_be_continued = 1;
				line[linelen-2] = '\0';
				linelen -= 2;  
				
				if (!buff->used)
					buffbegin = line_num;
			}
		}
	
		if(buff->used || to_be_continued) 
		{ 
			xbt_strbuff_append(buff,line);
	
			if (!to_be_continued) 
			{
				snprintf(file_pos,256,"%s:%d",fstream->name, buffbegin);
				unit_handle_line(unit, context, mutex, file_pos, buff->data);    
				xbt_strbuff_empty(buff);
			}
		} 
		else 
		{
			snprintf(file_pos,256,"%s:%d",fstream->name, line_num);
			unit_handle_line(unit, context, mutex, file_pos, line);      
		}
	}
	
	
	
	/* Check that last command of the file ran well */
	if(context->command_line)
	{
		if(!want_dry_run)
		{
			command_t command = command_new(unit, context, mutex);
			command_run(command);
		}
		
		context_reset(context);
	}
	
	

	/* Clear buffers */
	if (line)
		free(line);
		
	xbt_strbuff_free(buff);	
	context_free(&context);
}


