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
mkdir -p  ${myDIR}/.build/libudjat
git clone https://github.com/PerryWerneck/libudjat.git ${myDIR}/.build/libudjat > $LOGFILE 2>&1 || die "clone libudjat failure"
pushd ${myDIR}/.build/libudjat
echo "Buindi module"
./autogen.sh || die "Autogen failure"
./configure || die "Configure failure"
make clean || die "Make clean failure"
make all || die "Make failure"
make install || die "Install failure"
popd

#
# Build
#
echo "Buindi module"
./autogen.sh || die "Autogen failure"
./configure || die "Configure failure"
make clean || die "Make clean failure"
make all || die "Make failure"

echo "Build complete"

