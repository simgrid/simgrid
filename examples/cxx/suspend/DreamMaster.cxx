#include "DreamMaster.hpp"
#include "LazyGuy.hpp"
#include <Host.hpp>
#include <HostNotFoundException.hpp>
#include <Msg.hpp>

#include <iostream>
using namespace std;

MSG_IMPLEMENT_DYNAMIC(DreamMaster, Process);

int DreamMaster::main(int argc, char** argv)
{
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Hello I'm " << getName() << " on " << getHost().getName() << "!" << endl;
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Let's create a lazy guy.";
	

	LazyGuy* lazy;

	try
	{
		Host currentHost = Host::currentHost();
	
		cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Current host  name : " << currentHost.getName() << endl;
	
		lazy = new LazyGuy(currentHost,"LazyGuy");
	
		cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Let's wait a little bit..." << endl;
	
		Process::sleep(10.0);
	
		cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Let's wake the lazy guy up! >:) " << endl;
	
		lazy->resume();
	
	
	}
	catch(HostNotFoundException e)
	{
		cerr << e.toString() <<". Stopping Now!" << endl;
		exit(1);
	}
	
	//lazy->migrate(currentHost);
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "OK, goodbye now." << endl;

	return 0;
	
}