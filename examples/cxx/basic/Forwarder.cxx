#include "Forwarder.hpp"
#include "BasicTask.hpp"
#include "FinalizeTask.hpp"
#include <Host.hpp>
#include <HostNotFoundException.hpp>

#include <Msg.hpp>

MSG_IMPLEMENT_DYNAMIC(Forwarder, Process);


int Forwarder::main(int argc, char** argv)
{
	info("Hello");
	
	int slavesCount = argc;
	
	Host* slaves = new Host[slavesCount];
	
	for (int i = 0; i < argc; i++) 
	{
		try 
		{	      
			slaves[i] = Host::getByName(argv[i]);
		} 
		catch (HostNotFoundException e) 
		{
			error(TEXT_(e.toString()));
			error("Buggy deployment file");
			exit(1);
		}
	}
	
	int taskCount = 0;

	while(true) 
	{
		Task* t = Task::get(0);	
	
		if(t->isInstanceOf("FinalizeTask")) 
		{
			info("All tasks have been dispatched. Let's tell everybody the computation is over.");
	
			for (int cpt = 0; cpt< slavesCount; cpt++) 
				slaves[cpt].put(0, new FinalizeTask());

			delete t;

			break;
		}

		info(TEXT_("Received \"") + TEXT_(t->getName()) + TEXT_("\" "));
	
		info(TEXT_("Sending \"") + TEXT_(t->getName()) + TEXT_("\" to \"") + TEXT_(slaves[taskCount % slavesCount].getName()) + TEXT_("\""));
		
		slaves[taskCount % slavesCount].put(0, t);
	
		taskCount++;
	}
	
	
	info("I'm done. See you!");
	
	delete[] slaves;

	delete this;
      
	return 0;
}