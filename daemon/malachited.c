#define LOG_TAG "malachited"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/swap.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cutils/sockets.h>
#include <log/log.h>

#define ZRAM_DEV "/dev/block/zram0"
#define ZRAM_SYS "/sys/block/zram0"

static const char *const kWritable[] = {
    "/sys/devices/system/cpu/cpufreq/policy*/scaling_max_freq",
    "/sys/devices/system/cpu/cpufreq/policy*/scaling_governor",
    "/sys/devices/system/cpu/cpu*/online",
    "/proc/gpufreqv2/fix_target_opp_index",
    "/sys/kernel/ged/hal/custom_upbound_gpu_freq",
    "/sys/devices/platform/goodix_ts.0/goodix_ts_report_rate",
    "/sys/devices/platform/goodix_ts.0/goodix_ts_reset",
    "/sys/class/thermal/thermal_message/sconfig",
    "/sys/class/smartpa/re25_calib",
    "/sys/class/smartpa/f0_calib",
    "/proc/sys/vm/swappiness",
    NULL};

static const char *const kReadable[] = {
    "/sys/devices/system/cpu/cpufreq/policy*/*",
    "/sys/devices/system/cpu/cpu*/online",
    "/proc/gpufreqv2/fix_target_opp_index",
    "/sys/kernel/ged/hal/custom_upbound_gpu_freq",
    "/sys/kernel/ged/hal/current_freqency",
    "/sys/devices/platform/goodix_ts.0/goodix_ts_report_rate",
    "/sys/class/thermal/thermal_message/sconfig",
    "/sys/class/thermal/thermal_zone*/type",
    "/sys/class/thermal/thermal_zone*/temp",
    "/sys/class/smartpa/*",
    "/sys/block/zram0/*",
    "/proc/sys/vm/swappiness",
    "/proc/swaps",
    NULL};

static const char *const kListable[] = {
    "/sys/devices/system/cpu",
    "/sys/devices/system/cpu/cpufreq",
    "/sys/class/thermal",
    NULL};

static int allowed(const char *path, const char *const *list) {
    if (path[0] != '/' || strstr(path, "..") != NULL) return 0;
    for (; *list != NULL; list++) {
        if (fnmatch(*list, path, FNM_PATHNAME) == 0) return 1;
    }
    return 0;
}

static int valid_value(const char *v) {
    size_t n = strlen(v);
    if (n == 0 || n > 64) return 0;
    for (; *v; v++) {
        unsigned char ch = (unsigned char)*v;
        if (!(isalnum(ch) || ch == '.' || ch == '_' || ch == '-' || ch == ' ')) return 0;
    }
    return 1;
}

static void reply_n(int fd, const char *s, size_t n) {
    while (n > 0) {
        ssize_t w = write(fd, s, n);
        if (w <= 0) return;
        s += w;
        n -= (size_t)w;
    }
}

static void reply(int fd, const char *s) { reply_n(fd, s, strlen(s)); }

static void reply_err(int fd, const char *what, int err) {
    char b[128];
    snprintf(b, sizeof(b), "err %s: %s\n", what, strerror(err));
    reply(fd, b);
}

static int write_node(const char *path, const char *value) {
    int fd = open(path, O_WRONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) return -errno;
    char buf[96];
    int n = snprintf(buf, sizeof(buf), "%s\n", value);
    ssize_t w = write(fd, buf, (size_t)n);
    int err = (w < 0) ? -errno : 0;
    close(fd);
    return err;
}

static void do_read(int c, const char *path) {
    if (!allowed(path, kReadable)) { reply(c, "err denied\n"); return; }
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) { reply_err(c, "open", errno); return; }
    reply(c, "ok\n");
    char buf[4096];
    size_t total = 0;
    ssize_t n;
    while (total < 65536 && (n = read(fd, buf, sizeof(buf))) > 0) {
        reply_n(c, buf, (size_t)n);
        total += (size_t)n;
    }
    close(fd);
}

static void do_list(int c, const char *path) {
    if (!allowed(path, kListable)) { reply(c, "err denied\n"); return; }
    DIR *d = opendir(path);
    if (d == NULL) { reply_err(c, "opendir", errno); return; }
    reply(c, "ok\n");
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        reply(c, e->d_name);
        reply(c, "\n");
    }
    closedir(d);
}

static void do_write(int c, char *path, const char *value) {
    if (!allowed(path, kWritable)) { reply(c, "err denied\n"); return; }
    if (!valid_value(value)) { reply(c, "err bad value\n"); return; }
    int e = write_node(path, value);
    if (e < 0) { reply_err(c, "write", -e); return; }
    reply(c, "ok\n");
}

static int run_mkswap(void) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        execl("/system/bin/mkswap", "mkswap", ZRAM_DEV, (char *)NULL);
        _exit(127);
    }
    int st = 0;
    if (waitpid(pid, &st, 0) < 0) return -1;
    return (WIFEXITED(st) && WEXITSTATUS(st) == 0) ? 0 : -1;
}

