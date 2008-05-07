/*
 * src/command.c - type representing a command.
 *
 * Copyright 2008,2009 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * Purpose:
 * 		This file contains all the definitions of the functions related with
 * 		the tesh command type.
 *
 */
 
#include <command.h>
#include <context.h>
#include <writer.h>
#include <reader.h>
#include <timer.h>

#ifndef WIN32
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <xsignal.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

static void*
command_start(void* p);

command_t
command_new(unit_t unit, context_t context, xbt_os_mutex_t mutex)
{
	command_t command;
	/* TODO : check the parameters */
	
	command = xbt_new0(s_command_t, 1);
	
	/* get the context of the execution of the command */
	if(!(command->context = context_dup(context)))
	{
		free(command);
		return NULL;
	}
	
	/* the exit code of the command is indefinite */
	command->exit_code = INDEFINITE;
	
	/* the signal of the command is indefinite */
	command->signal = INDEFINITE_SIGNAL;
	
	command->failed = 0;
	command->interrupted = 0;
	
	/* the mutex used to safetly access to the command unit properties */
	command->mutex = mutex;
	
	if(context->output->used)
		command->output = xbt_strbuff_new();
	else
		command->output = NULL;

	command->pid = INDEFINITE_PID;
	
	command->stat_val = -1;
	
	/* set the unit of the command */
	command->unit = unit->root ? unit->root : unit; 
	
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
	
	/*vector_push_back(unit->commands, command);*/
	xbt_dynar_push(unit->commands, &command);
	command->unit->cmd_nb++;
	xbt_os_mutex_release(mutex);
	
	#ifndef WIN32
	command->killed = 0;
	command->execlp_errno = 0;
	#endif
	

	return command;
}

