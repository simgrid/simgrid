#include <unit.h>
#include <suite.h>
#include <command.h>
#include <context.h>
#include <fstream.h>
#include <variable.h>
#include <str_replace.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

static void
replace_variables(unit_t unit, char** line)
{
	variable_t variable;
	char name[MAX_PATH + 1] = {0};
	
	/* check if some commands have setted some environment variables */
	/* TODO */
	
	/*printf("repalce all the variables of the line %s\n", *line);*/
	
	
	xbt_os_mutex_acquire(unit->mutex);
	
	vector_rewind(unit->runner->variables);
	
	while((variable = vector_get(unit->runner->variables)))
	{
		sprintf(name, "$%s", variable->name);
		/*printf("try to replace all the variable %s\n",name);*/
		str_replace_all(line, name, variable->val);
		
		vector_move_next(unit->runner->variables);
		memset(name, 0, MAX_PATH + 1);
	}
	
	xbt_os_mutex_release(unit->mutex);
	
	/*printf("line after the variables replacement %s\n",*line);*/
	
}

/* the unit thread start routine */
static void*
unit_start(void* p);

unit_t
unit_new(runner_t runner, unit_t root, unit_t owner, fstream_t fstream)
{
	unit_t unit = xbt_new0(s_unit_t, 1);

	/* set the owner of the unit */
	unit->runner = runner;
	
	unit->fstream = fstream;

	unit->sem = NULL;

	unit->commands = vector_new(DEFAULT_COMMANDS_CAPACITY, (fn_finalize_t)command_free);
	unit->includes = vector_new(DEFAULT_INCLUDES, (fn_finalize_t)unit_free);
	
	unit->thread = NULL;

	unit->number_of_started_commands = 0;
	unit->number_of_interrupted_commands = 0;
	unit->number_of_failed_commands = 0;
	unit->number_of_successeded_commands = 0;
	unit->number_of_terminated_commands = 0;
	unit->number_of_waiting_commands = 0;
	unit->interrupted = 0;
	unit->failed = 0;
	unit->successeded = 0;
	unit->parsed = 0;
	unit->released = 0;
	unit->parsing_include_file = 0;
	
	
	unit->owner = owner;
	
	unit->root = root ?  root : unit;
	
	
	unit->number = 0;
	unit->suites = vector_new(DEFAULT_SUITES_CAPACITY, (fn_finalize_t)unit_free);
	unit->owner = owner;
	
	unit->running_suite = 0;
	unit->is_suite = 0;
	unit->description = NULL;

	return unit;

}

/*void
unit_add_suite(unit_t unit, suite_t suite)
{
	vector_push_back(unit->suites, suite);
}*/

int
unit_free(void** unitptr)
{
	unit_t* __unitptr = (unit_t*)unitptr;
	
	vector_free(&((*__unitptr)->commands));
	
	vector_free(&((*__unitptr)->includes));
	
	vector_free(&((*__unitptr)->suites));
	
	/* if the unit is interrupted during its run, the semaphore is NULL */
	if((*__unitptr)->sem)
		xbt_os_sem_destroy((*__unitptr)->sem);
		
	if((*__unitptr)->description)
		free((*__unitptr)->description);
	
	free((*__unitptr)->suites);

	free(*__unitptr);
	
	*__unitptr = NULL;
	
	return 0;
}

