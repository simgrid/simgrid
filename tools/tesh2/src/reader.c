/*
 * src/reader.c - type representing a stdout reader.
 *
 * Copyright 2008,2009 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * Purpose:
 * 		This file contains all the definitions of the functions related with
 * 		the tesh reader type.
 *
 */

#include <unit.h>
#include <reader.h>
#include <command.h>


XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);


static void*
reader_start_routine(void* p);

reader_t
reader_new(command_t command)
{
	reader_t reader;
	
	/* TODO : check the parameter */
	
	reader = xbt_new0(s_reader_t, 1);
	
	reader->thread = NULL;
	reader->command = command;
	reader->broken_pipe = 0;
	reader->failed = 0;
	reader->done = 0;
	
	reader->started = xbt_os_sem_init(0);
	
	return reader;
}

int
reader_free(reader_t* ptr)
{
	if((*ptr)->started)
		xbt_os_sem_destroy((*ptr)->started);
	
	free(*ptr);
	
	*ptr = NULL;
	
	return 0;
}

void
reader_read(reader_t reader)
{
	reader->thread = xbt_os_thread_create("", reader_start_routine, reader);
}

#ifdef _XBT_WIN32
static void*
reader_start_routine(void* p)
{
	reader_t reader = (reader_t)p;
	command_t command = reader->command;
	
	xbt_strbuff_t output = command->output;
	HANDLE stdout_fd = command->stdout_fd;
	
	DWORD number_of_bytes_to_read = 1024; /*command->context->output->used;*/
	DWORD number_of_bytes_readed = 0;
	
	char* buffer = (char*)calloc(number_of_bytes_to_read + 1,sizeof(char));
	char* clean = (char*)calloc(number_of_bytes_to_read + 1,sizeof(char));
	size_t i, j;

	xbt_os_sem_release(reader->started);

	while(!command->failed && !command->interrupted && !command->successeded && !reader->failed && !reader->broken_pipe)
	{
		if(!ReadFile(stdout_fd, buffer, number_of_bytes_to_read, &number_of_bytes_readed, NULL) || (0 == number_of_bytes_readed))
		{
			if(GetLastError() == ERROR_BROKEN_PIPE)
				reader->broken_pipe = 1;
			else
	    		reader->failed = 1;
				
		}
		else
		{
			if(number_of_bytes_readed > 0) 
			{
				for(i= 0, j= 0; i < number_of_bytes_readed; i++)
					if((buffer[i]) != '\r')
						clean[j++] = buffer[i];

				xbt_strbuff_append(output,clean);

				memset(buffer, 0, 1024);
				memset(clean, 0, 1024);
			} 
			else 
			{
				xbt_os_thread_yield();
			}
		}
	}

	free(clean);
	free(buffer);

	xbt_strbuff_chomp(command->output);
	xbt_strbuff_trim(command->output);

	reader->done = 1;
	
	if(command->failed)
	{
		if(command->reason == csr_write_failure)
		{
			if(command->output->used)
				INFO2("[%s] Output on write failure:\n%s",command->context->pos, command->output->data);
			else
				INFO1("[%s] No output before write failure",command->context->pos);
		}
		else if(command->reason == csr_write_pipe_broken)
		{
			if(command->output->used)
				INFO2("[%s] Output on broken pipe:\n%s",command->context->pos, command->output->data);
			else
				INFO1("[%s] No output before broken pipe",command->context->pos);
		}
	}
	else if(command->interrupted)
	{
		if(command->output->used)
			INFO2("[%s] Output on interruption:\n%s",command->context->pos, command->output->data);
		else
			INFO1("[%s] No output before interruption",command->context->pos);
	}
	else if(reader->failed && !command->failed && !command->interrupted && !command->successeded)
	{
		ERROR2("[%s] Error while reading output of child `%s'", command->context->pos, command->context->command_line);
		
		if(command->output->used)
			INFO2("[%s] Output on read failure:\n%s",command->context->pos, command->output->data);
		else
			INFO1("[%s] No output before read failure",command->context->pos);
		
		unit_set_error(command->unit, errno, 0, command->context->pos);
		command_kill(command);
		command_handle_failure(command, csr_read_failure);
	}

	
	
	return NULL;
}

#else
static void*
reader_start_routine(void* p) 
{
	reader_t reader = (reader_t)p;
	command_t command = reader->command;
	xbt_strbuff_t output = command->output;
	int stdout_fd = command->stdout_fd;
	int number_of_bytes_readed;
	int number_of_bytes_to_read = (1024 > SSIZE_MAX) ? SSIZE_MAX : 1024;

	int total = 0;
		
	char* buffer = (char*)calloc(number_of_bytes_to_read,sizeof(char));
	xbt_os_sem_release(reader->started);
	
	do 
	{
		number_of_bytes_readed = read(stdout_fd, buffer, number_of_bytes_to_read);
		
		if(number_of_bytes_readed < 0 && errno != EINTR && errno != EAGAIN) 
		{
			reader->failed = 1;
		}
		
		if(number_of_bytes_readed > 0) 
		{
			buffer[number_of_bytes_readed]='\0';
			xbt_strbuff_append(output,buffer);
			total += total;
		} 
		else 
		{
			xbt_os_thread_yield();
		}
	
	}while(!command->failed && !command->interrupted && !command->successeded && !reader->failed && (number_of_bytes_readed != 0 /* end of file <-> normal exit */));
	
	free(buffer);
	
	if(close(command->stdout_fd) < 0)
	{
		/* TODO */
	}
	else
		command->stdout_fd = INDEFINITE_FD;
	
	xbt_strbuff_chomp(command->output);
	xbt_strbuff_trim(command->output);

	reader->done = 1;

	if(command->failed)
	{
		if(command->reason == csr_write_failure)
		{
			if(command->output->used)
				INFO2("[%s] Output on write failure:\n%s",command->context->pos, command->output->data);
			else
				INFO1("[%s] No output before write failur",command->context->pos);
		}
		else if(command->reason == csr_write_pipe_broken)
		{
			if(command->output->used)
				INFO2("[%s] Output on broken pipe:\n%s",command->context->pos, command->output->data);
			else
				INFO1("[%s] No output before broken pipe",command->context->pos);
		}
	}
	else if(command->interrupted)
	{
		if(command->output->used)
			INFO2("[%s] Output on interruption:\n%s",command->context->pos, command->output->data);
		else
			INFO1("[%s] No output before interruption",command->context->pos);
	}
	else if(reader->failed && !command->failed && !command->interrupted && !command->successeded)
	{
		ERROR2("[%s] Error while reading output of child `%s'", command->context->pos, command->context->command_line);
		
		if(command->output->used)
			INFO2("[%s] Output on read failure:\n%s",command->context->pos, command->output->data);
		else
			INFO1("[%s] No output before read failure",command->context->pos);
		
		unit_set_error(command->unit, errno, 0, command->context->pos);
		command_kill(command);
		command_handle_failure(command, csr_read_failure);
	}
	
	
	return NULL;
} 
#endif

void
reader_wait(reader_t reader)
{
	xbt_os_thread_join(reader->thread, NULL);
}
