
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

	debug_name = "test";

	debugf("Hello world");
	debugf("I am %s to you", debug_attached() ? "attached" : "not attached");

	foo();

	debug_hex(&main, 64);

	check(var > 0);

	errorf("I am to blame");
	trespassf();

	return 0;
}

