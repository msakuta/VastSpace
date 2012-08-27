/** \file
 * \brief Definition of SqInitProcess class and its companions.
 */
#ifndef SQINITPROCESS_H
#define SQINITPROCESS_H

#include "export.h"
#include <squirrel.h>
#include <stddef.h> // Just for NULL

/// \brief Callback functionoid that processes Squirrel VM that is initialized in SqInit().
///
/// Multiple SqInitProcess derived class objects can be specified.
///
/// SqInit() uses temporary table that delegates the root table (can read from the root table but
/// writing won't affect it) that will be popped from the stack before the function returns.
/// It means the SqInitProcess derived class objects must process the defined values in the table
/// before SqInit() returns.
class EXPORT SqInitProcess{
public:
	SqInitProcess() : next(NULL){}

	/// \brief The method called after the VM is initialized.
	virtual void process(HSQUIRRELVM v)const = 0;

	/// \brief Multiple SqInitProcesses can be connected with this operator before passing to SqInit().
	const SqInitProcess &operator<<=(const SqInitProcess &o)const;

	const SqInitProcess &chain(const SqInitProcess &o)const;

protected:
	mutable const SqInitProcess *next;
	static bool SqInit(HSQUIRRELVM v, const SQChar *scriptFile, const SqInitProcess &procs);
	friend bool SqInit(HSQUIRRELVM v, const SQChar *scriptFile, const SqInitProcess &procs);
};

/// \brief Processes nothing.
class NullProcess : public SqInitProcess{
public:
	virtual void process(HSQUIRRELVM)const{}
};


inline bool SqInit(HSQUIRRELVM v, const SQChar *scriptFile, const SqInitProcess &procs = NullProcess()){
	return SqInitProcess::SqInit(v, scriptFile, procs);
}

class EXPORT IntProcess : public SqInitProcess{
public:
	int &value;
	const SQChar *name;
	bool mandatory;
	IntProcess(int &value, const char *name, bool mandatory = true) : value(value), name(name), mandatory(mandatory){}
	virtual void process(HSQUIRRELVM)const;
};

class EXPORT SingleDoubleProcess : public SqInitProcess{
public:
	double &value;
	const SQChar *name;
	bool mandatory;
	SingleDoubleProcess(double &value, const char *name, bool mandatory = true) : value(value), name(name), mandatory(mandatory){}
	virtual void process(HSQUIRRELVM)const;
};


//-----------------------------------------------------------------------------
//    Inline Implementation
//-----------------------------------------------------------------------------


/// Now this is interesting. If it is used properly, it does not use heap memory at all, so it
/// would be way faster than std::vector or something. In addition, the code is more readable too.
/// For example, expressions like SqInit(v, "file", A <<= B <<= C) will chain the objects in the
/// stack.
///
/// The selection of the operator is arbitrary (nothing to do with bit shifting), but needs to be
/// right associative in order to process them in the order they appear in the argument
/// (in the above example, order of A, B, C), or this operator or SqInit() has to do recursive calls
/// somehow.
///
/// Also, += could be more readable, but it implies the left object accumulates the right
/// and the right could be deleted thereafter. It's not true; both object must be allocated
/// until SqInit() exits.
///
/// In association with std::streams, it could have << operator overloaded to do the same thing.
/// But the operator is left associative and we cannot simply assign neighboring object to next
/// unless we chain them in the reverse order (if the expression is like SqInit("file, A << B << C),
/// order of C, B, A). It would be not obvious to the user (of this class).
///
/// The appearance and usage of the operator remind me of Haskell Monads, but internal functioning is
/// nothing to do with them.
///
/// The argument, the returned value and the this pointer all must be const. That's because they
/// are temporary objects which are rvalues. In fact, we want to modify the content of this object to
/// construct a linked list, and it requires a mutable pointer (which is next).
///
/// The temporary objects resides in memory long enough to pass to the outermost function call.
/// That's guaranteed by the standard; temporary objects shall be alive within a full expression.
inline const SqInitProcess &SqInitProcess::operator<<=(const SqInitProcess &o)const{
	next = &o;
	return *this;
}
/*
SqInitProcess &SqInitProcess::operator<<(SqInitProcess &o){
	o.next = this;
	return o;
}
*/

/// Ordinary member function version of operator<<=(). It connects objects from right to left, which is the opposite
/// of the operator. The reason is that function calls in method chaining are usually left associative.
/// Of course you can call like <code>SqInit("file", A.chain(B.chain(C)))</code> to make it
/// right associative (sort of), but it would be a pain to keep the parentheses matched like LISP.
///
/// We just chose usual method chaining style (<code>SqInit("file", A.chain(B).chain(C))</code>), which is
/// more straightforward and maintainable. In exchange, passed objects are applied in the reverse order of
/// their appearance.
inline const SqInitProcess &SqInitProcess::chain(const SqInitProcess &o)const{
	o.next = this;
	return o;
}

#endif
