#include "cpplib/dstring.h"
#include <stdio.h>

int main(void){
#ifdef _DEBUG
	int &cc = cpplib::dstring::allocs;
	int &cr = cpplib::dstring::reallocs;
	int &cd = cpplib::dstring::frees;
#else
	int cc = 0, cr = 0, cd = 0;
#endif
	cpplib::dstring td = "a string";
	printf("%d:%d:%d [%d]%s\n", cc, cr, cd, td.len(), static_cast<const char*>(td));
	td << " and a number " << 23;
	printf("%d:%d:%d [%d]%s\n", cc, cr, cd, td.len(), static_cast<const char*>(td));
	td = "reassigned";
	printf("%d:%d:%d [%d]%s\n", cc, cr, cd, td.len(), static_cast<const char*>(td));
	cpplib::dstring td2 = td;
	printf("%d:%d:%d [%d]%s\n", cc, cr, cd, td2.len(), static_cast<const char*>(td2));
	td2 << " and what's wrong?";
	printf("%d:%d:%d [%d]%s\n", cc, cr, cd, td2.len(), static_cast<const char*>(td2));

	return 0;
}

