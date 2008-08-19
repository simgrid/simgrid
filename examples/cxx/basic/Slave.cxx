#include "Slave.hpp"
#include "FinalizeTask.hpp"
#include <Host.hpp>
#include <HostNotFoundException.hpp>

#include <iostream>
using namespace std;

#include <Msg.hpp>

MSG_IMPLEMENT_DYNAMIC(Slave, Process)

int Slave::main(int argc, char** argv)
{
	info("Hello");;
	
	while(true) 
	{ 
		Task* t = Task::get(0);	
	
		if(t->isInstanceOf("FinalizeTask")) 
		{
			delete t;
			break;
		}
		
		info(TEXT_("Received \"") + TEXT_(t->getName()) + TEXT_("\" "));
	
		info(TEXT_("Processing \"") + TEXT_(t->getName()));

		t->execute();
		
		info(TEXT_("\"") + TEXT_(t->getName()) + TEXT_("\" done "));

		delete t;
	}
	
	info("Received Finalize. I'm done. See you!");
	
	delete this;

	return 0;
}
