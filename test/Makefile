
export CFLAGS += -std=c99 -Wall -Wextra

ifeq ($(shell uname -s),Linux)
export LDFLAGS += -rdynamic
endif

test: test.o ../libaw-debug.a
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -I.. -c $< -o $@

../libaw-debug.a:
	$(MAKE) -C..

.PHONY: clean
clean:
	rm -f test test.o

