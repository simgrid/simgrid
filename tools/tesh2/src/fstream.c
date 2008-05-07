/*
 * src/fstream.c - type representing the tesh file stream.
 *
 * Copyright 2008,2009 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * Purpose:
 * 		This file contains all the definitions of the functions related with
 * 		the tesh file stream type.
 *
 */

#include <fstream.h>
#include <xerrno.h>
#include <context.h>
#include <command.h>
#include <unit.h>
#include <str_replace.h>
#include <variable.h>



XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

static void
failure(unit_t unit)
{
	
	if(!keep_going_unit_flag)
	{
		unit_t root = unit->root ? unit->root : unit;
			
		if(!root->interrupted)
		{
			/* the unit interrupted (exit for the loop) */
			root->interrupted = 1;

			/* release the unit */
			xbt_os_sem_release(root->sem);
		}

		/* if the --keep-going option is not specified */
		if(!keep_going_flag)
		{
			if(!interrupted)
			{
				/* request an global interruption by the runner */
				interrupted = 1;

				/* release the runner */
				xbt_os_sem_release(units_sem);
			}
		}
	}
}

static void
syntax_error(unit_t unit)
{
	error_register("Unit failure", ESYNTAX, NULL, unit->fstream->name);
	
	failure(unit);
	
}

static void
internal_error(unit_t unit)
{
	failure(unit);
}

static void
sys_error(unit_t unit)
{
	unit_t root;
	
	error_register("System error", errno, NULL, unit->fstream->name);
	
	root = unit->root ? unit->root : unit;
			
	if(!root->interrupted)
	{
		/* the unit interrupted (exit for the loop) */
		root->interrupted = 1;

		/* release the unit */
		xbt_os_sem_release(root->sem);
	}
	
	if(!interrupted)
	{
		/* request an global interruption by the runner */
		interrupted = 1;

		/* release the runner */
		xbt_os_sem_release(units_sem);
	}
}

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
	
	if(!(fstream->name = strdup(name)))
	{
		free(fstream);
		return NULL;
	}
	
	if(!(fstream->directory = strdup(directory)))
	{
		free(fstream->name);
		free(fstream);
		return NULL;
	}
	
	fstream->stream = NULL;
	fstream->unit = NULL;
	
	
	return fstream;
}

int
fstream_open(fstream_t fstream)
{
	char path[PATH_MAX + 1] = {0};
	
	/* check the parameter */
	if(!(fstream))
    {
        errno = EINVAL;
        return -1;
    }
	
	if(!fstream || fstream->stream)
	{
		errno = EALREADY;
		return -1;
	}
		
	if(!strcmp(fstream->name, "stdin"))
	{
		fstream->stream = stdin;
		return 0;
	}
	
	#ifndef WIN32
	sprintf(path,"%s/%s",fstream->directory, fstream->name);
	#else
	sprintf(path,"%s\\%s",fstream->directory, fstream->name);
    #endif

	if(!(fstream->stream = fopen(path, "r")))
		return -1;
	
	return 0;
}

int
fstream_close(fstream_t fstream)
{
	/* check the parameter */
	if(!(fstream) || !strcmp(fstream->name, "stdin") )
    {
        errno = EINVAL;
        return -1;
    }
		
	if(!fstream->stream)
		return EBADF;	
	
	if(EOF == fclose(fstream->stream))
		return -1;
		
	fstream->stream = NULL;
	
	return 0;
}

int
fstream_free(fstream_t* ptr)
{
	
	/* check the parameter */
	if(!(*ptr))
    {
        errno = EINVAL;
        return -1;
    }
    
	if(!(*ptr))
		return EINVAL;
		
	if((*ptr)->stream)
		fclose((*ptr)->stream);
	
	free((*ptr)->name);
	
	if((*ptr)->directory)
		free((*ptr)->directory);
		
	free(*ptr);

	*ptr = NULL;
	
	return 0;
		
}

