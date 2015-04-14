CC=gcc -Wall
AR=ar
BIN=bin
INC=include
LIB=lib
OBJ=obj
SRC=src
LD_LIBRARY_PATH=${LIB}

CFLAGS=-O2 -I${INC}
#CPPFLAGS=
LDFLAGS=-Llib -lmyqueue

bin_=test1_myqueue test2_myqueue
obj_=
hdr_=
lib_=myqueue


hdr=$(patsubst %,${INC}/%,${hdr_})
lib=$(patsubst %,${LIB}/lib%.so,${lib_})
bin=$(patsubst %,${BIN}/%,${bin_})
obj=$(patsubst %,${OBJ}/%,${obj_})

runall: all run1 

all: directories ${lib} ${bin}

run1:
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH} bin/test1_myqueue

run2:
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH} bin/test2_myqueue

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

${LIB}/lib%.so: ${OBJ}/%.o
	${CC} -o $@ $^ -shared 

clean:
	rm -f ${OBJ}/* ${BIN}/* ${LIB}/*

cleanall:
	rm -rf ${OBJ} ${BIN} ${LIB}
	rm -f ${INC}/*~ ${SRC}/*~ *~

.PHONY: clean cleanall all
