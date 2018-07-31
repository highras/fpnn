#!/bin/sh

LOG_FILE=/root/autoMassTest.log

TargetHost=172.31.15.228
TargetServer=${TargetHost}:13530
ControlCenter=172.31.9.119:13555

testIdx=0
FakeAction=""

if [ $# -gt 0 ]
then
	FakeAction="echo"
fi

# $1: thread count, $2: client count, $3: QPS, $4: extra wait time
function runTest()
{
	clientCount=`expr $2 / 10000`
	CSV_FILE=massClient.m4.xlarge.$1.${clientCount}w.$3.csv

	let testIdx=testIdx+1
	
	$FakeAction sleep 2m
	$FakeAction ./MassConnLoadWaiter $ControlCenter $TargetHost 0.02 7

	echo "Test $testIdx: Begin " `date` "Params: param: $1 thread, ${clientCount}w clients, per client QPS $3"  >> $LOG_FILE

	$FakeAction ./MassConnController --contorlCenterEndpoint $ControlCenter --testEndpoint $TargetServer --stopPerCPULoad 2.1 --stopTimeCostMsec 800 --massThreadCount $1 --clientCount $2 --perClientQPS $3 --output $CSV_FILE --extraWaitTime $4 --actor ../MassConnActor/FPNNMassiveConnectionsActor
	
	echo "Test $testIdx: Done " `date` "Saved file: $CSV_FILE"  >> $LOG_FILE
}

# $1: QPS
function runTestPlus()
{
	runTest 100 10000 $1 10
}

echo "" > $LOG_FILE

runTestPlus 0.01
runTestPlus 0.02
runTestPlus 0.04
runTestPlus 0.06
runTestPlus 0.08
runTestPlus 0.10
runTestPlus 0.20
runTestPlus 0.40
runTestPlus 0.60
runTestPlus 0.80
runTestPlus 1.00
runTestPlus 2.00
runTestPlus 4.00
runTestPlus 6.00
runTestPlus 8.00
runTestPlus 10.0
runTestPlus 20.0
runTestPlus 30.0