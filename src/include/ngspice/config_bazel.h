/* config_bazel.h - Configuration header for Bazel builds of ngspice */
#ifndef NGSPICE_CONFIG_BAZEL_H
#define NGSPICE_CONFIG_BAZEL_H

#define PACKAGE "ngspice"
#define PACKAGE_NAME "ngspice"
#define PACKAGE_TARNAME "ngspice"
#define PACKAGE_STRING "ngspice 47"
#define PACKAGE_VERSION "47"
#define VERSION "47"

#define SIMULATOR 1
#define XSPICE 1
#define CIDER 1
#define OSDI 1
#define KLU 1
#define RFSPICE 1
#define WITH_PSS 1
#define STDC_HEADERS 1
#define NOINTHELP 1
#define NGSPICEBUILDDATE "Bazel"
#define NGSPICEBINDIR "/usr/local/bin"
#define NGSPICEDATADIR "/usr/local/share/ngspice"
#define X_DISPLAY_MISSING 1

#if defined(_WIN32) || defined(_MSC_VER)

#define HAS_WINGUI 1
#define HAVE_ACCESS 1
#define HAVE_GETCWD 1
#define HAVE_STRING_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STDIO_H 1
#define HAVE_TIME_H 1
#define HAVE_CTYPE_H 1
#define HAVE_MATH_H 1
#define HAVE_FLOAT_H 1
#define HAVE_LIMITS_H 1
#define HAVE_STDINT_H 1
#define HAVE_SNPRINTF 1
#define HAVE_STRDUP 1
#define HAVE_STRERROR 1
#define HAVE_VPRINTF 1
#define HAVE_QSORT 1

#else /* POSIX / Linux / macOS */

#define OS_COMPILED 6
#define HAVE_ACCESS 1
#define HAVE_ACOSH 1
#define HAVE_ASINH 1
#define HAVE_ATANH 1
#define HAVE_ARPA_INET_H 1
#define HAVE_CLOCK_GETTIME 1
#define HAVE_CTYPE_H 1
#define HAVE_DECL_ISINF 1
#define HAVE_DECL_ISNAN 1
#define HAVE_DIRENT_H 1
#define HAVE_DIRNAME 1
#define HAVE_DLFCN_H 1
#define HAVE_DUP2 1
#define HAVE_ERFC 1
#define HAVE_FCNTL_H 1
#define HAVE_FLOAT_H 1
#define HAVE_FORK 1
#define HAVE_GETCWD 1
#define HAVE_GETOPT_H 1
#define HAVE_GETOPT_LONG 1
#define HAVE_GETRLIMIT 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_INTTYPES_H 1
#define HAVE_ISATTY 1
#define HAVE_LIBM 1
#define HAVE_LIBPTHREAD 1
#define HAVE_LIMITS_H 1
#define HAVE_LOCALTIME 1
#define HAVE_LOGB 1
#define HAVE_MEMSET 1
#define HAVE_MODF 1
#define HAVE_NETDB_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_POPEN 1
#define HAVE_PWD_H 1
#define HAVE_QSORT 1
#define HAVE_SCALB 1
#define HAVE_SCALBN 1
#define HAVE_SELECT 1
#define HAVE_SIGSETJMP 1
#define HAVE_SIG_T 1
#define HAVE_SNPRINTF 1
#define HAVE_SOCKET 1
#define HAVE_STDBOOL_H 1
#define HAVE_STDDEF_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRCHR 1
#define HAVE_STRDUP 1
#define HAVE_STRERROR 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_STRNCASECMP 1
#define HAVE_STRRCHR 1
#define HAVE_STRSTR 1
#define HAVE_STRTOL 1
#define HAVE_SYS_FILE_H 1
#define HAVE_SYS_IOCTL_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_SELECT_H 1
#define HAVE_SYS_SOCKET_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_WAIT_H 1
#define HAVE_TCGETATTR 1
#define HAVE_TCSETATTR 1
#define HAVE_TERMIOS_H 1
#define HAVE_TIME 1
#define HAVE_UNISTD_H 1
#define HAVE_UTIMES 1
#define HAVE_VFORK 1
#define HAVE_VPRINTF 1
#define HAVE_WORKING_FORK 1
#define HAVE_WORKING_VFORK 1
#define HAVE__BOOL 1
#define IPC_UNIX_SOCKETS 1
#define USE_OMP 1

#endif /* _WIN32 */

#endif /* NGSPICE_CONFIG_BAZEL_H */
