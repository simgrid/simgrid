#include "LazyGuy.hpp"
#include <Host.hpp>

#include <iostream>
using namespace std;

MSG_IMPLEMENT_DYNAMIC(LazyGuy, Process);

int LazyGuy::main(int argc, char** argv)
{
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Hello !" << endl;
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Nobody's watching me ? Let's go to sleep." << endl;
	
	Process::currentProcess().suspend();
	
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Uuuh ? Did somebody call me ?" << endl;
	cout <<"[" << getName() << ":" << getHost().getName() << "] " << "Mmmh, goodbye now." << endl; 


	delete this;

	return 0;
}