READPROGRAM = read2ch
FAVOPROGRAM = favo2ch
CC = gcc
LINKER = gcc
CFLAGS = -O2 -Wall
X = -lcurl -lpthread
RM = rm -rf

SRCS = un2ch.c unstring.c unarray.c crc32.c unmap.c
OBJS = $(SRCS:.c=.o)
READSRCS = read.c
READOBJS = $(READSRCS:.c=.o)
FAVOSRCS = read2.c
FAVOOBJS = $(FAVOSRCS:.c=.o)

.PHONY: all
all: $(READPROGRAM) $(FAVOPROGRAM)

$(READPROGRAM): $(READOBJS) $(OBJS)
	$(LINKER) $^ -o $(READPROGRAM) $(CFLAGS) $(X)

%.o : %.c %.h
	$(CC) -c $(CFLAGS) $*.c


$(FAVOPROGRAM): $(FAVOOBJS) $(OBJS)
	$(LINKER) $^ -o $(FAVOPROGRAM) $(CFLAGS) $(X)

%.o : %.c %.h
	$(CC) -c $(CFLAGS) $*.c


.PHONY: clean
clean:
	$(RM) $(READPROGRAM) $(FAVOPROGRAM) $(OBJS) $(READOBJS) $(FAVOOBJS)

