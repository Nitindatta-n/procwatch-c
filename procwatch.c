#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum { TOP = 5 };
typedef struct { long pid; unsigned long kib; char name[128]; } Process;

static int load_status(const char *path, Process *p) {
    FILE *file = fopen(path, "r");
    if (!file) return 0; /* A process can exit or deny access between reads. */
    char line[1024];
    int have_rss = 0;
    while (fgets(line, sizeof line, file)) {
        if (strncmp(line, "Name:", 5) == 0) {
            char *start = line + 5;
            while (*start == ' ' || *start == '\t') ++start;
            size_t n = strcspn(start, "\r\n");
            if (n >= sizeof p->name) n = sizeof p->name - 1;
            for (size_t i = 0; i < n; ++i)
                p->name[i] = isprint((unsigned char)start[i]) ? start[i] : '?';
            p->name[n] = '\0';
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            char unit[8];
            have_rss = sscanf(line + 6, "%lu %7s", &p->kib, unit) == 2
                       && strcmp(unit, "kB") == 0;
        }
    }
    int ok = !ferror(file) && have_rss && p->name[0];
    fclose(file);
    return ok;
}

static int ranks_before(Process a, Process b) {
    return a.kib > b.kib || (a.kib == b.kib && a.pid < b.pid);
}

static int snapshot(const char *root) {
    DIR *dir = opendir(root);
    if (!dir) { perror(root); return 1; }
    Process best[TOP];
    size_t count = 0, skipped = 0;
    struct dirent *entry;
    for (;;) {
        errno = 0;
        entry = readdir(dir);
        if (!entry) break;
        const char *id = entry->d_name;
        if (!*id || id[strspn(id, "0123456789")]) continue;
        char path[4096];
        int length = snprintf(path, sizeof path, "%s/%s/status", root, id);
        Process p = {0};
        p.pid = strtol(id, NULL, 10);
        if (length < 0 || (size_t)length >= sizeof path || !load_status(path, &p)) {
            ++skipped;
            continue;
        }
        size_t pos = 0;
        while (pos < count && !ranks_before(p, best[pos])) ++pos;
        if (pos == TOP) continue;
        if (count < TOP) ++count;
        for (size_t i = count - 1; i > pos; --i) best[i] = best[i-1];
        best[pos] = p;
    }
    int read_error = errno;
    closedir(dir);
    if (read_error) { errno = read_error; perror("readdir"); return 1; }
    printf("%8s %12s  %s\n", "PID", "RSS (KiB)", "PROCESS");
    for (size_t i = 0; i < count; ++i)
        printf("%8ld %12lu  %s\n", best[i].pid, best[i].kib, best[i].name);
    printf("Skipped %zu entries without a readable name and RSS.\n", skipped);
    return fflush(stdout) == EOF;
}

int main(int argc, char **argv) {
    const char *root = "/proc";
    int once = 0;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--once")) once = 1;
        else if (!strcmp(argv[i], "--proc") && i+1 < argc) root = argv[++i];
        else { fprintf(stderr, "Usage: procwatch [--once] [--proc DIR]\n"); return 2; }
    }
    do {
        if (!once && isatty(STDOUT_FILENO)) fputs("\033[H\033[2J", stdout);
        if (snapshot(root)) return 1;
        if (!once) sleep(1);
    } while (!once);
    return 0;
}