int
fstream_parse(fstream_t fstream, xbt_os_mutex_t mutex)
{
	size_t len;
	char * line = NULL;
	int line_num = 0;
	char file_pos[256];
	xbt_strbuff_t buff;
	int buffbegin = 0; 
	context_t context;
	unit_t unit;
	
	/* Count the line length while checking wheather it's blank */
	int blankline;
	int linelen;    
	/* Deal with \ at the end of the line, and call handle_line on result */
	int to_be_continued;
	
	/* check the parameter */
	if(!(fstream) || !mutex)
    {
        errno = EINVAL;
        return -1;
    }
    
	buff = xbt_strbuff_new();
	
	if(!(context = context_new()))
		return -1;
		
	unit = fstream->unit;
	
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
				syntax_error(unit);
				break;
			}
			else if(unit->is_running_suite)
			{/* it's the end of a suite */
				unit->is_running_suite = 0;
			}
				
			if(context->command_line)
			{
				if(fstream_launch_command(fstream, context, mutex) < 0)
						break;
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
				fstream_lex_line(fstream, context, mutex, file_pos, buff->data);    
				xbt_strbuff_empty(buff);
			}
		} 
		else 
		{
			snprintf(file_pos,256,"%s:%d",fstream->name, line_num);
			fstream_lex_line(fstream, context, mutex, file_pos, line);      
		}
	}
	
	/* Check that last command of the file ran well */
	if(context->command_line)
	{
		if(fstream_launch_command(fstream, context, mutex) < 0)
			return -1;
	}
	
	/* clear buffers */
	if(line)
		free(line);
		
	xbt_strbuff_free(buff);	
	
	if(context_free(&context) < 0)
		return -1;
	
	return (exit_code || errno) ? -1 : 0;
}


void 
fstream_lex_line(fstream_t fstream, context_t context, xbt_os_mutex_t mutex, const char * filepos, char *line) 
{
	char* line2;
	variable_t variable;
	unsigned int i;
	char name[VAR_NAME_MAX + 1] = {0};
	unit_t unit = fstream->unit;
	xbt_dynar_t variables = unit->runner->variables;
	
	/* search end */
	xbt_str_rtrim(line + 2,"\n");
	
	line2 = strdup(line);
	
	/* replace each variable by its value */
	xbt_os_mutex_acquire(unit->mutex);

	xbt_dynar_foreach(variables, i, variable)
	{
		sprintf(name, "$%s", variable->name);
		str_replace_all(&line2, name, variable->val);
		memset(name, 0, VAR_NAME_MAX + 1);
	}
	
	xbt_os_mutex_release(unit->mutex);
	
	switch(line2[0]) 
	{
		case '#': 
		break;
		
		case '$':
		case '&':
			
		context->async = (line2[0] == '&');
		
		/* further trim useless chars which are significant for in/output */
		xbt_str_rtrim(line2 + 2," \t");
		
		/* deal with CD commands here, not in context */
		if(!strncmp("cd ",line2 + 2, 3)) 
		{
			char* dir = strdup(line2 + 4);
			
			if(context->command_line)
			{
				if(fstream_launch_command(fstream, context, mutex) < 0)
					return;
			}
		
			/* search begining */
			while(*(dir++) == ' ');
			
			dir--;
			
			if(!dry_run_flag)
			{
				if(!silent_flag)
					INFO1("tesh cd %s",dir);
				
				if(!just_print_flag)
				{
					if(chdir(dir))
					{
						ERROR1("chdir failed to set the directory to %s", dir);
						error_register("Unit failure", errno, NULL, unit->fstream->name);
						failure(unit);
					}
				}
			}
			
			break;
		}
		else
		{
			fstream_process_token(fstream, context, mutex, filepos, line2[0], line2 + 2);
			break;
		}
		
		case '<':
		case '>':
		case '!':
		fstream_process_token(fstream, context, mutex, filepos, line2[0], line2 + 2);    
		break;
		
		case 'p':
		if(!dry_run_flag)
			INFO2("[%s] %s",filepos,line2+2);
		break;
		
		case 'P':
		if(!dry_run_flag)
			CRITICAL2("[%s] %s",filepos,line2+2);
		break;
		
		case 'D':
			if(unit->description)
				WARN2("Description already specified %s at %s", line2, filepos); 
			else
				unit->description = strdup(line2 + 2);
		break;
		
		default:
		error_register("Unit failure", errno, NULL, unit->fstream->name);
		syntax_error(unit);
		break;
	}
	
	free(line2);
}

