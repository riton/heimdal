@BOTTOM@

#undef BINDIR 
#undef LIBDIR
#undef LIBEXECDIR
#undef SBINDIR

#undef HAVE_INT8_T
#undef HAVE_INT16_T
#undef HAVE_INT32_T
#undef HAVE_INT64_T
#undef HAVE_U_INT8_T
#undef HAVE_U_INT16_T
#undef HAVE_U_INT32_T
#undef HAVE_U_INT64_T

#ifdef HAVE_FOUR_VALUED_KRB_PUT_INT
#define KRB_PUT_INT(F, T, L, S) krb_put_int((F), (T), (L), (S))
#else
#define KRB_PUT_INT(F, T, L, S) krb_put_int((F), (T), (L))
#endif

#ifdef BROKEN_REALLOC
#define realloc(X, Y) isoc_realloc((X), (Y))
#define isoc_realloc(X, Y) ((X) ? realloc((X), (Y)) : malloc(Y))
#endif

#ifdef VOID_RETSIGTYPE
#define SIGRETURN(x) return
#else
#define SIGRETURN(x) return (RETSIGTYPE)(x)
#endif

#define RCSID(msg) \
static /**/const char *const rcsid[] = { (char *)rcsid, "\100(#)" msg }

#undef PROTOTYPES

/* Maximum values on all known systems */
#define MaxHostNameLen (64+4)
#define MaxPathLen (1024+4)

#if defined(HAVE_SGTTY_H) && defined(__NeXT__)
#define SGTTY
#endif

/*
 * Define NDBM if you are using the 4.3 ndbm library (which is part of
 * libc).  If not defined, 4.2 dbm will be assumed.
 */
#if defined(HAVE_DBM_FIRSTKEY)
#define NDBM
#endif

/* telnet stuff ----------------------------------------------- */

#if defined(ENCRYPTION) && !defined(AUTHENTICATION)
#define AUTHENTICATION 1
#endif

/* Set this to the default system lead string for telnetd 
 * can contain %-escapes: %s=sysname, %m=machine, %r=os-release
 * %v=os-version, %t=tty, %h=hostname, %d=date and time
 */
#undef USE_IM

/* Used with login -p */
#undef LOGIN_ARGS

/* set this to a sensible login */
#ifndef LOGIN_PATH
#define LOGIN_PATH BINDIR "/login"
#endif