static void*
unit_start(void* p) 
{
	
	xbt_os_thread_t thread;
	xbt_os_mutex_t mutex;
	/*context_t context;*/
	int i, j;

	unit_t unit = (unit_t)p;
	unit_t include;
	
	xbt_os_mutex_acquire(unit->mutex);
	unit->runner->number_of_runned_units++;
	xbt_os_mutex_release(unit->mutex);

	/* try to acquire the jobs semaphore to start */
	xbt_os_sem_acquire(jobs_sem);
	
	mutex = xbt_os_mutex_init();
	/*context = context_new();*/
	
	if(want_dry_run)
		INFO1("checking unit %s...",unit->fstream->name); 
	
	/* parse the file */
	/*unit_parse(unit, context, mutex, unit->fstream->name, unit->fstream->stream);*/
	
	fstream_parse(unit->fstream, unit, mutex);
	
	
	/* if the unit is not interrupted and not failed the unit, all the file is parsed
	 * so all the command are launched
	 */
	 
	
	if(!unit->interrupted)
	{
		unit->parsed = 1;
		
		/* all the commands have terminate before the end of the parsing of the tesh file
		 * so the unit release the semaphore itself
		 */
		if(!unit->released && (unit->number_of_started_commands == (unit->number_of_failed_commands + unit->number_of_interrupted_commands + unit->number_of_successeded_commands)))
		{
			/*INFO1("the unit %s is released", unit->fstream->name);*/
			xbt_os_sem_release(unit->sem);	
		}
		else
		{
			
			INFO1("the unit %s is not released", unit->fstream->name);
			INFO1("number of started commands %d", unit->number_of_started_commands);
			INFO1("number of failed commands %d", unit->number_of_failed_commands);
			INFO1("number of interrupted commands %d", unit->number_of_interrupted_commands);
			INFO1("number of successeded commands %d", unit->number_of_successeded_commands);
			INFO1("number of waiting commands %d", unit->number_of_waiting_commands);
			
			
			if(unit->number_of_waiting_commands)
			{
				command_t command;
				int i, j;
	
				for(i = 0; i < vector_get_size(unit->includes) ; i++)
				{
					include = vector_get_at(unit->includes, i);
				
					for(j = 0; j < vector_get_size(include->commands); j++)
					{
						command = vector_get_at(include->commands, j);
						
						if(command->status == cs_in_progress)
						{
							INFO2("the command %s PID %d is in process", command->context->command_line, command->pid);
							
							if(command->writer->done)
								INFO2("the writer of the command %s PID %d done", command->context->command_line, command->pid);	
							else
								INFO2("the writer of the command %s PID %d  doesn't done", command->context->command_line, command->pid);
						}
					}
					
				}
			}
		}
		
	}
	
	/* wait the end of all the commands or a command failure or an interruption */
	xbt_os_sem_acquire(unit->sem);
	
	
	if(unit->interrupted)
	{
		command_t command;
		
		/* interrupt all the running commands of the unit */ 
		for(i = 0; i < vector_get_size(unit->commands); i++)
		{
			command = vector_get_at(unit->commands, i);

			if(command->status == cs_in_progress)
				command_interrupt(command);
		}
		
		for(i = 0; i < vector_get_size(unit->includes); i++)
		{
			include = vector_get_at(unit->includes, i);
			
			for(j = 0; j < vector_get_size(include->commands); j++)
			{
				command = vector_get_at(include->commands, j);

				if(command->status == cs_in_progress)
					command_interrupt(command);
			}
		}
		
	}
	
	/* wait the end of the threads */
	for(i = 0; i < vector_get_size(unit->commands); i++)
	{
		command_t command = vector_get_at(unit->commands, i);
		thread = command->thread;
		
		if(thread)
			xbt_os_thread_join(thread,NULL);
	}
	
	for(i = 0; i < vector_get_size(unit->includes); i++)
	{
		include = vector_get_at(unit->includes, i);
		
		for(j = 0; j < vector_get_size(include->commands); j++)
		{
			command_t command = vector_get_at(include->commands, j);
			thread = command->thread;
		
			if(thread)
				xbt_os_thread_join(thread,NULL);
		}
	}
	
	/*context_free(&context);*/
	
	xbt_os_mutex_destroy(mutex);
	
	
	xbt_os_mutex_acquire(unit->mutex);

	/* increment the number of ended units */
	unit->runner->number_of_ended_units++;
	
	/* it's the last unit, release the runner */
	if(/*!unit->interrupted &&*/ (unit->runner->number_of_runned_units == unit->runner->number_of_ended_units))
	{
		if(unit->number_of_successeded_commands == unit->number_of_commands)
			unit->successeded = 1;

		/* first release the mutex */
		xbt_os_mutex_release(unit->mutex);
		
		xbt_os_sem_release(units_sem);
	}
	else
		xbt_os_mutex_release(unit->mutex);
	
	/* release the jobs semaphore, then the next unit can start */
	xbt_os_sem_release(jobs_sem);
	
	return NULL;

}

void
unit_parse(unit_t unit, context_t context, xbt_os_mutex_t mutex, const char* file_name, FILE* stream)
{
	size_t len;
	char * line = NULL;
	int line_num=0;
	char file_pos[256];
	xbt_strbuff_t buff;
	int buffbegin = 0; 
	
	/* Count the line length while checking wheather it's blank */
	int blankline;
	int linelen;    
	/* Deal with \ at the end of the line, and call handle_line on result */
	int to_be_continued;
	
	buff=xbt_strbuff_new();
	
	while(!unit->interrupted  && getline(&line, &len, stream) != -1)
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
				
				if(unit->parsing_include_file)
					ERROR1("Unit `%s': NOK (syntax error)", unit->fstream->name);
				else
					ERROR2("Unit `%s' inclued in `%s' : NOK (syntax error)", file_name, unit->fstream->name);	
				
				exit_code = ESYNTAX;
				unit_handle_failure(unit);
				break;
			}
			else if(unit->running_suite)
			{
				/* TODO */
			}
					
			
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
			if (linelen>2 && line[linelen-3] == '\\') 
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
				snprintf(file_pos,256,"%s:%d",file_name,buffbegin);
				unit_handle_line(unit, context, mutex, file_pos, buff->data);    
				xbt_strbuff_empty(buff);
			}
		} 
		else 
		{
			snprintf(file_pos,256,"%s:%d",file_name,line_num);
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
}



