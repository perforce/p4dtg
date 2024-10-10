#!/bin/bash
#
# Copyright 2005 Perforce Software.  All rights reserved.
#
if [ "$1" = "" ]; then
	echo "Usage: $0 <start|stop>"
	exit
fi

testport=3344
export P4USER=admin
export P4CLIENT=client1
export P4PORT=:$testport

if [ "$1" = "start" ]; then
	echo    "Starting p4d..."
	echo    " to kill it use the p4 command:"
	echo    "   p4 -usu -Ppw admin stop"

	#
	# Setup the depot root and copy the license for running the server`
	#
	mkdir depot.$$

	#if [ -d triggers/logs ]; then
		#echo "Trigger logs in triggers/logs"
	#else
		#mkdir triggers/logs
		#echo "Trigger logs in triggers/logs"
	#fi
	
	#
	# COPY LICENSE FILE
	#
	#cp /usr/perforce/build/main/p4-bin/license depot.$$
	cp license depot.$$
	
	#
	# Run the server
	#
	echo "Staring server with: p4d -vserver=1 -L$PWD/depot.$$/p4d.log -p $testport -r $PWD/depot.$$ &"
	p4d -vserver=1 -L$PWD/depot.$$/p4d.log -p $testport -r $PWD/depot.$$ &
	sleep 10 # wait to make sure server is running
	echo "running"
	p4 info
	
	
	#
	# Turn on the spec depot
	#
	p4 depot -i <<FOOBAR
Depot:  spec

Owner:  admin

Description:
	Created by admin.

Type:   spec

Address:        local

Suffix: .p4s

Map:    spec/...

FOOBAR

	#
	# Load the triggers table 
	#
	# p4 triggers -i < triggers.txt

	#
	# Load the jobspec and create sample users
	#
	p4 jobspec -i < jobspec.txt
	. p4user.setup one
	. p4user.setup two
	. p4user.setup three
	. p4user.setup four

elif [ "$1" = "stop" ]; then
	#
	# Stop the server
	#
	p4 -usu -Ppw admin stop
	sleep 5 # wait for it to stop
	echo "stopped"
	/bin/rm -rf depot.[0-9]* one two three four 
	/bin/rm -rf karen 
else
	echo "Usage: $0 <start|stop>"
fi
