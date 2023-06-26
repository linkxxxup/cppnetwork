#!/bin/bash

set -e


rm -rf `pwd`/build
mkdir `pwd`/build
mkdir -p `pwd`/log
cd `pwd`/build &&
        cmake .. &&
        make
cd ../bin
#echo '--------start output---------'
#./exeserver
#./execlient

