/*
 * Bgmi.c
 *
 * Safe TCP load tester (rate-limited).
 *
 * Usage:
 *   gcc -O2 Bgmi.c -o Bgmi -lpthread
 *   ./Bgmi <ip> <port> <threads> <rps_total> <duration_seconds>
 *
 * Example:
 *   ./Bgmi 127.0.0.1 8080 4 100 30
 *     -> 4 threads, ~100 msg/sec total, run 30s
 *
 * IMPORTANT: Only run against servers you own or are authorized to test.
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <errno.h>
#include <stdatomic.h>

typedef struct {
    char ip[64];
    int port;
    int id;
    double rps;
    long duration;
    atomic_long *sent;
    atomic_long *success;
    atomic_long *fail;
    atomic_int *stop;
} worker_t;

static double now_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void sleep_seconds(double sec) {
    if (sec <= 0) return;
    struct timespec req;
    req.tv_sec = (time_t)sec;
    req.tv_nsec = (long)((sec - req.tv_sec) * 1e9);
    nanosleep(&req, NULL);
}

void *worker(void *arg) {
    worker_t *w = (worker_t*)arg;
    double interval = (w->rps > 0) ? 1.0 / w->rps : 0.0;
    double end = now_seconds() + w->duration;

    while (!atomic_load(w->stop) && now_seconds() < end) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) { perror("socket"); atomic_fetch_add(w->fail,1); continue; }

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(w->port);
        inet_pton(AF_INET, w->ip, &addr.sin_addr);

        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            atomic_fetch_add(w->fail,1);
            close(sock);
        } else {
            const char *msg = "PING\n";
            if (send(sock, msg, strlen(msg), 0) > 0) {
                char buf[64];
                recv(sock, buf, sizeof(buf), 0); // ignore result
                atomic_fetch_add(w->success,1);
            } else {
                atomic_fetch_add(w->fail,1);
            }
            close(sock);
        }
        atomic_fetch_add(w->sent,1);

        if (interval > 0) sleep_seconds(interval);
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <ip> <port> <threads> <rps_total> <duration>\n", argv[0]);
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int threads = atoi(argv[3]);
    double rps_total = atof(argv[4]);
    long duration = atol(argv[5]);

    if (threads <= 0 || port <= 0 || duration <= 0) {
        fprintf(stderr, "Invalid args.\n");
        return 1;
    }

    double rps_thread = (rps_total > 0) ? rps_total / threads : 0;

    atomic_long sent = 0, success = 0, fail = 0;
    atomic_int stop = 0;

    pthread_t *tids = calloc(threads, sizeof(pthread_t));
    worker_t *workers = calloc(threads, sizeof(worker_t));

    printf("Target: %s:%d, threads=%d, total RPS=%.1f, duration=%lds\n",
           ip, port, threads, rps_total, duration);

    for (int i=0;i<threads;i++) {
        strncpy(workers[i].ip, ip, sizeof(workers[i].ip));
        workers[i].port = port;
        workers[i].id = i;
        workers[i].rps = rps_thread;
        workers[i].duration = duration;
        workers[i].sent = &sent;
        workers[i].success = &success;
        workers[i].fail = &fail;
        workers[i].stop = &stop;
        pthread_create(&tids[i], NULL, worker, &workers[i]);
    }

    double start = now_seconds();
    double next_report = start + 1.0;
    double end = start + duration;

    while (now_seconds() < end) {
        double t = now_seconds();
        if (t >= next_report) {
            long s = atomic_load(&success);
            long f = atomic_load(&fail);
            long total = atomic_load(&sent);
            double elapsed = t - start;
            double rps = (elapsed > 0)? total/elapsed : 0;
            printf("[%.0fs] sent=%ld ok=%ld fail=%ld rps=%.1f\n", elapsed, total, s, f, rps);
            next_report += 1.0;
        }
        sleep_seconds(0.1);
    }

    atomic_store(&stop, 1);
    for (int i=0;i<threads;i++) pthread_join(tids[i], NULL);

    double elapsed = now_seconds() - start;
    printf("=== Summary ===\n");
    printf("Sent: %ld, Success: %ld, Fail: %ld, Avg RPS: %.1f\n",
           (long)atomic_load(&sent), (long)atomic_load(&success),
           (long)atomic_load(&fail), atomic_load(&sent)/elapsed);

    free(tids);
    free(workers);
    return 0;
}
