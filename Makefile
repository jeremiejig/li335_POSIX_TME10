CC=gcc -Wall
AR=ar
BIN=bin
INC=include
LIB=lib
OBJ=obj
SRC=src

CFLAGS=-O2 -I${INC}
#CPPFLAGS=
LDFLAGS=-Llib

bin_=
obj_=
hdr_=


hdr=$(patsubst %,${INC}/%,${hdr_})
bin=$(patsubst %,${BIN}/%,${bin_})
obj=$(patsubst %,${OBJ}/%,${obj_})

all: directories ${bin}

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

${BIN}/%: ${OBJ}/%.o
	${CC} -o $@ $^  ${LDFLAGS}

###### library rules ######



clean:
	rm -f ${OBJ}/* ${BIN}/* ${LIB}/*

cleanall:
	rm -rf ${OBJ} ${BIN} ${LIB}
	rm -f ${INC}/*~ ${SRC}/*~ *~

.PHONY: clean cleanall all
