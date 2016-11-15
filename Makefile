CC=gcc
LD=gcc
CFLAGS=-g -Wall
CPPFLAGS=-I. -I/home/cs437/exercises/ex3/include
SP_LIBRARY_DIR=/home/cs437/exercises/ex3

all: m_user

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

m_user:  $(SP_LIBRARY_DIR)/libspread-core.a m_user.o
	$(LD) -o $@ m_user.o  $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a

class_user:  $(SP_LIBRARY_DIR)/libspread-core.a class_user.o
	$(LD) -o $@ class_user.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a

clean:
	rm -f *.o m_user