int
command_run(command_t command)
{
	/* TODO : check the parameter */
	
	if(!silent_flag)
		INFO1("tesh %s",command->context->command_line);
	
	if(!just_print_flag)
	{	
		if(!interrupted)
		{
			/* start the command */
			
			if(command->context->async)
			{
				command->thread = xbt_os_thread_create("", command_start, command);
			
			}
			else
				command_start(command);
		}
		else
		{
			command_interrupt(command);		
		}
	}
	
	return 0;

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
	(command->unit->started_cmd_nb)++;
	xbt_os_mutex_release(command->mutex);
	
	/* execute the command of the test */
	command_exec(command, command->context->command_line);
	
	if(cs_in_progress == command->status)
	{
		
		/* on attend la fin de la commande.
		 * la command peut soit se terminée normalement,
		 * soit se terminée à la suite d'un timeout, d'une erreur de lecture des son reader ou d'une erreur d'écriture de son writer
		 * soit à la suit d'une demande d'interruption
		 */
		
		command_wait(command);
	
		if(cs_failed != command->status && cs_interrupted != command->status)
			command_check(command);
	}
	
	xbt_os_mutex_acquire(command->mutex);
	
	/* if it's the last command release its unit */
	if(!unit->interrupted && unit->parsed && (unit->started_cmd_nb == (unit->failed_cmd_nb + unit->interrupted_cmd_nb + unit->successeded_cmd_nb)))
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
		error_register("CreatePipe() function failed", (int)GetLastError(), command->context->command_line, command->unit->fstream->name);
		command->failed = 1;
		command->status = cs_failed;	

		return;
    }
	
	
	if(!DuplicateHandle(GetCurrentProcess(),(child_stdin_handle[1]),GetCurrentProcess(),&(child_stderr),0,TRUE,DUPLICATE_SAME_ACCESS))
    {
		error_register("DuplicateHandle() function failed", (int)GetLastError(), command->context->command_line, command->unit->fstream->name);
		
		CloseHandle(child_stdin_handle[0]);
		CloseHandle(child_stdin_handle[1]);

		command->failed = 1;
		command->status = cs_failed;	

		return;
    }
	
	if(!CreatePipe(&(child_stdout_handle[0]),&(child_stdout_handle[1]),&sa,0))
    {
		error_register("CreatePipe() function failed", (int)GetLastError(), command->context->command_line, command->unit->fstream->name);
		
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

		error_register("DuplicateHandle() function failed", (int)GetLastError(), command->context->command_line, command->unit->fstream->name);
		
		return;
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

		error_register("DuplicateHandle() function failed", (int)GetLastError(), command->context->command_line, command->unit->fstream->name);
		return;
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
		error_register("CreateProcess() function failed", (int)GetLastError(), command->context->command_line, command->unit->fstream->name);
		command_handle_failure(command,csr_create_process_function_failure);
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
	
	if(command->writer)
	{
		if(pipe(child_stdin_fd)) 
		{
			error_register("pipe() function failed", errno, command->context->command_line, command->unit->fstream->name);
			command_handle_failure(command, csr_pipe_function_failed);	
	
			return;
		}
	}	
	
	if(command->reader)
	{
		if(pipe(child_stdout_fd)) 
		{
			error_register("write() function failed", errno, command->context->command_line, command->unit->fstream->name);
				
			if(command->writer)
			{
				close(child_stdin_fd[0]);
				close(child_stdin_fd[1]);
			}
			
			command_handle_failure(command, csr_pipe_function_failed);
			
			return;
		}
	}
	
	if(command->writer)
	{
		if(fcntl(child_stdin_fd[1], F_SETFL, fcntl(child_stdin_fd[1], F_GETFL) | O_NONBLOCK) < 0)
		{
			error_register("fcntl() function failed", errno, command->context->command_line, command->unit->fstream->name);
			
			close(child_stdin_fd[0]);
			close(child_stdin_fd[1]);
			
			if(command->reader)
			{
				close(child_stdout_fd[0]);
				close(child_stdout_fd[1]);
			}
			
			command_handle_failure(command, csr_fcntl_function_failed);	
				
			return;
		}
	}
	
	/* to write to the child stdin */
	command->stdin_fd = child_stdin_fd[1];
	
	/* to read from the child stdout */
	command->stdout_fd = child_stdout_fd[0];
	
	/* launch the reader if any*/
	if(command->reader)
		reader_read(command->reader);
    
    /* launch the writer if any */
	if(command->writer)
		writer_write(command->writer);
	
	/* launch the timer if any */
	if(command->timer)
		timer_time(command->timer);
		
	/* if there is a reader wait for its starting */
	if(command->reader)
		xbt_os_sem_acquire(command->reader->started);
	
	/* if there is a reader wait for its ending */
	if(command->writer)
		xbt_os_sem_acquire(command->writer->written);
	
	/* if there is a reader wait for its starting */
	if(command->timer)
		xbt_os_sem_acquire(command->timer->started);
	
	/* update the state of the command, assume it is in progress */
	command->status = cs_in_progress;
				
	command->pid= fork();
				
	if(command->pid < 0) 
	{/* error */
		if(command->writer)
		{
			close(child_stdin_fd[0]);
			close(child_stdin_fd[1]);
		}
		
		if(command->reader)
		{
			close(child_stdout_fd[0]);
			close(child_stdout_fd[1]);
		}
		
		error_register("fcntl() function failed", errno, command->context->command_line, command->unit->fstream->name);
		command_handle_failure(command,csr_fork_function_failure);
	}
	else
	{
		if(command->pid) 
		{/* father */
			
			/* close unused file descriptors */
			if(command->writer)
				close(child_stdin_fd[0]);
				
			if(command->reader)
				close(child_stdout_fd[1]);
		} 
		else 
		{/* child */
			
 			/* close unused file descriptors */
			if(command->writer)
				close(child_stdin_fd[1]);
				
			if(command->reader)
				close(child_stdout_fd[0]);
			
			if(command->writer)
			{
				/* redirect stdin to child_stdin_fd[0] (now fgets(), getchar() ... read from the pipe */  
				if(dup2(child_stdin_fd[0],STDIN_FILENO) < 0)
				{
					error_register("dup2() function failed", errno, command->context->command_line, command->unit->fstream->name);
					command_handle_failure(command,csr_dup2_function_failure);
				}
			
				/* close the unused file descriptor  */
				close(child_stdin_fd[0]);
			}
			
			if(command->reader)
			{
			
				/* redirect stdout and stderr to child_stdout_fd[1] (now printf(), perror()... write to the pipe */  
				if(dup2(child_stdout_fd[1],STDOUT_FILENO) < 0)
				{
					error_register("dup2() function failed", errno, command->context->command_line, command->unit->fstream->name);
					command_handle_failure(command, csr_dup2_function_failure);
				}
				
				if(dup2(child_stdout_fd[1], STDERR_FILENO) < 0)
				{
					error_register("dup2() function failed", errno, command->context->command_line, command->unit->fstream->name);
					command_handle_failure(command, csr_dup2_function_failure);
				}
			
				/* close the unused file descriptor  */
				close(child_stdout_fd[1]);
			}
			
			/* launch the command */
			if(execlp("/bin/sh", "sh", "-c", command->context->command_line, NULL) < 0)
				command->execlp_errno = errno;
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
	if(!command->execlp_errno)
	{
		
		/* let this thread wait for the child so that the main thread can detect the timeout without blocking on the wait */
		int pid = waitpid(command->pid, &(command->stat_val), 0);
	
		if(pid != command->pid) 
		{
			error_register("waitpid() function failed", errno, command->context->command_line, command->unit->fstream->name);
			command_handle_failure(command, csr_waitpid_function_failure);
		}
		else
		{
			if(WIFEXITED(command->stat_val))
				command->exit_code = WEXITSTATUS(command->stat_val);	
		}
	}
	else
	{
		error_register("execlp() function failed", command->execlp_errno, command->context->command_line, command->unit->fstream->name);
		command_handle_failure(command, csr_execlp_function_failure);
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
	}
	
	/* we have a signal and not signal is expected */
	if(WIFSIGNALED(command->stat_val) && !command->context->signal) 
	{
		success = 0;
		error_register("Command failure", EUNXPSIG, command->context->command_line, command->unit->fstream->name);
		reason = csr_unexpected_signal_caught;
	}
	
	/* we have a signal that differ form the expected signal */
	if(success && WIFSIGNALED(command->stat_val) && command->context->signal && strcmp(signal_name(WTERMSIG(command->stat_val),command->context->signal),command->context->signal)) 
	{
		success = 0;
		error_register("Command failure", ESIGNOTMATCH, command->context->command_line, command->unit->fstream->name);
		reason = csr_signals_dont_match;
	}
	
	/* we don't receipt the expected signal */
	if(success && !WIFSIGNALED(command->stat_val) && command->context->signal) 
	{
		success = 0;
		error_register("Command failure", ESIGNOTRECEIPT, command->context->command_line, command->unit->fstream->name);
		reason = csr_expected_signal_not_receipt;
	}
	
	/* if the command exit normaly and we expect a exit code : test it */
	if(success && WIFEXITED(command->stat_val) /* && INDEFINITE != command->context->exit_code*/)
	{
		/* the exit codes don't match */
		if(WEXITSTATUS(command->stat_val) != command->context->exit_code)
		{
			success = 0;
			error_register("Command failure", EEXITCODENOTMATCH, command->context->command_line, command->unit->fstream->name);
			reason = csr_exit_codes_dont_match;
		}
	}
	
	/* if ouput handling flag is specified check the output */
	if(success && oh_check == command->context->output_handling && command->reader)
	{
		/* make sure the reader done */
		while(!command->reader->done)
			xbt_os_thread_yield();
		
		#ifdef WIN32
		CloseHandle(command->stdout_fd);
		#else
		close(command->stdout_fd);
		#endif

		command->stdout_fd = INDEFINITE_FD;

		xbt_strbuff_chomp(command->output);
		xbt_strbuff_chomp(command->context->output);
		xbt_strbuff_trim(command->output);
		xbt_strbuff_trim(command->context->output);

		if(command->output->used != command->context->output->used || strcmp(command->output->data, command->context->output->data))
		{
			success = 0;
			error_register("Command failure", EOUTPUTNOTMATCH, command->context->command_line, command->unit->fstream->name);
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
			(command->unit->successeded_cmd_nb)++;
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
		command->status = cs_interrupted;	
		command->reason = csr_interruption_request;
		command->interrupted = 1;
		xbt_os_mutex_acquire(command->unit->mutex);
		(command->unit->interrupted_cmd_nb)++;
		xbt_os_mutex_release(command->unit->mutex);
		
		if(command->pid != INDEFINITE_PID)
			command_kill(command);
	}
	
	xbt_os_mutex_release(command->mutex);
	
	
}

void
command_summarize(command_t command)
{
	if(cs_successeded != command->status)
	{
			
		#ifndef WIN32
		if(command->killed)
			printf("          <killed command>\n");
		#endif
		
		/* display the reason of the status of the command */
		switch(command->reason)
		{
			/* the function pipe or CreatePipe() fails */
			case csr_pipe_function_failed :
			printf("          reason                      : pipe() or CreatePipe() function failed (system error)\n");
			break;
			
			/* reader failure reasons*/
			case csr_read_pipe_broken :
			printf("          reason                      : command read pipe broken\n");
			break;

			case csr_read_failure :
			printf("          reason                      : command stdout read failed\n");
			break;
	
			/* writer failure reasons */
			case csr_write_failure :
			printf("          reason                      : command stdin write failed\n");
			break;

			case csr_write_pipe_broken :
			printf("          reason                      : command write pipe broken\n");
			break;
			
			/* timer reason */
			case csr_timeout :
			printf("          reason                      : command timeouted\n");
			break;
			
			/* command failure reason */
			case csr_command_not_found :
			printf("          reason                      : command not found\n");
			break;
			
			/* context failure reasons */
			case csr_exit_codes_dont_match :
			printf("          reason                      : exit codes don't match\n");
			
			break;
			
			/* dup2 function failure reasons */
			case csr_dup2_function_failure :
			printf("          reason                      : dup2() function failed\n");
			
			break;
			
			/* execlp function failure reasons */
			case csr_execlp_function_failure :
			printf("          reason                      : execlp() function failed\n");
			
			break;
			
			/* waitpid function failure reasons */
			case csr_waitpid_function_failure :
			printf("          reason                      : waitpid() function failed\n");
			
			break;
			
			/* CreateProcess function failure reasons */
			case csr_create_process_function_failure :
			printf("          reason                      : CreateProcesss() function failed\n");
			
			break;
			
			case csr_outputs_dont_match :
			{
				char *diff;
				printf("          reason                      : ouputs don't match\n");
				diff = xbt_str_diff(command->context->output->data,command->output->data);	  	
				printf("          output diff :\n%s\n",diff);
				free(diff);
			}     

			break;

			case csr_signals_dont_match :
			printf("          reason                      : signals don't match\n"); 
			break;
			
			case csr_unexpected_signal_caught:
			printf("                reason                      : unexpected signal caught\n");
			break;
			
			case csr_expected_signal_not_receipt :
			printf("          reason                      : expected signal not receipt\n");
			break;

			/* system failure reasons */
			case csr_fork_function_failure :
			printf("          reason                      : fork function failed\n");
			break;
			
			case csr_wait_failure :
			printf("          reason                      : wait command failure\n");
			break;
			
			/* global/local interruption */
			case csr_interruption_request :
			printf("          reason                      : the command receive a interruption request\n");
			break;
			
			/* unknown ? */
			case csr_unknown :
			printf("          reason                      : unknown \n");
		}
	}

	if(csr_command_not_found != command->reason && csr_fork_function_failure != command->reason  && csr_execlp_function_failure != command->reason)
	{
		if(INDEFINITE != command->exit_code)
			/* the command exit code */
			printf("          exit code                   : %d\n",command->exit_code);
		
		/* if an expected exit code was specified display it */
		if(INDEFINITE != command->context->exit_code)
			printf("          expected exit code          : %d\n",command->context->exit_code);
		else
			printf("          no expected exit code specified\n");
		
		/* if an expected exit code was specified display it */
		if(NULL == command->context->signal)
			printf("          no expected signal specified\n");
		else
		{
			if(NULL != command->signal)
				printf("          signal                      : %s\n",command->signal);
			
			printf("          expected signal             : %s\n",command->context->signal);
		}
		
		/* if the command has out put and the metacommand display output is specified display it  */
		if(command->output && (0 != command->output->used) && (oh_display == command->context->output_handling))
		{
			xbt_dynar_t a = xbt_str_split(command->output->data, "\n");
			char *out = xbt_str_join(a,"\n||");
			xbt_dynar_free(&a);
			printf("          output :\n||%s",out);
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
		unit->failed_cmd_nb++;
		
		/* if the --ignore-failures option is not specified */
		if(!keep_going_unit_flag)
		{
			if(!unit->interrupted)
			{
				/* the unit interrupted (exit for the loop) */
				unit->interrupted = 1;

				/* release the unit */
				xbt_os_sem_release(unit->sem);
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

		xbt_os_mutex_release(unit->mutex);
	}

	xbt_os_mutex_release(command->mutex);
}

int
command_free(command_t* ptr)
{
	/* TODO : check the parameter */
	
	/* close the stdin and the stdout pipe handles */

	#ifdef WIN32
	if((*ptr)->stdin_fd != INDEFINITE_FD)
		CloseHandle((*ptr)->stdin_fd);
		
	if((*ptr)->stdout_fd != INDEFINITE_FD)
		CloseHandle((*ptr)->stdout_fd);
	
	#else
	
	if((*ptr)->stdin_fd != INDEFINITE_FD)
		close((*ptr)->stdin_fd);
	
	if((*ptr)->stdout_fd != INDEFINITE_FD)	
		close((*ptr)->stdout_fd);
	#endif
	
	if((*ptr)->timer)
	{
		if(timer_free(&((*ptr)->timer)) < 0)
			return -1;
			
	}
	
	if((*ptr)->writer)
	{
		if(writer_free(&((*ptr)->writer)) < 0)
			return -1;
	}
	
	if((*ptr)->reader)
	{
		if(reader_free(&((*ptr)->reader)) < 0)
			return -1;
	}
	
	xbt_strbuff_free((*ptr)->output);
	
	if((*ptr)->context)
	{
		if(context_free(&((*ptr)->context)) < 0)
			return -1;
	}

	if((*ptr)->signal)
		free((*ptr)->signal);

	free(*ptr);
	
	*ptr = NULL;
	
	return 0;
}



