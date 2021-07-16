#!/bin/sh

LOG_FILE=autoMassTest.log

TargetHost=10.13.240.152
TargetServer=${TargetHost}:13611
ControlCenter=10.13.240.67:13666

testIdx=0
FakeAction=""

if [ $# -gt 0 ]
then
	FakeAction="echo"
fi

# $1: thread count, $2: client count, $3: QPS, $4: extra wait time, $5: version
function runTest()
{
	clientCount=`expr $2 / 10000`
	CSV_FILE=udp.massClient.m5.2xlarge.$1.${clientCount}w.$3.v$5.csv

	let testIdx=testIdx+1
	
	if [ $testIdx -gt 1 ]
	then
		$FakeAction sleep 6m
	fi

	$FakeAction ./UDPMassConnLoadWaiter $ControlCenter $TargetHost 0.05 7

	echo "Test $testIdx: Begin " `date` "Params: param: $1 thread, ${clientCount}w clients, per client QPS $3, version $5"  >> $LOG_FILE

	$FakeAction ./UDPMassConnController --contorlCenterEndpoint $ControlCenter --testEndpoint $TargetServer --stopPerCPULoad 2.1 --stopTimeCostMsec 800 --massThreadCount $1 --clientCount $2 --perClientQPS $3 --output $CSV_FILE --extraWaitTime $4

	echo "Test $testIdx: Done " `date` "Saved file: $CSV_FILE"  >> $LOG_FILE
}

# $1: QPS
function runTestPlus()
{
	runTest 100 10000 $1 10 1
	runTest 100 10000 $1 10 2
#	runTest 100 10000 $1 10 3
}

echo "" > $LOG_FILE

#runTestPlus 0.01
#runTestPlus 0.02
runTestPlus 0.04
#runTestPlus 0.06
runTestPlus 0.08
runTestPlus 0.10
runTestPlus 0.20
#runTestPlus 0.40
runTestPlus 0.50
#runTestPlus 0.60
#runTestPlus 0.80
runTestPlus 1.00
#runTestPlus 2.00
#runTestPlus 4.00
runTestPlus 5.00
#runTestPlus 6.00
#runTestPlus 8.00
#runTestPlus 10.0
#runTestPlus 20.0
#runTestPlus 30.0
runTestPlus 50.0