/******************************************************************************/
/*                                                                            */
/* ENTITY IMPLEMENTATIONS                                                     */
/*                                                                            */
/******************************************************************************/

// Student names:
// Student computing IDs:
//
//
// This file contains the actual code for the functions that will implement the
// reliable transport protocols enabling entity "A" to reliably send information
// to entity "B".
//
// This is where you should write your code, and you should submit a modified
// version of this file.
//
// Notes:
// - One way network delay averages five time units (longer if there are other
//   messages in the channel for GBN), but can be larger.
// - Packets can be corrupted (either the header or the data portion) or lost,
//   according to user-defined probabilities entered as command line arguments.
// - Packets will be delivered in the order in which they were sent (although
//   some can be lost).
// - You may have global state in this file, BUT THAT GLOBAL STATE MUST NOT BE
//   SHARED BETWEEN THE TWO ENTITIES' FUNCTIONS. "A" and "B" are simulating two
//   entities connected by a network, and as such they cannot access each
//   other's variables and global state. Entity "A" can access its own state,
//   and entity "B" can access its own state, but anything shared between the
//   two must be passed in a `pkt` across the simulated network. Violating this
//   requirement will result in a very low score for this project (or a 0).
//
// To run this project you should be able to compile it with something like:
//
//     $ gcc entity.c simulator.c -o myproject
//
// and then run it like:
//
//     $ ./myproject 0.0 0.0 10 500 3 test1.txt
//
// Of course, that will cause the channel to be perfect, so you should test
// with a less ideal channel, and you should vary the random seed. However, for
// testing it can be helpful to keep the seed constant.
//
// The simulator will write the received data on entity "B" to a file called
// `output.dat`.

#include <stdio.h>
#include "simulator.h"
#include <limits.h>
#include <stdlib.h>

/**** GLOBAL FUNCS ****/

void debug_pkt(struct pkt p)
{
    printf("\tseq: %d\n\tack: %d\n\tlen: %d\n", p.seqnum, p.acknum, p.length);
}

/**
 * We would suggest a TCP-like checksum, 
 * which consists of the sum of the (integer) 
 * sequence and ack field values, added to a 
 * character-by-character sum of the valid 
 * payload field of the packet
*/
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


/* QUEUE */

