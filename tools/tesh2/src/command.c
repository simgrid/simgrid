
#include <command.h>
#include <context.h>
#include <writer.h>
#include <reader.h>
#include <timer.h>

#ifndef WIN32
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include "../include/_signal.h"


XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

static void*
command_start(void* p);


command_t
command_new(unit_t unit, context_t context, xbt_os_mutex_t mutex)
{
	command_t command = xbt_new0(s_command_t, 1);
	
	/* get the context of the execution of the command */
	command->context = context_dup(context);
	
	/* the exit code of the command is indefinite */
	command->exit_code = INDEFINITE;
	
	/* the signal of the command is indefinite */
	command->signal = INDEFINITE_SIGNAL;
	
	command->failed = 0;
	command->interrupted = 0;
	
	/* the mutex used to safetly access to the command unit properties */
	command->mutex = mutex;
	
	if(context->output->used)
		/* instantiate the buffer filled with the content of the command stdout */
		command->output = xbt_strbuff_new();
	else
		command->output = NULL;

	command->pid = INDEFINITE_PID;
	
	command->stat_val = -1;
	
	/* set the unit of the command */
	command->unit = unit; 
	
	/* all the commands are runned in a thread */
	command->thread = NULL;
	
	command->successeded = 0;
	
	if(context->output->used)
		command->reader = reader_new(command);
	else
		command->reader = NULL;

	if(context->input->used)
		command->writer = writer_new(command);
	else
		command->writer = NULL;

	if(context->timeout != INDEFINITE)
		command->timer = timer_new(command);
	else
		command->timer = NULL;

	command->status = cs_initialized;
	command->reason = csr_unknown;
	
	command->stdin_fd = INDEFINITE_FD;
	command->stdout_fd = INDEFINITE_FD;
	
	
	/* register the command */
	xbt_os_mutex_acquire(mutex);
	/*unit->commands[(unit->number_of_commands)++] = command;*/
	vector_push_back(unit->commands, command);
	(unit->number_of_commands)++;
	xbt_os_mutex_release(mutex);
	
	#ifndef WIN32
	command->killed = 0;
	#endif

	return command;
}

void
command_run(command_t command)
{
	if(!want_silent)
		INFO1("tesh %s",command->context->command_line);
	
	if(!want_just_display)
	{	
		if(!interrupted)
		{
			/* start the command */
			
			if(command->context->async)
			{
				command->thread = xbt_os_thread_create("", command_start, command);
			
				if(!command->thread)
					ERROR0("xbt_os_thread_create() failed\n");
			}
			else
				command_start(command);
		}
		else
		{
			command_interrupt(command);		
		}
	}

}

static void*
command_start(void* p)
{
	command_t command = (command_t)p;
	unit_t unit = command->unit;
	
	/* the command is started */
	command->status = cs_started;
	
	/* increment the number of started commands of the unit */
	xbt_os_mutex_acquire(command->mutex);
	(command->unit->number_of_started_commands)++;
	xbt_os_mutex_release(command->mutex);
	
	/* execute the command of the test */
	command_exec(command, command->context->command_line);
	
	if(cs_in_progress == command->status)
	{
		/*printf("the command %p is in progress\n",command);*/
		
		/* on attend la fin de la commande.
		 * la command peut soit se terminée normalement,
		 * soit se terminée à la suite d'un timeout, d'une erreur de lecture des son reader ou d'une erreur d'écriture de son writer
		 * soit à la suit d'une demande d'interruption
		 */
		
		command_wait(command);
	
		if(cs_failed != command->status && cs_interrupted != command->status)
		{
			/*printf("checking the command %p\n",command);*/
			command_check(command);
		}
	}
	
	
	xbt_os_mutex_acquire(command->mutex);
	
	/* if it's the last command release its unit */
	if(!unit->interrupted && unit->parsed && (unit->number_of_started_commands == (unit->number_of_failed_commands + unit->number_of_interrupted_commands + unit->number_of_successeded_commands)))
	{
		/* first release the mutex */
		unit->released = 1;
		xbt_os_mutex_release(command->mutex);
		/* the last command release the unit */
		xbt_os_sem_release(command->unit->sem);
	}
	else
		xbt_os_mutex_release(command->mutex);
		
	
	/* wait the end of the timer, the reader and the writer */
	if(command->timer && command->timer->thread)
		timer_wait(command->timer);

	if(command->writer && command->writer->thread)
		writer_wait(command->writer);

	if(command->reader && command->reader->thread)
		reader_wait(command->reader);
	
	return NULL;
}

