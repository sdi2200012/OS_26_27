#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "header/common.h"
#include "header/errexit.h"
#include <string.h>
#include <errno.h>
#include <signal.h>

void report(const char* name, pid_t pid , int status){
    printf("Parent: child %s with pid %d and raw status = 0x%04x.\n", name,(int) pid,(unsigned) status);
    printf("  WIFEXITED   = %d\n", WIFEXITED(status));
    printf("  WIFSIGNALED = %d\n", WIFSIGNALED(status));
    if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        printf("  WTERMSIG    = %d (%s)\n", sig, strsignal(sig));
    }
}


int main(int argc, char* argv[]){
    //Usage ./start -p -n 
    
    if (argc != 5 || strcmp(argv[1], "-p") != 0 || strcmp(argv[3], "-n") != 0){
        fprintf(stderr, "Usage: %s -p <probability> -n <tries>\n"
            " -p real number in [0,1]\n"
            " -n integer >= 1\n", argv[0]);
        return 1;
    }

    char* end;
    errno = 0;

    double p = strtod(argv[2], &end);
    if (end == argv[2] || *end != '\0' || errno == ERANGE || p < 0.0 || p > 1.0){
        fprintf(stderr, "Usage: %s -p <probability> -n <tries>\n"
            " -p real number in [0,1]\n"
            " -n integer >= 1\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[4]);
    if (n < 1){
        fprintf(stderr, "Usage: %s -p <probability> -n <tries>\n"
            " -p real number in [0,1]\n"
            " -n integer >= 1\n", argv[0]);
        return 1;
    }

    printf("The p is %f and the n is %d\n", p, n);


    //ipc initialize 
    shared_t * s = ipc_create(p,n);
    fflush(stdout);

    //fork the enc1,2 chan childs 
    pid_t pid_enc1, pid_enc2, pid_chan; 
    pid_enc1 = fork();
    if (pid_enc1 < 0){
        ipc_destroy(s);
        die("fork");
    }
    if (pid_enc1 == 0){
        enc_run(1, s);
        exit(0);
    }

    pid_enc2 = fork();
    if (pid_enc2 < 0){
        printf("Killing previous child enc1.\n");
        if (kill(pid_enc1, SIGTERM) < 0){
            ipc_destroy(s);
            die("kill");
        }
        waitpid(pid_enc1, NULL, 0);
        ipc_destroy(s);
        die("fork");
    }
    if (pid_enc2 == 0){
        enc_run(2, s);
        exit(0);
    }

    pid_chan = fork();
    if(pid_chan < 0){
        printf("Killing previous children enc1, enc2.\n");
        if (kill(pid_enc1, SIGTERM) < 0){
            ipc_destroy(s);
            die("kill");
        }
        if (kill(pid_enc2, SIGTERM) < 0){
            ipc_destroy(s);
            die("kill");
        }
        waitpid(pid_enc1, NULL, 0);
        waitpid(pid_enc2, NULL, 0);
        ipc_destroy(s);
        die("fork");
    }

    if (pid_chan == 0){
        chan_run(s);
        exit(0);
    }
    int status;
    if (waitpid(pid_enc1, &status, 0 ) < 0) die("waitpid");
    report("ENC1", pid_enc1, status);

    if (waitpid(pid_enc2, &status, 0 ) < 0) die("waitpid");
    report("ENC2", pid_enc2, status);

    if (waitpid(pid_chan, &status, 0 ) < 0) die("waitpid");
    report("CHAN", pid_chan, status);
    
    
    
    //when TERM 
    //ipc destroy
    ipc_destroy(s);
    return 0;
}