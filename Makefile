PROGRAM = read2ch
CC = gcc
LINKER = gcc
CFLAGS = -g -Wall
X = -lcurl
RM = rm -rf

SRCS = read.c un2ch.c unstring.c
OBJS = $(SRCS:.c=.o)

$(PROGRAM): $(OBJS)
	$(LINKER) $^ -o $(PROGRAM) $(CFLAGS) $(X)

%.o : %.c
	$(CC) -c $(CFLAGS) $*.c

%.o : %.h

.PHONY: clean
clean:
	$(RM) $(PROGRAM) $(OBJS)

