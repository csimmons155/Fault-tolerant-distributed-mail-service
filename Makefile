CC=gcc
LD=gcc
CFLAGS=-g -Wall
CPPFLAGS=-I. -I/home/cs437/exercises/ex3/include
SP_LIBRARY_DIR=/home/cs437/exercises/ex3

all: m_user m_server mutils.o

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

m_user:  $(SP_LIBRARY_DIR)/libspread-core.a m_user.o
	$(LD) -o $@ m_user.o  $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a

m_server:  $(SP_LIBRARY_DIR)/libspread-core.a m_server.o mutils.o
	$(LD) -o $@ m_server.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a mutils.o

m_utils.o: mutils.c mutils.h
	$(CC) -c -o mutils.o mutils.c

clean:
	rm -f *.o m_user m_server
