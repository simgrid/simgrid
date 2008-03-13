#include <reader.h>
#include <command.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

static void*
reader_start_routine(void* p);

reader_t
reader_new(command_t command)
{
	reader_t reader = xbt_new0(s_reader_t, 1);
	
	reader->thread = NULL;
	reader->command = command;
	reader->broken_pipe = 0;
	reader->failed = 0;
	
	reader->started = xbt_os_sem_init(0);
	
	return reader;
}

void
reader_free(reader_t* reader)
{
	free(*reader);
	*reader = NULL;
}

void
reader_read(reader_t reader)
{
	reader->thread = xbt_os_thread_create("", reader_start_routine, reader);
}

#ifdef WIN32
static void*
reader_start_routine(void* p)
{
	reader_t reader = (reader_t)p;
	command_t command = reader->command;
	
	xbt_strbuff_t output = command->output;
	HANDLE stdout_fd = command->stdout_fd;
	
	DWORD number_of_bytes_to_read = 4096;
	DWORD number_of_bytes_readed;
	
	char* buffer = (char*)calloc(number_of_bytes_to_read,sizeof(char));
	
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
				buffer[number_of_bytes_readed]='\0';
				xbt_strbuff_append(output,buffer);
			} 
			else 
			{
				xbt_os_thread_yield();
			}
		}
	}
	
	free(buffer);

	if(reader->failed && !command->failed && !command->interrupted && !command->successeded)
	{
		command_kill(command);
		exit_code = EREAD;
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
			usleep(100);
		}
	
	}while(!command->failed && !command->interrupted && !command->successeded && !reader->failed && (number_of_bytes_readed != 0 /* end of file <-> normal exit */));
	
	free(buffer);

	if(reader->failed && !command->failed && !command->interrupted && !command->successeded)
	{
		command_kill(command);
		exit_code = EREAD;
		command_handle_failure(command, csr_read_failure);
	}
	
	reader->broken_pipe = 1;
	
	/*printf("the reader of the command %p is ended\n",command);*/
	
	return NULL;
} 
#endif

void
reader_wait(reader_t reader)
{
	xbt_os_thread_join(reader->thread, NULL);
}
