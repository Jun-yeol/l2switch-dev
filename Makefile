CC=gcc
CFLAGS=-g
TARGET:test.exe
LIBS=-lpthread -L ./CommandParser -lcli
OBJS=gluethread/glthread.o	\
		graph.o				\
		topologies.o		\
		net.o				\
		comm.o				\
		Layer2/layer2.o		\
		Layer2/l2switch.o	\
		utils.o				\
		nwcli.o				\

test.exe:testapp.o ${OBJS} CommandParser/libcli.a
	${CC} ${CFLAGS}	testapp.o ${OBJS} -o test.exe ${LIBS} 

gluethread/glthread.o:gluethread/glthread.c
	${CC} ${CFLAGS} -c -I gluethread gluethread/glthread.c -o gluethread/glthread.o
graph.o:graph.c
	${CC} ${CFLAGS} -c -I . graph.c -o graph.o
topologies.o:topologies.c
	${CC} ${CFLAGS} -c -I . topologies.c -o topologies.o
net.o:net.c
	${CC} ${CFLAGS} -c -I . net.c -o net.o
comm.o:comm.c
	${CC} ${CFLAGS} -c -I . comm.c -o comm.o
Layer2/layer2.o:Layer2/layer2.c
	${CC} ${CFLAGS} -c -I . Layer2/layer2.c -o Layer2/layer2.o
Layer2/l2switch.o:Layer2/l2switch.c
	${CC} ${CFLAGS} -c -I . Layer2/l2switch.c -o Layer2/l2switch.o
nwcli.o:nwcli.c
	${CC} ${CFLAGS} -c -I . nwcli.c -o nwcli.o
utils.o:utils.c
	${CC} ${CFLAGS} -c -I . utils.c -o utils.o
CommandParser/libcli.a:
	(cd CommandParser; make)

clean:
	rm *.o
	rm Layer2/*.o
	rm gluethread/glthread.o
	rm *exe

all:
	make
	(cd CommandParser; make)
