#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#ifndef STDOUT_FILENO
#define STDOUT_FILENO	1
#endif

typedef unsigned char byte_t;

int
main(int argc, char* argv[])
{
	FILE* stream;
	struct stat s = {0};
	char* buffer;
	int i = 0;
	char c;
	int failed;

	stream = fopen(argv[1], "r");

	if(!stream)
	{
		fprintf(stderr, "fopen() failed withe the error %d\n", errno);
		return EXIT_FAILURE;
	}
	
	if(stat(argv[1], &s) < 0)
	{
		fclose(stream);
		fprintf(stderr, "stat() failed withe the error %d\n", errno);
		return EXIT_FAILURE;
	}
	
	
	if(!(buffer = (byte_t*) calloc(s.st_size + 1, sizeof(byte_t))))
	{
		fclose(stream);
		fprintf(stderr, "calloc() failed withe the error %d\n", errno);
		return EXIT_FAILURE;
	}
	
	
	while((c = getc(stream)) != EOF)
		if((int)c != 13)
			buffer[i++] = c;
	
	failed = ferror(stream);
	
	if(!failed || buffer)
		write(STDOUT_FILENO, buffer, strlen(buffer));
		
	fclose(stream);
	free(buffer);
	
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
		
}

