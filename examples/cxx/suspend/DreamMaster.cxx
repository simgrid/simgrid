#include "DreamMaster.hpp"
#include "LazyGuy.hpp"
#include <Host.hpp>
#include <HostNotFoundException.hpp>
#include <Msg.hpp>

#include <Msg.hpp>


MSG_IMPLEMENT_DYNAMIC(DreamMaster, Process);

int DreamMaster::main(int argc, char** argv)
{
	
	info("Hello");
	
	info("Let's create a lazy guy.");
	

	LazyGuy* lazy;

	try
	{
		Host currentHost = Host::currentHost();
	
		info(TEXT_("Current host  name : ") + TEXT_(currentHost.getName()));
	
		lazy = new LazyGuy(currentHost,"LazyGuy");
	
		info("Let's wait a little bit...");
	
		Process::sleep(10.0);
	
		info("Let's wake the lazy guy up! >:) ");
	
		lazy->resume();
	
	
	}
	catch(HostNotFoundException e)
	{
		error(TEXT_(e.toString()) + TEXT_(". Stopping Now!"));
		exit(1);
	}
	
	//lazy->migrate(currentHost);
	
	info("OK, goodbye now.");

	delete this;

	return 0;
	
}