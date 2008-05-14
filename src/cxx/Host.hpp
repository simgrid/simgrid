#ifndef MSG_HOST_HPP
#define MSG_HOST_HPP


// Compilation C++ recquise
#ifndef __cplusplus
	#error Host.hpp requires C++ compilation (use a .cxx suffix)
#endif

namespace msg
{
class Host
{
	private :
		// Default constructor.
		Host();
		
		// Copy constructor.
		Host(const Host& rHost);
		
		// Destructor.
		virtual ~Host();
	
	public:	
		
	// Operations
	
	static Host& getByName(const char* hostName)
	throw(HostNotFoundException);
	
	static int getNumber(void);
	
	static Host& currentHost(void);
	
	static int all(Host** hosts);
	
	
	const char* getName(void) const;
	
	void setData(void* data);
	
	// Getters/setters
	
	void* getData(void) const;
	
	int Host::getRunningTaskNumber(void) const;
		
	double getSpeed(void) const;
	
	bool hasData(void) const;
	
	bool isAvailble(void) const;
	
	void put(int channel, const Task& rTask)
	throw(NativeException);
	
	void put(int channel, Task task, double timeout) 
	throw(NativeException);
	
	void Host::putBounded(int channel, const Task& rTask, double maxRate) 
	throw(NativeException);

	
	void send(const Task& rTask) 
	throw(NativeException);
	
	void send(const char* alias, const Task& rTask) 
	throw(NativeException);
	
	void send(const Task& rTask, double timeout) 
	throw(NativeException);
	
	void send(const char* alias, const Task& rTask, double timeout) 
	throw(NativeException);
	
	void sendBounded(const Task& rTask, double maxRate) 
	throw(NativeException);
	
	void sendBounded(const char* alias, const Task& rTask, double maxRate) 
	throw(NativeException);   
	
	
	private:
		// Attributes.
		
		m_host_t nativeHost;
		
		void* data;
	};
}

#endif // !MSG_HOST_HPP