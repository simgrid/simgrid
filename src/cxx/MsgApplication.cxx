/*
 * Application.cxx
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */
 
 /* Application member functions implementation.
  */  

#include <MsgApplicationHandler.hpp>

#include <MsgApplication.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

#include <surf/surfxml_parse.h>

#ifndef S_ISREG
	#define S_ISREG(__mode) (((__mode) & S_IFMT) == S_IFREG)
#endif

namespace SimGrid
{
	namespace Msg
	{
	
		Application::Application()
		{
			this->file = NULL;
			this->deployed = false;
		}
				
		Application::Application(const Application& rApplication)
		{
				
			this->file = rApplication.getFile();
			this->deployed = rApplication.isDeployed();
		}
	
		Application::Application(const char* file)
		throw(NullPointerException, FileNotFoundException)
		{
			// check parameters
			
			if(!file)
				throw NullPointerException("file");
			
			struct stat statBuf = {0};
				
			if(stat(file, &statBuf) < 0 || !S_ISREG(statBuf.st_mode))
				throw FileNotFoundException(file);
				
			this->file = file;
			this->deployed = false;
		}
		
		Application::~Application()
		{
			// NOTHING TODO
		}
			
		void Application::deploy(const char* file)
		throw(NullPointerException, FileNotFoundException, LogicException, MsgException)
		{
			// check logic
			
			if(this->deployed)
				throw LogicException("application already deployed");
			
			// check the parameters
				
			if(!file)
				throw NullPointerException("file");
			
			struct stat statBuf = {0};
				
			if(stat(file, &statBuf) < 0 || !S_ISREG(statBuf.st_mode))
				throw FileNotFoundException(file);
					
			surf_parse_reset_parser();
			
			// set the begin of the xml process element handler
  			surfxml_add_callback(STag_surfxml_process_cb_list, ApplicationHandler::onBeginProcess);
  				
  			// set the process arg handler
  			surfxml_add_callback(ETag_surfxml_argument_cb_list, ApplicationHandler::onProcessArg);
  				
  			// set the properties handler
  			surfxml_add_callback(STag_surfxml_prop_cb_list, ApplicationHandler::OnProperty);
  				
  			// set the end of the xml process element handler
  			surfxml_add_callback(ETag_surfxml_process_cb_list, ApplicationHandler::onEndProcess);

  			surf_parse_open(file);
  			
  			// initialize the process factory used by the process handler to build the processes.
  			ApplicationHandler::onStartDocument();
  				
  			if(surf_parse())
  				throw MsgException("surf_parse() failed");
  			
  			surf_parse_close();
  			
  			// release the process factory
  			ApplicationHandler::onEndDocument();	
  			
  			this->file = file;
  			this->deployed = true;
		}
		
		void Application::deploy(void)
		throw(LogicException, MsgException)
		{
			// check logic
			
			if(this->deployed)
				throw LogicException("application already deployed");
			
			// check the parameters
			if(!this->file)
				throw LogicException("you must specify the xml file which describe the application\nuse Application::setFile()"); 
			
			surf_parse_reset_parser();
  			surfxml_add_callback(STag_surfxml_process_cb_list, ApplicationHandler::onBeginProcess);
  			surfxml_add_callback(ETag_surfxml_argument_cb_list, ApplicationHandler::onProcessArg);
  			surfxml_add_callback(STag_surfxml_prop_cb_list, ApplicationHandler::OnProperty);
  			surfxml_add_callback(ETag_surfxml_process_cb_list, ApplicationHandler::onEndProcess);
			
			// initialize the process factory used by the process handler to build the processes.
  			ApplicationHandler::onStartDocument();

  			surf_parse_open(file);
  			
  			if(surf_parse())
  				throw MsgException("surf_parse() failed");
  			
  			surf_parse_close();
  			
  			this->deployed = true;	
		}
		
		bool Application::isDeployed(void) const
		{
			return this->deployed;
		}
		
		void Application::setFile(const char* file)
		throw (NullPointerException, FileNotFoundException, LogicException)
		{
			// check logic
			
			if(this->deployed)
				throw LogicException("your are trying to change the file of an already deployed application");
				
			// check parameters
			
			if(!file)
				throw NullPointerException("file");
			
			struct stat statBuf = {0};
				
			if(stat(file, &statBuf) < 0 || !S_ISREG(statBuf.st_mode))
				throw FileNotFoundException("file (file not found)");
				
			this->file = file;
			
		}
		
		const char* Application::getFile(void) const
		{
			return this->file;
		}
		
		const Application& Application::operator = (const Application& rApplication)
		throw(LogicException)
		{
			// check logic
			
			if(this->deployed)
				throw LogicException("application already deployed");
			
			this->file = rApplication.getFile();
			this->deployed = rApplication.isDeployed();
			
			return *this;
		}
	} // namespace Msg
} // namespace SimGrid



