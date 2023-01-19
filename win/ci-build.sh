#!/bin/bash
#
# References:
#
#	* https://www.msys2.org/docs/ci/
#
#
echo "Running ${0}"

LOGFILE=build.log
rm -f ${LOGFILE}

die ( ) {
	[ -s $LOGFILE ] && tail $LOGFILE
	[ "$1" ] && echo "$*"
	exit -1
}

myDIR=$(dirname $(dirname $(readlink -f ${0})))
cd ${myDIR}

#
# Build LIBUDJAT
#
echo "Building libudjat"
mkdir -p  ./.build/libubjdat
git clone https://github.com/PerryWerneck/libudjat.git ${myDIR}/.build/libudjat > $LOGFILE 2>&1 || die "clone libudjat failure"
pushd ${myDIR}/.build/libubjdat
./autogen.sh > $LOGFILE 2>&1 || die "Autogen failure"
./configure > $LOGFILE 2>&1 || die "Configure failure"
make clean > $LOGFILE 2>&1 || die "Make clean failure"
make all  > $LOGFILE 2>&1 || die "Make failure"
make install  > $LOGFILE 2>&1 || die "Install failure"
popd

#
# Build
#
./autogen.sh > $LOGFILE 2>&1 || die "Autogen failure"
./configure > $LOGFILE 2>&1 || die "Configure failure"
make clean > $LOGFILE 2>&1 || die "Make clean failure"
make all  > $LOGFILE 2>&1 || die "Make failure"

echo "Build complete"

