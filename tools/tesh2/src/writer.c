#include <writer.h>
#include <command.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

static void*
writer_start_routine(void* p);

writer_t
writer_new(command_t command)
{
	writer_t writer = xbt_new0(s_writer_t, 1);

	writer->thread = NULL;
	writer->command = command;
	writer->written = xbt_os_sem_init(0);
	writer->can_write = xbt_os_sem_init(0);
	
	writer->done = 0;

	return writer;
}

void
writer_free(writer_t* writer)
{
	
	/*xbt_os_sem_destroy((*writer)->started);
	xbt_os_sem_destroy((*writer)->can_write);*/
	
	free(*writer);
	*writer = NULL;
}

void
writer_write(writer_t writer)
{
	writer->thread = xbt_os_thread_create("", writer_start_routine, writer);
}

#ifdef WIN32
static void*
writer_start_routine(void* p)
{
	writer_t writer = (writer_t)p;
	command_t command = writer->command;
	
	char* input = (char*)(command->context->input->data);
	
	DWORD number_of_bytes_to_write = command->context->input->used;
	DWORD number_of_bytes_written = 0;

	
	while(!command->failed && !command->interrupted && !command->successeded && ! writer->failed && ! writer->broken_pipe && number_of_bytes_to_write)
	{
		if(!WriteFile(writer->command->stdin_fd, input, number_of_bytes_to_write, &number_of_bytes_written, NULL))
		{
			if(GetLastError() ==  ERROR_NO_DATA)
				writer->broken_pipe = 1;
			else
		    	writer->failed = 1;
				
		}
		else
		{
			input += number_of_bytes_written;
			number_of_bytes_to_write -= number_of_bytes_written;
		}
	}
	
	command->context->input->data[0]='\0';
	command->context->input->used=0;
	
	if(writer->failed  && !command->successeded && !command->failed && !command->interrupted)
	{
		command_kill(command);
		exit_code = EWRITE;
		command_handle_failure(command, csr_write_failure);
	}
	/*else if(writer->broken_pipe && !command->successeded && !command->failed && !command->interrupted)
	{
		command_kill(command);
		command_handle_failure(command, csr_write_pipe_broken);
	}*/
	

  	return NULL;
}
#else
/*static void* 
writer_start_routine(void* p) 
{
	writer_t writer = (writer_t)p;
	command_t command = writer->command;
	int number_of_bytes_written = 0;
	int number_of_bytes_to_write = command->context->input->used;
	char* input = (char*)(command->context->input->data);
	int got;
	
	xbt_os_sem_release(writer->started);
	
	
	
	while(!command->failed && !command->interrupted && !command->successeded && ! writer->failed && ! writer->broken_pipe && (number_of_bytes_written < number_of_bytes_to_write))
	{
		got = write(writer->command->stdin_fd, input + number_of_bytes_written, number_of_bytes_to_write - number_of_bytes_written);
		
		if(got > 0)
			number_of_bytes_written += got;
		
		if(got < 0) 
		{
			if(errno == EPIPE) 
			{
				writer->broken_pipe = 1;
			} 
			else if(errno != EINTR && errno != EAGAIN && errno != EPIPE) 
			{
				writer->failed = 1;
			}
		}
	
		if(got <= 0)
			usleep(100);
	}
	
	command->context->input->data[0]='\0';
	command->context->input->used=0;
	
	if(writer->failed  && !command->successeded && !command->failed && !command->interrupted)
	{
		command_kill(command);
		exit_code = EWRITE;
		command_handle_failure(command, csr_write_failure);
	}
	else if(writer->broken_pipe && !command->successeded && !command->failed && !command->interrupted)
	{
		command_kill(command);
		exit_code = EWRITEPIPE;
		command_handle_failure(command, csr_write_pipe_broken);
	}
	
	
	close(command->stdin_fd);
	command->stdin_fd = INDEFINITE_FD;
	
  	return NULL;
}*/

static void* 
writer_start_routine(void* p) 
{
	writer_t writer = (writer_t)p;
	command_t command = writer->command;
	int number_of_bytes_to_write = command->context->input->used;
	char* input = (char*)(command->context->input->data);
	int got;
	int released = 0;
	
	
	xbt_os_sem_acquire(writer->can_write);
	
	
	while(!command->failed && !command->interrupted && !command->successeded && number_of_bytes_to_write > 0)
	{
		got = number_of_bytes_to_write > PIPE_BUF ? PIPE_BUF : number_of_bytes_to_write;
		got = write(writer->command->stdin_fd, input, got );
		
		if(got < 0) 
		{
			if(EINTR == errno)
				continue;
				
			else if(EAGAIN == errno)
			{/* the pipe is full */
				if(!released)
				{
					xbt_os_sem_release(writer->written);
					released = 1;
				}
				
				continue;
			}
			else if(EPIPE == errno) 
			{
				writer->broken_pipe = 1;
				break;
			} 
			else
			{
				writer->failed = 1;
				break;
			}
			
		}
		
		number_of_bytes_to_write -= got;
		input += got;
		
		if(got == 0)
			xbt_os_thread_yield();
		
	}
	
	if(!released)
	{
		xbt_os_sem_release(writer->written);
		released = 1;
	}
	

	close(command->stdin_fd);
	command->stdin_fd = INDEFINITE_FD;
	
	command->context->input->data[0]='\0';
	command->context->input->used=0;
	
	if(writer->failed  && !command->successeded && !command->failed && !command->interrupted)
	{
		command_kill(command);
		exit_code = EWRITE;
		command_handle_failure(command, csr_write_failure);
	}
	else if(writer->broken_pipe && !command->successeded && !command->failed && !command->interrupted)
	{
		command_kill(command);
		exit_code = EWRITEPIPE;
		command_handle_failure(command, csr_write_pipe_broken);
	}
	
	writer->done = 1;
	
  	return NULL;
	
}

#endif
void
writer_wait(writer_t writer)
{
	xbt_os_thread_join(writer->thread, NULL);

}
