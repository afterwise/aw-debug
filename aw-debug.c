
/*
   Copyright (c) 2014 Malte Hildingsson, malte (at) afterwi.se

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
 */

#include "aw-debug.h"

#if __CELLOS_LV2__
# if __PPU__
#  include <sys/tty.h>
# elif __SPU__
#  include <spu_printf.h>
# endif
#elif __ANDROID__
# include <android/log.h>
#endif

#if __APPLE__ && !__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__
# include <sys/sysctl.h>
#endif

#if (__linux__ && !__ANDROID__) || (__APPLE__ && !__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__)
# include <execinfo.h>
# include <stdlib.h>
# include <unistd.h>
#endif

#if __linux__
# include <stdio_ext.h>
#endif

#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#define array_count(a) (sizeof (a) / sizeof (a)[0])
#define align(i,a) (((i) + (a) - 1) & ~((a) - 1))

const char *debug_name = "aw-debug";

#if !NDEBUG

int debug_getchar(void) {
#if _WIN32
	FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
#elif __linux__
	__fpurge(stdin);
#else
	fpurge(stdin);
#endif
	return getchar();
}

bool debug_isatty(void) {
#if _WIN32
	return !!_isatty(_fileno(stdin));
#else
	return !!isatty(fileno(stdin));
#endif
}

bool debug_attached(void) {
#if _WIN32
	return !!IsDebuggerPresent();
#elif __APPLE__ && !__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__
	struct kinfo_proc info;
	size_t n = sizeof info;
	int err, mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid()};

	if ((err = sysctl(mib, array_count(mib), &info, &n, NULL, 0)) < 0)
		return false;

	return !!(info.kp_proc.p_flag & P_TRACED);
#else
	return false;
#endif
}

static void output(const char *str, int len) {
	(void) len;

#if _WIN32
	if (IsDebuggerPresent())
		OutputDebugStringA(str);
	else
		fputs(str, stderr);
#elif __CELLOS_LV2__
# if __PPU__
	unsigned res;
	sys_tty_write(SYS_TTYP3, str, len, &res);
# elif __SPU__
	spu_printf("%.*s\n", len, str);
# endif
#elif __ANDROID__
	__android_log_print(ANDROID_LOG_INFO, debug_name, str);
#else
	fputs(str, stderr);
#endif
}

void debugf(const char *fmt, ...) {
	va_list ap;
	int len;
	char buf[256];

	va_start(ap, fmt);

	if ((len = vsnprintf(buf, sizeof(buf) - 1, fmt, ap)) > 0) {
		buf[len = ((int) sizeof buf - 2 < len) ? (int) sizeof buf - 2 : len] = '\n';
		buf[len += !!(buf[len - 1] - '\n')] = '\0';

		output(buf, len);
	}

	va_end(ap);
}

void errorf(const char *fmt, ...) {
#if _WIN32
	CONSOLE_SCREEN_BUFFER_INFO info;
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

	va_list ap;
	int len;
	char buf[256];

	va_start(ap, fmt);

	if ((len = vsnprintf(buf, sizeof(buf) - 1, fmt, ap)) > 0) {
		buf[len = ((int) sizeof buf - 2 < len) ? (int) sizeof buf - 2 : len] = '\n';
		buf[len += !!(buf[len - 1] - '\n')] = '\0';

#if _WIN32
		GetConsoleScreenBufferInfo(handle, &info);
		SetConsoleTextAttribute(handle, FOREGROUND_RED);
#elif (__linux__ && !__ANDROID__) || (__APPLE__ && !__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__)
		output("\033[0;31m", 7);
#endif

		output(buf, len);

#if _WIN32
		SetConsoleTextAttribute(handle, info.wAttributes);
#elif (__linux__ && !__ANDROID__) || (__APPLE__ && !__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__)
		output("\033[0m", 4);
#endif
	}

	va_end(ap);
}

void debug_hex(const void *p, size_t n) {
	size_t x, y, m;

	for (x = 0, m = n; x < align(m, 16u); x += 16, n -= 16) {
		char s[3 * 16 + 18];

		for (y = 0; y < array_count(s); ++y)
			s[y] = ' ';

		for (y = 0; y < ((n < 16ull) ? n : 16ull); ++y) {
			const unsigned char b = ((unsigned char *) p)[x + y];
			const unsigned char c = b >> 4, d = b & 0xf;

			s[y * 3 + 0] = (c < 0xa ? '0' : 'W') + c;
			s[y * 3 + 1] = (d < 0xa ? '0' : 'W') + d;
			s[3 * 16 + 0 + y] = isprint(b) ? b : '.';
			s[3 * 16 + 1 + y] = 0;
		}

		debugf("%p %s", (unsigned char *) p + x, s);
	}
}

void debug_trace(void) {
#if (__linux__ && !__ANDROID__) || (__APPLE__ && !__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__)
	void *trace[64];
	size_t n;

	n = backtrace(trace, array_count(trace));
	backtrace_symbols_fd(trace, n, STDERR_FILENO);
#elif __CELLOS_LV2__ && __PPU__
	uintptr_t trace[64];
	size_t i;
	unsigned n;

	if (_debug_unlikely(cellDbgPpuThreadCountStackFrames(&n) < 0))
		return;

	if (n > array_count(trace))
		n = array_count(trace);

	if (_debug_unlikely(cellDbgPpuThreadGetStackBackTrace(0, n, trace, NULL) < 0))
		return;

	for (i = 0; i < n; ++i)
		debugf("0x%08x", trace[i]);
#endif
}
 
#endif /* !NDEBUG */

