#!/bin/bash

#
# Test runner for the PubSubClient library
#
#  - Uses ino tool - http://inotool.org/
#  - Verifies all of the sketches in the examples directory
#    compile cleanly
#


DIR=`dirname $0`
cd $DIR

# Check we can find the library to test
if [ ! -d ../PubSubClient ]
then
   echo "Cannot find PubSubClient library"
   exit 1
fi

# Check ino tool is installed
which ino > /dev/null
if [ $? == 1 ]
then
   echo "Cannot find ino tool"
   exit 1
fi

# Create tmp workspace and logs dir
[ -d tmpbin ] && rm -rf tmpbin
mkdir tmpbin
[ -d logs ] && rm -rf logs
mkdir logs

cd tmpbin

# Initialise the ino workspace
ino init 

# Remove the default sketch
rm src/sketch.ino

printf -v pad '%0.1s' "."{1..50}

# Copy in the library
cp -R ../../PubSubClient lib/

PASS=0
FAIL=0

echo -e "\e[01;36mExamples\e[00m"

for f in ../../PubSubClient/examples/*
do
   e=`basename $f`
   echo -ne " \e[01;33m$e\e[00m"
   printf ' %*.*s %s' 0 $((${#pad} - ${#e})) $pad

   cp $f/$e.ino ./src/
   ino build >../logs/$e.log 2>../logs/$e.err.log
   if [ $? == 0 ]
   then
      echo -e "\e[00;32mPASS\e[00m"
      PASS=$(($PASS+1))
   else
      echo -e "\e[00;31mFAIL\e[00m"
      cat ../logs/$e.err.log
      FAIL=$(($FAIL+1))
   fi
   rm ./src/*
   ino clean
done

exit $FAIL 
