
/*
   Copyright (c) 2014-2021 Malte Hildingsson, malte (at) afterwi.se

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

#ifndef AW_DEBUG_H
#define AW_DEBUG_H

#if !_MSC_VER
# include <stdbool.h>
#endif

#include <sys/types.h>
#include <stdlib.h>

#if __GNUC__
# define _debug_format(a,b) __attribute__((format(__printf__, a, b)))
# define _debug_likely(x) __builtin_expect(!!(x), 1)
# define _debug_unlikely(x) __builtin_expect(!!(x), 0)
#else
# define _debug_format(a,b)
# define _debug_likely(x) (x)
# define _debug_unlikely(x) (x)
#endif

#define _debug_join(a,b) _debug_join_impl(a, b)
#define _debug_join_impl(a,b) a ## b
#define _debug_strize(a) _debug_strize_impl(a)
#define _debug_strize_impl(a) #a

#ifdef __cplusplus
extern "C" {
#endif

#if _WIN32 && _MSC_VER
# define breakpoint() do { \
		static unsigned char _debug_join(_bkpt__, __LINE__); \
		if (_debug_likely(!_debug_join(_bkpt__, __LINE__))) { \
			if (debug_isatty()) \
				debugf("-- Skip breakpoint? [y/a/N] --"); \
				switch(debug_getchar()) { \
				case 'A': case 'a':  _debug_join(_bkpt__, __LINE__) = 1; \
				case 'Y': case 'y':  goto _debug_join(_label__, __LINE__); \
			} \
			if (debug_attached()) __debugbreak(); \
			else { debug_trace(); abort(); } \
		} _debug_join(_label__, __LINE__): ((void) 0); \
	} while (0)
#elif _WIN32 && __GNUC__
# define breakpoint() do { \
		static unsigned char _debug_join(_bkpt__, __LINE__); \
		if (_debug_likely(!_debug_join(_bkpt__, __LINE__))) { \
			if (debug_isatty()) \
				debugf("-- Skip breakpoint? [y/a/N] --"); \
				switch(debug_getchar()) { \
				case 'A': case 'a':  _debug_join(_bkpt__, __LINE__) = 1; \
				case 'Y': case 'y':  goto _debug_join(_label__, __LINE__); \
			} \
			if (debug_attached()) __builtin_trap(); \
			else { debug_trace(); abort(); } \
		} _debug_join(_label__, __LINE__): ((void) 0); \
	} while (0)
#elif (__APPLE__ && !__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) || (__linux__ && !__ANDROID__)
# define breakpoint() do { \
		static unsigned char _debug_join(_bkpt__, __LINE__); \
		if (_debug_likely(!_debug_join(_bkpt__, __LINE__))) { \
			if (debug_isatty()) \
				debugf("-- Skip breakpoint? [y/a/N] --"); \
				switch(debug_getchar()) { \
				case 'A': case 'a':  _debug_join(_bkpt__, __LINE__) = 1; \
				case 'Y': case 'y':  goto _debug_join(_label__, __LINE__); \
			} \
			if (debug_attached()) __builtin_trap(); \
			else { debug_trace(); exit(1); } \
		} _debug_join(_label__, __LINE__): ((void) 0); \
	} while (0)
#elif __PPU__
# define breakpoint() do { __asm__ __volatile__ ("tw 31, 1, 1"); } while (0)
#elif __SPU__
# define breakpoint() do { __asm__ __volatile__ ("stopd 0, 1, 1"); } while (0)
#elif _MSC_VER
# define breakpoint() do { __debugbreak(); } while (0)
#elif __GNUC__
# define breakpoint() do { __builtin_trap(); } while (0)
#else
# define breakpoint() do { abort(); } while (0)
#endif

#if !NDEBUG
# define _check_impl(f,l,d) do { \
		errorf(f ":" _debug_strize(l) ": " d); \
		breakpoint(); \
	} while (0)
# define _checkf_impl(f,l,d,...) do { \
		errorf(f ":" _debug_strize(l) ": " d __VA_ARGS__); \
		breakpoint(); \
	} while (0)
# define check(_expr) do { \
		if (_debug_unlikely(!(_expr))) { \
			_check_impl( \
				__FILE__, __LINE__, \
				"check `" #_expr "' failed"); \
		} \
	} while (0)
# define checkf(_expr,...) do { \
		if (_debug_unlikely(!(_expr))) { \
			_checkf_impl( \
				__FILE__, __LINE__, \
				"check `" #_expr "' failed: ", __VA_ARGS__); \
		} \
	} while (0)
# define trespass() _check_impl(__FILE__, __LINE__, "should not be here")
# define trespassf(...) _checkf_impl(__FILE__, __LINE__, "should not be here: ", __VA_ARGS__)
#else
# define check(expr) ((void) 0)
# define checkf(expr,...) ((void) 0)
# define trespass() ((void) 0)
# define trespassf(...) ((void) 0)
#endif

#define _try(res) \
	ssize_t _debug_join(_try__, __LINE__) = (res); \
	if (_debug_unlikely(_debug_join(_try__, __LINE__) < 0)) \
		switch (_debug_join(_try__, __LINE__))

extern const char *debug_name;

#if !NDEBUG
int debug_getchar(void);
bool debug_isatty(void);
bool debug_attached(void);
void debugf(const char *fmt, ...) _debug_format(1, 2);
void errorf(const char *fmt, ...) _debug_format(1, 2);
void debug_hex(const void *p, size_t n);
void debug_trace(void);
#else
# define debug_getchar() (0)
# define debug_isatty() (0)
# define debug_attached() (0)
# define debugf(...) ((void) 0)
# define errorf(...) ((void) 0)
# define debug_hex(...) ((void) 0)
# define debug_trace() ((void) 0)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AW_DEBUG_H */

