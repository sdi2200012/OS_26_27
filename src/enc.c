#include "header/common.h"
#include "header/errexit.h"
#include <stdio.h>
#include <unistd.h>

void enc_run(int id, shared_t* s) { printf("ENC%d started (pid %d)\n", id, (int) getpid()); sleep(15); }