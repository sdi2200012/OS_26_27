#include "header/common.h"
#include "header/errexit.h"
#include <stdio.h>
#include <unistd.h>

void chan_run(shared_t* s) { printf("CHAN started (pid %d)\n", (int) getpid()); sleep(15); }