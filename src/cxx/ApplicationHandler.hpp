#ifndef MSG_APPLICATION_HANDLER_HPP
#define MSG_APPLICATION_HANDLER_HPP

// Compilation C++ recquise
#ifndef __cplusplus
	#error ApplicationHandler.hpp requires C++ compilation (use a .cxx suffix)
#endif


namespace SimGrid
{
	namespace Msg
	{
		// Declaration of the class ApplicationHandler.
		class ApplicationHandler
		{
		private :
			
			// Desable the default constructor, the copy constructor , the assignement operator
			// and the destructor of this class. Assume that this class is static.
			
			// Default constructor.
			ApplicationHandler();
			
			// Copy constructor.
			ApplicationHandler(const ApplicationHandler& rApplicationHandler);
			
			// Destructor
			virtual ~ApplicationHandler();
			
			// Assignement operator.
			const ApplicationHandler& operator = (const ApplicationHandler& rApplicationHandler);
			
			class ProcessFactory 
			{
				public:
					
					// the list of the argument of the process to create.
					xbt_dynar_t args;
					// the properties of the process to create
					xbt_dict_t properties;
		      
				private:
					
					// the current host name parsed
					const char* hostName;
					// the name of the class of the process
					const char* function;
		        
				public :
			
				// Default constructor.
				ProcessFactory(); 
				
				// Copy constructor.
				ProcessFactory(const ProcessFactory& rProcessFactory);
				
				// Destructor.
				virtual ~ProcessFactory();
				
				// Set the identity of the current process.
		  		void setProcessIdentity(const char* hostName, const char* function);
		    	
		    	// Register an argument of the current process.
		    	void registerProcessArg(const char* arg); 
				
				// Set the property of the current process.
				void setProperty(const char* id, const char* value);
				
				// Return the host name of the current process.
				const const char* getHostName(void);
				
				// Create the current process.
		    	void createProcess(void); 
		    	
			};
			
			// the process factory used by the application handler.
			static ProcessFactory* processFactory;
			
			public:
			
			// Handle the begining of the parsing of the xml file describing the application.
			static void onStartDocument(void);
			
			// Handle at the end of the parsing.
			static void onEndDocument(void);
			
			// Handle the begining of the parsing of a xml process element.
			static void onBeginProcess(void);
			
			// Handle the parsing of an argument of the current xml process element.
			static void onProcessArg(void);
			
			// Handle the parsing of a property of the currnet xml process element.
			static void OnProperty(void);
			
			// Handle the end of the parsing of a xml process element
			static void onEndProcess(void);
		};

	} // namespace Msg
} // namespace SimGrid

#endif // !MSG_APPLICATION_HANDLER_HPP
	