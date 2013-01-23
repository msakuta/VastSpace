#include "TIN.h"
#if _WIN32
#define exit something_meanless
#undef exit
#include <windows.h>
#endif
#include <gl/GL.h>
#include <fstream>

TIN::TIN(const char *fname){
	std::ifstream is(fname);
	do{
		char buf[256];
		is.getline(buf, sizeof buf);
		int xyz[3];
		if(3 <= sscanf(buf, "%d %d %d", &xyz[0], &xyz[1], &xyz[2])){
			vertices.push_back(Vec3i(xyz).cast<double>());
		}
	} while(!is.eof());
}

double TIN::width(){
	return 10.;
}

void TIN::draw(){
	glPushMatrix();
	glScaled(10./1201, 10./1201, 1./1201);
	glBegin(GL_LINES);
	for(int i = 0; i < vertices.size(); i++)
		glVertex3dv(vertices[i]);
	glEnd();
	glPopMatrix();
}

TIN::~TIN(){
}
