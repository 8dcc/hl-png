
#ifndef UTIL_H_
#define UTIL_H_ 1

/* Wrapper for `die_func' */
#define DIE(...) die_func(__func__, __VA_ARGS__)

/* Print program name, function name and error message; and exit. */
void die_func(const char* func, const char* fmt, ...);

#endif /* UTIL_H_ */