static void do_zram(int c, const char *size_str, const char *algo) {
    char *end = NULL;
    unsigned long long size = strtoull(size_str, &end, 10);
    if (size_str[0] == '\0' || *end != '\0' || size > (16ULL << 30)) {
        reply(c, "err bad size\n");
        return;
    }
    if (swapoff(ZRAM_DEV) < 0 && errno != EINVAL) { reply_err(c, "swapoff", errno); return; }
    int e = write_node(ZRAM_SYS "/reset", "1");
    if (e < 0) { reply_err(c, "reset", -e); return; }
    if (strcmp(algo, "-") != 0) {
        size_t n = strlen(algo);
        if (n == 0 || n > 16) { reply(c, "err bad algo\n"); return; }
        for (size_t i = 0; i < n; i++) {
            unsigned char ch = (unsigned char)algo[i];
            if (!(islower(ch) || isdigit(ch) || ch == '-')) { reply(c, "err bad algo\n"); return; }
        }
        e = write_node(ZRAM_SYS "/comp_algorithm", algo);
        if (e < 0) { reply_err(c, "algo", -e); return; }
    }
    if (size == 0) { reply(c, "ok\n"); return; }
    char v[32];
    snprintf(v, sizeof(v), "%llu", size);
    e = write_node(ZRAM_SYS "/disksize", v);
    if (e < 0) { reply_err(c, "disksize", -e); return; }
    if (run_mkswap() < 0) { reply(c, "err mkswap failed\n"); return; }
    if (swapon(ZRAM_DEV, 0) < 0) { reply_err(c, "swapon", errno); return; }
    reply(c, "ok\n");
}

#define GF_NODE "/sys/devices/virtual/mi_display/disp_feature/disp-DSI-0/disp_param"

static int gf_fix(void) {
    struct stat st;
    if (stat(GF_NODE, &st) == 0 && st.st_uid == 1000 && st.st_gid == 1000 && (st.st_mode & 0777) == 0664) return 0;
    if (chown(GF_NODE, 1000, 1000) < 0) return -errno;
    if (chmod(GF_NODE, 0664) < 0) return -errno;
    return 0;
}

static time_t gf_watch_until;

/* The display driver resets this node to root:0644 on panel changes, including mid-test.
 * Fix it now, then keep re-applying it every 0.5 s for 3 minutes in a child process. */
static void do_gfprep(int c) {
    int e = gf_fix();
    if (e < 0) { reply_err(c, "gfprep", -e); return; }
    time_t now = time(NULL);
    if (now >= gf_watch_until) {
        pid_t pid = fork();
        if (pid == 0) {
            close(c);
            for (int i = 0; i < 360; i++) { gf_fix(); usleep(500000); }
            _exit(0);
        }
        if (pid > 0) gf_watch_until = now + 180;
    }
    reply(c, "ok\n");
}

static void handle(int c) {
    struct timeval tv = {2, 0};
    setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    char line[320];
    size_t len = 0;
    while (len < sizeof(line) - 1) {
        ssize_t n = read(c, line + len, 1);
        if (n <= 0) break;
        if (line[len] == '\n') break;
        len++;
    }
    line[len] = '\0';
    if (len > 0 && line[len - 1] == '\r') line[len - 1] = '\0';

    if (strcmp(line, "ping") == 0) {
        reply(c, "ok\n");
    } else if (strncmp(line, "r ", 2) == 0) {
        do_read(c, line + 2);
    } else if (strncmp(line, "l ", 2) == 0) {
        do_list(c, line + 2);
    } else if (strncmp(line, "w ", 2) == 0) {
        char *path = line + 2;
        char *sp = strchr(path, ' ');
        if (sp == NULL) { reply(c, "err usage\n"); return; }
        *sp = '\0';
        do_write(c, path, sp + 1);
    } else if (strncmp(line, "zram ", 5) == 0) {
        char *sz = line + 5;
        char *sp = strchr(sz, ' ');
        if (sp == NULL) { reply(c, "err usage\n"); return; }
        *sp = '\0';
        do_zram(c, sz, sp + 1);
    } else if (strcmp(line, "gfprep") == 0) {
        do_gfprep(c);
    } else {
        reply(c, "err unknown\n");
    }
}

int main(void) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);  /* reap the gfprep watcher automatically */
    int s = android_get_control_socket("malachited");
    if (s < 0) {
        ALOGE("no control socket: %s", strerror(errno));
        return 1;
    }
    if (listen(s, 4) < 0) {
        ALOGE("listen failed: %s", strerror(errno));
        return 1;
    }
    ALOGI("ready");
    for (;;) {
        int c = accept4(s, NULL, NULL, SOCK_CLOEXEC);
        if (c < 0) {
            if (errno != EINTR) { ALOGE("accept: %s", strerror(errno)); sleep(1); }
            continue;
        }
        handle(c);
        close(c);
    }
}
