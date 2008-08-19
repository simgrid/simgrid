#ifndef LAZYGUY_HPP
#define LAZYGUY_HPP

#include <Process.hpp>
using namespace SimGrid::Msg;

class LazyGuy : public Process
{
	// Not needed : DreamMaster directly constructs the object
	// MSG_DECLARE_DYNAMIC(LazyGuy);

	public:
	
	// Default constructor.
	LazyGuy(){}
	
	// Destructor.
	virtual ~LazyGuy(){}
	
	LazyGuy(const Host& rHost,const char* name)
	throw(NullPointerException)
	:Process(rHost, name){}
	
	int main(int argc, char** argv);
		
};


#endif // !LAZYGUY_HPP

