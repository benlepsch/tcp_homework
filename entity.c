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


void printp(struct pkt p)
{
    printf("\tSYN: %d\n\tACK: %d\n\tLEN: %d\n", p.seqnum, p.acknum, p.length);
}

typedef struct queue {
  struct pkt *buffer;
  int front;
  int rear;
  int size;
  unsigned capacity;
} queue;
queue* initalizequeue(unsigned c) {
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

#define ATIMER 2000.0
#define BUFFER 1000

int aseqnum, aacknum, aseqfrom_b, aackfrom_b;
queue * abuffer;

void A_init() 
{
    aseqnum = 1;
    aacknum = 1;
    aseqfrom_b = 1;
    aackfrom_b = 0;

    abuffer = initalizequeue(BUFFER);
    starttimer_A(ATIMER);
}

void A_output(struct msg message) 
{
    struct pkt p = message_to_packet(message, aseqnum, aacknum);
    enqueue(abuffer,p);
    tolayer3_A(p);
    aseqnum += p.length;
}

void A_input(struct pkt packet) 
{
    if(checksum(packet) != packet.checksum) {
        return;
    }
    if(packet.acknum > aseqnum + 1) {
        return;
    }
    if (packet.seqnum != aseqfrom_b){
        return;
    }
    while(!isempty(abuffer) && peek(abuffer).seqnum < packet.acknum) {
        dequeue(abuffer);
    }
    aacknum = packet.seqnum + packet.length;
    aseqfrom_b = aacknum;
    if(packet.acknum > aackfrom_b ) {
        aackfrom_b = packet.acknum;
        printf("Got ACK back: %i\n",aackfrom_b);
    }
    stoptimer_A();
    starttimer_A(ATIMER);
}

void A_timerinterrupt() 
{
    int i;
    int start = abuffer->front;
    for(i = 0; i < abuffer->size; i++) {
        struct pkt p;
        p = abuffer->buffer[(i+start)%abuffer->capacity];
        if(p.seqnum >= aackfrom_b) {
            tolayer3_A(p);
        }
    }
    
    if(abuffer->size != 0) {
        starttimer_A(ATIMER);
    }

}

#define BTIMER 1000.0
int bseqnum, backnum, bseqfrom_A,timer_count;

void B_init() 
{
    timer_count = 0;
    bseqfrom_A = 1;
    bseqnum = 1;
    backnum = 1;
}

void B_input(struct pkt packet) 
{
    timer_count = 0;
    if(checksum(packet) != packet.checksum) {
        return;
    }
    if (packet.seqnum != bseqfrom_A){
        return;
    }

    struct msg m = packet_to_message(packet);
    tolayer5_B(m);

    backnum = packet.seqnum + packet.length;
    bseqfrom_A = backnum;

    struct pkt p;
    p.seqnum = packet.acknum;
    p.acknum = backnum;
    p.length = 0;
    p.checksum = checksum(p);

    tolayer3_B(p);
    stoptimer_B();
    starttimer_B(BTIMER);
}

void B_timerinterrupt() 
{
    timer_count = timer_count + 1;
    struct pkt p;
    p.seqnum = 1;
    p.acknum = bseqfrom_A;
    p.length = 0;
    p.checksum = checksum(p);
    
    tolayer3_B(p);
    if(timer_count < 30){
        stoptimer_B();
        starttimer_B(BTIMER);
    }

}