void 
unit_handle_line(unit_t unit, context_t context, xbt_os_mutex_t mutex, const char * filepos, char *line) 
{
	char* line2;
	/* Search end */
	xbt_str_rtrim(line+2,"\n");
	
	line2 = strdup(line);
	
	replace_variables(unit, &line2);
	
	switch (line2[0]) 
	{
		case '#': 
		break;
		
		case '$':
		case '&':
			
		context->async = (line2[0] == '&');
		
		/* further trim useless chars which are significant for in/output */
		xbt_str_rtrim(line2+2," \t");
		
		/* Deal with CD commands here, not in rctx */
		if(!strncmp("cd ",line2 + 2, 3)) 
		{
			/*char *dir=line2+4; */
			char* dir = strdup(line2 + 4);
			
			if(context->command_line)
			{
				if(!want_dry_run)
				{
					command_t command = command_new(unit, context, mutex);
					command_run(command);
				}
				
				context_reset(context);
			}
		
			/* search begining */
			while (*(dir++) == ' ');
			
			dir--;
			
			VERB1("Saw cd '%s'",dir);
			
			
			if(!want_dry_run)
			{
			
				if(chdir(dir))
				{
					ERROR2("Chdir to %s failed: %s",dir,strerror(errno));
					ERROR1("Test suite `%s': NOK (system error)", unit->fstream->name);
					exit_code = ECHDIR;
					unit_handle_failure(unit);
				}
				
				
			}
			
			break;
		} /* else, pushline */
		else
		{
			unit_pushline(unit, context, mutex, filepos, line2[0], line2+2 /* pass '$ ' stuff*/);
			break;
		}
		
		case '<':
		case '>':
		case '!':
		unit_pushline(unit, context, mutex, filepos, line2[0], line2+2 /* pass '$ ' stuff*/);    
		break;
		
		case 'p':
		if(!want_dry_run)
			INFO2("[%s] %s",filepos,line2+2);
		break;
		
		case 'P':
		if(!want_dry_run)
			CRITICAL2("[%s] %s",filepos,line2+2);
		break;
		
		case 'D':
			if(unit->description)
				WARN2("description already specified %s %s", filepos, line2); 
			else
				unit->description = strdup(line2 + 2);
		break;
		
		default:
		ERROR2("[%s] Syntax error: %s",filepos, line2);
		ERROR1("Test suite `%s': NOK (syntax error)",unit->fstream->name);
		exit_code = ESYNTAX;
		unit_handle_failure(unit);
		break;
	}
	
	free(line2);
}

