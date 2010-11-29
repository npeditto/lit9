#Makefile of LIT9
VERSION = 0.1

CC       = g++
LIBS	= `pkg-config --cflags --libs gtk+-2.0`
LFLAGS   = -lX11 -lpthread 
PROGS    = lit9
PROGS_O  = lit9.o 

all:    progs

progs:
	$(CC) *.cpp -o $(PROGS) $(LFLAGS) $(LIBS)

clean:
	rm -f *.o
	rm -f $(PROGS)

#dep             :
#        rm -f .depend
#        make .depend

# DO NOT DELETE

