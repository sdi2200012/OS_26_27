/*
 * temp.c - test program for the IPC layer (src/ipc.c).
 *
 * Tests:
 *   1. ipc_create initialises shared memory (p, n, empty channels)
 *   2. channel_put / channel_get keep FIFO order and wrap around the ring
 *   3. ipc_attach in a child process sees the same shared memory
 *   4. blocking: a producer sending far more than CAP messages through a
 *      CAP-sized channel works with a concurrent consumer (semaphores)
 *   5. two-way flow on both lanes (k = 0,1): parent -> child -> parent
 *   6. ipc_destroy removes the shared memory (attach then fails)
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "header/common.h"
#include "header/errexit.h"

#define N_MSG 1000

static int failures = 0;
#define CHECK(cond, name) do { \
    if (cond) printf("[ OK ] %s\n", name); \
    else { printf("[FAIL] %s\n", name); failures++; } } while (0)

static void make_msg(msg_t *m, msg_type t, int i)
{
    m->type = t;
    snprintf(m->text, sizeof m->text, "message-%d", i);
}

/* Child: for lane k, read N_MSG msgs from p_to_enc[k], answer on enc_to_p[k]
 * with type ACK and text prefixed by "echo:". Exit code 0 = all good. */
static void child_main(int k)
{
    shared_t *s = ipc_attach();
    if (s == NULL)
        _exit(2);
    if (s->n != 7 || s->p != 0.25)
        _exit(3);

    for (int i = 0; i < N_MSG; i++) {
        msg_t in, out;
        channel_get(&s->p_to_enc[k], &in);
        char expect[MAX_LENGTH_TEXT];
        snprintf(expect, sizeof expect, "message-%d", i);
        if (in.type != MSG_DATA || strcmp(in.text, expect) != 0)
            _exit(4);
        out.type = MSG_ACK;
        snprintf(out.text, sizeof out.text, "echo:%.100s", in.text);
        channel_put(&s->enc_to_p[k], &out);
    }
    ipc_detach(s);
    _exit(0);
}

int main(void)
{
    /* 1. create */
    shared_t *s = ipc_create(0.25, 7);
    CHECK(s != NULL, "ipc_create returns a mapping");
    CHECK(s->p == 0.25 && s->n == 7, "p and n stored in shared memory");

    /* 2. FIFO + wraparound, single process (never exceed CAP in flight) */
    int fifo_ok = 1;
    int next_put = 0, next_get = 0;
    for (int round = 0; round < 5; round++) {      /* 5*5 = 25 msgs > CAP */
        for (int i = 0; i < 5; i++) {
            msg_t m; make_msg(&m, MSG_DATA, next_put++);
            channel_put(&s->p_to_enc[0], &m);
        }
        for (int i = 0; i < 5; i++) {
            msg_t m; char e[64];
            channel_get(&s->p_to_enc[0], &m);
            snprintf(e, sizeof e, "message-%d", next_get++);
            if (strcmp(m.text, e) != 0 || m.type != MSG_DATA)
                fifo_ok = 0;
        }
    }
    CHECK(fifo_ok, "FIFO order preserved across ring wraparound");
    CHECK(s->p_to_enc[0].in == s->p_to_enc[0].out, "channel empty again (in == out)");

    int v = -1;
    sem_getvalue(&s->p_to_enc[0].empty, &v);
    CHECK(v == CAP, "empty semaphore back to CAP");
    sem_getvalue(&s->p_to_enc[0].full, &v);
    CHECK(v == 0, "full semaphore back to 0");

    /* 3-5. two children (lane 0 and lane 1), parent feeds and checks echoes.
     * Parent sends/receives interleaved via a forked sender to avoid deadlock
     * is unnecessary: echo channel has CAP slots, so we alternate put/get. */
    pid_t pid[2];
    for (int k = 0; k < 2; k++) {
        pid[k] = fork();
        if (pid[k] < 0) die("fork");
        if (pid[k] == 0) child_main(k);
    }

    int echo_ok = 1;
    for (int i = 0; i < N_MSG; i++) {
        for (int k = 0; k < 2; k++) {
            msg_t m; make_msg(&m, MSG_DATA, i);
            channel_put(&s->p_to_enc[k], &m);
        }
        for (int k = 0; k < 2; k++) {
            msg_t r; char e[MAX_LENGTH_TEXT + 8];
            channel_get(&s->enc_to_p[k], &r);
            snprintf(e, sizeof e, "echo:message-%d", i);
            if (r.type != MSG_ACK || strcmp(r.text, e) != 0)
                echo_ok = 0;
        }
    }
    CHECK(echo_ok, "1000 msgs per lane echoed in order via child processes");

    for (int k = 0; k < 2; k++) {
        int st = 0;
        waitpid(pid[k], &st, 0);
        char name[64];
        snprintf(name, sizeof name, "child lane %d exited cleanly (status %d)",
                 k, WIFEXITED(st) ? WEXITSTATUS(st) : -1);
        CHECK(WIFEXITED(st) && WEXITSTATUS(st) == 0, name);
    }

    /* 6. destroy */
    ipc_destroy(s);
    CHECK(ipc_attach() == NULL, "after ipc_destroy, ipc_attach fails");

    printf("\n%s (%d failure%s)\n", failures ? "TESTS FAILED" : "ALL TESTS PASSED",
           failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
