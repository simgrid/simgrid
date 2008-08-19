#include "LazyGuy.hpp"
#include <Host.hpp>

#include <Msg.hpp>

// Not needed : DreamMaster directly constructs the object
// MSG_IMPLEMENT_DYNAMIC(LazyGuy, Process)

int LazyGuy::main(int argc, char** argv)
{
	info("Hello !");
	
	info("Nobody's watching me ? Let's go to sleep.");
	
	Process::currentProcess().suspend();
	
	info("Uuuh ? Did somebody call me ?");
	
	info("Mmmh, goodbye now."); 

	delete this;

	return 0;
}

