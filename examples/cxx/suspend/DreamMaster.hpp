#ifndef DREAMMASTER_HPP
#define DREAMMASTER_HPP

#include <MsgProcess.hpp>
using namespace SimGrid::Msg;

class DreamMaster : public Process
{
	MSG_DECLARE_DYNAMIC(DreamMaster);

	public:
	
	// Default constructor.
	DreamMaster(){}
	
	// Destructor.
	virtual ~DreamMaster(){}
	
	int main(int argc, char** argv);
		
};


#endif // !DREAMMASTER_HPP
