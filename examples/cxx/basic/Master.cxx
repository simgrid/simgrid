#include "Master.hpp"
#include "BasicTask.hpp"
#include "FinalizeTask.hpp"
#include <Host.hpp>
#include <HostNotFoundException.hpp>

#include <Msg.hpp>

#include <iostream>
using namespace std;


#ifndef BUFFMAX
#define BUFFMAX 260
#endif

MSG_IMPLEMENT_DYNAMIC(Master, Process);

int Master::main(int argc, char** argv)
{
	int channel = 0;
	char buff[BUFFMAX + 1] = {0};
	int numberOfTasks;		
	double taskComputeSize;		
	double taskCommunicateSize;
	StringHelper s;

	if (argc < 3) 
	{
		error("Master needs 3 arguments");
		exit(1);
	}
	
	info("Hello");
	
	int slaveCount = 0;
	Host* slaves = NULL;

	
	info( TEXT_("argc=") + TEXT_(argc));
	
	for (int i = 0; i< argc; i++)	    
		info(TEXT_("argv:") + TEXT_(argv[i]));
	
	sscanf(argv[0],"%d", &numberOfTasks);
	
	sscanf(argv[1],"%lg", &taskComputeSize);
	
	sscanf(argv[2],"%lg", &taskCommunicateSize);
	
	BasicTaskPtr* todo = new BasicTaskPtr[numberOfTasks];
	
	for (int i = 0; i < numberOfTasks; i++) 
		todo[i] = new BasicTask((TEXT_("Task_") + TEXT_(i)), taskComputeSize, taskCommunicateSize);
	
	slaveCount = argc - 3;
	slaves = new Host[slaveCount];
	
	for(int i = 3; i < argc ; i++)  
	{
		try 
		{
			slaves[i-3] = Host::getByName(argv[i]);
		}
		
		catch(HostNotFoundException e) 
		{
			cerr << e.toString() <<". Stopping Now!" << endl;
			exit(1);
		}
	}

	info(TEXT_("Got slave(s) :") + TEXT_(slaveCount));
	
	for (int i = 0; i < slaveCount; i++)
		info(TEXT_("\t") + TEXT_(slaves[i].getName()));
	
	 info(TEXT_("Got ") + TEXT_(numberOfTasks) + TEXT_(" task to process."));
	
	for (int i = 0; i < numberOfTasks; i++) 
	{
		info(TEXT_("Sending \"") + TEXT_(todo[i]->getName()) + TEXT_("\" to \"") + TEXT_(slaves[i % slaveCount].getName()) + TEXT_("\""));
	
		if(!strcmp(Host::currentHost().getName(), slaves[i % slaveCount].getName())) 
			info("Hey ! It's me ! ");

		slaves[i % slaveCount].put(channel, todo[i]);
	}
	
	info("Send completed");
	
	info("All tasks have been dispatched. Let's tell everybody the computation is over.");
	
	for (int i = 0; i < slaveCount; i++) 
			slaves[i].put(channel, new FinalizeTask());

	delete[] todo;
	delete[] slaves;
	
	info("Goodbye now!");

	delete this;

	return 0;
}