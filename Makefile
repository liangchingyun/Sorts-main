CC = gcc
CFLAGS = -O2

all: main

mainOBJS := main.o list_sort.o shiverssort.o \
        timsort.o inplace_timsort.o
measureOBJS := measure.o list_sort.o shiverssort.o \
		timsort.o inplace_timsort.o

deps := $(OBJS:%.o=.%.o.d)

main: $(mainOBJS)
	$(CC) -o $@ $(LDFLAGS) $^

%.o: %.c
	$(CC) -o $@ $(CFLAGS) -c -MMD -MF .$@.d $<

test: main
	@./main

measure: $(measureOBJS)
	$(MAKE) main
	$(CC) -o $@ $(CFLAGS) $^

clean:
	rm -f $(mainOBJS) $(deps) *~ main
	rm -f $(measureOBJS) $(deps) *~ main
	rm -rf *.dSYM

-include $(deps)
