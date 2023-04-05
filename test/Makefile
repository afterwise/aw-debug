
export CFLAGS += -std=c99 -Wall -Wextra

ifeq ($(shell uname -s),Linux)
export LDFLAGS += -rdynamic
endif

test: test.o ../libaw-debug.a
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.x
	$(CC) $(CFLAGS) -I.. -xc -c $< -o $@

../libaw-debug.a:
	$(MAKE) -C..

.PHONY: clean
clean:
	rm -f test test.o

