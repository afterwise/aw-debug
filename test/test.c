
#include "aw-debug.h"

void bar() {
	debugf("I am here:");
	debug_trace();
}

void foo() {
	bar();
}

int i_will_fail() {
	return -1;
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

	_try(i_will_fail())
		default: errorf("function did fail");

	for (int i = 0; i < 5; ++i) {
		errorf("I am to blame");
		trespass();
	}

	return 0;
}