typedef struct queue {
  //int length;
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
    //debug_pkt(p);
}
void printq(queue *q) {
    int i;
    int start = q->front;

    for(i = 0; i < q->size; i++) {
        printf("packet %i: \n",i);
        debug_pkt(q->buffer[(i+start)%q->capacity]);
        printf("---------------\n");
    }
}
struct pkt dequeue(queue *q) {
    struct pkt p;
    if(isempty(q)) {
        //return a weird packet idk
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
        //return a weird packet idk
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
/**** A ENTITY ****/

#define A_TIMER_LEN 2000.0
#define BUFSIZE 8

int A_seqnum, A_acknum, A_receievedseqnumfromB, A_receivedacknumfromB;
queue * A_buffer;

int loops,B_loops;



/**
 * do i need this?
*/
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

/**
 * construct a packet based on message 'message'
 * call tolayer3_A(packet)
 * (maybe) call starttimer_A
*/
void A_output(struct msg message) 
{
    struct pkt p = message_to_packet(message, A_seqnum, A_acknum);
    enqueue(A_buffer,p);
    //printq(A_buffer);
    tolayer3_A(p);
    A_seqnum += p.length;
}

/**
 * called whenever A receives a packet from B
 * these should all be ACK or NACK packets i think
*/
void A_input(struct pkt packet) 
{
    // debug_pkt(packet);
    //printf("A Looking for %i, got %i\n", A_receievedseqnumfromB,packet.seqnum);
    
    if(checksum(packet) != packet.checksum) {
        printf("A Recieved corrupted\n");
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
    printf("A RECEIVED PACKET:\n");
    debug_pkt(packet);
    while(!isempty(A_buffer) && peek(A_buffer).seqnum < packet.acknum) {
        
        printf("A Just dequeued this: \n");
        debug_pkt(dequeue(A_buffer));
        //printf("%i\n",peek(A_buffer).seqnum);
        //printf("size: %i\n",A_buffer->size);
    }
    A_acknum = packet.seqnum + packet.length;
    A_receievedseqnumfromB = A_acknum;//packet.acknum;
    if(packet.acknum > A_receivedacknumfromB ) {
        
        A_receivedacknumfromB = packet.acknum;
        printf("Updated A_receivedacknumfromB to %i\n", A_receivedacknumfromB);
    }
    stoptimer_A();
    starttimer_A(A_TIMER_LEN);
}

/**
 * aaa timer ran out!
 * resend last WINDOWSIZE (8) packets
*/
void A_timerinterrupt() 
{
    // int i;
    // for(i =0; i < A_buffer->size;i++) {
    //     //tolayer3_A(dequeue(A_buffer));
    //     //can't dequeue i have to send everything without dequeuing
        
    // }
    int i;
    int start = A_buffer->front;
    //printf("size of buffer: %i\n", A_buffer->size);

    for(i = 0; i < A_buffer->size; i++) {
        //printf("packet %i: \n",i);
        struct pkt p;
        p = A_buffer->buffer[(i+start)%A_buffer->capacity];
        //printf("INSIDE");
        //debug_pkt(p);
        if(p.seqnum >= A_receivedacknumfromB) {
            tolayer3_A(p);
        }
        //printf("sent packet\n");
        //printf("---------------\n");
    }
    //printq(A_buffer);
    //printf("TIMER INTERRUPT AUUUUUUUUUUGH\n");
    if(A_buffer->size != 0 /*&& loops < 1000*/) {
        loops = loops + 1;
        printf("BUFFER ************************** A_receivedacknumfromB: %i\n", A_receivedacknumfromB);
        printq(A_buffer);
        starttimer_A(A_TIMER_LEN);
    }
    else {
        printf("buffersize:%i",A_buffer->size);
        printf("DONE!!!");
    }
}


/**** B ENTITY ****/

#define B_TIMER_LEN 1000.0
int B_seqnum, B_acknum, B_receievedseqnumfromA;

void B_init() 
{
    B_receievedseqnumfromA = 1;
    B_seqnum = 1;
    B_acknum = 1;
    B_loops = 0;
}

/**
 * receive a packet from A
 * call tolayer5_B(msg) with decoded message
 * ALSO sends an ACK back to A
*/

void B_input(struct pkt packet) 
{
    // debug_pkt(packet);
    /*
    if (packet.seqnum != B_acknum)
        return;*/
    
    printf("B Looking for %i, got %i\n", B_receievedseqnumfromA,packet.seqnum);
    if(checksum(packet) != packet.checksum) {
        printf("B Recieved corrupted\n");
        return;
    }
    if (packet.seqnum != B_receievedseqnumfromA){
        printf("B doesn't want this packet %i\n", packet.seqnum);
        return;
    }
    printf("B RECEIVED PACKET:\n");
    debug_pkt(packet);
    // send packet data to file
    struct msg m = packet_to_message(packet);
    tolayer5_B(m);

    // send ACK to A
    B_acknum = packet.seqnum + packet.length;
    B_receievedseqnumfromA = B_acknum;//packet.seqnum;

    struct pkt p;
    p.seqnum = packet.acknum;
    p.acknum = B_acknum;
    p.length = 0;
    p.checksum = checksum(p);

    tolayer3_B(p);
    printf("//////////////////////");
    printf("Just sent ACK: %i\n", p.acknum);
    stoptimer_B();
    starttimer_B(B_TIMER_LEN);
}

/**
 * i think this just re-sends the last ACK
*/
void B_timerinterrupt() 
{
    struct pkt p;
    p.seqnum = 1;
    p.acknum = B_receievedseqnumfromA;
    p.length = 0;
    p.checksum = checksum(p);
    printf("RESENDING FROM B ACK: %i\n",p.acknum);
    tolayer3_B(p);
    if(A_buffer->size > 0 /*&& loops < 1000*/){
        //B_loops = B_loops + 1;
        stoptimer_B();
        starttimer_B(B_TIMER_LEN);
    }
}


/*TESTING*/
// void main() {
    
//     struct pkt p;
//     p.seqnum = -1;
//     p.acknum = -1;
//     p.length = -1;
//     p.checksum = -1;

//     queue * q;
//     q = makeQueue(BUFSIZE);
//     int i;
//     for(i =0; i < 3; i++){
//         enqueue(q,p);
//     }
//     for(i = 0; i < 5; i++){
//         debug_pkt(dequeue(q));
//     }
//     printq(q);
//     //printf("size:%i",q->size);
//     //debug_pkt(p);
// }
