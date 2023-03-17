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
#include "queue.h"

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
    int sum, i;

    sum = packet.seqnum;
    sum += packet.acknum;

    for (i = 0; i < packet.length; i++)
        sum += packet.payload[i];

    return sum;
}

struct pkt message_to_packet(struct msg message, int seqnum, int acknum)
{
    struct pkt p;
    p.seqnum = seqnum;
    p.acknum = acknum;
    p.length = message.length;

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

    int i;
    for (i = 0; i < m.length; i++) 
        m.data[i] = packet.payload[i];
    
    return m;
}

/**** A ENTITY ****/

#define A_TIMER_LEN 2000

int A_seqnum, A_acknum, A_receievedseqnumfromB;
struct pkt buffer[BUFSIZE];

/**
 * do i need this?
*/
void A_init() 
{
    A_seqnum = 1;
    A_receievedseqnumfromB = 0;
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
    tolayer3_A(p);
    A_seqnum += p.length;
}

/**
 * called whenever A receives a packet from B
 * these should all be ACK or NACK packets i think
*/
void A_input(struct pkt packet) 
{
    printf("A RECEIVED PACKET:\n");
    debug_pkt(packet);

    if (packet.acknum < A_receievedseqnumfromB)
        return;

    A_receievedseqnumfromB = packet.acknum;
    stoptimer_A();
    starttimer_A(A_TIMER_LEN);
}

/**
 * aaa timer ran out!
 * resend last WINDOWSIZE (8) packets
*/
void A_timerinterrupt() 
{
    printf("TIMER INTERRUPT AUUUUUUUUUUGH\n");
}


/**** B ENTITY ****/

#define B_TIMER_LEN 1000
int B_seqnum, B_acknum;

void B_init() 
{
    B_seqnum = 1;
    B_acknum = 1;
}

/**
 * receive a packet from A
 * call tolayer5_B(msg) with decoded message
 * ALSO sends an ACK back to A
*/
void B_input(struct pkt packet) 
{
    printf("B RECEIVED PACKET:\n");
    debug_pkt(packet);
    if (packet.seqnum != B_acknum)
        return;
    
    // send packet data to file
    struct msg m = packet_to_message(packet);
    tolayer5_B(m);

    // send ACK to A
    B_acknum = packet.seqnum + packet.length;

    struct pkt p;
    p.seqnum = 1;
    p.acknum = B_acknum;
    p.length = 0;
    p.checksum = checksum(p);

    tolayer3_B(p);
}

/**
 * i think this just re-sends the last ACK
*/
void B_timerinterrupt() 
{
}
