CC=gcc -Wall -ansi
LDFLAGS=-Llib
AR=ar
BIN=bin
INC=include
LIB=lib
OBJ=obj
SRC=src

CFLAGS=-DSTACK_SIZE=14 -I${INC}

bin_=
obj_=
hdr_=list.h stack.h fifo.h


hdr=$(patsubst %,${INC}/%,${hdr_})
bin=$(patsubst %,${BIN}/%,${bin_})
obj=$(patsubst %,${OBJ}/%,${obj_})

all: directories

directories: ${OBJ} ${BIN} ${LIB}

${OBJ}:
	mkdir ${OBJ}
${BIN}:
	mkdir ${BIN}
${LIB}:
	mkdir ${LIB}

${OBJ}/%.o: ${SRC}/%.c ${hdr}
	${CC} -c -o $@ $<  ${CFLAGS}

###### binary rules ######

###### library rules ######



clean:
	rm -f ${OBJ}/* ${BIN}/* ${LIB}/*

cleanall:
	rm -rf ${OBJ} ${BIN} ${LIB}
	rm -f ${INC}/*~ ${SRC}/*~ *~

.PHONY: clean cleanall all
