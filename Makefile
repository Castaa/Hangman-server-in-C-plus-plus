#IDIR =../include
IDIR =tinyXML/

CC=g++
CFLAGS=-I$(IDIR) -g -std=c++0x -Wall

#ODIR=obj
ODIR=.

#LDIR =../lib
LDIR=.

#LIBS=-lstdc++
LIBS=

#_DEPS = server.h
#DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))
DEPS=

XMLD = tinyXML/

XMLOBS = $(XMLD)tinyxmlparser.o $(XMLD)tinyxmlerror.o $(XMLD)tinyxml.o $(XMLD)tinystr.o

_OBJ = server.o HiScores.o kbhit.o LoadWordFile.o 
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(XMLD)%.o: %.cpp $(DEPS)
	$(CC) -c $< -o $@ $< $(CFLAGS)

$(ODIR)/%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

hserver: $(OBJ) $(XMLOBS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f $(ODIR)/*.o $(XMLD)*.o *~ core $(IDIR)*~ 

.PHONY: clean
