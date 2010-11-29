#Makefile of LIT9
VERSION = 0.1

CC       = g++
LIBS	= `pkg-config --cflags --libs gtk+-2.0`
CFLAGS	= -c -Wall
LFLAGS   = -lX11 -lpthread 
PROGS    = lit9
PROGS_O  = lit9.o 
SOURCE = *.cpp

all:    progs

progs: $(SOURCE)
	$(CC) $(LFLAGS) -o $(PROGS) $(SOURCE) $(LIBS)

objs: $(SOURCE)
	$(CC) $(CFLAGS) $(LFLAGS) $(PROGS_0) $(SOURCE) $(LIBS)

clean:
	rm -f *.o
	rm -f $(PROGS)