void 
fstream_process_token(fstream_t fstream, context_t context, xbt_os_mutex_t mutex, const char* filepos, char token, char *line) 
{
	unit_t unit = fstream->unit;
	
	switch (token) 
	{
		case '$':
		case '&':
		
		if(context->command_line) 
		{
			
			if(context->output->used || context->input->used) 
			{
				ERROR2("[%s] More than one command in this chunk of lines (previous: %s).\nDunno which input/output belongs to which command.",filepos,context->command_line);
				syntax_error(unit);
				return;
			}
			
			if(fstream_launch_command(fstream, context, mutex) < 0)
					return;
			
			VERB1("[%s] More than one command in this chunk of lines",filepos);
		}
		
		context->command_line = strdup(line);
		context->line = strdup(filepos);
	
		
		break;
		
		case '<':
		xbt_strbuff_append(context->input,line);
		xbt_strbuff_append(context->input,"\n");
		break;
		
		case '>':
		xbt_strbuff_append(context->output,line);
		xbt_strbuff_append(context->output,"\n");
		break;
		
		case '!':
		
		if(context->command_line)
			if(fstream_launch_command(fstream, context, mutex) < 0)
					return;
		
		if(!strncmp(line,"timeout no",strlen("timeout no"))) 
		{
			VERB1("[%s] (disable timeout)", filepos);
			context->timeout = INDEFINITE;
		} 
		else if(!strncmp(line,"timeout ",strlen("timeout "))) 
		{
			int i = 0;
			char* p = line + strlen("timeout ");
	
			while(p[i] != '\0')
			{
				if(!isdigit(p[i]))
				{
					ERROR2("Invalid timeout value `%s' at %s ", line + strlen("timeout "), filepos);
					syntax_error(unit);
					failure(unit);
				}

				i++;
			}
			
			context->timeout = atoi(line + strlen("timeout"));
			VERB2("[%s] (new timeout value: %d)",filepos,context->timeout);
		
		} 
		else if (!strncmp(line,"expect signal ",strlen("expect signal "))) 
		{
			context->signal = strdup(line + strlen("expect signal "));
			
			#ifdef WIN32
			if(!strstr("SIGSEGVSIGTRAPSIGBUSSIGFPESIGILL", context->signal))
			{
				ERROR2("Incompatible signal \'%s\' detected at %s", context->signal, filepos);
				syntax_error(unit);
			}
			#endif

			xbt_str_trim(context->signal," \n");
			VERB2("[%s] (next command must raise signal %s)", filepos, context->signal);
		
		} 
		else if (!strncmp(line,"expect return ",strlen("expect return "))) 
		{

			int i = 0;
			char* p = line + strlen("expect return ");
			
			while(p[i] != '\0')
			{
				if(!isdigit(p[i]))
				{
					ERROR2("Invalid exit code value `%s' at %s ", line + strlen("expect return "), filepos);
					syntax_error(unit);
				}

				i++;
			}

			context->exit_code = atoi(line+strlen("expect return "));
			VERB2("[%s] (next command must return code %d)",filepos, context->exit_code);
		
		} 
		else if (!strncmp(line,"output ignore",strlen("output ignore"))) 
		{
			context->output_handling = oh_ignore;
			VERB1("[%s] (ignore output of next command)", filepos);
		
		} 
		else if (!strncmp(line,"output display",strlen("output display"))) 
		{
			context->output_handling = oh_display;
			VERB1("[%s] (ignore output of next command)", filepos);
		
		} 
		else if(!strncmp(line,"include ", strlen("include ")))
		{
			char* p1;
			char* p2;
			
			p1 = line + strlen("include");
			
			while(*p1 == ' ' || *p1 == '\t')
				p1++;
				
			
			if(p1[0] == '\0')
			{
				ERROR1("include file not specified %s ", filepos);
				syntax_error(unit);
			}
			else
			{
				char file_name[PATH_MAX + 1] = {0};
				
				p2 = p1;
				
				while(*p2 != '\0' && *p2 != ' ' && *p2 != '\t')
					p2++;
					
				strncpy(file_name, p1, p2 - p1);
				
				
				if(p2[0] != '\0')
					while(*p2 == ' ' || *p2 == '\t')
						p2++;
					
				fstream_handle_include(fstream, context, mutex, file_name, p2[0] != '\0' ? p2 : NULL);
				
			}
		}
		else if(!strncmp(line,"suite ", strlen("suite ")))
		{
			if(unit->is_running_suite)
			{
				ERROR1("Suite already in process at %s", filepos);
				syntax_error(unit);
				return;
			}
			
			fstream_handle_suite(fstream, context, mutex, line + strlen("suite "));
		}
		else if(!strncmp(line,"unsetenv ", strlen("unsetenv ")))
		{
			unsigned int i;
			int exists = 0;
			int env = 0;
			variable_t variable;
			char* name = line + strlen("unsetenv ");
			
			xbt_os_mutex_acquire(unit->mutex);
			

			xbt_dynar_foreach(unit->runner->variables, i, variable)
			{
				if(!strcmp(variable->name, name))
				{
					env = variable->env;
					exists = 1;
					break;
				}
			}
				
			if(env)
			{
				if(exists)
					/*xbt_dynar_remove_at(unit->runner->variables, i - 1, NULL);*/
					xbt_dynar_cursor_rm(unit->runner->variables, &i);
				else
					WARN3("environment variable %s not found %s %s at", name, line, filepos);	
			}
			else
			{
				if(exists)
					WARN3("%s is an not environment variable use unset metacommand to delete it %s %s", name, line, filepos);
				else
					WARN3("%s environment variable not found %s %s", name, line, filepos);
			}
			
			xbt_os_mutex_release(unit->mutex);	
			
				
		}
		else if(!strncmp(line,"setenv ", strlen("setenv ")))
		{
			char* val;
			char name[PATH_MAX + 1] = {0};
			char* p;
			unsigned int i;
			
			p = line + strlen("setenv ");
			
			val = strchr(p, '=');
			
			if(val)
			{
				variable_t variable;
				int exists = 0;
				int env = 0;
				val++;
				
				/* syntax error */
				if(val[0] == '\0')
				{
					ERROR2("Indefinite variable value %s %s", line, filepos);
					syntax_error(unit);
					failure(unit);	
				}
				
				
				strncpy(name, p, (val - p -1));
				
				/* test if the variable is already registred */
				xbt_os_mutex_acquire(unit->mutex);

				xbt_dynar_foreach(unit->runner->variables, i, variable)
				{
					if(!strcmp(variable->name, name))
					{
						variable->env = 1;
						exists = 1;
						break;
					}
				}
				
				/* if the variable is already registred, update its value;
				 * otherwise register it.
				 */
				if(exists)
				{
					if(env)
					{
						free(variable->val);
						variable->val = strdup(val);
						#ifdef WIN32
						SetEnvironmentVariable(variable->name, variable->val);
						#else
						setenv(variable->name, variable->val, 1);
						#endif
					}
					else
						WARN3("%s variable already exists %s %s", name, line, filepos);	
				}
				else
				{
					variable = variable_new(name, val);
					variable->env = 1;
					
					xbt_dynar_push(unit->runner->variables, &variable);
					
					#ifdef WIN32
					SetEnvironmentVariable(variable->name, variable->val);
					#else
					setenv(variable->name, variable->val, 0);
					#endif
				}
				
				xbt_os_mutex_release(unit->mutex);
				
			}
		}
		else if(!strncmp(line,"unset ", strlen("unset ")))
		{
			unsigned int i;
			int exists = 0;
			int env = 0;
			int err = 0;
			variable_t variable;
			char* name = line + strlen("unset ");
			
			xbt_os_mutex_acquire(unit->mutex);

			xbt_dynar_foreach(unit->runner->variables, i, variable)
			{
				if(!strcmp(variable->name, name))
				{
					env = variable->env;
					err = variable->err;
					exists = 1;
					break;
				}
			}
				
			if(!env)
			{
				if(exists)
					/*xbt_dynar_remove_at(unit->runner->variables, i, NULL);*/
					xbt_dynar_cursor_rm(unit->runner->variables, &i);
				else
					WARN3("variable %s not found %s %s", name, line, filepos);	
			}
			else if(env)
				WARN3("%s is an environment variable use unsetenv metacommand to delete it %s %s", name, line, filepos);	
			else
				WARN3("%s is an error variable : you are not allowed to delete it %s %s", name, line, filepos);
			

			xbt_os_mutex_release(unit->mutex);
				
		}
		else
		{
			char* val;
			char name[PATH_MAX + 1] = {0};
			
			val = strchr(line, '=');
			
			if(val)
			{
				variable_t variable;
				int exists = 0;
				unsigned int i;
				val++;
				
				
				/* syntax error */
				if(val[0] == '\0')
				{
					ERROR2("Indefinite variable value %s %s", line, filepos);
					syntax_error(unit);
				}
				
				
				/* assume it's a varibale */
				strncpy(name, line, (val - line -1));
				
				xbt_os_mutex_acquire(unit->mutex);
				
				/* test if the variable is already registred */
				xbt_dynar_foreach(unit->runner->variables, i, variable)
				{
					if(!strcmp(variable->name, name))
					{
						exists = 1;
						break;
					}
				}
				
				/* if the variable is already registred, update its value;
				 * otherwise register it.
				 */
				if(exists)
				{
					free(variable->val);
					variable->val = strdup(val);
				}
				else
				{
					variable_t new_var = variable_new(name, val);
					xbt_dynar_push(unit->runner->variables, &new_var);
				}

					
				xbt_os_mutex_release(unit->mutex);
				
			}
			else 
			{
				ERROR2("%s: Malformed metacommand: %s",filepos,line);
				syntax_error(unit);
				return;
			}
		}
		
		
		break;
	}
}

