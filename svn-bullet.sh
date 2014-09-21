#!/bin/sh
# First, checkout the working tree from a tag in the Subversion repository.
svn checkout http://bullet.googlecode.com/svn/tags/bullet-2.81 bullet

pushd bullet

# We want to use double precision
cmake . -DUSE_DOUBLE_PRECISION=ON -DBUILD_EXTRAS=off -DBUILD_DEMOS=off -DBUILD_SHARED_LIBS=on -G "Unix Makefiles"

popd

