#ifndef GLTESTPLUS_DSTRING_H
#define GLTESTPLUS_DSTRING_H
/** \file
 * \brief Definition of dynamic string for gltestplus
 */
#include "export.h"

namespace gltestp{

	/// Dynamic string embedded in the executable's source itself.
	///
	/// We cannot export member functions of this class as DLL entry points if this
	/// class is defined in a static library.
	/// This silly issue is MSVC's.
	/// Since dynamic strings use heap memory to store variable length strings, we
	/// need to export the methods to avoid multiple memory managers.
	///
	/// The re-invention of wheels, copy-on-write string.
	/// Unicode strings cannot be handled.
	/// It does not pool the same string appears in the heap, unlike MFC CString.
	class EXPORT dstring{
	public:
		dstring();
		dstring(const char*);
		dstring(const dstring &);
		dstring(char);
		dstring(long);
		dstring(int);
		dstring(unsigned long);
		dstring(unsigned);
		dstring(unsigned long long);
		dstring(double);
		dstring(float);
		dstring operator=(const dstring &);
		long len()const;
		long strncpy(const char *src, unsigned long len);
		long strcpy(const char *src);
		dstring &strncat(const char *src, unsigned long len);
		dstring &strcat(const char *src);
		dstring &strcat(const dstring &);
		~dstring();
		operator const char *()const; // conversion to c-str
		dstring operator +(const dstring &o)const{return dstring(*this).strcat(o);} // returns newly created string with string passed by argument concatenated
		dstring &operator +=(const dstring &o){strcat(o); return *this;} // append to the string
		dstring &operator <<(const dstring &o){strcat(o); return *this;} // equivalent to += but more comfortable binding direction
		dstring &operator <<(const char *src){return strcat(src);} // optimize c-str concatenation
		dstring &operator <<(char); // optimize single character concatenation
		dstring &operator <<(int a){return operator <<(dstring(a));}
		dstring &operator <<(long a){return operator <<(dstring(a));}
		dstring &operator <<(unsigned a){return operator <<(dstring(a));}
		dstring &operator <<(unsigned long a){return operator<<(dstring(a));}
		dstring &operator <<(unsigned long long a){return operator<<(dstring(a));}
		dstring &operator <<(double a){return operator <<(dstring(a));}
		bool operator ==(const dstring &o)const;
		bool operator !=(const dstring &o)const{return !operator==(o);}
		bool operator ==(const char *o)const;
		bool operator !=(const char *o)const{return !operator==(o);}
		bool operator <(const dstring &o)const;
		friend bool operator ==(const char *a, const dstring &b){return b == a;}
		friend bool operator !=(const char *a, const dstring &b){return b != a;}
#ifdef _DEBUG
		static int cats;
		static int allocs;
		static int reallocs;
		static int frees;
#endif
	private:
		struct in{
			unsigned long size;
			unsigned refs;
			char s[1];
		} *p;
	};

	inline dstring::dstring() : p(0){}

	inline dstring::dstring(const dstring &ds) : p(ds.p){
		if(p)
			p->refs++;
	}

	inline dstring::dstring(char c) : p(0){
		char buf[2] = {c, '\0'};
		strcpy(buf);
	}

	inline dstring::dstring(int a){this->dstring::dstring(long(a));}
	inline dstring::dstring(unsigned a){this->dstring::dstring((unsigned long)(a));}
	inline dstring::dstring(unsigned long long a){this->dstring::dstring((unsigned long)(a));}
	inline dstring::dstring(float a){this->dstring::dstring(double(a));}

	inline dstring dstring::operator =(const dstring &ds){
		this->dstring::~dstring();
		this->dstring::dstring(ds);
		return *this;
	}

	// Optimized implementation of single character concatenation reduces unnecessary memory allocations to the bottom.
	inline dstring &dstring::operator <<(char c){
		return strncat(&c, 1);
	}

	inline long dstring::len()const{
		return p ? p->size : 0;
	}

	inline dstring::operator const char *()const{
		return p ? p->s : "";
	}
}



#endif
