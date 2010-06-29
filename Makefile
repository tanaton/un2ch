PROGRAM = read2ch
CC = gcc
LINKER = gcc
CFLAGS = -O2 -Wall
X = -lcurl -lpthread -ltcmalloc
RM = rm -rf

SRCS = read.c un2ch.c unstring.c unarray.c crc32.c unmap.c
OBJS = $(SRCS:.c=.o)

$(PROGRAM): $(OBJS)
	$(LINKER) $^ -o $(PROGRAM) $(CFLAGS) $(X)

%.o : %.c %.h
	$(CC) -c $(CFLAGS) $*.c

.PHONY: clean
clean:
	$(RM) $(PROGRAM) $(OBJS)

