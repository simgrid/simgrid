#include <unit.h>
#include <suite.h>
#include <command.h>
#include <context.h>
#include <fstream.h>



XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);


/* the unit thread start routine */
static void*
unit_start(void* p);

unit_t
unit_new(runner_t runner, suite_t owner, fstream_t fstream)
{
	unit_t unit = xbt_new0(s_unit_t, 1);

	/* set the owner of the unit */
	unit->runner = runner;
	
	unit->fstream = fstream;

	unit->sem = NULL;

	unit->commands = vector_new(DEFAULT_COMMANDS_CAPACITY, (fn_finalize_t)command_free);
	
	unit->thread = NULL;

	unit->number_of_started_commands = 0;
	unit->number_of_interrupted_commands = 0;
	unit->number_of_failed_commands = 0;
	unit->number_of_successeded_commands = 0;
	unit->number_of_terminated_commands = 0;
	unit->interrupted = 0;
	unit->failed = 0;
	unit->successeded = 0;
	unit->parsed = 0;
	unit->released = 0;
	unit->parsing_include_file = 0;
	
	
	unit->owner = owner;
	unit->number = 0;
	unit->suites = vector_new(DEFAULT_COMMANDS_CAPACITY, (fn_finalize_t)suite_free);
	unit->owner = owner;
	
	unit->running_suite = 0;

	return unit;

}

void
unit_add_suite(unit_t unit, suite_t suite)
{
	vector_push_back(unit->suites, suite);
}

int
unit_free(void** unitptr)
{
	unit_t* __unitptr = (unit_t*)unitptr;
	
	vector_free(&((*__unitptr)->commands));
	
	vector_free(&((*__unitptr)->suites));
	
	/* if the unit is interrupted during its run, the semaphore is NULL */
	if((*__unitptr)->sem)
		xbt_os_sem_destroy((*__unitptr)->sem);
		
		
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
	context_t context;
	int i;

	unit_t unit = (unit_t)p;
	
	xbt_os_mutex_acquire(unit->mutex);
	unit->runner->number_of_runned_units++;
	xbt_os_mutex_release(unit->mutex);

	/* try to acquire the jobs semaphore to start */
	xbt_os_sem_acquire(jobs_sem);
	
	mutex = xbt_os_mutex_init();
	context = context_new();
	
	if(want_dry_run)
		INFO1("checking unit %s...",unit->fstream->name); 
	
	/* parse the file */
	unit_parse(unit, context, mutex, unit->fstream->name, unit->fstream->stream);
	
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
			xbt_os_sem_release(unit->sem);	
		
	}
	
	/* wait the end of all the commands or a command failure or an interruption */
	
	
	xbt_os_sem_acquire(unit->sem);
	
	if(unit->interrupted)
	{
		command_t command;

		/* interrupt all the running commands of the unit */ 
		for(i = 0; i < unit->number_of_commands; i++)
		{
			/*command = unit->commands[i];*/
			command = vector_get_at(unit->commands, i);

			if(command->status == cs_in_progress)
				/*command_interrupt(unit->commands[i]);*/
				command_interrupt(command);
		}
	}
	

	/* wait the end of the threads */
	for(i = 0; i < unit->number_of_commands; i++)
	{
		/*thread = unit->commands[i]->thread;*/
		
		command_t command = vector_get_at(unit->commands, i);
		thread = command->thread;
		
		if(thread)
			xbt_os_thread_join(thread,NULL);
	}
	
	context_free(&context);

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
	/* Search end */
	xbt_str_rtrim(line+2,"\n");
	
	switch (line[0]) 
	{
		case '#': 
		break;
		
		case '$':
		case '&':
			
		context->async = (line[0] == '&');
		
		/* further trim useless chars which are significant for in/output */
		xbt_str_rtrim(line+2," \t");
		
		/* Deal with CD commands here, not in rctx */
		if (!strncmp("cd ",line+2,3)) 
		{
			char *dir=line+4;
		
			if (context->command_line)
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
			
			/*if(want_dry_run)
			{
				unit->number_of_successeded_commands++;
			}*/
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
			unit_pushline(unit, context, mutex, filepos, line[0], line+2 /* pass '$ ' stuff*/);
			break;
		}
		
		case '<':
		case '>':
		case '!':
		unit_pushline(unit, context, mutex, filepos, line[0], line+2 /* pass '$ ' stuff*/);    
		break;
		
		case 'p':
		if(!want_dry_run)
			INFO2("[%s] %s",filepos,line+2);
		break;
		
		case 'P':
		if(!want_dry_run)
			CRITICAL2("[%s] %s",filepos,line+2);
		break;
		
		default:
		ERROR2("[%s] Syntax error: %s",filepos, line);
		ERROR1("Test suite `%s': NOK (syntax error)",unit->fstream->name);
		exit_code = ESYNTAX;
		unit_handle_failure(unit);
		break;
	}
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
		else if(!strncmp(line,"include", strlen("include")))
		{
			unit_handle_include(unit, context, mutex, line + strlen("include "));
		}
		
		else if(!strncmp(line,"suite", strlen("suite")))
		{
			unit_handle_suite(unit, context, mutex, line + strlen("suite "));
		}
		else 
		{
			ERROR2("%s: Malformed metacommand: %s",filepos,line);
			ERROR1("Test suite `%s': NOK (syntax error)",unit->fstream->name);
			exit_code = ESYNTAX;
			unit_handle_failure(unit);
			return;
		}
		
		break;
	}
}


