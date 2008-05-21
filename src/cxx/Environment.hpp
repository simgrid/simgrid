/*
 * Environment.hpp
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
 
#ifndef MSG_ENVIRONMENT_HPP
#define MSG_ENVIRONMENT_HPP

#ifndef __cplusplus
	#error Environment.hpp requires C++ compilation (use a .cxx suffix)
#endif

#include <NullPointerException.hpp>
#include <FileNotFoundException.hpp>
#include <LogicException.hpp>
#include <MsgException.hpp>


namespace SimGrid
{
	namespace Msg
	{
		class Environment
		{
			public:
				
				/*! \brief Default constructor.
				 */
				Environment();
				
				/*! \brief Copy constructor.
				 */
				Environment(const Environment& rEnvironment);
				
				/*! \brief Constructor.
				 *
				 * \param file			The xml file describing the environment of the simulation.
				 *
				 * \exception			If this constructor fails, it throws one of the exception
				 *						described below:
				 *
				 *						[NullPointerException]		if the parameter file is NULL.
				 *
				 *						[FileNotFoundException]		if the file is not found.
				 */
				Environment(const char* file)
				throw(NullPointerException, FileNotFoundException);
				
				/*! \brief Destructor.
				 */
				virtual ~Environment();
				
			// Operations.
				
				/*! brief Environment::load() - Load the environment of a simulation.
				 *
				 * \exception			If this method fails, it throws the exception described below:
				 *
				 *						[LogicException]			if the file of the environment is not yet specified or
				 *													if the environment is already loaded.
				 */
				void load(void)
				throw(LogicException);
				
				/*! \brief Environment::load() - Load the environment of a simulation.
				 *
				 * \param file			The xml file describing the environment of the simulation.
				 *
				 * \exception			If this method fails, it throws one of the exceptions described below.
				 *
				 *						[NullPointerException]		if the parameter file is NULL.
				 *
				 *						[FileNotFoundException]		if the specified file is not found.
				 *
				 *						[LogicException]			if the environment is already loaded.
				 */
				void load(const char* file)
				throw(NullPointerException, FileNotFoundException, LogicException);
				
				/*! \brief Environment::isLoaded() - Tests if an environment is loaded.
				 *
				 * \return				If the environment is loaded, the method returns true. Otherwise the method
				 *						returns false.
				 */
				bool isLoaded(void) const;
				
			// Getters/setters
				/*! \brief Environment::setFile() - Sets the xml file of the environment.
				 *
				 * \param file			The file describing the environment.
				 *
				 * \exception			If the method fails, it throws one of the exceptions described below:
				 *
				 *						[NullPointerException]		if the parameter file is NULL.
				 *				
				 *						[FileNotFoundException]		if the file is not found.
				 *
				 *						[LogicException]			if the environment is already loaded.
				 */
				void setFile(const char* file)
				throw(NullPointerException, FileNotFoundException, LogicException);
				
				/*! \brief Environment::getFile() - Gets the xml file environment description.
				 *
				 * \return				The xml file describing the environment.				
				 *
				 */
				const char* getFile(void) const;
				
			// Operators.
				
				/*! \brief Assignment operator.
				 *
				 * \exception			If this operator fails, it throws the exception described below:
				 *
				 *						[LogicException]			if you try to assign a loaded environment.
				 */
				const Environment& operator = (const Environment& rEnvironment)
				throw(LogicException);
				
			private:
				
				// Attributes.
				
				// the xml file which describe the environment of the simulation.
				const char* file;
				
				// flag : is true the environment of the simulation is loaded.
				bool loaded.
		};
		
	} // namespace Msg
} // namespace SimGrid


#endif // !MSG_ENVIRONMENT_HPP