#ifdef WIN32
void
command_exec(command_t command, const char* command_line)
{
	
	STARTUPINFO si = {0};					/* contains the informations about the child process windows*/
	PROCESS_INFORMATION pi = {0};			/* contains child process informations						*/
	SECURITY_ATTRIBUTES sa = {0};			/* contains the security descriptor for the pipe handles	*/
	HANDLE child_stdin_handle[2] = {NULL};	/* child_stdin_handle[1]	<-> stdout of the child process	*/
	HANDLE child_stdout_handle[2] = {NULL};	/* child_stdout_handle[0]	<-> stdin of the child process	*/
	HANDLE child_stderr = NULL;
	
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;			/* use default security for the pipe handles				*/
	
	sa.bInheritHandle = TRUE;				/* the pipe handles can be inherited						*/
	
	if(!CreatePipe(&(child_stdin_handle[0]),&(child_stdin_handle[1]),&sa,0))
    {
		ERROR1("CreatePipe1() failed (%lu)",GetLastError());
		command->failed = 1;
		command->status = cs_failed;	

		return;
    }
	
	
	if(!DuplicateHandle(GetCurrentProcess(),(child_stdin_handle[1]),GetCurrentProcess(),&(child_stderr),0,TRUE,DUPLICATE_SAME_ACCESS))
    {
		ERROR1("DuplicateHandle1() failed (%lu)",GetLastError());
		
		CloseHandle(child_stdin_handle[0]);
		CloseHandle(child_stdin_handle[1]);

		command->failed = 1;
		command->status = cs_failed;	

		return;
    }
	
	if(!CreatePipe(&(child_stdout_handle[0]),&(child_stdout_handle[1]),&sa,0))
    {
		ERROR1("CreatePipe2() failed (%lu)",GetLastError()); 
		
		CloseHandle(child_stdout_handle[0]);
		CloseHandle(child_stdout_handle[1]);
		CloseHandle(child_stdin_handle[0]);
		CloseHandle(child_stdin_handle[1]);

		command->failed = 1;
		command->status = cs_failed;	

		return;
    }
		
	/* Read handle for read operations on the child std output. */
	if(!DuplicateHandle(GetCurrentProcess(),(child_stdin_handle[0]),GetCurrentProcess(),&(command->stdout_fd),0,FALSE, DUPLICATE_SAME_ACCESS))
    {
		CloseHandle(child_stdout_handle[0]);
		CloseHandle(child_stdout_handle[1]);
		CloseHandle(child_stdin_handle[0]);
		CloseHandle(child_stdin_handle[1]);

		command->failed = 1;
		command->status = cs_failed;	

		ERROR1("DuplicateHandle2() failed (%lu)",GetLastError()); 
    }
	
	
	/* Write handle for write operations on the child std input. */
	if(!DuplicateHandle(GetCurrentProcess(),(child_stdout_handle[1]),GetCurrentProcess(),&(command->stdin_fd), 0,FALSE,DUPLICATE_SAME_ACCESS))
    {
		CloseHandle(child_stdout_handle[0]);
		CloseHandle(child_stdout_handle[1]);
		CloseHandle(child_stdin_handle[0]);
		CloseHandle(child_stdin_handle[1]);

		command->failed = 1;
		command->status = cs_failed;	

		ERROR1("DuplicateHandle3() failed (%lu)",GetLastError());
    }

	
	CloseHandle(child_stdin_handle[0]);
	CloseHandle(child_stdout_handle[1]);
	
	if(command->timer)
	{
		/* launch the timer */
		timer_time(command->timer);
	}
	
	if(command->reader)
	{
		/* launch the reader */
		reader_read(command->reader);
	}
    

	if(command->writer)
	{
		/* launch the writer */
		writer_write(command->writer);
	}

    si.cb = sizeof(STARTUPINFO);
	
	si.dwFlags |= STARTF_USESTDHANDLES;
	si.hStdOutput = child_stdin_handle[1];
	si.hStdInput  = child_stdout_handle[0];
	si.hStdError  = child_stderr;

	/* launch the process */
	if(!CreateProcess(
						NULL,
						(char*)command_line,
						NULL,
						NULL,
						TRUE,
						CREATE_NO_WINDOW,
						NULL,
						NULL,
						&si,
						&pi)
	)
	{
		
		if(ERROR_FILE_NOT_FOUND == GetLastError())
		{
			exit_code = ECMDNOTFOUND;
			command_handle_failure(command,csr_command_not_found);
		}
		else
		{
			exit_code = EEXEC;
			command_handle_failure(command,csr_exec_failure);
		}
    }
	else
	{
		/* the command is running */
		command->status = cs_in_progress;

		/* save the pid of the command */
		command->pid = pi.hProcess;

		/* close non used thread handle */
		CloseHandle(pi.hThread);
		
	}

	
	/* close non used handles */
	CloseHandle(child_stdin_handle[1]);
    CloseHandle(child_stdout_handle[0]);
   	CloseHandle(child_stderr);


}
#else
void
command_exec(command_t command, const char* command_line)
{
	int child_stdin_fd[2] ;
	int child_stdout_fd[2];
	
	if(pipe(child_stdin_fd) || pipe(child_stdout_fd)) 
	{
		ERROR1("pipe() failed (%d)",errno);
		command_handle_failure(command, csr_pipe_function_failed);	

		return;
	}
	
	command->pid= fork();
	
	if(command->pid < 0) 
	{
		close(child_stdin_fd[0]);
		close(child_stdin_fd[1]);
		close(child_stdout_fd[0]);
		close(child_stdout_fd[1]);
		
		exit_code = EEXEC;
		ERROR1("fork() failed (%d)",errno);
		command_handle_failure(command,csr_exec_failure);
	}
	else
	{
		if(command->pid) 
		{/* father */
			close(child_stdin_fd[0]);
			close(child_stdout_fd[1]);
			
			command->stdin_fd = child_stdin_fd[1];
			command->stdout_fd = child_stdout_fd[0];

			if(command->reader)
			{
				/* launch the reader */
				reader_read(command->reader);
			}
		    
			if(command->writer)
			{
				/* launch the writer */
				writer_write(command->writer);
			}
			
			if(command->timer)
			{
				/* launch the timer */
				timer_time(command->timer);
			}
			
			/* the command is running */
			command->status = cs_in_progress;
		
		} 
		else 
		{/* child */
		
			close(child_stdin_fd[1]);
			
			dup2(child_stdin_fd[0],0);
			
			close(child_stdin_fd[0]);
			
			close(child_stdout_fd[0]);
			
			dup2(child_stdout_fd[1],1);
			
			dup2(child_stdout_fd[1],2);
			
			close(child_stdout_fd[1]);
			
			if(command->reader)
				xbt_os_sem_acquire(command->reader->started);
			
			if(command->writer)
				xbt_os_sem_acquire(command->writer->started);
				
			if(command->timer)
				xbt_os_sem_acquire(command->timer->started);	
			
			execlp ("/bin/sh", "sh", "-c", command->context->command_line, NULL);
		}
	}
}
#endif