void
unit_handle_failure(unit_t unit)
{
	if(!want_keep_going_unit)
	{
		if(!unit->interrupted)
		{
			/* the unit interrupted (exit for the loop) */
			unit->interrupted = 1;

			/* release the unit */
			xbt_os_sem_release(unit->sem);
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
unit_verbose(unit_t unit)
{
	int i, test_count;
	command_t command;

	printf("\nUnit : %s (%s)\n", unit->fstream->name, unit->fstream->directory);
	printf("Status informations :");

	if(unit->parsed && unit->number_of_successeded_commands == unit->number_of_started_commands)
		printf(" - (success)");	
	else
	{
		if(unit->interrupted)
			printf(" - (interruped)");
		else
			printf(" - (failed)");
	}
	
	printf("\n");

	printf("    number of commands               : %d\n", unit->number_of_commands);
	printf("    number of runned commands        : %d\n", unit->number_of_started_commands);
	printf("    number of successeded commands   : %d\n", unit->number_of_successeded_commands);
	printf("    number of failed commands        : %d\n", unit->number_of_failed_commands);
	printf("    number of interrupted commands   : %d\n", unit->number_of_interrupted_commands);
	
	test_count = unit->number_of_commands;
	
	
	for(i = 0; i < test_count; i++)
	{
		command = vector_get_at(unit->commands, i);
		
		/*command_display_status(unit->commands[i]);*/
		command_display_status(command);
	}


}

void
unit_handle_include(unit_t unit, context_t context, xbt_os_mutex_t mutex, const char* file_name)
{
	directory_t include;
	char* prev_directory = NULL;
	fstream_t fstream = NULL;
	struct stat buffer = {0};
	
	
	if(!stat(file_name, &buffer) && S_ISREG(buffer.st_mode))
	{
		INFO1("the file stream %s is in the current directory", file_name);
		
		fstream = fstream_new(getcwd(NULL, 0), file_name);
		fstream_open(fstream);
	}
	/* the file to include is not in the current directory, check if it is in a include directory */
	else
	{
		prev_directory = getcwd(NULL, 0);
		
		vector_rewind(includes);
		
		while((include = vector_get(includes)))
		{
			chdir(include->name);
			
			if(!stat(file_name, &buffer) && S_ISREG(buffer.st_mode))
			{
				fstream = fstream_new(include->name, file_name);
				fstream_open(fstream);
				break;
			}
			
			
			vector_move_next(includes);
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
		/* parse the include file */
		
		/*unit_t __unit = unit_new(unit->runner, NULL, fstream);*/
		unit->parsing_include_file = 1;
		
		/* add the unit to the list of the runned units */
		
		/*xbt_os_mutex_acquire(unit->mutex);
		vector_push_back(__unit->runner->units->items, __unit);
		xbt_os_mutex_release(unit->mutex);*/
		
		
		if(want_dry_run)
			INFO2("checking unit %s including in %s...",fstream->name, unit->fstream->name); 
		
		unit_parse(unit, context_new(), mutex, fstream->name, fstream->stream);
		
		fstream_free((void**)&fstream);
		unit->parsing_include_file = 0;
	}
}

void
unit_handle_suite(unit_t unit, context_t context, xbt_os_mutex_t mutex, const char* description)
{
	suite_t suite = suite_new(unit, description);
	unit_add_suite(unit, suite);
	unit->running_suite = 1;
}

int
unit_reset(unit_t unit)
{
	fseek(unit->fstream->stream,0L, SEEK_SET);
	unit->parsed = 0;
	unit->number_of_commands = 0;
	
	return 0;
}
