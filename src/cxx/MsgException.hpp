#ifndef MSG_EXCEPTION_HPP
#define MSG_EXCEPTION_HPP

namespace msg
{
	class MsgException
	{
	public:
		// Default constructor.
		MsgException();
		
		// This constructor takes as parameter the message of the 
		// MsgException object.
		MsgException(const char* msg);
		
		// Copy constructor.
		MsgException(const MsgException& rMsgException);
		
		// Destructor.
		virtual ~MsgException();
		
		
	// Operations.
	
	// Returns the message of the exception.
	const char* what(void) const;
	
	
	// Getters/setters.
	
	// Operators.
		
	private:
		
		// Attributes.
		
		// The message of the exception.
		const char* msg;
		
	};
}

#endif // !MSG_EXCEPTION_HPP