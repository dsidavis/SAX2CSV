XML2_CONFIG=xml2-config
CFLAGS=-g $(shell $(XML2_CONFIG) --cflags)
LIBS=$(shell $(XML2_CONFIG) --libs)

sax2csv: sax2csv.o
	$(CC) -o $@ $^ $(LIBS)

#%.o: %.c
