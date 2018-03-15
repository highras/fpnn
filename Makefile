dirs = base proto core extends

all:
	for x in $(dirs); do (cd $$x; make -j4) || exit 1; done

test:
	for x in $(dirs); do (cd $$x; make -C test) || exit 1; done

clean:
	for x in $(dirs); do (cd $$x; make clean) || exit 1; done

install:
	make install

deploy:
	make deploy
