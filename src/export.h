#ifndef PARLIB_EXPORT_H
#define PARLIB_EXPORT_H

#ifdef __GNUC__
# define EXPORT_SYMBOL __attribute__((visibility("default")))
# define TLS_INITIAL_EXEC __attribute__((tls_model("initial-exec")))
# define EXPORT_ALIAS(name, aliasname) EXPORT_ALIAS_INTERNAL(name, aliasname)
# define EXPORT_ALIAS_INTERNAL(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((weak, alias (#name))) EXPORT_SYMBOL;
#else
# define EXPORT_SYMBOL
# define TLS_INITIAL_EXEC
#endif

#endif
