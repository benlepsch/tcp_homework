/******************************************************************************/
/*                                                                            */
/* ENTITY IMPLEMENTATIONS                                                     */
/*                                                                            */
/******************************************************************************/

// Student names: Asher Saunders
// Student computing IDs: aas9x
//

#include <stdio.h>
#include "simulator.h"
#include <limits.h>
#include <stdlib.h>

void printp(struct pkt p)
{
    printf("\tSYN: %d\n\tACK: %d\n\tLEN: %d\n", p.seqnum, p.acknum, p.length);
}

int checksum(struct pkt packet) 
{
    int sum, i, l;

    if(packet.length > 20) {
        l = 20;
    }
    else {
        l = packet.length;
    }
    sum = packet.seqnum;
    sum += packet.acknum;
    sum += packet.length;
    for (i = 0; i < l; i++)
        sum += packet.payload[i];

    return sum;
}

struct pkt message_to_packet(struct msg message, int seqnum, int acknum)
{
    struct pkt p;
    p.seqnum = seqnum;
    p.acknum = acknum;
    if(message.length > 20) {
        p.length = 20;
    }
    else {
        p.length = message.length;
    }
    int i;
    for (i = 0; i < p.length; i++)
        p.payload[i] = message.data[i];

    p.checksum = checksum(p);
    return p;
}

struct msg packet_to_message(struct pkt packet)
{
    struct msg m;
    m.length = packet.length;
    if(m.length > 20) {
        m.length = 20;
    }
    int i;
    for (i = 0; i < m.length; i++) 
        m.data[i] = packet.payload[i];
    
    return m;
}


typedef struct queue {
  struct pkt *buffer;
  int front;
  int rear;
  int size;
  unsigned capacity;
} queue;
queue* makeQueue(unsigned c) {
    struct queue* q = (queue*)malloc(
        sizeof(queue *));
    q->capacity = c;
    q->front = q->size = 0; 
    q->rear = q->capacity -1;
    q->buffer = (struct pkt *)malloc(q->capacity * sizeof(struct pkt));
    return q;
}
int full(queue *q ) {
    return(q->size == q->capacity);
}
int isempty(queue *q) {
    return (q->size == 0);
}
void enqueue(queue *q, struct pkt p) {
    if(full(q)) {
        printf("FULL!!%i %i", q->size, q->capacity);
        
        return;
    }
    q->rear = (q->rear + 1)%q->capacity;
    (q->buffer[q->rear]) = p;
    q->size = q->size + 1;
}
void printq(queue *q) {
    int i;
    int start = q->front;
    for(i = 0; i < q->size; i++) {
        printp(q->buffer[(i+start)%q->capacity]);
    }
}
struct pkt dequeue(queue *q) {
    struct pkt p;
    if(isempty(q)) {
        p.seqnum = -1;
        p.acknum = -1;
        p.length = -1;
        p.checksum = -1;
    }
    else {
        p = (q->buffer[q->front]);
        q->front = (q->front + 1)% q->capacity;
        q->size = q->size -1;
    }
    return p;

}
struct pkt peek(queue *q) {
    struct pkt p;
    if(isempty(q)) {
        p.seqnum = -1;
        p.acknum = -1;
        p.length = -1;
        p.checksum = -1;
    }
    else {
        p = (q->buffer[q->front]);
        
    }
    return p;
}

#define A_TIMER_LEN 2000.0
#define BUFSIZE 1000

int A_seqnum, A_acknum, A_receievedseqnumfromB, A_receivedacknumfromB;
queue * A_buffer;

int loops;
void A_init() 
{
    loops = 0;
    A_seqnum = 1;
    A_acknum = 1;
    A_receievedseqnumfromB = 1;
    A_receivedacknumfromB = 0;
    A_buffer = makeQueue(BUFSIZE);
    starttimer_A(A_TIMER_LEN);
}

void A_output(struct msg message) 
{
    struct pkt p = message_to_packet(message, A_seqnum, A_acknum);
    enqueue(A_buffer,p);
    tolayer3_A(p);
    A_seqnum += p.length;
}

void A_input(struct pkt packet) 
{
    if(checksum(packet) != packet.checksum) {
        return;
    }
    if(packet.acknum > A_seqnum + 1) {
        printf("packet had acknum %i which is greater than our seqnum + 1: %i\n", packet.acknum, A_seqnum + 1);
        return;
    }
    if (packet.seqnum != A_receievedseqnumfromB){
        printf("A doesn't want this packet %i\n", packet.seqnum);
        return;
    }
    while(!isempty(A_buffer) && peek(A_buffer).seqnum < packet.acknum) {
        dequeue(A_buffer);
    }
    A_acknum = packet.seqnum + packet.length;
    A_receievedseqnumfromB = A_acknum;
    if(packet.acknum > A_receivedacknumfromB ) {
        
        A_receivedacknumfromB = packet.acknum;
        printf("Updated A_receivedacknumfromB to %i\n", A_receivedacknumfromB);
    }
    stoptimer_A();
    starttimer_A(A_TIMER_LEN);
}

void A_timerinterrupt() 
{
    int i;
    int start = A_buffer->front;
    for(i = 0; i < A_buffer->size; i++) {
        struct pkt p;
        p = A_buffer->buffer[(i+start)%A_buffer->capacity];
        if(p.seqnum >= A_receivedacknumfromB) {
            tolayer3_A(p);
        }
    }
    
    if(A_buffer->size != 0) {
        loops = loops + 1;
        starttimer_A(A_TIMER_LEN);
    }

}

#define B_TIMER_LEN 1000.0
int B_seqnum, B_acknum, B_receievedseqnumfromA,timer_count;

void B_init() 
{
    timer_count = 0;
    B_receievedseqnumfromA = 1;
    B_seqnum = 1;
    B_acknum = 1;
}

void B_input(struct pkt packet) 
{
    timer_count = 0;
    if(checksum(packet) != packet.checksum) {
        return;
    }
    if (packet.seqnum != B_receievedseqnumfromA){
        return;
    }

    struct msg m = packet_to_message(packet);
    tolayer5_B(m);

    B_acknum = packet.seqnum + packet.length;
    B_receievedseqnumfromA = B_acknum;

    struct pkt p;
    p.seqnum = packet.acknum;
    p.acknum = B_acknum;
    p.length = 0;
    p.checksum = checksum(p);

    tolayer3_B(p);
    stoptimer_B();
    starttimer_B(B_TIMER_LEN);
}

void B_timerinterrupt() 
{
    timer_count = timer_count + 1;
    struct pkt p;
    p.seqnum = 1;
    p.acknum = B_receievedseqnumfromA;
    p.length = 0;
    p.checksum = checksum(p);
    
    tolayer3_B(p);
    if(timer_count < 30){
        stoptimer_B();
        starttimer_B(B_TIMER_LEN);
    }

}
