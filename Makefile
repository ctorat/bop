CFILES = $(shell find core/ -name "*.c")
OFILES = $(CFILES:.c=.o)
DFILES = $(CFILES:.c=.d)

CC = clang
CFLAGS = -Wall -pedantic -Ihead

.PHONY: all
all: bop

.PHONY: bop
bop: $(OFILES)
	$(CC) -lssl -lcrypto $(OFILES) -o $@

-include $(DFILES)
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@
