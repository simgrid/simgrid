#ifndef FORWARDER_HPP
#define FORWARDER_HPP

#include <Process.hpp>
using namespace SimGrid::Msg;

class Forwarder : public Process
{
	MSG_DECLARE_DYNAMIC(Forwarder);

	public:
	
	// Default constructor.
	Forwarder(){}
	
	// Destructor.
	virtual ~Forwarder(){}
	
	int main(int argc, char** argv);
		
};


#endif // !FORWARDER_HPP
