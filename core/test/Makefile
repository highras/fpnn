dirs = AutoDistributedTest tcp-test udp-test TCP.vs.UDP UniversalFunctionalTests

all:
	for x in $(dirs); do (cd $$x; make -j4) || exit 1; done

clean:
	for x in $(dirs); do (cd $$x; make clean) || exit 1; done