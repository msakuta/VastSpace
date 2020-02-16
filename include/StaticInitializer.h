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

#include "export.h"

/// \brief A template class object for initializing things before main().
///
/// An object of this template class should be global static.
/// Do not new-create it.
template<typename T = void (*)()>
class StaticInitializerTemp{
public:
	StaticInitializerTemp<T>(T callback, bool immediate = false){
		if(immediate)
			callback();
		else
			getInitializers().push_back(callback);
	}

	static void runInit() {
		for (auto& it : getInitializers()) {
			it();
		}
	}

protected:
	
	EXPORT static std::vector<T>& getInitializers(){
		static std::vector<T> initializers;
		return initializers;
	}
};

/// \brief An instance of StaticInitializerTemp, with the simplest function signnature as the argument.
typedef StaticInitializerTemp<void (*)()> StaticInitializer;

#endif
