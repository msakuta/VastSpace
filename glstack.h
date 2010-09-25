#ifndef GLSTACK_H
#define GLSTACK_H
/// \file 
/// Defines classes to be used on the stack (automatic variable) for OpenGL state stack.
/// These objects ensure glPush*() and glPop*() to be called in pairs.
/// You must use curly end bracket to pop the stack.
///
/// This is one interesting example of multiple inheritance that majority of other
/// so-called object-oriented programming languages than C++ cannot run.
/// But in practice, too many nested brackets decreases readability, so cases that
/// these classes worth using is limited.


/// glPushAttrib() / glPopAttrib()
class GLattrib{
public:
	GLattrib(GLbitfield mask){
		glPushAttrib(mask);
	}
	~GLattrib(){
		glPopAttrib();
	}
};

/// glPushMatrix() / glPopMatrix() with unspecified matrix mode.
class GLmatrix{
public:
	GLmatrix(){
		glPushMatrix();
	}
	~GLmatrix(){
		glPopMatrix();
	}
};

/// glPushMatrix() / glPopMatrix() with projection matrix mode.
class GLpmatrix{
	GLdouble mat[16];
public:
	GLpmatrix(){
		glMatrixMode(GL_PROJECTION);
		glGetDoublev(GL_PROJECTION_MATRIX, mat);
//		glPushMatrix();
		glMatrixMode(GL_MODELVIEW);
	}
	~GLpmatrix(){
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixd(mat);
//		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	}
};

// Attributes and matrix.
class GLma : public GLmatrix, public GLattrib{
public:
	GLma(GLbitfield mask) : GLmatrix(), GLattrib(mask){}
	~GLma(){}
};

/// Modelview and projection
class GLpmmatrix : public GLmatrix, public GLpmatrix{
public:
	GLpmmatrix(){}
	~GLpmmatrix(){}
};

#endif
