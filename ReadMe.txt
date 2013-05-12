========================================================================
    cpplib the static library
========================================================================

1. Overview

This library had originally project-independent codes, but imported into
VastSpace's subdirectory for project-specific customization.
This library contains mainly linear algebraic data types and logics like
vector, quaternion and matrix manipulation.


2. Usage

This library is intended to be used as a static library.  Making it a DLL
or shared library is not tested.

If you have Visual Studio, create a static library project.
Output .lib file can be linked to final product .exe or .dll.

If you have gcc, provided GNUmakefile will build .a file.