void
fstream_handle_include(fstream_t fstream, context_t context, xbt_os_mutex_t mutex, const char* file_name, const char* description)
{
	directory_t dir;
	char* prev_directory = NULL;
	fstream_t _fstream = NULL;
	struct stat buffer = {0};
	unit_t unit = fstream->unit;
	
	if(!stat(file_name, &buffer) && S_ISREG(buffer.st_mode))
	{
		/* the file is in the current directory */
		_fstream = fstream_new(getcwd(NULL, 0), file_name);
		fstream_open(_fstream);
	}
	/* the file to include is not in the current directory, check if it is in a include directory */
	else
	{
		unsigned int i;
		prev_directory = getcwd(NULL, 0);
		
		xbt_dynar_foreach(include_dirs, i, dir)
		{
			chdir(dir->name);
			
			if(!stat(file_name, &buffer) && S_ISREG(buffer.st_mode))
			{
				_fstream = fstream_new(dir->name, file_name);
				fstream_open(_fstream);
				break;
			}
		}

		chdir(prev_directory);
		free(prev_directory);
	}
	
	
	
	/* the file to include is not found handle the failure */
	if(!_fstream)
	{
		exit_code = EINCLUDENOTFOUND;
		ERROR1("Include file %s not found",file_name);
		failure(unit);
	}
	else
	{
		
		if(!unit->is_running_suite)
		{/* it's the unit of a suite */
			unit_t include = unit_new(unit->runner,unit->root, unit, _fstream);
			
			include->mutex = unit->root->mutex;
		
			if(description)
				include->description = strdup(description);
		
			xbt_dynar_push(unit->includes, &include);
		
			fstream_parse(_fstream, mutex);
		}
		else
		{/* it's a include */

			unit_t* owner;
			unit_t include;

			owner = xbt_dynar_get_ptr(unit->suites, xbt_dynar_length(unit->suites) - 1);
			
			include = unit_new(unit->runner,unit->root, *owner, _fstream);
			
			include->mutex = unit->root->mutex;
			
			if(description)
				include->description = strdup(description);
		
			xbt_dynar_push((*owner)->includes, &include);
			fstream_parse(_fstream, mutex);	
			
		}
	}
}

