
export CFLAGS += -Wall -Wextra

test: test.o ../libaw-debug.a
	clang $(LDFLAGS) -o $@ $^

%.o: %.c
	clang $(CFLAGS) -I.. -c $< -o $@

../libaw-debug.a:
	$(MAKE) -C..

.PHONY: clean
clean:
	rm -f test test.o

