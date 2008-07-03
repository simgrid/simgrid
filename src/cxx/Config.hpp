#ifndef MSG_CONFIG_HPP
#define MSG_CONFIG_HPP

namespace SimGrid
{
	namespace Msg
	{
		#if defined(WIN32)
			#if defined(SIMGRIDX_EXPORTS)
				#define SIMGRIDX_EXPORT __declspec(dllexport)
			#else
				#define SIMGRIDX_EXPORT 
			#endif
		#else
			#define SIMGRIDX_EXPORT
		#endif 
	} // namespace Msg
} // namespace SimGrid

#endif // MSG_CONFIG_HPP