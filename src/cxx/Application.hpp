#ifndef MSG_APPLICATION_HPP
#define MSG_APPLICATION_HPP

#ifndef __cplusplus
	#error Application.hpp requires C++ compilation (use a .cxx suffix)
#endif

#include <InvalidParameterException.hpp>
#include <LogicException.hpp>
#include <MsgException.hpp>

namespace SimGrid
{
	namespace Msg
	{
		class Application
		{
		public:
			
		// Default constructor.
			Application();
			
			/*! \brief Copy constructor.
			 */
			Application(const Application& rApplication);
			
			/* \brief A constructor which takes as parameter the xml file of the application.
			 *
			 * \exception		[InvalidParameterException]	if the parameter file is invalid
			 *					(NULL or if the file does not exist).
			 */
			Application(const char* file)
			throw(InvalidParameterException);
			
			/*! \brief Destructor.
			 */
			virtual ~Application();
			
		// Operations.
			
			/*! \brief Application::deploy() - deploy the appliction.
			 *
			 * \return			If successfuly the application is deployed. Otherwise
			 *					the method throws an exception listed below.
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
			
			/*! \brief Application::deploy() - deploy the appliction.
			 *
			 * \return			If successfuly the application is deployed. Otherwise
			 *					the method throws an exception listed below.
			 *
			 * \exception		[InvalidParameterException]	if the parameter file is invalid
			 *					(NULL or not a xml deployment file name).
			 *					[MsgException]				if a internal exception occurs.
			 *					[LogicException]			if the application is already deployed.
			 *
			 * \
			 */
			void deploy(const char* file)
			throw(InvalidParameterException, LogicException, MsgException);
			
			/*! \brief Application::isDeployed() - tests if the application is deployed.
			 *
			 * \return			This method returns true is the application is deployed.
			 *					Otherwise the method returns false.
			 */
			bool isDeployed(void) const;
			
			
		// Getters/setters
			
			/*! \brief Application::setFile() - this setter sets the value of the file of the application.
			 *
			 * \return			If successful, the value of the file of the application is setted.
			 *					Otherwise the method throws on of the exceptions listed below.
			 *
			 * \exception		[InvalidParameterException]	if the parameter file is invalid
			 *												(NULL or does no exist).
			 *					[LogicException]			if you try to set the value of the file of an
			 *												application which is already deployed.
			 */
			void setFile(const char* file)
			throw (InvalidParameterException, LogicException);
			
			/*! \brief Application::getFile() - this getter returns the file of an application object.
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
			// the xml file which describe the application.
			const char* file;
			
		};
		
	} // namespace Msg
} // namespace SimGrid

#endif // !MSG_APPLICATION_HPP