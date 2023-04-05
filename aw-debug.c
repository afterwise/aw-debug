/* vim: set ts=4 sw=4 noet : */
/*
   Copyright (c) 2014-2023 Malte Hildingsson, malte (at) afterwi.se

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

#ifndef _debug_nofeatures
# if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN 1
# elif defined(__linux__)
#  define _BSD_SOURCE 1
#  define _DEFAULT_SOURCE 1
#  define _POSIX_C_SOURCE 200809L
#  define _SVID_SOURCE 1
# elif defined(__APPLE__)
#  define _DARWIN_C_SOURCE 1
# endif
#endif /* _debug_nofeatures */

#include "aw-debug.h"

#if defined(_WIN32)
# include <windows.h>
# include <dbghelp.h>
# include <io.h>
#endif

#if defined(__CELLOS_LV2__)
# if defined(__PPU__)
#  include <sys/tty.h>
# elif defined(__SPU__)
#  include <spu_printf.h>
# endif
#elif defined(__ANDROID__)
# include <android/log.h>
#endif

#if defined(__APPLE__) && !defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__)
# include <sys/sysctl.h>
#endif

#if (defined(__linux__) && !defined(__ANDROID__)) || (defined(__APPLE__) && !defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__))
# include <execinfo.h>
# include <stdlib.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
# include <unistd.h>
#endif

#if defined(__linux__) && !defined(__ANDROID__)
# include <stdio_ext.h>
#endif

#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#define _debug_array_count(a) (sizeof (a) / sizeof (a)[0])
#define _debug_align(v,n) (((v) + (n) - 1) & ~((n) - 1))

const char *_debug_name = "aw-debug";

#if defined(DEBUG_ENABLE)

int debug_getchar(void) {
#if defined(_WIN32)
	FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
#elif defined(__linux__) && !defined(__ANDROID__)
	__fpurge(stdin);
#else
	fpurge(stdin);
#endif
	return getchar();
}

bool debug_isatty(void) {
#if defined(_WIN32)
	return !!_isatty(_fileno(stdin));
#else
	return !!isatty(fileno(stdin));
#endif
}

bool debug_attached(void) {
#if defined(_WIN32)
	return !!IsDebuggerPresent();
#elif defined(__APPLE__) && !defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__)
	struct kinfo_proc info;
	size_t n = sizeof info;
	int err, mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid()};

	if ((err = sysctl(mib, _debug_array_count(mib), &info, &n, NULL, 0)) < 0)
		return false;

	return !!(info.kp_proc.p_flag & P_TRACED);
#else
	return false;
#endif
}

static void output(const char *str, int len) {
	(void) len;

#if defined(_WIN32)
	if (IsDebuggerPresent())
		OutputDebugStringA(str);
	fputs(str, stderr);
# if defined(__MINGW32__)
	fflush(stderr);
# endif
#elif defined(__CELLOS_LV2__)
# if defined(__PPU__)
	unsigned res;
	sys_tty_write(SYS_TTYP3, str, len, &res);
# elif defined(__SPU__)
	spu_printf("%.*s\n", len, str);
# endif
#elif defined(__ANDROID__)
	__android_log_print(ANDROID_LOG_INFO, _debug_name, str);
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
#if defined(_WIN32)
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

#if defined(_WIN32)
		GetConsoleScreenBufferInfo(handle, &info);
		SetConsoleTextAttribute(handle, FOREGROUND_RED);
#elif (defined(__linux__) && !defined(__ANDROID__)) || (defined(__APPLE__) && !defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__))
		output("\033[0;31m", 7);
#endif

		output(buf, len);

#if defined(_WIN32)
		SetConsoleTextAttribute(handle, info.wAttributes);
#elif (defined(__linux__) && !defined(__ANDROID__)) || (defined(__APPLE__) && !defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__))
		output("\033[0m", 4);
#endif
	}

	va_end(ap);
}

void debug_hex(const void *p, size_t n) {
	size_t x, y, m;

	for (x = 0, m = n; x < _debug_align(m, 16u); x += 16, n -= 16) {
		char s[3 * 16 + 18];

		for (y = 0; y < _debug_array_count(s); ++y)
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
#if defined(_WIN32)
	HANDLE proc = GetCurrentProcess();
	void *trace[64];
	size_t i, n;
	union {
		SYMBOL_INFO sym;
		unsigned char buf[256];
	} u;

	SymInitialize(proc, NULL, TRUE);
	n = CaptureStackBackTrace(0, _debug_array_count(trace), trace, NULL);

	for (i = 0; i < n; ++i) {
		memset(&u.sym, 0, sizeof u.sym);
		u.sym.SizeOfStruct = sizeof u.sym;
		u.sym.MaxNameLen = (sizeof u.buf - sizeof u.sym) * sizeof(CHAR);

		if (SymFromAddr(proc, (uintptr_t) trace[i], NULL, &u.sym))
			debugf("%08p %s\n", trace[i], u.sym.Name);
	}

	SymCleanup(proc);
#elif (defined(__linux__) && !defined(__ANDROID__)) || (defined(__APPLE__) && !defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__))
	void *trace[64];
	size_t n;

	n = backtrace(trace, _debug_array_count(trace));
	backtrace_symbols_fd(trace, n, STDERR_FILENO);
#elif defined(__CELLOS_LV2__) && defined(__PPU__)
	uintptr_t trace[64];
	size_t i;
	unsigned n;

	if (_debug_unlikely(cellDbgPpuThreadCountStackFrames(&n) < 0))
		return;

	if (n > _debug_array_count(trace))
		n = _debug_array_count(trace);

	if (_debug_unlikely(cellDbgPpuThreadGetStackBackTrace(0, n, trace, NULL) < 0))
		return;

	for (i = 0; i < n; ++i)
		debugf("0x%08x", trace[i]);
#endif
}

#endif /* defined(DEBUG_ENABLE) */

