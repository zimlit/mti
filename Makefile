CFLAGS = -Wall -Wextra #-Werror

mti: main.o chunk.o memory.o debug.o value.o vm.o compiler.o scanner.o
	cc $(CFLAGS) -o mti main.o chunk.o memory.o debug.o \
		value.o vm.o compiler.o scanner.o

main.o: main.c common.h chunk.h vm.h
	cc $(CFLAGS) -c main.c

chunk.o: chunk.c common.h memory.h value.h
	cc $(CFLAGS) -c chunk.c

memory.o: memory.c memory.h
	cc $(CFLAGS) -c memory.c

debug.o: debug.c debug.h chunk.h value.h
	cc $(CFLAGS) -c debug.c

value.o: value.c value.h common.h
	cc $(CFLAGS) -c value.c

vm.o: vm.c vm.h chunk.h debug.h value.h
	cc $(CFLAGS) -c vm.c

compiler.o: compiler.c compiler.h common.h scanner.h
	cc $(CFLAGS) -c compiler.c

scanner.o: scanner.c scanner.h common.h
