CC=gcc -Wall -ansi
AR=ar
BIN=bin
INC=include
LIB=lib
OBJ=obj
SRC=src

CFLAGS=-I${INC}
#CPPFLAGS=
LDFLAGS=-Llib

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

${OBJ}/%.o: ${SRC}/%.c
	${CC} -c -o $@ $<  ${CFLAGS} ${CPPFLAGS}

###### binary rules ######

###### library rules ######



clean:
	rm -f ${OBJ}/* ${BIN}/* ${LIB}/*

cleanall:
	rm -rf ${OBJ} ${BIN} ${LIB}
	rm -f ${INC}/*~ ${SRC}/*~ *~

.PHONY: clean cleanall all
