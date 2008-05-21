#include <ApplicationHandler.hpp>

namespace SimGrid
{
	namespace Msg
	{

		ApplicationHandler::ProcessFactory::processFactory = NULL;
			
		// Desable the default constructor, the copy constructor , the assignement operator
		// and the destructor of this class. Assume that this class is static.
		
		// Default constructor.
		ApplicationHandler::ApplicationHandler()
		{
			// NOTHING TODO
		}
		
		// Copy constructor.
		ApplicationHandler::ApplicationHandler(const ApplicationHandler& rApplicationHandler)
		{
			// NOTHING TODO
		}
		
		// Destructor
		ApplicationHandler::~ApplicationHandler()
		{
			// NOTHING TODO
		}
		
		// Assignement operator.
		const ApplicationHandler& ApplicationHandler::operator = (const ApplicationHandler& rApplicationHandler)
		{
			return *this;
		}
			
		void ApplicationHandler::onStartDocument(void)
		{
			// instanciate the factory at the begining of the parsing
			processFactory = new ProcessFactory();	
		}
		
		void ApplicationHandler::onEndDocument(void)
		{
			// release the handler at the end of the parsing.
			if(processFactory)
				delete processFactroy;
		}
			
		void ApplicationHandler::onBeginProcess(void)
		{
			// set the process identity at the begin of the xml process element.
			processFactory->setProcessIdentity(A_surfxml_process_host, A_surfxml_process_function);
		}		
		
		void ApplicationHandler::onProcessArg(void)
		{
			// register the argument of the current xml process element.
			processFactory->registerProcessArg(A_surfxml_argument_value);
		}
		
		void ApplicationHandler::OnProperty(void)
		{
			// set the property of the current xml process element.
			 processFactory->setProperty(A_surfxml_prop_id, A_surfxml_prop_value);
		}
		
		void ApplicationHandler::onEndProcess(void)
		{
			// at the end of the xml process element create the wrapper process (of the native Msg process)
			processFactory->createProcess();
		}
		
		/////////////////////////////////////////////////////////////////////////////////////////////////
		// Process factory connected member functions
		
		ApplicationHandler::ProcessFactory::ProcessFactory() 
		{
			this->args = xbt_dynar_new(sizeof(char*),ProcessFactory::freeCstr);
			this->properties = NULL; // TODO instanciate the dictionary
			this->hostName = NULL;
			this->function = NULL;
		}      
		
		// create the cxx process wrapper.
		void ApplicationHandler::ProcessFactory::createProcess() 
		{
			Host host;
			Process* process;
			
			// dynamic creation of a instance fo the process from its name (which is specified by the element function
			// in the xml application file.
			try
			{
				process = (Process*)Class::fromName(this->function);
			}
			catch(ClassNotFoundException e)
			{
				cerr << e.toString();	
			}
			
			// try to retrieve the host of the process from its name
			try
			{
				host = Host::getByName(this->hostName);	
			}
			catch(HostNotFoundException(this->hostName))
			{
				cerr << e.toString();
			}
				
			
			// build the list of the arguments of the newly created process.
			int argc = xbt_dynar_length(this->args);
			
			char** argv = (char**)calloc(argc, sizeof(char*));
			
			for(int i = 0; i < argc; i++)
				xbt_dynar_pop(this->args, &(argv[i]));
			
			// finaly create the process (for more detail on the process creation see Process::create()
			process->create(host, this->function , argc, argv);
			
		  	// TODO add the properties of the process
			/*process->properties = this->properties;
			this->properties = new Properties();*/
		}
			
		void ApplicationHandler::ProcessFactory::setProcessIdentity(const string& hostName, const string& function) 
		{
			this->hostName = hostName;
			this->function = function;
		
			/*if (!this->args->empty())
				this->args->clear();
		
			if(!this->properties->empty())
				this->properties->clear();*/
		}
		
		// callback function used by the dynamic array to cleanup all of its elements.
		void ApplicationHandler::ProcessFactory::freeCstr(void* cstr)
		{
			free(*(void**)str);
		}
		
		void ApplicationHandler::ProcessFactory::registerProcessArg(const char* arg) 
		{
			char* cstr = strdup(arg);
			xbt_dynar_push(this->args, &cstr);
		}
		
		void ApplicationHandler::ProcessFactory::setProperty(const char* id, const char* value)
		{
			// TODO implement this function;	
		}
		
		const const char* ApplicationHandler::ProcessFactory::getHostName(void)
		{
			return this->hostName;
		}

	} // namespace Msg
} // namespace SimGrid