#ifdef WIN32
void
command_wait(command_t command)
{
	/* wait for the command terminaison */
	DWORD rv;

	if(WAIT_FAILED == WaitForSingleObject(command->pid, INFINITE))
	{
		ERROR0("WaitForSingleObject() failed");
		/* TODO : see for the interruption	*/	
	}
	else
	{
		/* don't take care of the timer or the writer or the reader failue */
		if(cs_failed != command->status && cs_interrupted != command->status)
		{
			if(!GetExitCodeProcess(command->pid,&rv))
			{
				ERROR1("GetExitCodeProcess() failed for the child %s",command->context->command_line);
				/* TODO : see for the interruption	*/	
			}
			else
				command->stat_val = command->exit_code = rv;
		}
	}
}
#else
void
command_wait(command_t command)
{
	
	int pid;
	
	/* let this thread wait for the child so that the main thread can detect the timeout without blocking on the wait */
	
	pid = waitpid(command->pid, &(command->stat_val), 0);
	
	/*printf("The %p command ended\n",command);*/
	if(pid != command->pid) 
	{
		ERROR1("waitpid() failed for the child %s",command->context->command_line);
		exit_code = EWAIT;
		command_handle_failure(command, csr_wait_failure);
	}
	else
	{
		if(WIFEXITED(command->stat_val))
			command->exit_code = WEXITSTATUS(command->stat_val);	
	}
}
#endif