void 
unit_pushline(unit_t unit, context_t context, xbt_os_mutex_t mutex, const char* filepos, char kind, char *line) 
{
	switch (kind) 
	{
		case '$':
		case '&':
		
		if(context->command_line) 
		{
			command_t command;
			
			if(context->output->used || context->input->used) 
			{
				ERROR2("[%s] More than one command in this chunk of lines (previous: %s).\nDunno which input/output belongs to which command.",filepos,context->command_line);
				ERROR1("Test suite `%s': NOK (syntax error)",unit->fstream->name);
				exit_code = ESYNTAX;
				unit_handle_failure(unit);
				return;
			}
			
			if(!want_dry_run)
			{
				command = command_new(unit, context, mutex);
				command_run(command);
			}
			
			context_reset(context);
			
			VERB1("[%s] More than one command in this chunk of lines",filepos);
		}
		
		context->command_line = strdup(line);
		
		
		context->line = strdup(filepos);
		/*INFO2("[%s] %s",filepos,context->command_line);*/
		
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
			if(!want_dry_run)
			{
				command_t command = command_new(unit, context, mutex);
				command_run(command);
			}
			
			context_reset(context);
		}
		
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
					exit_code = ESYNTAX;
					ERROR2("Invalid timeout value `%s' at %s ", line + strlen("timeout "), filepos);
					unit_handle_failure(unit);
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
			if(!strstr(context->signal,"SIGSEGVSIGTRAPSIGBUSSIGFPESIGILL"))
			{
				exit_code = ESYNTAX;
				/*ERROR2("Signal `%s' not supported at %s", line + strlen("expect signal "), filepos);*/
				unit_handle_failure(unit);
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
					exit_code = ESYNTAX;
					ERROR2("Invalid exit code value `%s' at %s ", line + strlen("expect return "), filepos);
					unit_handle_failure(unit);
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
				
				exit_code = ESYNTAX;
				ERROR1("include file not specified %s ", filepos);
				unit_handle_failure(unit);
			}
			else
			{
				char file_name[MAX_PATH + 1] = {0};
				
				/*INFO1("p1 is %s",p1);*/
				
				p2 = p1;
				
				while(*p2 != '\0' && *p2 != ' ' && *p2 != '\t')
				{
					/*INFO1("p2 is %s",p2);*/
					p2++;
					
				}
				/*INFO1("p2 is %s",p2);*/
					
				strncpy(file_name, p1, p2 - p1);
				
				/*INFO1("filename is %s", file_name);*/
				
				if(p2[0] != '\0')
					while(*p2 == ' ' || *p2 == '\t')
						p2++;
					
				unit_handle_include(unit, context, mutex, file_name, p2[0] != '\0' ? p2 : NULL);
				
			}
		}
		else if(!strncmp(line,"suite ", strlen("suite ")))
		{
			unit_handle_suite(unit, context, mutex, line + strlen("suite "));
		}
		else if(!strncmp(line,"unsetenv ", strlen("unsetenv ")))
		{
			int i;
			int number_of_variables;
			int exists = 0;
			int env;
			variable_t variable;
			char* name = line + strlen("unsetenv ");
			
			xbt_os_mutex_acquire(unit->mutex);
			
			number_of_variables = vector_get_size(unit->runner->variables);
			
			for(i = 0; i < number_of_variables; i++)
			{
				variable = vector_get_at(unit->runner->variables, i);
				
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
					vector_erase_at(unit->runner->variables, i);
				else
					WARN3("environment variable %s not found %s %s", name, line, filepos);	
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
			char name[MAX_PATH + 1] = {0};
			char* p;
			
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
					
					exit_code = ESYNTAX;
					ERROR2("indefinite variable value %s %s", line, filepos);
					unit_handle_failure(unit);	
				}
				
				
				
				strncpy(name, p, (val - p -1));
				
				/* test if the variable is already registred */
				
				xbt_os_mutex_acquire(unit->mutex);
				
				vector_rewind(unit->runner->variables);
				
				while((variable = vector_get(unit->runner->variables)))
				{
					
					if(!strcmp(variable->name, name))
					{
						variable->env = 1;
						exists = 1;
						break;
					}
					
					vector_move_next(unit->runner->variables);
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
						setenv(variable->name, variable->val, 1);
					}
					else
						WARN3("%s variable already exists %s %s", name, line, filepos);	
				}
				else
				{
					variable = variable_new(name, val);
					variable->env = 1;
					
					vector_push_back(unit->runner->variables, variable);
					
					setenv(variable->name, variable->val, 0);
				}
				
				xbt_os_mutex_release(unit->mutex);
				
			}
		}
		else if(!strncmp(line,"unset ", strlen("unset ")))
		{
			int i;
			int number_of_variables;
			int exists = 0;
			int env;
			int err;
			variable_t variable;
			char* name = line + strlen("unset ");
			
			xbt_os_mutex_acquire(unit->mutex);
			
			number_of_variables = vector_get_size(unit->runner->variables);
			
			for(i = 0; i < number_of_variables; i++)
			{
				variable = vector_get_at(unit->runner->variables, i);
				
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
					vector_erase_at(unit->runner->variables, i);
				else
					WARN3("variable %s not found %s %s", name, line, filepos);	
			}
			else if(env)
			{
				WARN3("%s is an environment variable use unsetenv metacommand to delete it %s %s", name, line, filepos);	
			}
			else
			{
				WARN3("%s is an error variable : you are not allowed to delete it %s %s", name, line, filepos);
			}
			xbt_os_mutex_release(unit->mutex);
				
		}
		else
		{
			/* may be a variable */
			char* val;
			char name[MAX_PATH + 1] = {0};
			
			val = strchr(line, '=');
			
			if(val)
			{
				variable_t variable;
				int exists = 0;
				val++;
				
				/* syntax error */
				if(val[0] == '\0')
				{
					
					exit_code = ESYNTAX;
					ERROR2("indefinite variable value %s %s", line, filepos);
					unit_handle_failure(unit);	
				}
				
				
				/* assume it's a varibale */
				strncpy(name, line, (val - line -1));
				
				xbt_os_mutex_acquire(unit->mutex);
				
				/* test if the variable is already registred */
				
				vector_rewind(unit->runner->variables);
				
				while((variable = vector_get(unit->runner->variables)))
				{
					
					if(!strcmp(variable->name, name))
					{
						exists = 1;
						break;
					}
					
					vector_move_next(unit->runner->variables);
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
					vector_push_back(unit->runner->variables,variable_new(name, val));
					
				xbt_os_mutex_release(unit->mutex);
				
			}
			else 
			{
				ERROR2("%s: Malformed metacommand: %s",filepos,line);
				ERROR1("Test suite `%s': NOK (syntax error)",unit->fstream->name);
				
				exit_code = ESYNTAX;
				unit_handle_failure(unit);
				return;
			}
		}
		
		
		break;
	}
}


