#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#ifndef STDIN_FILENO
#define STDIN_FILENO	0
#endif

int
main(int argc, char* argv[])
{
	size_t number_of_bytes_to_read = 4096;
	int number_of_bytes_readed = 0;
	char buffer[4096] = {0};
	int failed = 0;
	int fd;
	
	if((fd = creat(argv[1], _S_IREAD | _S_IWRITE)) < 0)
	{
		fprintf(stderr,"creat() failed with the error %d\n",errno);
		return EXIT_FAILURE;
	}
		
	do
	{
		number_of_bytes_readed = read(STDIN_FILENO, buffer, number_of_bytes_to_read);
		
		if(number_of_bytes_readed < 0 && errno != EINTR && errno != EAGAIN) 
		{
			fprintf(stderr, "read failure %d\n", errno);
			failed = 1;
			break;
		}
		
		if(number_of_bytes_readed > 0) 
		{
			write(fd, buffer, strlen(buffer));
			memset(buffer, 0, number_of_bytes_to_read);
		}
		
	
	}while(number_of_bytes_readed);
	
	close(fd);
	
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
		
}

