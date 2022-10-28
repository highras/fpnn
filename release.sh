#!/bin/sh

TARGET_DIR="../infra-fpnn-release"
DIRS="base proto core extends"

make clean
sync
make

mkdir -p ${TARGET_DIR}
for x in ${DIRS}
do
	rm -rf ${TARGET_DIR}/$x
	cp -rf $x ${TARGET_DIR}/$x
	rm -f ${TARGET_DIR}/$x/*.c
	rm -f ${TARGET_DIR}/$x/*.cpp
	rm -f ${TARGET_DIR}/$x/*.o
	rm -f ${TARGET_DIR}/$x/Makefile
done

cp -f conf.template def.mk ${TARGET_DIR}
rm -f ${TARGET_DIR}/core/micro-ecc/*.o
