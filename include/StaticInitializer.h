/** \file
 * \brief Definition of StaticInitializerTemp class template and its instance StaticInitializer.
 *
 * The "dynamic" initializer can be found in sqadapt.h.
 * It will be invoked for each Game object or Squirrel VM.
 *
 * \sa sqadapt.h
 */
#ifndef STATICINITIALIZER_H
#define STATICINITIALIZER_H

/// \brief A template class object for initializing things before main().
///
/// An object of this template class should be global static.
/// Do not new-create it.
template<typename T = void (*)()>
class StaticInitializerTemp{
public:
	StaticInitializerTemp<T>(T callback){
		callback();
	}
};

/// \brief An instance of StaticInitializerTemp, with the simplest function signnature as the argument.
typedef StaticInitializerTemp<void (*)()> StaticInitializer;

#endif
