#include "Master.hpp"
#include "BasicTask.hpp"
#include "FinalizeTask.hpp"

#include <MsgHost.hpp>
#include <HostNotFoundException.hpp>

#include <Msg.hpp>

MSG_IMPLEMENT_DYNAMIC(Master, Process)

int Master::main(int argc, char** argv)
{
	int taskCount;		
	double taskComputeSize;		
	double taskCommunicateSize;
	
	info("Hello");
		
		
	info(TEXT_("argc=") + TEXT_(argc));
		
	for (int i = 0; i< argc; i++)	    
		info(TEXT_("argv:") + TEXT_(argv[i]));
	
	sscanf(argv[0],"%d", &taskCount);
	
	sscanf(argv[1],"%lg", &taskComputeSize);
	
	sscanf(argv[2],"%lg", &taskCommunicateSize);
	
	BasicTaskPtr* basicTasks = new BasicTaskPtr[taskCount];
		
	for (int i = 0; i < taskCount; i++) 
		basicTasks[i] = new BasicTask(TEXT_("Task_") + TEXT_(i), taskComputeSize, taskCommunicateSize);		
	
	int aliasCount = argc - 3;
	
	char** aliases = (char**) calloc(aliasCount, sizeof(char*));
		
	for(int i = 3; i < argc ; i++) 
		aliases[i - 3] = _strdup(argv[i]);
		
	info(TEXT_("Got ") + TEXT_(aliasCount) + TEXT_(" alias(es) :"));
		
	for (int i = 0; i < aliasCount; i++)
		info(TEXT_("\t") + TEXT_(aliases[i]));
		
	info(TEXT_("Got ") + TEXT_(taskCount) + TEXT_(" task to process."));
		
	for (int i = 0; i < taskCount; i++) 
	{	
		info(TEXT_("Sending \"") + TEXT_(basicTasks[i]->getName()) + TEXT_("\" to \"") + TEXT_(aliases[i % aliasCount]) + TEXT_("\""));

		
		/*if((Host::currentHost().getName()).equals((aliases[i % aliasCount].split(":"))[0]))
			info("Hey ! It's me ! ");
		*/
			
		basicTasks[i]->send(aliases[i % aliasCount]);
	}
		
	info("Send completed");
	
	info("All tasks have been dispatched. Let's tell everybody the computation is over.");
	
	FinalizeTask* finalizeTask;
	
	for (int i = 0; i < aliasCount; i++) 
	{
		finalizeTask = new FinalizeTask();
		finalizeTask->send(aliases[i]);
		
	}
	
	info("Goodbye now!");

	delete[] basicTasks;
	delete[] aliases;

	delete this;

	return 0;
}
