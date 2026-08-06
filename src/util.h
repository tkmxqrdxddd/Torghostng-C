#ifndef TORGHOSTNG_UTIL_H
#define TORGHOSTNG_UTIL_H

void log_message(const char *fmt, ...);
void log_error(const char *fmt, ...);

int run_command(const char *fmt, ...);
char *read_file_alloc(const char *path);
int write_file(const char *path, const char *data);
int write_file_atomic(const char *path, const char *data);

#endif /* TORGHOSTNG_UTIL_H */
