
#include "aw-debug.h"

void bar() {
	debugf("I am here:");
	debug_trace();
}

void foo() {
	bar();
}

int var = 1;

int main(int argc, char *argv[]) {
	(void) argc;
	(void) argv;

	_debug_name = "test";

	debugf("Hello world");
	debugf("This is %s", debug_isatty() ? "live" : "only a recording");
	debugf("I am %s to you", debug_attached() ? "attached" : "not attached");

	foo();

	debugf("These are my innermost:");
	debug_hex(&main, 64);

	check(var > 0);

	errorf("I am to blame");

	return 0;
}

