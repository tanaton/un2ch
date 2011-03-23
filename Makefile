READPROGRAM = read2ch
READ2PROGRAM = read2ch_db
FAVOPROGRAM = favo2ch
NOFAVOPROGRAM = nofavo2ch
CC = gcc
LINKER = gcc
CFLAGS = -O2 -Wall -I/usr/local/mysql/include/ -I/opt/local/include/
X = -lcurl -lpthread -lunmap -lunstring -lunarray
RM = rm -rf

SRCS = un2ch.c
OBJS = $(SRCS:.c=.o)
READSRCS = read.c
READOBJS = $(READSRCS:.c=.o)
READ2SRCS = read2.c
READ2OBJS = $(READ2SRCS:.c=.o)
FAVOSRCS = favo.c
FAVOOBJS = $(FAVOSRCS:.c=.o)
NOFAVOSRCS = nofavo.c
NOFAVOOBJS = $(NOFAVOSRCS:.c=.o)

.PHONY: all
all: $(READPROGRAM) $(READ2PROGRAM) $(FAVOPROGRAM) $(NOFAVOPROGRAM)

$(READPROGRAM): $(READOBJS) $(OBJS)
	$(LINKER) $^ -o $(READPROGRAM) $(CFLAGS) $(X)

%.o : %.c %.h
	$(CC) -c $(CFLAGS) $*.c


$(READ2PROGRAM): $(READ2OBJS) $(OBJS)
	$(LINKER) $^ -o $(READ2PROGRAM) $(CFLAGS) $(X) -L/usr/local/mysql/lib/ -L/opt/local/lib/ -lmysqlclient -lonig

%.o : %.c %.h
	$(CC) -c $(CFLAGS) $*.c


$(FAVOPROGRAM): $(FAVOOBJS) $(OBJS)
	$(LINKER) $^ -o $(FAVOPROGRAM) $(CFLAGS) $(X)

%.o : %.c %.h
	$(CC) -c $(CFLAGS) $*.c


$(NOFAVOPROGRAM): $(NOFAVOOBJS) $(OBJS)
	$(LINKER) $^ -o $(NOFAVOPROGRAM) $(CFLAGS) $(X)

%.o : %.c %.h
	$(CC) -c $(CFLAGS) $*.c


.PHONY: clean
clean:
	$(RM) $(READPROGRAM) $(READ2PROGRAM) $(FAVOPROGRAM) $(NOFAVOPROGRAM) $(OBJS) $(READOBJS) $(FAVOOBJS) $(NOFAVOOBJS)

