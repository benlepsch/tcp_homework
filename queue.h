#pragma once

#include "simulator.h"
#define BUFSIZE 1000

typedef struct Queue {
    int length = 0;
    struct pkt buf[BUFSIZE];
} colorado;


void enqueue(struct pkt p, colorado *q);
struct pkt peek(colorado *q);
struct pkt pop(colorado *q);