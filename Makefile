CC      = gcc
CFLAGS  = -Wall -Wextra -pthread
TARGETS = main ul krolowa pszczelarz

.PHONY: all clean

all: $(TARGETS)

main: main.c
	$(CC) $(CFLAGS) -o $@ $^

ul: ul.c
	$(CC) $(CFLAGS) -o $@ $^

krolowa: krolowa.c
	$(CC) $(CFLAGS) -o $@ $^

pszczelarz: pszczelarz.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGETS)
