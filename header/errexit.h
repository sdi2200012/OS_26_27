#ifndef ERREXIT_H
#define ERREXIT_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

#endif