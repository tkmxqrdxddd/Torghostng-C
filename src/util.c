#include "torghostng.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void log_message(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    char time_str[32];
    time_t now = time(NULL);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

    printf("[%s] ", time_str);
    vprintf(fmt, ap);
    printf("\n");

    va_end(ap);
}

void log_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    char time_str[32];
    time_t now = time(NULL);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(stderr, "[%s] ERROR: ", time_str);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");

    va_end(ap);
}

int run_command(const char *fmt, ...) {
    char cmd[4096];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(cmd, sizeof(cmd), fmt, ap);
    va_end(ap);

    int rc = system(cmd);
    if (rc == -1) {
        log_error("Failed to execute command: %s", cmd);
        return -1;
    }
    if (!WIFEXITED(rc)) {
        log_error("Command terminated by signal: %s", cmd);
        return -1;
    }

    int status = WEXITSTATUS(rc);
    if (status != 0) {
        log_error("Command failed (exit %d): %s", status, cmd);
    }
    return status;
}

char *read_file_alloc(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return NULL;
    }
    rewind(f);

    char *buf = malloc((size_t)size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t got = fread(buf, 1, (size_t)size, f);
    fclose(f);
    buf[got] = '\0';
    return buf;
}

int write_file(const char *path, const char *data) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        return -1;
    }

    size_t n = strlen(data);
    int rc = 0;
    if (fwrite(data, 1, n, f) != n) {
        rc = -1;
    }
    if (fclose(f) != 0) {
        rc = -1;
    }
    return rc;
}

int write_file_atomic(const char *path, const char *data) {
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s.torghostng.tmp", path);

    FILE *f = fopen(tmp, "wb");
    if (!f) {
        return -1;
    }

    size_t n = strlen(data);
    int rc = 0;
    if (fwrite(data, 1, n, f) != n) {
        rc = -1;
    }
    if (fclose(f) != 0) {
        rc = -1;
    }
    if (rc != 0) {
        unlink(tmp);
        return -1;
    }

    mode_t mode = 0644;
    struct stat st;
    if (stat(path, &st) == 0) {
        mode = st.st_mode & 07777;
    }
    chmod(tmp, mode);

    if (rename(tmp, path) != 0) {
        unlink(tmp);
        return -1;
    }
    return 0;
}