void
unit_handle_failure(unit_t unit)
{
	
	if(!want_keep_going_unit)
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
		if(!want_keep_going)
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

void
unit_run(unit_t unit, xbt_os_mutex_t mutex)
{
	if(!interrupted)
	{
		unit->mutex = mutex;
		
		unit->sem = xbt_os_sem_init(0);

		/* start the unit */
		unit->thread = xbt_os_thread_create("", unit_start, unit);
	}
	else
		/* the unit is interrupted by the runner before its starting 
		 * in this case the unit semaphore is NULL take care of that
		 * in the function unit_free()
		 */
		unit->interrupted = 1;

	
}

void
unit_interrupt(unit_t unit)
{
	/* interrupt the loop */
	unit->interrupted = 1;
	xbt_os_sem_release(unit->sem);
}

void 
display_title(const char* description)
{
	int i;
	char title[80];
	int len = strlen(description);
		
	title[0]=' ';
		
	for (i = 1; i < 79; i++)
		title[i]='=';
		
	title[i++]='\n';
	title[79]='\0';
	
	sprintf(title + 40 - (len + 4)/2, "[ %s ]",description);
	title[40 + (len + 5 ) / 2] = '=';
		
	printf("\n%s\n",title);	
}

void
unit_verbose(unit_t unit)
{
	int i, j, k;
	command_t command;
	unit_t include;
	unit_t suite;
	char* p;
	char title[MAX_PATH + 1] = {0};
	
	int number_of_tests = 0;						/* number of tests of a unit contained by this unit					*/
	int number_of_failed_tests = 0;					/* number of failed test of a unit contained by this unit			*/
	int number_of_successeded_tests = 0;			/* number of successeded tests of a unit contained by this unit		*/
	int number_of_interrupted_tests = 0;			/* number of interrupted tests of a unit contained by this unit		*/
	
	int number_of_tests_of_suite = 0;				/* number of tests of a suite contained by this unit				*/
	int number_of_interrupted_tests_of_suite = 0;	/* number of interrupted tests of a suite contained by this unit	*/	
	int number_of_failed_tests_of_suite = 0;		/* number of failed tests of a suite contained by this unit					*/
	int number_of_successeded_tests_of_suite = 0;	/* number of successeded tests of a suite contained by this			*/
	
	int number_of_units = 0;						/* number of units contained by a suite								*/
	int number_of_failed_units = 0;					/* number of failed units contained by a suite						*/
	int number_of_successeded_units = 0;			/* number of successeded units contained by a suite					*/
	int number_of_interrupted_units = 0;			/* number of interrupted units contained by a suite					*/
	
	int total_of_tests = 0;							/* total of the tests contained by this unit						*/
	int total_of_failed_tests = 0;					/* total of failed tests contained by this unit						*/
	int total_of_successeded_tests = 0; 			/* total of successeded tests contained by this unit				*/
	int total_of_interrupted_tests = 0;				/* total of interrupted tests contained by this unit				*/
	
	int total_of_units = 0;							/* total of units contained by this unit							*/
	int total_of_failed_units = 0;					/* total of failed units contained by this unit						*/
	int total_of_successeded_units = 0;				/* total of successeded units contained by this unit				*/
	int total_of_interrupted_units = 0;				/* total of interrutped units contained by this unit				*/
	
	int total_of_suites = 0;						/* total of suites contained by this unit							*/
	int total_of_failed_suites = 0;					/* total of failed suites contained by this unit					*/
	int total_of_successeded_suites = 0;			/* total of successeded suites contained by this unit				*/ 
	int total_of_interrupted_suites = 0;			/* total of interrupted suites contained by this unit				*/
	
	
	if(unit->description)
		strcpy(title, unit->description);
	else
		sprintf(title, "file : %s",unit->fstream->name);
		
	if(unit->interrupted)
		strcat(title, " (interrupted)");
		
	display_title(title);
	
	number_of_tests = vector_get_size(unit->commands);
	
	
	/* tests */
	for(i = 0; i < number_of_tests; i++)
	{
		command = vector_get_at(unit->commands, i);
		
		if(command->status == cs_interrupted)
			number_of_interrupted_tests++;
		else if(command->status == cs_failed)
			number_of_failed_tests++;
		else if(command->status == cs_successeded)
			number_of_successeded_tests++;
			
	}
	
	if(number_of_tests)
	{
		asprintf(&p," Test(s): .........................................................................");
			
		p[70] = '\0';
		printf("%s", p);
		free(p);	
	
		if(number_of_failed_tests > 0) 
			printf(".. failed\n");
		else if(number_of_interrupted_tests > 0) 
			printf("interrupt\n");
		else 
			printf(".... ..ok\n"); 
	
	
		for(i = 0; i < number_of_tests; i++)
		{
			command = vector_get_at(unit->commands, i);
				
			printf("        %s: %s [%s]\n", 
				command->status == cs_interrupted ? "INTR  " 
				: command->status == cs_failed ? "FAILED" 
				: command->status == cs_successeded ? "PASS  " 
				: "UNKNWN",
				command->context->command_line, 
				command->context->line);
				
			if(want_detail_summary)
				command_display_status(command);
				
		}
	
		printf(" =====================================================================%s\n",
		number_of_failed_tests ? "== FAILED": number_of_interrupted_tests ? "==== INTR" : "====== OK");
		
		printf("    Summary: Test(s): %.0f%% ok (%d test(s): %d ok",
		((1-((double)number_of_failed_tests + (double)number_of_interrupted_tests)/(double)number_of_tests)*100.0),
		 number_of_tests, number_of_successeded_tests);
		
		if(number_of_failed_tests > 0)
			printf(", %d failed", number_of_failed_tests);
		
		if(number_of_interrupted_tests > 0)
		 	printf(", %d interrupted)", number_of_interrupted_tests);
		 	
		 printf(")\n\n");
				
		total_of_tests = number_of_tests;
		total_of_failed_tests = number_of_failed_tests;
		total_of_interrupted_tests = number_of_interrupted_tests;
		total_of_successeded_tests = number_of_successeded_tests;
	}
	
	
	
	/* includes */
	
	total_of_failed_units = total_of_interrupted_units = total_of_successeded_units = 0;
	
	number_of_failed_units = number_of_successeded_units = number_of_interrupted_units = 0;
	
	number_of_units = vector_get_size(unit->includes);
	
	for(i = 0; i < number_of_units ; i++)
	{
		include = vector_get_at(unit->includes, i);
		
		number_of_interrupted_tests = number_of_failed_tests = number_of_successeded_tests = 0;
		
		number_of_tests = vector_get_size(include->commands);
		
		for(j = 0; j < number_of_tests; j++)
		{
			command = vector_get_at(include->commands, j);
			
			if(command->status == cs_interrupted)
				number_of_interrupted_tests++;
			else if(command->status == cs_failed)
				number_of_failed_tests++;
			else if(command->status == cs_successeded)
				number_of_successeded_tests++;	
		}
		
		asprintf(&p," Unit: %s ............................................................................", include->description ? include->description : include->fstream->name);
			
		p[70] = '\0';
		printf("%s", p);
		free(p);	
		
		
		if(number_of_failed_tests > 0) 
		{
			total_of_failed_units++;
			printf(".. failed\n");
		}
		else if(number_of_interrupted_tests > 0) 
		{
			total_of_interrupted_units++;
			printf("interrupt\n");
		}
		else 
		{
			total_of_successeded_units++;
			printf(".... ..ok\n"); 
		}
		
		if(want_detail_summary)
		{		
	
			for(j = 0; j < vector_get_size(include->commands); j++)
			{
				command = vector_get_at(include->commands, j);
				
				printf("        %s: %s [%s]\n", 
				command->status == cs_interrupted ? "INTR  " 
				: command->status == cs_failed ? "FAILED" 
				: command->status == cs_successeded ? "PASS  " 
				: "UNKNWN",
				command->context->command_line, 
				command->context->line);
				
				command_display_status(command);
			}
			
					
		}
		
		printf(" =====================================================================%s\n",
		number_of_failed_tests ? "== FAILED": number_of_interrupted_tests ? "==== INTR" : "====== OK");
	
		
		printf("    Summary: Test(s): %.0f%% ok (%d test(s): %d ok",
		(number_of_tests ? (1-((double)number_of_failed_tests + (double)number_of_interrupted_tests)/(double)number_of_tests)*100.0 : 100.0),
		 number_of_tests, number_of_successeded_tests);
		
		if(number_of_failed_tests > 0)
			printf(", %d failed", number_of_failed_tests);
		
		if(number_of_interrupted_tests > 0)
		 	printf(", %d interrupted)", number_of_interrupted_tests);
		 	
		 printf(")\n\n");	
		 
		
		total_of_tests += number_of_tests;
		total_of_failed_tests += number_of_failed_tests;
		total_of_interrupted_tests += number_of_interrupted_tests;
		total_of_successeded_tests += number_of_successeded_tests;
	}
	
	/* suites */
	
	total_of_units = number_of_units;
	
	total_of_failed_suites = total_of_successeded_suites = total_of_interrupted_suites = 0;
	
	total_of_suites = vector_get_size(unit->suites);
	
	for(k = 0; k < total_of_suites; k++)
	{
		suite = vector_get_at(unit->suites, k);
		
		display_title(suite->description);
		
		number_of_tests_of_suite = number_of_interrupted_tests_of_suite = number_of_failed_tests_of_suite = number_of_successeded_tests_of_suite = 0;
		
		number_of_interrupted_units = number_of_failed_units = number_of_successeded_units = 0;
		
		number_of_units = vector_get_size(suite->includes);
		
		for(i = 0; i < number_of_units; i++)
		{
			number_of_interrupted_tests = number_of_failed_tests = number_of_successeded_tests = 0;
			
			number_of_tests = vector_get_size(include->commands);
			
			for(j = 0; j < vector_get_size(include->commands); j++)
			{
				command = vector_get_at(include->commands, j);
				
				if(command->status == cs_interrupted)
					number_of_interrupted_tests++;
				else if(command->status == cs_failed)
					number_of_failed_tests++;
				else if(command->status == cs_successeded)
					number_of_successeded_tests++;
					
			}
			
			
			include = vector_get_at(suite->includes, i);
			asprintf(&p," Unit: %s ............................................................................", include->description ? include->description : include->fstream->name);
			
			p[70] = '\0';
			printf("%s", p);
			free(p);	
		
			if(number_of_failed_tests > 0) 
			{
				number_of_failed_units++;
				printf(".. failed\n");
			}
			else if(number_of_interrupted_tests > 0) 
			{
				number_of_interrupted_units++;
				printf("interrupt\n");
			}
			else 
			{
				number_of_successeded_units++;
				printf(".... ..ok\n"); 
			} 
			
			number_of_interrupted_tests_of_suite += number_of_interrupted_tests;
			number_of_failed_tests_of_suite += number_of_failed_tests;
			number_of_successeded_tests_of_suite += number_of_successeded_tests;
			
			number_of_tests_of_suite += number_of_tests;
			
			total_of_tests += number_of_tests;
			total_of_failed_tests += number_of_failed_tests;
			total_of_interrupted_tests += number_of_interrupted_tests;
			total_of_successeded_tests += number_of_successeded_tests;
			
			if(want_detail_summary)
			{
				for(j = 0; j < vector_get_size(include->commands); j++)
				{
					command = vector_get_at(include->commands, j);
					
					printf("        %s: %s [%s]\n", 
					command->status == cs_interrupted ? "INTR  " 
					: command->status == cs_failed ? "FAILED" 
					: command->status == cs_successeded ? "PASS  " 
					: "UNKNWN",
					command->context->command_line, 
					command->context->line);
					
					command_display_status(command);
						
				}
				
				
			}
				
		}
		
		
		
		printf(" =====================================================================%s\n",
		number_of_failed_tests_of_suite ? "== FAILED": number_of_interrupted_tests_of_suite ? "==== INTR" : "====== OK");
		
		if(number_of_failed_tests_of_suite > 0)
			total_of_failed_suites++;
		else if(number_of_interrupted_tests_of_suite)
			total_of_interrupted_suites++;
		else
			total_of_successeded_suites++;
			
		total_of_failed_units += number_of_failed_units;
		total_of_interrupted_units += number_of_interrupted_units;
		total_of_successeded_units += number_of_successeded_units;
			
		total_of_units += number_of_units;
		
		printf("    Summary: Unit(s): %.0f%% ok (%d unit(s): %d ok",
		(number_of_units ? (1-((double)number_of_failed_units + (double)number_of_interrupted_units)/(double)number_of_units)*100.0 : 100.0),
		 number_of_units, number_of_successeded_units);
		 
		if(number_of_failed_units > 0)
			printf(", %d failed", number_of_failed_units);
		
		if(number_of_interrupted_units > 0)
		 	printf(", %d interrupted)", number_of_interrupted_units);
		 	
		printf(")\n");	
		
		printf("             Test(s): %.0f%% ok (%d test(s): %d ok",
		(number_of_tests_of_suite ? (1-((double)number_of_failed_tests_of_suite + (double)number_of_interrupted_tests_of_suite)/(double)number_of_tests_of_suite)*100.0 : 100.0),
		number_of_tests_of_suite, number_of_successeded_tests_of_suite);
		 
		if(number_of_failed_tests_of_suite > 0)
			printf(", %d failed", number_of_failed_tests_of_suite);
		
		if(number_of_interrupted_tests_of_suite > 0)
		 	printf(", %d interrupted)", number_of_interrupted_tests_of_suite);
		 	
		 printf(")\n\n");	
	}
	
	printf(" TOTAL : Suite(s): %.0f%% ok (%d suite(s): %d ok",
		(total_of_suites ? (1-((double)total_of_failed_suites + (double)total_of_interrupted_suites)/(double)total_of_suites)*100.0 : 100.0),
		 total_of_suites, total_of_successeded_suites);
	
	if(total_of_failed_suites > 0)
			printf(", %d failed", total_of_failed_suites);
		
	if(total_of_interrupted_suites > 0)
	 	printf(", %d interrupted)", total_of_interrupted_suites);
	 	
	printf(")\n");	
	
	printf("         Unit(s):  %.0f%% ok (%d unit(s): %d ok",
		(total_of_units ? (1-((double)total_of_failed_units + (double)total_of_interrupted_units)/(double)total_of_units)*100.0 : 100.0),
		 total_of_units, total_of_successeded_units);
	
	if(total_of_failed_units > 0)
			printf(", %d failed", total_of_failed_units);
		
	if(total_of_interrupted_units > 0)
	 	printf(", %d interrupted)", total_of_interrupted_units);
	 	
	printf(")\n");
	
	printf("         Test(s):  %.0f%% ok (%d test(s): %d ok",
		(total_of_tests ? (1-((double)total_of_failed_tests + (double)total_of_interrupted_tests)/(double)total_of_tests)*100.0 : 100.0),
		 total_of_tests, total_of_successeded_tests); 
		 
	if(total_of_failed_tests > 0)
			printf(", %d failed", total_of_failed_tests);
		
	if(total_of_interrupted_tests > 0)
	 	printf(", %d interrupted)", total_of_interrupted_tests);
	 	
	printf(")\n\n");
	
	
	if(unit->interrupted)
		unit->runner->total_of_interrupted_units++;
	else if(total_of_failed_tests > 0)
		unit->runner->total_of_failed_units++;
	else
		unit->runner->total_of_successeded_units++;
	
	
	unit->runner->total_of_tests += total_of_tests;
	unit->runner->total_of_failed_tests += total_of_failed_tests;
	unit->runner->total_of_successeded_tests += total_of_successeded_tests;
	unit->runner->total_of_interrupted_tests += total_of_interrupted_tests;
	
	
	unit->runner->total_of_units += total_of_units + 1;
	unit->runner->total_of_successeded_units += total_of_successeded_units;
	unit->runner->total_of_failed_units += total_of_failed_units;
	unit->runner->total_of_interrupted_units += total_of_interrupted_units;
	
	
	unit->runner->total_of_suites += total_of_suites;
	unit->runner->total_of_successeded_suites += total_of_successeded_suites;
	unit->runner->total_of_failed_suites += total_of_failed_suites;
	unit->runner->total_of_interrupted_suites += total_of_interrupted_suites;
	
	
	
}




void
unit_handle_include(unit_t unit, context_t context, xbt_os_mutex_t mutex, const char* file_name, const char* description)
{
	directory_t dir;
	char* prev_directory = NULL;
	fstream_t fstream = NULL;
	struct stat buffer = {0};
	
	if(!stat(file_name, &buffer) && S_ISREG(buffer.st_mode))
	{
		/* the file is in the current directory */
		
		fstream = fstream_new(getcwd(NULL, 0), file_name);
		fstream_open(fstream);
	}
	/* the file to include is not in the current directory, check if it is in a include directory */
	else
	{
		prev_directory = getcwd(NULL, 0);
		
		vector_rewind(include_dirs);
		
		while((dir = vector_get(include_dirs)))
		{
			chdir(dir->name);
			
			if(!stat(file_name, &buffer) && S_ISREG(buffer.st_mode))
			{
				fstream = fstream_new(dir->name, file_name);
				fstream_open(fstream);
				break;
			}
			
			
			vector_move_next(include_dirs);
		}
		
		chdir(prev_directory);
		free(prev_directory);
	}
	
	
	
	/* the file to include is not found handle the failure */
	if(!fstream)
	{
		exit_code = EINCLUDENOTFOUND;
		ERROR1("Include file %s not found",file_name);
		unit_handle_failure(unit);
	}
	else
	{
		
		if(!unit->running_suite)
		{/* it's the unit of a suite */
			unit_t include = unit_new(unit->runner,unit->root, unit, fstream);
			
			include->mutex = unit->root->mutex;
		
			if(description)
				include->description = strdup(description);
		
			vector_push_back(unit->includes, include);
		
			fstream_parse(fstream, include, mutex);
		}
		else
		{/* it's a include */
			unit_t owner = vector_get_back(unit->suites);
			unit_t include = unit_new(unit->runner,unit->root, owner, fstream);
			
			include->mutex = unit->root->mutex;
			
			if(description)
				include->description = strdup(description);
		
			vector_push_back(owner->includes, include);
		
			fstream_parse(fstream, include, mutex);	
		}
	}
}

void
unit_handle_suite(unit_t unit, context_t context, xbt_os_mutex_t mutex, const char* description)
{
	if(unit->running_suite)
	{
		exit_code = ESYNTAX;
		unit_handle_failure(unit);
	}
	else
	{
		unit_t suite = unit_new(unit->runner, unit->root, unit, NULL);
		suite->is_suite = 1;
		suite->description = strdup(description);
		vector_push_back(unit->suites, suite);
		unit->running_suite = 1;
	}
}

int
unit_reset(unit_t unit)
{
	fseek(unit->fstream->stream,0L, SEEK_SET);
	unit->parsed = 0;
	unit->number_of_commands = 0;
	
	return 0;
}
