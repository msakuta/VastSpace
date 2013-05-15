@rem First, checkout the working tree from a tag in the Subversion repository.
svn checkout http://bullet.googlecode.com/svn/tags/bullet-2.81 bullet

pushd bullet\build

@rem We want to use double precision
premake4  --with-double-precision vs2008

@rem We really need vs2012.
premake4  --with-double-precision vs2010

popd