void
command_check(command_t command)
{
	int success = 1;
	cs_reason_t reason;
	
	/* we have a signal, store it */
	if(WIFSIGNALED(command->stat_val))
	{
		command->signal = strdup(signal_name(WTERMSIG(command->stat_val),command->context->signal));
		INFO3("the command -PID %d %s receive the signal : %s",command->pid, command->context->command_line, command->signal);
	}
	
	/* we have a signal and not signal is expected */
	if(WIFSIGNALED(command->stat_val) && !command->context->signal) 
	{
		success = 0;
		exit_code = EUNEXPECTEDSIG;
		reason = csr_unexpected_signal_caught;
	}
	
	/* we have a signal that differ form the expected signal */
	if(success && WIFSIGNALED(command->stat_val) && command->context->signal && strcmp(signal_name(WTERMSIG(command->stat_val),command->context->signal),command->context->signal)) 
	{
		success = 0;
		exit_code = ESIGNOTMATCH;
		reason = csr_signals_dont_match;
	}
	
	/* we don't receipt the expected signal */
	if(success && !WIFSIGNALED(command->stat_val) && command->context->signal) 
	{
		success = 0;
		exit_code = ESIGNOTRECEIPT;
		reason = csr_expected_signal_not_receipt;
	}
	
	/* if the command exit normaly and we expect a exit code : test it */
	if(success && WIFEXITED(command->stat_val) /* && INDEFINITE != command->context->exit_code*/)
	{
		/* the exit codes don't match */
		if(WEXITSTATUS(command->stat_val) != command->context->exit_code)
		{
			success = 0;
			exit_code = EEXITCODENOTMATCH;
			reason = csr_exit_codes_dont_match;
		}
	}
	
	/* if ouput handling flag is specified check the output */
	if(success && oh_check == command->context->output_handling && command->reader)
	{
		/* make sure the reader done */
		while(!command->reader->broken_pipe)
			xbt_os_thread_yield();

			xbt_strbuff_chomp(command->output);
			xbt_strbuff_chomp(command->context->output);
			xbt_strbuff_trim(command->output);
			xbt_strbuff_trim(command->context->output);

			if(command->output->used != command->context->output->used || strcmp(command->output->data, command->context->output->data))
			{
				success = 0;
				exit_code = EOUTPUTNOTMATCH;
				reason = csr_outputs_dont_match;
			}
	}
	
	if(success)
	{
		xbt_os_mutex_acquire(command->mutex);
		
		if(command->status != cs_interrupted)
		{
		
			/* signal the success of the command */
			command->status = cs_successeded;
			command->successeded = 1;

			/* increment the number of successeded command of the unit */
			/*xbt_os_mutex_acquire(command->mutex);*/
			(command->unit->number_of_successeded_commands)++;
		}
		
		xbt_os_mutex_release(command->mutex);
			
	}
	else
	{
		command_handle_failure(command,reason);
	}
}

#ifdef WIN32
void
command_kill(command_t command)
{
	if(INDEFINITE_PID != command->pid)
		TerminateProcess(command->pid, INDEFINITE);
}
#else
void
command_kill(command_t command)
{
	if(INDEFINITE_PID != command->pid)
	{
		/*INFO1("Kill the command - PID %d",command->pid);*/
		
		kill(command->pid,SIGTERM);
		
		if(!command->context->signal)
			command->context->signal = strdup("SIGTERM");
			
		command->exit_code = INDEFINITE;
		command->killed = 1;
		
		usleep(100);
		kill(command->pid,SIGKILL); 

		
	}
}
#endif

void
command_interrupt(command_t command)
{
	xbt_os_mutex_acquire(command->mutex);
	
	if((command->status != cs_interrupted) && (command->status != cs_failed) && (command->status != cs_successeded))
	{
		/*INFO1("Begin interrupt the command - PID %d",command->pid);*/
		
		command->status = cs_interrupted;	
		command->reason = csr_interruption_request;
		command->interrupted = 1;
		xbt_os_mutex_acquire(command->unit->mutex);
		(command->unit->number_of_interrupted_commands)++;
		xbt_os_mutex_release(command->unit->mutex);
		
		if(command->pid != INDEFINITE_PID)
			command_kill(command);
		
		
		/*INFO1("End interrupt the command - PID %d",command->pid);*/
	}
	
	xbt_os_mutex_release(command->mutex);
	
	
}

