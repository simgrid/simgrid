#include "Master.hpp"
#include "CommTimeTask.hpp"
#include "FinalizeTask.hpp"
#include <Host.hpp>
#include <HostNotFoundException.hpp>
#include <Msg.hpp>

#include <Msg.hpp>

MSG_IMPLEMENT_DYNAMIC(Master, Process)

int Master::main(int argc, char** argv)
{
	int numberOfTasks;		
	double taskComputeSize;		
	double taskCommunicateSize;

	if (argc < 3) 
	{
		error("Master needs 3 arguments");
		exit(1);
	}
	
	info("Hello");
	
	int slaveCount = 0;
	Host* slaves = NULL;
	
	sscanf(argv[0],"%d", &numberOfTasks);
	
	sscanf(argv[1],"%lg", &taskComputeSize);
	
	sscanf(argv[2],"%lg", &taskCommunicateSize);
	
	
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
			error(TEXT_(e.toString()) + TEXT_(". Stopping Now!"));
			exit(1);
		}
	}
	
	info(TEXT_("Got slave(s) :" ) + TEXT_(slaveCount));
			
	for (int i = 0; i < slaveCount; i++)
		info(TEXT_("\t") + TEXT_(slaves[i].getName()));
	
	info(TEXT_("Got ") + TEXT_(numberOfTasks) + TEXT_(" task to process."));
	
	
	for (int i = 0; i < numberOfTasks; i++) 
	{			
		CommTimeTask* task = new CommTimeTask(TEXT_("Task_") + TEXT_(i), taskComputeSize, taskCommunicateSize);
		task->setTime(getClock());
		slaves[i % slaveCount].put(0, task);
		
	}
	
	info("All tasks have been dispatched. Let's tell everybody the computation is over.");
	
	for (int i = 0; i < slaveCount; i++) 
	{ 
		info(TEXT_("Finalize host ") + TEXT_(slaves[i].getName()) + TEXT_(" [") + TEXT_(i) + TEXT_("]"));
		slaves[i].put(0, new FinalizeTask());
	}
	
	info("All finalize messages have been dispatched. Goodbye now!");
	

	delete[] slaves;
	delete this;
	return 0;
}