void
fstream_handle_suite(fstream_t fstream, context_t context, xbt_os_mutex_t mutex, const char* description)
{
	unit_t unit = fstream->unit;
	unit_t suite = unit_new(unit->runner, unit->root, unit, NULL);

	suite->description = strdup(description);
	xbt_dynar_push(unit->suites, &suite);
	unit->is_running_suite = 1;
	
}


int
fstream_launch_command(fstream_t fstream, context_t context, xbt_os_mutex_t mutex)
{
	unit_t unit = fstream->unit;
	
	/* TODO : check the parameters */
	
	if(!dry_run_flag)
	{
		command_t command;
		
		if(!(command = command_new(unit, context, mutex)))
		{
			if(EINVAL == errno)
			{
				ERROR2("Internal error : command_new() failed with the error %s (%d)",strerror(errno), errno);	
				internal_error(unit);
				return -1;
			}
			else if(ENOMEM == errno)
			{
				ERROR1("System error : command_new() failed with the error code %d",errno);	
				sys_error(unit);
				return -1;
			}
		}
		
		if(command_run(command) < 0)
		{
			ERROR2("Internal error : command_run() failed with the error %s (%d)",strerror(errno), errno);	
			internal_error(unit);
			return -1;	
		}
	}
	
	if(context_reset(context) < 0)
	{
		ERROR2("Internal error : command_run() failed with the error %s (%d)",strerror(errno), errno);	
		internal_error(unit);
		return -1;	
	}
	
	return 0;
}




