/*
 * ApplicationHandler.cxx
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */
 
 /* ApplicationHandler member functions implementation.
  */  


#include <Object.hpp>
#include <MsgApplicationHandler.hpp>

#include <MsgProcess.hpp>
#include <MsgHost.hpp>





#include <stdlib.h>

#include <surf/surfxml_parse.h>
#include <xbt/sysdep.h>


namespace SimGrid
{
	namespace Msg
	{

		ApplicationHandler::ProcessFactory* ApplicationHandler::processFactory = NULL;
			
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
				delete processFactory;
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
			this->args = xbt_dynar_new(sizeof(char*),ApplicationHandler::ProcessFactory::freeCstr);
			this->properties = NULL; // TODO instanciate the dictionary
			this->hostName = NULL;
			this->function = NULL;
		} 

		ApplicationHandler::ProcessFactory::~ProcessFactory()
		{
			xbt_dynar_free(&(this->args));
		}
		
		// create the cxx process wrapper.
		void ApplicationHandler::ProcessFactory::createProcess() 
		throw (ClassNotFoundException, HostNotFoundException)
		{
			Host host;
			Class* c;
			Process* process;
			
			// try to dynamicaly create an instance of the process from its name (which is specified by the element function
			// in the xml application file.
			// if this static method fails, it throws an exception of the class ClassNotFoundException
			c = Class::fromName(this->function);
			process = reinterpret_cast<Process*>(c->createObject());
			
			// try to retrieve the host of the process from its name
			// if this method fails, it throws an exception of the class HostNotFoundException
			host = Host::getByName(this->hostName);	
			
			// build the list of the arguments of the newly created process.
			int argc = xbt_dynar_length(this->args);
			
			char** argv = (char**)calloc(argc, sizeof(char*));
			
			for(int i = argc -1; i >= 0; i--)
				xbt_dynar_pop(this->args, &(argv[i]));
			
			// finaly create the process (for more detail on the process creation see Process::create()
			process->create(host, this->function , argc, argv);
			
		  	// TODO add the properties of the process
			/*process->properties = this->properties;
			this->properties = new Properties();*/
		}
			
		void ApplicationHandler::ProcessFactory::setProcessIdentity(const char* hostName, const char* function) 
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
			free(*(void**)cstr);
		}
		
		void ApplicationHandler::ProcessFactory::registerProcessArg(const char* arg) 
		{
			char* cstr = _strdup(arg);
			xbt_dynar_push(this->args, &cstr);
		}
		
		void ApplicationHandler::ProcessFactory::setProperty(const char* id, const char* value)
		{
			// TODO implement this function;	
		}
		
		const char* ApplicationHandler::ProcessFactory::getHostName(void)
		{
			return this->hostName;
		}

	} // namespace Msg
} // namespace SimGrid

