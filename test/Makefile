
export CFLAGS += -Wall -Wextra

test: test.o ../libaw-debug.a
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -I.. -c $< -o $@

../libaw-debug.a:
	$(MAKE) -C..

.PHONY: clean
clean:
	rm -f test test.o

