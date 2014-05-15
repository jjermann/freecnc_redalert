#ifndef _PORTABLE_SNPRINTF_H_
#define _PORTABLE_SNPRINTF_H_

#define PORTABLE_SNPRINTF_VERSION_MAJOR 2
#define PORTABLE_SNPRINTF_VERSION_MINOR 2

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdarg.h>

#ifdef HAVE_SNPRINTF
#include <stdio.h>
#else
extern int snprintf(char *, size_t, const char *, /*args*/ ...) throw();
extern int vsnprintf(char *, size_t, const char *, va_list ap) throw();
#endif

#if defined(HAVE_SNPRINTF) && defined(PREFER_PORTABLE_SNPRINTF)
extern int portable_snprintf(char *str, size_t str_m, const char *fmt, /*args*/ ...) throw();
extern int portable_vsnprintf(char *str, size_t str_m, const char *fmt, va_list ap) throw();
#define snprintf  portable_snprintf
#define vsnprintf portable_vsnprintf
#endif

extern int asprintf  (char **ptr, const char *fmt, /*args*/ ...) throw();
extern int vasprintf (char **ptr, const char *fmt, va_list ap) throw();
extern int asnprintf (char **ptr, size_t str_m, const char *fmt, /*args*/ ...) throw();
extern int vasnprintf(char **ptr, size_t str_m, const char *fmt, va_list ap) throw();

#ifdef __cplusplus
}
#endif

#endif
