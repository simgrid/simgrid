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
	/* TODO : check the parameter */
	
	free(*ptr);
	*ptr = NULL;
	
	return 0;
}

void
reader_read(reader_t reader)
{
	reader->thread = xbt_os_thread_create("", reader_start_routine, reader);
}

#ifdef WIN32
/*static void*
reader_start_routine(void* p)
{
	reader_t reader = (reader_t)p;
	command_t command = reader->command;
	
	xbt_strbuff_t output = command->output;
	HANDLE stdout_fd = command->stdout_fd;
	
	DWORD number_of_bytes_to_read = 4096;
	DWORD number_of_bytes_readed = 0;
	
	char* buffer = (char*)calloc(number_of_bytes_to_read + 1,sizeof(char));
	char* clean;
	char* cp = buffer;
	size_t i, j;

	while(!command->failed && !command->interrupted && !command->successeded && !reader->failed && !reader->broken_pipe)
	{
		if(!ReadFile(reader->command->stdout_fd, cp, number_of_bytes_to_read, &number_of_bytes_readed, NULL) || (0 == number_of_bytes_readed))
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
				number_of_bytes_to_read -= number_of_bytes_readed;
				cp +=number_of_bytes_readed;
			} 
			else 
			{
				xbt_os_thread_yield();
			}
		}
	}

	if(reader->failed && !command->failed && !command->interrupted && !command->successeded)
	{
		error_register("read() function failed", (int)GetLastError(), command->context->command_line, command->unit->fstream->name);
		
		command_kill(command);
		command_handle_failure(command, csr_read_failure);
	}
	
	clean = (char*)calloc((unsigned int)strlen(buffer) + 1, sizeof(char));
	
	j = 0;
	
	for(i= 0; i < strlen(buffer); i++)
		if((int)(buffer[i]) != 13)
			clean[j++] = buffer[i];

	xbt_strbuff_append(output,clean);
	
	free(clean);
	free(buffer);

	reader->done = 1;
	
	return NULL;
}*/

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

	while(!command->failed && !command->interrupted && !command->successeded && !reader->failed && !reader->broken_pipe)
	{
		if(!ReadFile(reader->command->stdout_fd, buffer, number_of_bytes_to_read, &number_of_bytes_readed, NULL) || (0 == number_of_bytes_readed))
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
					if((int)(buffer[i]) != 13)
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

	if(reader->failed && !command->failed && !command->interrupted && !command->successeded)
	{
		error_register("read() function failed", (int)GetLastError(), command->context->command_line, command->unit->fstream->name);
		
		command_kill(command);
		command_handle_failure(command, csr_read_failure);
	}

	reader->done = 1;
	
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

	if(reader->failed && !command->failed && !command->interrupted && !command->successeded)
	{
		error_register("read() function failed", errno, command->context->command_line, command->unit->fstream->name);
		
		command_kill(command);
		command_handle_failure(command, csr_read_failure);
	}
	
	reader->done = 1;
	
	return NULL;
} 
#endif

void
reader_wait(reader_t reader)
{
	xbt_os_thread_join(reader->thread, NULL);
}
