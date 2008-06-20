/*
 * Application.hpp
 *
 * This file contains the declaration of the wrapper class of the native MSG task type.
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */  
 
#ifndef MSG_APPLICATION_HPP
#define MSG_APPLICATION_HPP

#ifndef __cplusplus
	#error Application.hpp requires C++ compilation (use a .cxx suffix)
#endif

#include <NullPointerException.hpp>
#include <FileNotFoundException.hpp>
#include <LogicException.hpp>
#include <MsgException.hpp>

namespace SimGrid
{
	namespace Msg
	{
		// Application wrapper class declaration.
		class Application
		{
		public:
			
			/*! \brief Default constructor.
			 */
			Application();
			
			/*! \brief Copy constructor.
			 */
			Application(const Application& rApplication);
			
			/* \brief A constructor which takes as parameter the xml file of the application.
			 *
			 * \exception		If this constructor fails, it throws on of the exceptions described
			 *					below:
			 *		
			 *					[NullPointerException]	if the parameter file is NULL.
			 *
			 *					[FileNotFoundException]	if the file is not found.
			 */
			Application(const char* file)
			throw(NullPointerException, FileNotFoundException);
			
			/*! \brief Destructor.
			 */
			virtual ~Application();
			
		// Operations.
			
			/*! \brief Application::deploy() - deploy the appliction.
			 *
			 * \exception		If this method fails, it throws an exception listed below:
			 *
			 * \exception		[LogicException]			if the xml file which describes the application
			 *												is not yet specified or if the application is already
			 *												deployed.
			 *					[MsgException]				if a internal exception occurs.
			 *
			 * \remark			Before use this method, you must specify the xml file of the application to
			 *					deploy.
			 *
			 * \see				Application::setFile()
			 */
			 
			void deploy(void)
			throw(LogicExeption, MsgException);
			
			/*! \brief Application::deploy() - Deploy the appliction.
			 *
			 * \return			If successfuly the application is deployed. Otherwise
			 *					the method throws an exception listed below.
			 *
			 * \exception		[NullPointerException]		if the parameter file is NULL.
			 *					
			 *					[FileNotFoundException]		if the file is not found.
			 *
			 *					[MsgException]				if a internal exception occurs.
			 *
			 *					[LogicException]			if the application is already deployed.
			 *
			 * \
			 */
			void deploy(const char* file)
			throw(NullPointerException, FileNotFoundException, LogicException, MsgException);
			
			/*! \brief Application::isDeployed() - Tests if the application is deployed.
			 *
			 * \return			This method returns true is the application is deployed.
			 *					Otherwise the method returns false.
			 */
			bool isDeployed(void) const;
			
			
		// Getters/setters
			
			/*! \brief Application::setFile() - this setter sets the value of the file of the application.
			 *
			 * \exception		If this method fails, it throws on of the exceptions listed below:
			 *
			 * 					[NullPointerException]		if the parameter file is NULL.
			 *
			 *					[FileNotFoundException]		if the file is not found.
			 
			 *					[LogicException]			if you try to set the value of the file of an
			 *												application which is already deployed.
			 */
			void setFile(const char* file)
			throw (NullPointerException, FileNotFoundException, LogicException);
			
			/*! \brief Application::getFile() - This getter returns the name of the xml file which describes the 
			 * application of the simulation.
			 */
			const char* getFile(void) const;
			
		// Operators.
			
			/*! \brief Assignement operator.
			 *
			 * \exception		[LogicException]			if you try to assign an application already deployed.
			 */
			const Application& operator = (const Application& rApplication)
			throw(LogicException);
			
		private:
		// Attributes.
			
			// flag : if true the application was deployed.
			bool deployed;
			
			// the xml file which describes the application of the simulation.
			const char* file;
		};
		
	} // namespace Msg
} // namespace SimGrid

#endif // !MSG_APPLICATION_HPP