void
command_display_status(command_t command)
{
	#ifdef WIN32
	printf("\nCommand : PID - %p\n%s\n",command->pid,command->context->command_line);
	#else
	printf("\nCommand : PID - %d\n%s\n",command->pid,command->context->command_line);
	#endif
	printf("Status informations :\n");
	printf("    position in the tesh file   : %s\n",command->context->line);
	
	/* the command successeded */
	if(cs_successeded == command->status)
	{
		/* status */
		printf("    status                      : success\n");
	}
	else
	{
		
		/* display if the command is interrupted, failed or in a unknown status */
		if(cs_interrupted == command->status)
			printf("    status                      : interrupted\n");
		else if(cs_failed == command->status)
			printf("    status                      : failed\n");
		else
			printf("    status                      : unknown\n");
			
		#ifndef WIN32
		if(command->killed)
			printf("    <killed command>\n");
		#endif
		
		/* display the reason of the status of the command */
		switch(command->reason)
		{
			/* the function pipe or CreatePipe() fails */
			case csr_pipe_function_failed :
			printf("    reason                      : pipe() or CreatePipe() function failed (system error)\n");
			break;
			
			/* reader failure reasons*/
			case csr_read_pipe_broken :
			printf("    reason                      : command read pipe broken\n");
			break;

			case csr_read_failure :
			printf("    reason                      : command stdout read failed\n");
			break;
	
			/* writer failure reasons */
			case csr_write_failure :
			printf("    reason                      : command stdin write failed\n");
			break;

			case csr_write_pipe_broken :
			printf("    reason                      : command write pipe broken\n");
			break;
			
			/* timer reason */
			case csr_timeout :
			printf("    reason                      : command timeouted\n");
			break;
			
			/* command failure reason */
			case csr_command_not_found :
			printf("    reason                      : command not found\n");
			break;
			
			/* context failure reasons */
			case csr_exit_codes_dont_match :
			printf("    reason                      : exit codes don't match\n");
			
			break;

			case csr_outputs_dont_match :
			{
				char *diff;
				printf("    reason                      : ouputs don't match\n");
				diff = xbt_str_diff(command->context->output->data,command->output->data);	  	
				printf("    output diff :\n%s\n",diff);
				free(diff);
			}     

			break;

			case csr_signals_dont_match :
			printf("    reason                      : signals don't match\n"); 
			break;
			
			case csr_unexpected_signal_caught:
			printf("    reason                      : unexpected signal caught\n");
			break;
			
			case csr_expected_signal_not_receipt :
			printf("    reason                      : expected signal not receipt\n");
			break;

			/* system failure reasons */
			case csr_exec_failure :
			printf("    reason                      : can't excute the command\n");
			break;
			
			case csr_wait_failure :
			printf("    reason                      : wait command failure\n");
			break;
			
			/* global/local interruption */
			case csr_interruption_request :
			printf("    reason                      : the command receive a interruption request\n");
			break;
			
			/* unknown ? */
			case csr_unknown :
			printf("    reason                      : unknown \n");
		}
	}

	if(csr_command_not_found != command->reason && csr_exec_failure != command->reason)
	{
		if(INDEFINITE != command->exit_code)
			/* the command exit code */
			printf("    exit code                   : %d\n",command->exit_code);
		
		/* if an expected exit code was specified display it */
		if(INDEFINITE != command->context->exit_code)
			printf("    expected exit code          : %d\n",command->context->exit_code);
		else
			printf("    no expected exit code specified\n");
		
		/* if an expected exit code was specified display it */
		if(NULL == command->context->signal)
			printf("    no expected signal specified\n");
		else
		{
			if(NULL != command->signal)
				printf("    signal                      : %s\n",command->signal);
			
			printf("    expected signal             : %s\n",command->context->signal);
		}
		
		/* if the command has out put and the metacommand display output is specified display it  */
		if(command->output && (0 != command->output->used) && (oh_display == command->context->output_handling))
		{
			xbt_dynar_t a = xbt_str_split(command->output->data, "\n");
			char *out = xbt_str_join(a,"\n||");
			xbt_dynar_free(&a);
			printf("    output :\n||%s",out);
			free(out);
		}
	}

	printf("\n");
		

}

void
command_handle_failure(command_t command, cs_reason_t reason)
{
	
	unit_t unit = command->unit;
	
	xbt_os_mutex_acquire(command->mutex);

	if((command->status != cs_interrupted) && (command->status != cs_failed))
	{
		command->status = cs_failed;
		command->reason = reason;
		command->failed = 1;
		
		xbt_os_mutex_acquire(unit->mutex);
		
		/* increment the number of failed command of the unit */
		unit->number_of_failed_commands++;
		
		/* if the --ignore-failures option is not specified */
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

		xbt_os_mutex_release(unit->mutex);
	}

	xbt_os_mutex_release(command->mutex);
}

void
command_free(command_t* command)
{
	/* close the stdin and the stdout pipe handles */

	#ifdef WIN32
	if((*command)->stdin_fd != INDEFINITE_FD)
		CloseHandle((*command)->stdin_fd);
	if((*command)->stdout_fd != INDEFINITE_FD)
		CloseHandle((*command)->stdout_fd);
	#else
	if((*command)->stdin_fd != INDEFINITE_FD)
		close((*command)->stdin_fd);
	
	if((*command)->stdout_fd != INDEFINITE_FD)	
		close((*command)->stdout_fd);
	#endif

	timer_free(&((*command)->timer));
	writer_free(&((*command)->writer));
	reader_free(&((*command)->reader));
	xbt_strbuff_free((*command)->output);
	context_free(&((*command)->context));

	if((*command)->signal)
		free((*command)->signal);

	free(*command);
	*command = NULL;
}



