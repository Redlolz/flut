CFLAGS=-std=c11 -g -Wall -Wextra -pedantic
DEPS=flut.o lexer.o parser.o vm.o
BINNAME=flut

all: $(BINNAME)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(BINNAME): $(DEPS)
	$(CC) -o $@ $(DEPS) $(CFLAGS)

.PHONY: vm-test parser-test clean

vm-test: vm.o vm.h vm-test.o
	$(CC) -o $@ vm.o vm-test.o $(CFLAGS)

parser-test: parser.o parser.h parser-test.o
	$(CC) -o $@ parser.o parser-test.o $(CFLAGS)

clean:
	$(RM) $(BINNAME) vm-test parser-test *.o
