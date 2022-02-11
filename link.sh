#!/bin/bash
make Debug
if [ "$?" != "0" ]; then
	exit -1
fi

sudo ln -sf $(readlink -f .bin/Debug/lib*.so.*) /usr/lib64
if [ "$?" != "0" ]; then
	exit -1
fi

sudo ln -sf $(readlink -f .bin/Debug/udjat-module-*.so) /usr/lib64/udjat-modules
if [ "$?" != "0" ]; then
	exit -1
fi

