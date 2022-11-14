#!/bin/bash
make Debug
if [ "$?" != "0" ]; then
	exit -1
fi

LIBDIR=$(pkg-config --variable libdir libudjat)
MODULEDIR=$(pkg-config --variable module_path libudjat)

sudo ln -sf $(readlink -f .bin/Debug/lib*.so.*) ${LIBDIR}
if [ "$?" != "0" ]; then
	exit -1
fi

mkdir -p ${MODULEDIR}
sudo ln -sf $(readlink -f .bin/Debug/udjat-module-*.so) ${MODULEDIR}
if [ "$?" != "0" ]; then
	exit -1
fi

cat > .bin/sdk.pc << EOF
prefix=/usr
exec_prefix=/usr
libdir=$(readlink -f .bin/Debug)
includedir=$(readlink -f ./src/include)

Name: udjat-module-users
Description: UDJAT User Monitor library
Version: 1.0
Libs: -L$(readlink -f .bin/Debug) -ludjatusers
Libs.private: -ludjat -lsystemd
Cflags: -I$(readlink -f ./src/include)

EOF

sudo ln -sf $(readlink -f .bin/sdk.pc) /usr/lib64/pkgconfig/udjat-users.pc

