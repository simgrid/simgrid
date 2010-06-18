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

#include <readline.h>

#include <is_cmd.h>
#include <getpath.h>

#ifndef _XBT_WIN32
#include <xsignal.h>
#endif

#ifdef _XBT_WIN32
static int
is_w32_cmd(char* cmd, char** path)
{
	size_t i = 0;
	struct stat stat_buff = {0};
	char buff[PATH_MAX + 1] = {0};
	


	if(!cmd)
	{
		errno = EINVAL;
		return 0;
	}
	
	if(stat(cmd, &stat_buff) || !S_ISREG(stat_buff.st_mode))
	{
		if(path)
		{
			for (i = 0; path[i] != NULL; i++)
			{
				/* use Cat.exe on Windows */
				if(!strcmp(cmd, "cat"))
					cmd[0] = 'C';
				
				sprintf(buff,"%s\\%s",path[i], cmd);
				
				if(!stat(buff, &stat_buff) && S_ISREG(stat_buff.st_mode))
					return 1;
			}
		}
	}
	else
		return 1;
		

	return 0;
}
#endif


XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);


long fstream_getline(fstream_t fstream, char **buf, size_t *n) {

	return readline(fstream->stream, buf, n);
	
}

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
	fstream->parsed = 0;
	
	
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
	
	#ifndef _XBT_WIN32
	sprintf(path,"%s/%s",fstream->directory, fstream->name);
	#else
	sprintf(path,"%s\\%s",fstream->directory, fstream->name);
    #endif

	if(!(fstream->stream = fopen(path, "r")))
	{
		return -1;
	}
	
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
	
	if((*ptr)->name)
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
	
	/*while(!(unit->root->interrupted)  && getline(&line, &len, fstream->stream) != -1)*/
	while(!(unit->root->interrupted)  && fstream_getline(fstream, &line, &len) != -1)
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
				snprintf(file_pos,256,"%s:%d",fstream->name, line_num);
				ERROR1("[%s] Error : no command found in the last chunk of lines", file_pos);
				
				unit_set_error(fstream->unit, ESYNTAX, 1, file_pos);

				failure(unit);
				break;
			}
			else if(unit->is_running_suite)
			{/* it's the end of a suite */
				
				unit_t* current_suite = xbt_dynar_get_ptr(unit->suites, xbt_dynar_length(unit->suites) - 1);

				if(!xbt_dynar_length((*current_suite)->includes))
				{
					ERROR2("[%s] Malformated suite `(%s)' : include missing", file_pos, (*current_suite)->description);
				
					unit_set_error(*current_suite, ESYNTAX, 1, file_pos);

					failure(unit);
					
				}
			
				unit->is_running_suite = 0;
			}
				
			if(context->command_line)
			{
				#ifdef _XBT_WIN32
				if(!context->is_not_found)
				{
				#endif
				if(fstream_launch_command(fstream, context, mutex) < 0)
						break;

				#ifdef _XBT_WIN32
				}
				#endif
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
		#ifdef _XBT_WIN32
		if(!context->is_not_found)
		{
		#endif

		if(fstream_launch_command(fstream, context, mutex) < 0)
			return -1;

		#ifdef _XBT_WIN32
		}
		#endif
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
	char exp[PATH_MAX + 1] = {0};
	unit_t unit = fstream->unit;
	xbt_dynar_t variables = unit->runner->variables;
	char* p= NULL;
	char* end = NULL;
	char* val = NULL;
	char buff[PATH_MAX + 1] = {0}; 
	size_t len;
	char delimiters[4] = {' ', '\t', '\n', '\0'}; 
	
	int j;
	
	if(line[0] == '#')
		return;

	if(unit->is_running_suite && strncmp(line, "! include", strlen("! include")))
	{/* it's the end of a suite */
		
		unit_t* current_suite = xbt_dynar_get_ptr(unit->suites, xbt_dynar_length(unit->suites) - 1);

		if(!xbt_dynar_length((*current_suite)->includes))
			ERROR2("[%s] Malformated suite `(%s)': include missing", filepos, (*current_suite)->description);
		else
			ERROR2("[%s] Malformated suite `(%s)': blank line missing", filepos, (*current_suite)->description);
		
		unit_set_error(*current_suite, ESYNTAX, 1, filepos);

		failure(fstream->unit);
	}
	
	context->line = strdup(filepos);
	
	/* search end */
	xbt_str_rtrim(line + 2,"\n");
	
	line2 = strdup(line);

	len = strlen(line2 + 2) + 1;

	/* replace each variable by its value */
	xbt_os_mutex_acquire(unit->mutex);
	

	/* replace all existing
	   ${var}
	   ${var:=val}
	   ${var:+val}
	   ${var:-val}
	   ${var:?val}
	   ${#var}
   */
	
	xbt_dynar_foreach(variables, i, variable)
	{
		if(!(p = strstr(line2 + 2, "${")))
			break;

		memset(buff, 0, len);

		sprintf(buff,"${%s",variable->name);
		
		/* FALSE */
		if((p = strstr(line2 + 2, buff)))
		{
			memset(buff, 0, len);
			p--;
			j = 0;

			while(*(p++) != '\0')
			{
				buff[j++] = *p;

				if(*p == '}')
					break;
			}

			if(buff[j - 1] != '}')
			{
				xbt_os_mutex_release(unit->mutex);	
				
				
				ERROR2("[%s] Syntax error : `%s'.",filepos, p - j);
				
				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
				failure(fstream->unit);
				return;
			}

			if((p = strstr(buff , ":=")))
			{
				/* ${var:=val} */
				
				/* if the value of the variable is empty, update its value by the value*/
				p += 2;
				
				end = strchr(p, '}');

				if(!end || (end == p))
				{
					xbt_os_mutex_release(unit->mutex);	
				
				
					ERROR2("[%s] Bad substitution : `%s'.",filepos, strstr(buff, "${"));
				
					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
					failure(fstream->unit);
					return;
				}

				val = (char*) calloc((size_t)(end - p) + 1, sizeof(char));

				strncpy(val, p,(end - p));
				
				
				/* replace the expression by the expression of the value of the variable*/
				sprintf(exp, "${%s:=%s}", variable->name, val);

				if(variable->val)
					str_replace_all(&line2, exp, variable->val, NULL);
				else
				{
					str_replace_all(&line2, exp, val, NULL);

					variable->val = strdup(val);
				}

				memset(exp, 0, VAR_NAME_MAX + 1);

				if(val)
				{
					free(val);
					val = NULL;
				}

			}
			else if((p = strstr(buff, ":-")))
			{
				/* ${var:-val} */
				
				/* if the value of the variable is empty, replace the expression by the value */
				p += 2;
				end = strchr(p, '}');

				if(!end || (end == p))
				{
					xbt_os_mutex_release(unit->mutex);	
				
					ERROR2("[%s] Bad substitution : `%s'.",filepos, strstr(line2, "${"));
				
					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
					failure(fstream->unit);
					return;
				}

				val = (char*) calloc((size_t)(end - p) + 1, sizeof(char));

				strncpy(val, p,(end - p)); 

				sprintf(exp, "${%s:-%s}", variable->name, val);
				
				str_replace_all(&line2, exp, variable->val ? variable->val : val, NULL);
				

				memset(exp, 0, VAR_NAME_MAX + 1);
				
				if(val)
				{
					free(val);
					val = NULL;
				}

			}
			else if((p = strstr(buff, ":+")))
			{
				/* ${var:+val} */
	
				/* if the value of the variable is not empty, replace the expression by the value */
				p += 2;

				end = strchr(p, '}');

				if(!end || (end == p))
				{
					xbt_os_mutex_release(unit->mutex);	
				
				
					ERROR2("[%s] Bad substitution : `%s'.",filepos, strstr(line2, "${"));
				
					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
					failure(fstream->unit);
					return;
				}

				val = (char*) calloc((size_t)(end - p) + 1, sizeof(char));

				strncpy(val, p,(end - p));

				sprintf(exp, "${%s:+%s}", variable->name, val);

				if(variable->val)
				{
					str_replace_all(&line2, exp, val, NULL);
				}
				else
				{
					str_replace_all(&line2, exp, NULL , NULL);
					variable->val = strdup(val);
				}
				
				memset(exp, 0, VAR_NAME_MAX + 1);
				
				if(val)
				{
					free(val);
					val = NULL;
				}
			}
			else if((p = strstr(buff, ":?")))
			{
				/*  ${var:?val} */
	
				/* if the value of the variable is not empty, replace the expression by the value */
				p += 2;
				end = strchr(p, '}');

				if(!end || (end == p))
				{
					xbt_os_mutex_release(unit->mutex);	
				
					ERROR2("[%s] Bad substitution : `%s'.",filepos, strstr(line2, "${"));
				
					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
					failure(fstream->unit);
					return;
				}

				val = (char*) calloc((size_t)(end - p) + 1, sizeof(char));

				strncpy(val, p,(end - p));

				sprintf(exp, "${%s:?%s}", variable->name, val);
				
				if(variable->val)
					str_replace_all(&line2, exp, variable->val, NULL);
				else
				{

					xbt_os_mutex_release(unit->mutex);	

					ERROR2("[%s] %s.",filepos, val);

					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
					failure(fstream->unit);
					return;
				}
				
				memset(exp, 0, VAR_NAME_MAX + 1);
				
				if(val)
				{
					free(val);
					val = NULL;
				}
			}
		}
	}

	/* replace all existing $var */
	xbt_dynar_foreach(variables, i, variable)
	{
		if(!strchr(line2 + 2, '$'))
			break;

		if(strstr(line2 + 2, variable->name))
		{

			sprintf(exp, "${#%s}", variable->name);
			
			if(strstr(line2 + 2, exp))
			{

				if(variable->val)
				{
					char slen[4] = {0};
					sprintf(slen,"%d", (int)strlen(variable->val));
					str_replace_all(&line2, exp, slen, NULL);
				}
				else
					str_replace_all(&line2, exp, "0", NULL);
			}

			memset(exp, 0, VAR_NAME_MAX + 1);

			sprintf(exp, "${%s}", variable->name);

			if(strstr(line2 + 2, exp))
			{
				if(variable->val)
					str_replace_all(&line2, exp, variable->val, NULL);
				else
					str_replace_all(&line2, exp, NULL, NULL);
			}

			memset(exp, 0, VAR_NAME_MAX + 1);

			sprintf(exp, "$%s", variable->name);
			
			if((p = strstr(line2 + 2, exp)))
			{
				if((p + strlen(variable->name) + 1)[0] != '\0' && !(isalpha((p + strlen(variable->name) + 1)[0])))
					delimiters[0] = (p + strlen(variable->name) + 1)[0];

				if(variable->val)
					str_replace_all(&line2, exp, variable->val,  delimiters);
				else
					str_replace_all(&line2, exp, NULL, delimiters);
			}

			memset(exp, 0, VAR_NAME_MAX + 1);

		}
	}

	while((p = strstr(line2 + 2, "${")))
	{
		/*if(*(p+1) != '{')
		{
			j = 0;
			p --;

			while(*(p++) != '\0')
			{
				if(*p != ' ' && *p !='\t')
					exp[j++] = *p;
				else
					break;

			}
			
			str_replace_all(&line2, exp, NULL, " \t\n\r");
			memset(exp, 0, VAR_NAME_MAX + 1);
		}.
		else
		*/
		{
			char* begin = NULL;
			
			j = 0;
			p --;

			while(*(p++) != '\0')
			{
				if((!begin && *p != ' ' && *p !='\t') || begin)
				{
					/* `:' must be before this caracter, bad substitution : exit loop 
					    ||
						the current character is already present, bad substitution : exit loop
						*/
					if(
							(
								*(p - 1) != ':' && (
														(*p == '=') || (*p == '-') || (*p == '+') || (*p == '?')
													)
							)
						|| 
							(
								begin &&	(
												(*p == ':') || (*p == '=') || (*p == '-') || (*p == '+') || (*p == '?')
											)
							)
						)
						break;
					else
						exp[j++] = *p;

					if(*p == ':')
					{
						/* save the begining of the value */
						if((*(p+1) == '=') || (*(p+1) == '-') || (*(p+1) == '+') || (*(p+1) == '?'))
						{
							begin = p + 2;
							exp[j++] = *(p+1);
							p++;
							continue;

						}
						else
						/* the current char is `:' but the next is invalid, bad substitution : exit loop */
							break;
					}
					/* end of the substitution : exit loop */
					else if(*p == '}')
						break;
				}
				else
					break;
			}
			
			if(exp[j - 1] == '}')
			{
				if(exp[2] == '#')
				{
					/* ${#var} */


					if(4 == strlen(exp))
					{
						xbt_os_mutex_release(unit->mutex);	
					
						ERROR2("[%s] Bad substitution : `%s'.",filepos, strchr(line2 + 2, '$'));
					
						unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
						failure(fstream->unit);
						return;
					}
					
					str_replace_all(&line2, exp, "0", NULL);	
				}
				else if(strstr(exp,":="))
				{
					/* ${var:=value} */	
					
					end = strchr(p, '}');

					if(!end || (end == begin))
					{
						xbt_os_mutex_release(unit->mutex);	
					
					
						ERROR2("[%s] Bad substitution : `%s'.",filepos, strchr(line2 + 2, '$'));
					
						unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
						failure(fstream->unit);
						return;
					}

					variable = xbt_new0(s_variable_t, 1);

					variable->val = (char*) calloc((size_t)(end - begin) + 1, sizeof(char));

					strncpy(variable->val, begin ,(end - begin));

					begin = exp + 2;
					end = strchr(exp, ':');

					if(!end || (end == begin))
					{
						xbt_os_mutex_release(unit->mutex);	
					
					
						ERROR2("[%s] Bad substitution : `%s'.",filepos, strchr(line2 + 2, '$'));
					
						unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
						failure(fstream->unit);
						return;
					}

					variable->name = (char*) calloc((size_t)(end - begin) + 1, sizeof(char));

					strncpy(variable->name, exp + 2 ,(end - begin));

					str_replace_all(&line2, exp, variable->val, NULL);

					xbt_dynar_push(variables, &variable);

				}
				else if(strstr(exp,":-"))
				{
					/* ${var:-value} */	

					
					end = strchr(p, '}');

					if(!end || (end == begin))
					{
						xbt_os_mutex_release(unit->mutex);	
					
					
						ERROR2("[%s] Bad substitution : `%s'.",filepos, strchr(line2 + 2, '$'));
					
						unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
						failure(fstream->unit);
						return;
					}

					val = (char*) calloc((size_t)(end - begin) + 1, sizeof(char));

					strncpy(val, begin ,(end - begin));

					str_replace_all(&line2, exp, val, NULL);

					if(val)
						free(val);

				}
				else if(strstr(exp,":+"))
				{
					/* ${var:+value} */	

					end = strchr(p, '}');

					if(!end || (end == begin))
					{
						xbt_os_mutex_release(unit->mutex);	
					
						ERROR2("[%s] Bad substitution : `%s'.",filepos, strchr(line2 + 2, '$'));
					
						unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
						failure(fstream->unit);
						return;
					}

					str_replace_all(&line2, exp, NULL, NULL);
				}
				else if(strstr(exp,":?"))
				{
					/* ${var:?value} */
					
					end = strchr(p, '}');

					if(!end || (end == begin))
					{
						xbt_os_mutex_release(unit->mutex);	
					
						ERROR2("[%s] Bad substitution : `%s'.",filepos, strchr(line2 + 2, '$'));
					
						unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
						failure(fstream->unit);
						return;
					}

					val = (char*) calloc((size_t)(end - begin) + 1, sizeof(char));

					strncpy(val, begin ,(end - begin));

					xbt_os_mutex_release(unit->mutex);	
					
					ERROR2("[%s] : `%s'.",filepos, val);

					if(val)
						free(val);
					
					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
					failure(fstream->unit);

					return;
					
				}
				else
				{
					/* ${var} */

					if(3 == strlen(exp))
					{
						xbt_os_mutex_release(unit->mutex);	
					
						ERROR2("[%s] Bad substitution : `%s'.",filepos, strchr(line2 + 2, '$'));
					
						unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
						failure(fstream->unit);
						return;
					}

					str_replace_all(&line2, exp, NULL, NULL);
					
				}

				memset(exp, 0, VAR_NAME_MAX + 1);
			}
			else
			{
				xbt_os_mutex_release(unit->mutex);	
				
				if(strstr(line2 + 2, "${"))
					ERROR2("[%s] Bad substitution : `%s'.",filepos, strstr(line2, "${"));
				else
					ERROR2("[%s] Syntax error : `%s'.",filepos, strstr(line2, "${"));

				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
				failure(fstream->unit);
				return;
			}

		}
		
	}
	
	while(1)
	{
		p = line2 + (line2[0] =='<' ? 4 : 2);
		
		if((p = strchr(p, '$')))
		{
			if(*(p+1) != ' ')
			{
				j = 0;
				p --;

				while(*(p++) != '\0')
				{
					if(*p != ' ' && *p !='\t')
						exp[j++] = *p;
					else
						break;

				}
				
				str_replace_all(&line2, exp, NULL, " \t\n\r");
				memset(exp, 0, VAR_NAME_MAX + 1);
			}
			else
			{
				/* maybe < $ cmd */
				p++;
			}
		}
		else
			break;
	}

	xbt_os_mutex_release(unit->mutex);	
	
	switch(line2[0]) 
	{
		/*case '#': 
		break;
		*/
		
		case '$':
		case '&':

		if(line[1] != ' ')
		{
			
			if(line2[0] == '$')
				ERROR1("[%s] Missing space after `$' `(usage : $ <command>)'", filepos);
			else
				ERROR1("[%s] Missing space after & `(usage : & <command>)'", filepos);
		
			unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

			failure(unit);
			return;
		}
			
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
					INFO2("[%s] cd %s", filepos, dir);
				
				if(!just_print_flag)
				{
					if(chdir(dir))
					{
						ERROR3("[%s] Chdir to %s failed: %s",filepos, dir,error_to_string(errno, 0));
						unit_set_error(fstream->unit, errno, 0, filepos);

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
		
		if(line[0] == '!' && line[1] != ' ')
		{
			ERROR1("[%s] Missing space after `!' `(usage : ! <command> [[=]value])'", filepos);
		
			unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

			failure(unit);
			return;
		}

		fstream_process_token(fstream, context, mutex, filepos, line2[0], line2 + 2);    
		break;
		
		case 'p':
		
		{
			unsigned int j;
			int is_blank = 1;
			
			char* prompt = line2 + 2;

			for(j = 0; j < strlen(prompt); j++)
			{
				if (prompt[j] != ' ' && prompt[j] != '\t')
				{
					is_blank = 0;
					break;
				}
			}

			if(is_blank)
			{
				ERROR1("[%s] Bad usage of the metacommand p `(usage : p <prompt>)'", filepos);
				
				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

				failure(unit);
				return;
			}

			if(!dry_run_flag)
				INFO2("[%s] %s",filepos,prompt);
		}

		
		break;
		
		case 'P':
		
		{
			unsigned int j;
			int is_blank = 1;
				
			char* prompt = line2 + 2;

			for(j = 0; j < strlen(prompt); j++) 
				if (prompt[j] != ' ' && prompt[j] != '\t')
					is_blank = 0;

			if(is_blank)
			{
				ERROR1("[%s] Bad usage of the metacommand P `(usage : P <prompt>)'", filepos);
				
				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

				failure(unit);
				return;
			}

			if(!dry_run_flag)		
				CRITICAL2("[%s] %s",filepos, prompt);
		}

		break;
		
		case 'D':
			if(unit->description)
				WARN2("[%s] Description already specified `%s'",filepos, line2 + 2); 
			else
			{
				unsigned int j;
				int is_blank = 1;
				
				char* desc = line2 + 2;

				for(j = 0; j < strlen(desc); j++) 
					if (desc[j] != ' ' && desc[j] != '\t')
						is_blank = 0;

				if(is_blank)
				{
					ERROR1("[%s] Bad usage of the metacommand D `(usage : D <Description>)'", filepos);
					
					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

					failure(unit);
					return;
				}

				unit->description = strdup(desc);
			}
		break;
		
		default:
		ERROR2("[%s] Syntax error `%s'", filepos, line2);
		unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
		failure(unit);
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
				ERROR2("[%s] More than one command in this chunk of lines (previous: %s).\nDunno which input/output belongs to which command.",filepos, context->command_line);

				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
				failure(unit);
				return;
			}
			
			if(fstream_launch_command(fstream, context, mutex) < 0)
					return;
			
			VERB1("[%s] More than one command in this chunk of lines",filepos);
		}
		
		{
			size_t j,
			is_blank = 1;

			for(j = 0; j < strlen(line); j++) 
				if (line[j] != ' ' && line[j] != '\t')
					is_blank = 0;

			if(is_blank)
			{
				if(token == '$')
				ERROR1("[%s] Undefinite command for `$' `(usage: $ <command>)'", filepos);
				else
				ERROR1("[%s] Undefinite command for `&' `(usage: & <command>)'", filepos);

				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

				failure(unit);
				return;
			}
		}
		
		context->command_line = strdup(line);

		xbt_str_ltrim(context->command_line," ");
		
		context->line = /*strdup(filepos)*/ filepos;
		context->pos = strdup(filepos);
		
		#ifdef _XBT_WIN32
		{

		/* translate the command line */

		char* path = NULL;
		char* delimiter;
		char command_line[PATH_MAX + 1] = {0};
		size_t i = 0;
		char* args = NULL;

		

		/*if(strstr(context->command_line,".exe"))
			strcpy(command_line,context->command_line);*/
		
		{
			size_t len;
			
			size_t j = 0;
	
			len = strlen(context->command_line);
			
			while(i < len)
			{
				if(context->command_line[i] != ' ' && context->command_line[i] != '\t' && context->command_line[i] != '>')
					command_line[j++] = context->command_line[i];
				else
					break;
					
				i++;
			}
			
			if(!strstr(context->command_line,".exe"))
				strcat(command_line,".exe");

			args = strdup(context->command_line + i);
		}
		
		if(!is_w32_cmd(command_line, fstream->unit->runner->path) && getpath(command_line, &path) < 0)
		{
			command_t command;

			ERROR3("[%s] `%s' : NOK (%s)", filepos, command_line, error_to_string(ECMDNOTFOUND, 1));
			unit_set_error(fstream->unit, ECMDNOTFOUND, 1, filepos);

			context->is_not_found = 1;
			
			command = command_new(fstream->unit, context, mutex);

			command->status = cs_failed;
			command->reason = csr_command_not_found;

			failure(unit);
			
			
			return;
		}
		
		delimiter = strrchr(command_line,'/');

		if(!delimiter)
			delimiter = strrchr(command_line,'\\');
		
		/*free(context->command_line);*/
		
		
		if(path)
		{
			if(args)
			{
				context->t_command_line = (char*)calloc(strlen(path) + strlen(delimiter ? delimiter + 1 : command_line) + strlen(args) + 2, sizeof(char));
				sprintf(context->t_command_line,"%s\\%s%s",path,delimiter ? delimiter + 1 : command_line, args);

				free(args);

			}
			else
			{
				context->t_command_line = (char*)calloc(strlen(path) + strlen(delimiter ? delimiter + 1 : command_line) + 2, sizeof(char));
				sprintf(context->t_command_line,"%s\\%s",path,delimiter ? delimiter + 1 : command_line);
			}
		
			free(path);
		}
		else
		{
			if(args)
			{

				context->t_command_line = (char*)calloc(strlen(command_line) + strlen(args) + 1, sizeof(char));
				sprintf(context->t_command_line,"%s%s",command_line, args);

			
				free(args);

			}
			else
			{
				context->t_command_line = (char*)calloc(strlen(command_line) + 1, sizeof(char));
				strcpy(context->t_command_line,command_line);
			}
		}


		}
		#endif


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
		{
			if(fstream_launch_command(fstream, context, mutex) < 0)
					return;
		}
		
		if(!strncmp(line,"timeout no",strlen("timeout no"))) 
		{
			VERB1("[%s] (disable timeout)", filepos);
			context->timeout = INDEFINITE;
		} 
		else if(!strncmp(line,"timeout ",strlen("timeout "))) 
		{
			int i = 0;
			unsigned int j;
			int is_blank = 1;
			char* p = line + strlen("timeout ");


			for(j = 0; j < strlen(p); j++) 
				if (p[j] != ' ' && p[j] != '\t')
					is_blank = 0;

			if(is_blank)
			{
				ERROR1("[%s] Undefinite timeout value `(usage :timeout <seconds>)'", filepos);
				
				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

				failure(unit);
				return;
			}
	
			while(p[i] != '\0')
			{
				if(!isdigit(p[i]))
				{
					ERROR2("[%s] Invalid timeout value `(%s)' : `(usage :timeout <seconds>)'", filepos, line + strlen("timeout "));

					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
 
					failure(unit);
					return;
				}

				i++;
			}
			
			context->timeout = atoi(line + strlen("timeout"));
			VERB2("[%s] (new timeout value: %d)",filepos,context->timeout);
		
		} 
		else if (!strncmp(line,"expect signal ",strlen("expect signal "))) 
		{
			unsigned int j;
			int is_blank = 1;

			
			char* p = line + strlen("expect signal ");


			for(j = 0; j < strlen(p); j++) 
				if (p[j] != ' ' && p[j] != '\t')
					is_blank = 0;

			if(is_blank)
			{
				ERROR1("[%s] Undefinite signal name `(usage :expect signal <signal name>)'", filepos);
				
				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

				failure(unit);
				return;
			}

			context->signal = strdup(line + strlen("expect signal "));
			
			xbt_str_trim(context->signal," \n");

			#ifdef _XBT_WIN32
			if(!strstr("SIGSEGVSIGTRAPSIGBUSSIGFPESIGILL", context->signal))
			{
				ERROR2("[%s] Signal `%s' not supported by this platform", filepos, context->signal);

				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);


				failure(unit);
				return;
			}
			#else
			if(!sig_exists(context->signal))
			{
				ERROR2("[%s] Signal `%s' not supported by Tesh", filepos, context->signal);
				
				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);


				failure(unit);
				return;
			}

			#endif

			
			VERB2("[%s] (next command must raise signal %s)", filepos, context->signal);
		
		} 
		else if (!strncmp(line,"expect return ",strlen("expect return "))) 
		{

			int i = 0;
			unsigned int j;
			int is_blank = 1;
			char* p = line + strlen("expect return ");


			for(j = 0; j < strlen(p); j++) 
				if (p[j] != ' ' && p[j] != '\t')
					is_blank = 0;

			if(is_blank)
			{
				ERROR1("[%s] Undefinite return value `(usage :expect return <return value>)'", filepos);
				
				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

				failure(unit);
				return;
			}
			
			while(p[i] != '\0')
			{
				if(!isdigit(p[i]))
				{
					ERROR2("[%s] Invalid exit code value `(%s)' : must be an integer >= 0 and <=255",  filepos, line + strlen("expect return "));

					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

					failure(unit);
					return;
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
				ERROR1("[%s] no file specified : `(usage : include <file> [<description>])'", filepos);

				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

				failure(unit);
				return;
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
			unsigned int j;
			int is_blank = 1;
			char* p = line + strlen("suite ");


			for(j = 0; j < strlen(p); j++) 
				if (p[j] != ' ' && p[j] != '\t')
					is_blank = 0;
			
			if(is_blank)
			{
				ERROR1("[%s] Undefinite suit description : `(usage : suite <description>)", filepos);

				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

				failure(unit);
				return;
			}

			if(unit->is_running_suite)
			{
				ERROR1("[%s] Suite already in progress", filepos);

				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

				failure(unit);
				return;
			}
			
			fstream_handle_suite(fstream, line + strlen("suite "), filepos);
		}
		else if(!strncmp(line,"unsetenv ", strlen("unsetenv ")))
		{
			unsigned int i, j;
			int exists = 0;
			int env = 0;
			int err = 0;
			variable_t variable;
			int is_blank;

			char* name = line + strlen("unsetenv ");

			is_blank = 1;

			for(j = 0; j < strlen(name); j++) 
				if (name[j] != ' ' && name[j] != '\t')
					is_blank = 0;

			if(is_blank)
			{
				ERROR1("[%s] Bad usage of the metacommand unsetenv : `(usage : unsetenv variable)'", filepos);
				
				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

				failure(unit);
				return;
			}
			
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
				
			if(env)
			{
				if(exists)
				{
					#ifndef _XBT_WIN32
					unsetenv(name);
					#else
					SetEnvironmentVariable(name, NULL);
					#endif
					xbt_dynar_cursor_rm(unit->runner->variables, &i);
				}
				else
				{
					ERROR2("[%s] `(%s)' environment variable not found : impossible to unset it",filepos, name);	
					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
					xbt_os_mutex_release(unit->mutex);
					failure(unit);
					return;
				}
			}
			else
			{
				if(exists)
				{
					if(!err)
					{
						ERROR2("[%s] `(%s)' is not an environment variable : use `unset' instead `unsetenv'",filepos, name);	
						unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
						failure(unit);
						xbt_os_mutex_release(unit->mutex);
						return;
					}
					else
					{
						ERROR2("[%s] `(%s)' is not an environment variable (it's a system variable) : impossible to unset it",filepos, name);	
						unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
						xbt_os_mutex_release(unit->mutex);
						failure(unit);
						return;
					}
				}
				else
				{
					ERROR2("[%s] `(%s)' environment variable not found : impossible to unset it",filepos, name);	
					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
					xbt_os_mutex_release(unit->mutex);
					failure(unit);
					return;
				}
			}
			
			xbt_os_mutex_release(unit->mutex);	
			
				
		}
		else if(!strncmp(line,"setenv ", strlen("setenv ")))
		{
			char* val;
			char name[PATH_MAX + 1] = {0};
			char* p;
			unsigned int i;
			int is_blank;
			unsigned int j;
			
			p = line + strlen("setenv ");
			
			val = strchr(p, '=');
			
			if(val)
			{
				variable_t variable;
				int exists = 0;
				int env = 0;
				int err = 0;
				val++;
				
				/* syntax error */
				if(val[0] == '\0' || val[0] ==' ' || val[0] =='\t')
				{
					ERROR1("[%s] Bad usage of the metacommand setenv `(usage : setenv variable=value)'", filepos);	

					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

					failure(unit);
					return;
				}
				
				
				
				strncpy(name, p, (val - p -1));

				is_blank = 1;

				for(j = 0; j < strlen(name); j++) 
					if (name[j] != ' ' && name[j] != '\t')
						is_blank = 0;

				if(is_blank)
				{
					
					ERROR1("[%s] Bad usage of the metacommand setenv `(usage : setenv variable=value)'", filepos);
					
					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

					failure(unit);
					return;
				}
				
				/* test if the variable is already registred */
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
				
				/* if the variable is already registred, update its value;
				 * otherwise register it.
				 */
				if(exists)
				{
					if(env)
					{
						if(!strcmp(val, variable->val))
							WARN3("[%s] This environment variable `(%s)' is already set with the value `(%s)'", filepos, name, val);

						free(variable->val);
						variable->val = strdup(val);

						#ifdef _XBT_WIN32
						SetEnvironmentVariable(variable->name, variable->val);
						#else
						setenv(variable->name, variable->val, 1);
						#endif
					}
					else
					{
						if(err)
							ERROR2("[%s] Conflict : a system variable `(%s)' already exists", filepos, name);
						else
							ERROR2("[%s] Conflict : (none environment) variable `(%s)' already exists", filepos, name);	

						unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
						xbt_os_mutex_release(unit->mutex);
						failure(unit);
						return;
					}
				}
				else
				{
					if(err)
					{
						ERROR2("[%s] A system variable named `(%s)' already exists", filepos, name);
					
						unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
						xbt_os_mutex_release(unit->mutex);
						failure(unit);
						return;
					}
					else
					{
						variable = variable_new(name, val);
						variable->env = 1;
						
						xbt_dynar_push(unit->runner->variables, &variable);
						
						#ifdef _XBT_WIN32
						SetEnvironmentVariable(variable->name, variable->val);
						#else
						setenv(variable->name, variable->val, 0);
						#endif
					}
				}
				
				xbt_os_mutex_release(unit->mutex);
				
			}
			else
			{
				ERROR1("[%s] Bad usage of the metacommand setenv `(usage : setenv variable=value)'", filepos);
					
				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
				failure(unit);
				return;
			}
		}
		else if(!strncmp(line,"unset ", strlen("unset ")))
		{
			unsigned int i, j;
			int exists = 0;
			int env = 0;
			int err = 0;
			variable_t variable;
			int is_blank;

			char* name = line + strlen("unset ");

			is_blank = 1;

			for(j = 0; j < strlen(name); j++) 
				if (name[j] != ' ' && name[j] != '\t')
					is_blank = 0;

			if(is_blank)
			{
				
				ERROR1("[%s] Bad usage of the metacommand unset `(usage : unset variable)'", filepos);
				
				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

				failure(unit);
				return;
			}

			
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
				
			if(!env && !err)
			{
				if(exists)
				{
					/*xbt_dynar_remove_at(unit->runner->variables, i, NULL);*/
					/*xbt_dynar_cursor_rm(unit->runner->variables, &i);*/
					if(variable->val)
					{
						free(variable->val);
						variable->val = NULL;
					}
					else
					{
						WARN2("[%s] Variable `(%s)' already unseted",filepos, variable->name);
					}
				}	
				else
				{
					ERROR2("[%s] `(%s)' variable not found",filepos, name);	
					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
					xbt_os_mutex_release(unit->mutex);
					failure(unit);
					return;
				}
			}
			else if(env)
			{
				ERROR2("[%s] `(%s)' is an environment variable use `unsetenv' instead `unset'",filepos, name);	
				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
				xbt_os_mutex_release(unit->mutex);
				failure(unit);
				return;
			}
			else if(err)
			{
				ERROR2("[%s] `(%s)' is system variable : you can unset it",filepos, name);	
				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
				xbt_os_mutex_release(unit->mutex);
				failure(unit);
				return;
			}
			
			xbt_os_mutex_release(unit->mutex);
				
		}
		else if(!strncmp(line,"set ", strlen("set ")))
		{
			char* val;
			char name[PATH_MAX + 1] = {0};
			unsigned int j; 
			int is_blank;
			
			val = strchr(line + strlen("set "), '=');
			
			if(val)
			{
				variable_t variable;
				int exists = 0;
				unsigned int i;
				int err;
				int env;

				val++;
				
				
				/* syntax error */
				if(val[0] == '\0')
				{
					ERROR1("[%s] Bad usage of the metacommand set `(usage : set variable=value)'", filepos);
					
					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

					failure(unit);
					return;
				}
				else if(val[0] ==' ' || val[0] =='\t')
				{
					strncpy(name, line + strlen("set "), (val - (line + strlen("set "))));

					ERROR2("[%s] No space avaible after`(%s)'", filepos, name);

					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

					failure(unit);
					return;
				}
				
				
				/* assume it's a varibale */
				
				strncpy(name, line + strlen("set "), (val - (line + strlen("set ")) -1));
				
				is_blank = 1;

				for(j = 0; j < strlen(name); j++) 
					if (name[j] != ' ' && name[j] != '\t')
						is_blank = 0;

				if(is_blank)
				{
					
					ERROR1("[%s] Bad usage of the metacommand set `(usage : set variable=value)'", filepos);
					
					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

					failure(unit);
					return;
				}

				xbt_os_mutex_acquire(unit->mutex);
				
				/* test if the variable is already registred */
				xbt_dynar_foreach(unit->runner->variables, i, variable)
				{
					if(!strcmp(variable->name, name))
					{
						exists = 1;
						err = variable->err;
						env = variable->env;
						break;
					}
				}
				
				/* if the variable is already registred, update its value (if same value warns);
				 * otherwise register it.
				 */
				if(exists)
				{
					if(err)
					{
						ERROR2("[%s] A system variable named `(%s)' already exists", filepos, name);
					
						unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
						xbt_os_mutex_release(unit->mutex);

						failure(unit);
						return;
					}
					if(env)
					{
						ERROR2("[%s] `(%s)' is an environment variable use `setenv' instead `set'", filepos, name);
					
						unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
						xbt_os_mutex_release(unit->mutex);

						failure(unit);
						return;
					}
					else
					{
						if(!strcmp(val, variable->val))
							WARN3("[%s] Variable `(%s)' already contains value `<%s>'",filepos, variable->name, val);

						free(variable->val);
						variable->val = strdup(val);
					}
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
				ERROR1("[%s] Bad usage of the metacommand set `(usage : set variable=value)'", filepos);
					
				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
				failure(unit);
				return;
			}
		}
		else
		{/* assume it's	a variable */
			char* val;
			char name[PATH_MAX + 1] = {0};
			unsigned int i, j; 
			int is_blank;
			
			val = strchr(line, '=');

			if(val)
			{
				variable_t variable;
				int exists = 0;
				int err;
				int env;
				val++;
				
				
				/* syntax error */
				if(val[0] == '\0')
				{
					strncpy(name, line, (val - line -1));

					is_blank = 1;

					for(j = 0; j < strlen(name); j++) 
						if (name[j] != ' ' && name[j] != '\t')
							is_blank = 0;

					if(is_blank)
						ERROR1("[%s] Bad usage of Tesh variable mechanism `(usage : variable=value)'", filepos);
					else if(!strcmp("setenv", name))
						ERROR1("[%s] Bad usage of the metacommand setenv `(usage : setenv variable=value)'", filepos);
					else if(!strcmp("set", name))
						ERROR1("[%s] Bad usage of the metacommand set `(usage : set variable=value)'", filepos);
					else
						ERROR2("[%s] Undefined variable `(%s)'", filepos, name);

					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

					failure(unit);
					return;
				}
				else if(val[0] ==' ' || val[0] =='\t')
				{
					strncpy(name, line, (val - line));

					ERROR2("[%s] No space avaible after`(%s)'", filepos, name);

					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

					failure(unit);	 
				}
				
				
				/* assume it's a varibale */
				
				strncpy(name, line, (val - line -1));

				is_blank = 1;

				for(j = 0; j < strlen(name); j++) 
					if (name[j] != ' ' && name[j] != '\t')
						is_blank = 0;

				if(is_blank)
				{
					
					ERROR1("[%s] Bad usage of Tesh variable capability `(usage : variable=value)'", filepos);

					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

					failure(unit);
					return;
				}
				
				if(!strcmp("set", name))
				{
					ERROR1("[%s] Bad usage of the metacommand set `(usage : set variable=value)'", filepos);
					
					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
					failure(unit);
					return;
				}
				else if(!strcmp("setenv", name))
				{
					ERROR1("[%s] Bad usage of the metacommand setenv `(usage : setenv variable=value)'", filepos);
					
					unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
					failure(unit);
					return;
				}

				xbt_os_mutex_acquire(unit->mutex);
				
				/* test if the variable is already registred */
				xbt_dynar_foreach(unit->runner->variables, i, variable)
				{
					if(!strcmp(variable->name, name))
					{
						exists = 1;
						err = variable->err;
						env = variable->env;
						break;
					}
				}
				
				/* if the variable is already registred, update its value (if same value warns);
				 * otherwise register it.
				 */
				if(exists)
				{
					if(err)
					{
						ERROR2("[%s] A system variable named `(%s)' already exists", filepos, name);
					
						unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
						xbt_os_mutex_release(unit->mutex);
						failure(unit);
						return;
					}
					if(env)
					{
						ERROR2("[%s] `(%s)' is an environment variable use `setenv' metacommand", filepos, name);
					
						unit_set_error(fstream->unit, ESYNTAX, 1, filepos);
						xbt_os_mutex_release(unit->mutex);

						failure(unit);
						return;
					}
					else
					{
						if(!strcmp(val, variable->val))
							WARN3("[%s] Variable `(%s)' already contains value `<%s>'",filepos, variable->name, val);

						free(variable->val);
						variable->val = strdup(val);
					}
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
				if(!strncmp("setenv", line, strlen("setenv")))
					ERROR1("[%s] Bad usage of the metacommand setenv : `(usage : setenv variable=value)'", filepos);
				else if(!strncmp("set", line, strlen("set")))
					ERROR1("[%s] Bad usage of the metacommand set : `(usage : set variable=value)'", filepos);
				else if(!strncmp("unsetenv", line, strlen("unsetenv")))
					ERROR1("[%s] Bad usage of the metacommand unsetenv : `(usage : unsetenv variable)'", filepos);
				else if(!strncmp("unset", line, strlen("unset")))
					ERROR1("[%s] Bad usage of the metacommand unset : `(usage : unset variable)'", filepos);
				else if(!strncmp("timeout", line, strlen("timeout")))
					ERROR1("[%s] Bad usage of the metacommand timeout : `(usage : timeout <integral positive integer>)'", filepos);
				else if(!strncmp("expect signal", line, strlen("expect signal")))
					ERROR1("[%s] Bad usage of the metacommand expect signal : `(usage : expect signal <sig_name>)'", filepos);
				else if(!strncmp("expect return", line, strlen("expect return")))
					ERROR1("[%s] Bad usage of the metacommand expect return : `(usage : expect return <return value (>=0 <=255)>)'", filepos);
				else if(!strncmp("include", line, strlen("include")))
					ERROR1("[%s] Bad usage of the metacommand include  :`(usage : include <file> [<description>])'", filepos);
				else if(!strncmp("suite", line, strlen("suite")))
					ERROR1("[%s] Bad usage of the metacommand suite : `(usage : suite <description>)'", filepos);
				else
					ERROR2("[%s] Unknown metacommand: `%s'",filepos,line);

				unit_set_error(fstream->unit, ESYNTAX, 1, filepos);

				failure(unit);
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
		if(file_name[0] == '$')
		{
			ERROR3("[%s] Include file `(%s)' not found or variable `(%s)' doesn't exist",context->line, file_name, file_name + 1);
		
		}
		else
		{
			/* may be a variable */
			variable_t variable;
			int exists = 0;
			unsigned int i;

			xbt_dynar_foreach(unit->runner->variables, i, variable)
			{
				if(!strcmp(variable->name, file_name))
				{
					exists = 1;
					break;
				}
			}

			if(exists)
				ERROR3("[%s] Include file `(%s)' not found (if you want to use the variable <%s> add the prefix `$')",context->line, file_name, file_name);
			else
				ERROR2("[%s] Include file `(%s)' not found",context->line, file_name);
		}
		
		unit_set_error(fstream->unit, EINCLUDENOTFOUND, 1, context->line);

		failure(fstream->unit);

		return;
	}
	else
	{
		if(!unit->is_running_suite)
		{/* it's the unit of a suite */
			unit_t include = unit_new(unit->runner, unit->root, unit, _fstream);
			
			include->mutex = unit->root->mutex;
		
			if(description)
				include->description = strdup(description);
		
			xbt_dynar_push(unit->includes, &include);
		 
			if(!dry_run_flag)
			{
				if(description)
					INFO2("Include from %s (%s)", _fstream->name, description);
				else
					INFO1("Include from %s", _fstream->name);

			}
			else
				INFO1("Checking include %s...",_fstream->name);
			
			fstream_parse(_fstream, mutex);
		}
		else
		{/* it's a include */

			unit_t* owner;
			unit_t include;

			owner = xbt_dynar_get_ptr(unit->suites, xbt_dynar_length(unit->suites) - 1);
			
			include = unit_new(unit->runner, unit->root, *owner, _fstream);
			
			include->mutex = unit->root->mutex;
			
			if(description)
				include->description = strdup(description);
		
			xbt_dynar_push((*owner)->includes, &include);
			
			if(!dry_run_flag)
			{
				if(description)
					INFO2("Include from %s (%s)", _fstream->name, description);
				else
					INFO1("Include from %s", _fstream->name);
			}
			else
				INFO1("Checking include %s...",_fstream->name);
			
			fstream_parse(_fstream, mutex);
		}
	}
}

void
fstream_handle_suite(fstream_t fstream, const char* description, const char* filepos)
{
	unit_t unit = fstream->unit;
	unit_t suite = unit_new(unit->runner, unit->root, unit, NULL);
	
	if(description)
		suite->description = strdup(description);

	suite->filepos = strdup(filepos); 

	xbt_dynar_push(unit->suites, &suite);
	unit->is_running_suite = 1;
	
	if(!dry_run_flag)
		INFO1("Test suite %s", description);
	else
		INFO1("Checking suite %s...",description);
	
}

int
fstream_launch_command(fstream_t fstream, context_t context, xbt_os_mutex_t mutex)
{
	unit_t unit = fstream->unit;

	if(!dry_run_flag)
	{
		command_t command;
		
		if(!(command = command_new(unit, context, mutex)))
		{
			if(EINVAL == errno)
			{
				ERROR3("[%s] Cannot instantiate the command `%s' (%d)",context->pos, strerror(errno), errno);	

				unit_set_error(unit, errno, 0, context->pos);
				failure(unit);
				return -1;
			}
			else if(ENOMEM == errno)
			{
				ERROR3("[%s] Cannot instantiate the command `%s' (%d)",context->pos, strerror(errno), errno);

				unit_set_error(unit, errno, 0, context->pos);

				failure(unit);
				return -1;
			}
		}
		
		if(command_run(command) < 0)
		{
			ERROR3("[%s] Cannot run the command `%s' (%d)",context->pos, strerror(errno), errno);	
			unit_set_error(unit, errno, 0, context->pos);
			failure(unit);
			return -1;	
		}
	}
	
	if(context_reset(context) < 0)
	{
		ERROR3("[%s] Cannot reset the context of the command `%s' (%d)",context->pos, strerror(errno), errno);	

		unit_set_error(fstream->unit, errno, 0, context->pos);

		failure(unit);
		return -1;	
	}
	
	return 0;
}




