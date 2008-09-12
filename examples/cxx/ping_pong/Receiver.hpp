#ifndef RECEIVER_HPP
#define RECEIVER_HPP

#include <MsgProcess.hpp>
using namespace SimGrid::Msg;
	
class Receiver : public Process
{
	MSG_DECLARE_DYNAMIC(Receiver);

	public:
	
	// Default constructor.
	Receiver(){}
	
	// Destructor.
	virtual ~Receiver(){}
	
	int main(int argc, char** argv);
		
};


#endif // !RECEIVER_HPP

