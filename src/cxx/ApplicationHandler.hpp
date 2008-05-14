#ifndef MSG_APPLICATION_HANDLER_HPP
#define MSG_APPLICATION_HANDLER_HPP

// Compilation C++ recquise
#ifndef __cplusplus
	#error ApplicationHandler.hpp requires C++ compilation (use a .cxx suffix)
#endif

#include <vector>
#include <map>

namespace msg
{
	
	struct Compare
	{
		public bool operator(const string& first, const string& second)
		{
			return first.compare(second);
		}
	};
	
	typedef vector<string> Strings;
	
	typedef map<string, string, Compare> Properties;
	
	
class ApplicationHandler
{
private :
	// Default constructor.
	ApplicationHandler();
	
	// Copy constructor.
	ApplicationHandler(const ApplicationHandler& rApplicationHandler);
	
	// Destructor
	virtual ~ApplicationHandler();
	
	class ProcessFactory 
	{
		public:
		
			Strings* args;
			Properties* properties;
      
		private:
			string hostName;
			string function;
        
		public :
	
		ProcessFactory() 
		{
  			this->args = new Strings();
  			this->properties = new Properties();
  			this->hostName = NULL;
  			this->function = NULL;
		}      
	
  		void setProcessIdentity(const string& hostName, const string& function);
    
    	void registerProcessArg(const string& arg) 
   		{
  			this->args->push_back(arg);
		}

		void setProperty(const string& id, const string& value)
		{
			this->properties->insert(Properties::value_type(id,value));	
		}

		const String& getHostName(void)
		{
			return this->hostName;
		}

    	void createProcess(void); 
    	
	};
	
	static ProcessFactory* processFactory;
	
	public:
		
	static void onStartDocument(void);
	
	static void onEndDocument(void);
		
	static void onBeginProcess(void);
	
	static void onProcessArg(void);
	
	static void OnProperty(void);
	
	static void onEndProcess(void);
};

}

#endif // !MSG_APPLICATION_HANDLER_HPP
	