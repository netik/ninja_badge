/*
 * Copyright (c) 2010, Mariano Alvira <mar@devl.org> and other contributors
 * to the MC1322x project (http://mc1322x.devl.org)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of libmc1322x: see http://mc1322x.devl.org
 * for details. 
 *
 * $Id$
 */

#include <mc1322x.h>
#include <task.h>
#include <globals.h>
#include <pinio.h>
#include <taskdefs.h>
#include <info.h>
#include <debug.h>
#include <rfio.h>
#include <task.h>
#include <misc.h>
#include <put.h>

#ifndef DEBUG_MACA 
#define DEBUG_MACA 0
#endif

#if (DEBUG_MACA == 0)
#define PRINTF(...) 
#else
#define PRINTF(...) dongs(__VA_ARGS__)
#endif

#ifndef MACA_BOUND_CHECK
#define MACA_BOUND_CHECK 0
#endif
#if (MACA_BOUND_CHECK == 0)
#define BOUND_CHECK(x)
#else
#define BOUND_CHECK(x) bound_check(x)
#endif

#ifndef NUM_PACKETS
#define NUM_PACKETS 32
#endif

/* for 250kHz clock */
#define MACA_CLOCK_DIV 95
/* (32 chips/sym) * (sym/4bits) * (8bits/byte) = (64 chips/byte)  */
/* (8 chips/clk) * (byte/64 chips) = byte/8clks */
#define CLK_PER_BYTE 8 

#ifndef RECV_SOFTIMEOUT
#define RECV_SOFTIMEOUT (32*128*CLK_PER_BYTE) 
#endif

#ifndef CPL_TIMEOUT
#define CPL_TIMEOUT (2*128*CLK_PER_BYTE) 
#endif

#define CPL_READTIMEOUT (100*128*CLK_PER_BYTE)

#define reg(x) (*(volatile uint32_t *)(x))

int count_packets(void);
void post_receive(unsigned int starttime, unsigned int length, unsigned int asap);
void wakeup_rx(u_int32_t val);

static volatile packet_t packet_pool[NUM_PACKETS];
static volatile packet_t *free_head, *rx_end, *tx_end, *dma_tx, *dma_rx;

volatile int rf_waiting;
volatile unsigned long long rf_waiting_time = 0, rf_rx_end = 0;

/* rx_head and tx_head are visible to the outside */
/* so you can peek at it and see if there is data */
/* waiting for you, or data still to be sent */
volatile packet_t *rx_head, *tx_head;

/* used for ack recpetion if the packet_pool goes empty */
/* doesn't go back into the pool when freed */
static volatile packet_t dummy_ack;

/* incremented on every maca entry */
/* you can use this to detect that the receive loop is still running */
volatile uint32_t maca_entry = 0;

enum posts {
	NO_POST = 0,
	TX_POST,
	RX_POST,
	MAX_POST,
};
static volatile uint8_t last_post = NO_POST;

volatile uint8_t fcs_mode = USE_FCS; 

/* call periodically to */
/* check that maca_entry is changing */
/* if it is not, it will do a manual call to maca_isr which should */
/* get the ball rolling again */
/* also checks that the clock is running --- if it isn't then */
/* it calls redoes the maca intialization but _DOES NOT_ free all packets */ 

void check_maca(void) {
	static volatile uint32_t last_time;
    static volatile uint32_t last_entry;
	volatile uint32_t i;

    safe_irq_disable(MACA);
	/* if *MACA_CLK == last_time */
	/* try waiting for one clock period */
	/* since maybe check_maca is getting called quickly */	
	for(i=0; (i < 1024) && (*MACA_CLK == last_time); i++) { continue; }

	if(*MACA_CLK == last_time) {
		putstr("check maca: maca_clk stopped, restarting\r\n last post");
        put_hex32(last_post);
        putstr("\r\n");
		/* clock isn't running */
		ResumeMACASync();
        if(bit_is_set(*NIPEND, INT_NUM_MACA)) { *INTFRC = (1 << INT_NUM_MACA); }
        force_maca = 1;
	}
	last_time = *MACA_CLK;
    last_entry = maca_entry;
    if(rf_waiting == 1) {
        unsigned long long check = *MACA_CLK;
        if(check > (rf_waiting_time + 1000)) {
            putstr("XXX: stopped, trying reset\r\n");
            putstr("XXX last_post ");
            put_hex32(last_post);
            putstr(NEWLINE);
            force_maca = 1;
            *INTFRC = (1<<INT_NUM_MACA);
        }
    } else if(tx_head != 0) {
        *INTFRC = (1 << INT_NUM_MACA); 
    }
    irq_restore();
		

#if DEBUG_MACA
	if(count_packets() != NUM_PACKETS) {
		putstr("check maca: count_packets 0x");
        put_hex32(count_packets());
        putstr(NEWLINE);
	}
#endif /* DEBUG_MACA */
}

void maca_init(void) {
	reset_maca();
	radio_init();
	flyback_init();
	init_phy();
	free_head = 0; tx_head = 0; rx_head = 0; rx_end = 0; tx_end = 0; dma_tx = 0; dma_rx = 0;
	free_all_packets();
	
	/* initial radio command */
    /* nop, promiscuous, no cca */
	*MACA_CONTROL = (1 << PRM) | (NO_CCA << MODE); 
	
	enable_irq(MACA);
	maca_isr(); 
}

#define print_packets(x) Print_Packets(x)
void Print_Packets(char *s) {
	volatile packet_t *p;
	int i = 0;
	putstr("packet pool after ");
    putstr(s);
    putstr(NEWLINE);
	p = free_head;	
    putstr("free_head: 0x");
    put_hex32((u_int32_t)free_head);
    putstr(" ");
	while(p != 0) {
		i++;
		p = p->left;
        putstr("->0x");
        put_hex32((uint32_t)p);
	}
	putstr(NEWLINE);

	p = tx_head;
    putstr("tx_head: 0x");
    put_hex32((uint32_t)tx_head);
	while(p != 0) {
		i++;
		p = p->left;
        putstr("->0x");
        put_hex32((uint32_t)p);
	}
    putstr(NEWLINE);

	p = rx_head;
    putstr("rx_head: 0x");
    put_hex32((uint32_t)tx_head);
    putstr(" ");
	while(p != 0) {
		i++;
		p = p->left;
        putstr("->0x");
        put_hex32((uint32_t)p);
	}
    putstr("\r\nfound 0x");
    put_hex32(i);
    putstr(" packets\r\n");
}

inline void bad_packet_bounds(void) {
	putstr("bad packet bounds! Halting.\r\n");
	while(1) { continue; }
}

int count_packets(void) {
	volatile packet_t *pk;
	volatile uint8_t tx, rx, free, total;

	pk = tx_head; tx = 0;
	while( pk != 0 ) {
		tx++;
		pk = pk->left;
	}
	pk = rx_head; rx = 0;
	while( pk != 0 ) {
		rx++;
		pk = pk->left;
	}
	pk = free_head; free = 0;
	while( pk != 0 ) {
		free++;
		pk = pk->left;
	}

	total = free + rx + tx;
	if(dma_rx && (dma_rx != rx_head)) { total++; }
	if(dma_tx && (dma_tx != tx_head)) { total++; }

	return total;
}	

void bound_check(volatile packet_t *p) {
	volatile int i;

	if((p == 0) ||
	   (p == &dummy_ack)) { return; }
	for(i=0; i < NUM_PACKETS; i++) {
		if(p == &packet_pool[i]) { return; }
	}

	bad_packet_bounds();
}


/* public packet routines */
/* heads are to the right */
/* ends are to the left */
void free_packet(volatile packet_t *p) {
	safe_irq_disable(MACA);

	BOUND_CHECK(p);

	if(!p) {  putstr("free_packet passed packet 0\r\n"); return; }
	if(p == &dummy_ack) { return; }

	BOUND_CHECK(free_head);

	p->length = 0; p->offset = 0;
	p->left = free_head; p->right = 0;
	free_head = p;

	BOUND_CHECK(free_head);

	irq_restore();
    if(bit_is_set(*NIPEND, INT_NUM_MACA)) { *INTFRC = (1 << INT_NUM_MACA); }
	return;
}

volatile packet_t* get_free_packet(void) {
	volatile packet_t *p;

	safe_irq_disable(MACA);

	BOUND_CHECK(free_head);

	p = free_head;
	if( p != 0 ) {		
		free_head = p->left;
		free_head->right = 0;
	}

	BOUND_CHECK(free_head);

//	print_packets("get_free_packet");
    p->syncup = 0;
    p->offset = 0;
    p->timessent = 0;
	irq_restore();
    if(bit_is_set(*NIPEND, INT_NUM_MACA)) { *INTFRC = (1 << INT_NUM_MACA); }
	return p;
}

void post_receive(unsigned int starttime, unsigned int length, unsigned int asap) {
	last_post = RX_POST;
	/* this sets the rxlen field */
	/* this is undocumented but very important */
	/* you will not receive anything without setting it */
	*MACA_TXLEN = (MAX_PACKET_SIZE << 16);
	if(dma_rx == 0) {
		dma_rx = get_free_packet();
		if (dma_rx == 0) {
			putstr("trying to fill MACA_DMARX in post_receieve but out of packet buffers\r\n");		
			/* set the sftclock so that we return to the maca_isr */
            // XXX: remove this?
			*MACA_SFTCLK = *MACA_CLK + RECV_SOFTIMEOUT; /* soft timeout */ 
			*MACA_TMREN = (1 << maca_tmren_sft);
			/* no free buffers, so don't start a reception */
			enable_irq(MACA);
			return;
		}
	}
	BOUND_CHECK(dma_rx);
	BOUND_CHECK(dma_tx);
	*MACA_DMARX = (uint32_t)&(dma_rx->data[0]);
    if(asap) {
        *MACA_SFTCLK = starttime + length; /* soft timeout */ 
        *MACA_TMREN = (1 << maca_tmren_sft);
        *MACA_CONTROL = ( (1 << maca_ctrl_asap) |
                  ( 4 << PRECOUNT) |
                  ( fcs_mode << NOFC ) |
                  (1 << maca_ctrl_auto) |
                  (1 << maca_ctrl_prm) |
                  (maca_ctrl_seq_rx));
    } else {
        *MACA_SFTCLK = starttime + length; /* soft timeout */ 
        *MACA_CPLCLK = starttime + length + CPL_READTIMEOUT;
        *MACA_STARTCLK = starttime;
        *MACA_TMREN = (1 << maca_tmren_sft) | (1 << maca_tmren_strt) | (1 << maca_tmren_cpl);
        /* start the receive sequence */
        *MACA_CONTROL = ( 
                  ( 4 << PRECOUNT) |
                  ( fcs_mode << NOFC ) |
                  (1 << maca_ctrl_auto) |
                  (1 << maca_ctrl_prm) |
                  (maca_ctrl_seq_rx));
    }
    rf_waiting = 1;
    rf_rx_end = starttime+length;
    rf_waiting_time = starttime + length + CPL_READTIMEOUT;
	/* status bit 10 is set immediately */
    /* then 11, 10, and 9 get set */ 
    /* they are cleared once we get back to maca_isr */ 
}


volatile packet_t* rx_packet(void) {
	volatile packet_t *p;
	safe_irq_disable(MACA);

	BOUND_CHECK(rx_head);

	p = rx_head;
	if( p != 0 ) {
		rx_head = p->left;
		rx_head->right = 0;
	}

//	print_packets("rx_packet");
	irq_restore();
    if(bit_is_set(*NIPEND, INT_NUM_MACA)) { *INTFRC = (1 << INT_NUM_MACA); }
	return p;
}

void post_tx(void) {
	/* set dma tx pointer to the payload */
	/* and set the tx len */
	//disable_irq(MACA);
	safe_irq_disable(MACA);
	last_post = TX_POST;
	dma_tx = tx_head; 
	*MACA_TXLEN = (uint32_t)((dma_tx->length) + 3);
	*MACA_DMATX = (uint32_t)&(dma_tx->data[ 0 + dma_tx->offset]);
	if(dma_rx == 0) {
		dma_rx = get_free_packet();
		if (dma_rx == 0) { 
			dma_rx = &dummy_ack;
			putstr("trying to fill MACA_DMARX on post_tx but out of packet buffers\r\n");
		}
	}	
	BOUND_CHECK(dma_rx);
	BOUND_CHECK(dma_tx);
	*MACA_DMARX = (uint32_t)&(dma_rx->data[0]);
	/* disable soft timeout clock */
	/* disable start clock */
	*MACA_TMRDIS = (1 << maca_tmren_sft) | ( 1<< maca_tmren_cpl) | ( 1 << maca_tmren_strt ) ;
	
        /* set complete clock to long value */
	/* acts like a watchdog in case the MACA locks up */
	*MACA_CPLCLK = dma_tx->starttime + CPL_TIMEOUT;
    *MACA_STARTCLK = dma_tx->starttime;
	*MACA_TMREN = (1 << maca_tmren_cpl) | (1 << maca_tmren_strt);
	
	//enable_irq(MACA);
	irq_restore();

    rf_waiting = 1;
    rf_waiting_time = dma_tx->starttime + CPL_TIMEOUT;
	*MACA_CONTROL = ( (1 << maca_ctrl_prm) | ( 4 << PRECOUNT) |
			  (maca_ctrl_mode_no_cca << maca_ctrl_mode) |
			  (maca_ctrl_seq_tx));	
    if(bit_is_set(*NIPEND, INT_NUM_MACA)) { 
        *INTFRC = (1 << INT_NUM_MACA); 
    }
}

volatile packet_t *swi_rfreceive(unsigned int starttime, unsigned int length) {
    volatile packet_t *p;
    if((p = rx_packet()) != NULL) {
        return p;
    }
    if(length == 0) {
        return NULL;
    }
    //XXX: remove this check later
    task *ct = &tasks[curtask];
    if(ct->timer_waiting) {
        DEBUG_TASK("rx: task already waiting\r\n");
        putstr("XXX ");
        put_hex32(curtask);
        putstr("\r\n");
    }
    task_clear_ready(curtask);
    ct->wakeup_reason = 0;
    ct->rfreceive_waiting = 1;
    task_reschedule();
    if(globals.now_fighting) {
        *INTFRC = (1 << INT_NUM_MACA); 
    } else {
        post_receive(starttime, length, 0);
    }
    return NULL;        // ignored
}
unsigned int swi_txpacket(volatile packet_t *pack) {
    task *ct = &tasks[curtask];
    if(ct->wakeup_reason) {
        DEBUG_TASK("XXX: task already waiting!\r\n");
    }
    pack->task = curtask;
    task_clear_ready(curtask);
    ct->wakeup_reason = 0;
    ct->txpacket_waiting = 1;
    task_reschedule();
    tx_packet(pack);
    //post_tx();
    return 0;   // ignored
}

void tx_packet(volatile packet_t *p) {
	safe_irq_disable(MACA);

	BOUND_CHECK(p);

	if(!p) {  
        putstr("XXX: tx_packet passed packet 0\r\n"); 
        return; 
    }
	if(tx_head == 0) {
		/* start a new queue if empty */
		tx_end = p;
		tx_end->left = 0; tx_end->right = 0;
		tx_head = tx_end; 
	} else {
		/* add p to the end of the queue */
		tx_end->left = p;
		p->right = tx_end;
		/* move the queue */
		tx_end = p; tx_end->left = 0;
	}
//	print_packets("tx packet");
	irq_restore();
    if(bit_is_set(*NIPEND, INT_NUM_MACA)) { 
        *INTFRC = (1 << INT_NUM_MACA); 
    }
    if(last_post == NO_POST) { 
        *INTFRC = (1<<INT_NUM_MACA); 
    }
    if(last_post == RX_POST) { 
        *MACA_SFTCLK = *MACA_CLK+10; 
        *MACA_TMREN |= (1 << maca_tmren_sft);
        *MACA_CONTROL = maca_ctrl_seq_wait
			   | (1 << maca_ctrl_asap);
    }
	return;
}

void free_all_packets(void) {
	volatile int i;
	safe_irq_disable(MACA);

	free_head = 0;
	for(i=0; i<NUM_PACKETS; i++) {
		free_packet((volatile packet_t *)&(packet_pool[i]));		
	}
	rx_head = 0; rx_end = 0;
	tx_head = 0; tx_end = 0;

	irq_restore();
    if(bit_is_set(*NIPEND, INT_NUM_MACA)) { *INTFRC = (1 << INT_NUM_MACA); }
	return;
}

void wakeup_tx(void) {
	volatile packet_t *p;
	safe_irq_disable(MACA);

	BOUND_CHECK(tx_head);

	p = tx_head;
    tx_head = tx_head->left;
    if(tx_head == 0) { 
        tx_end = 0; 
    }
    if(p == 0) {
        DEBUG_RFIO("XXX: tx_head is null after completion!\r\n");
    } else {
        p->finishedtime = *MACA_CPLTIM;
        if(p->task > 0 && p->task < NTASKS) {
            task_wakeup_txpacket(p->task, TXSTATUS_SUCCESS);
        }
    }
	
	irq_restore();
    if(bit_is_set(*NIPEND, INT_NUM_MACA)) { *INTFRC = (1 << INT_NUM_MACA); }
	return;
}

void add_to_rx(volatile packet_t *p) {
	safe_irq_disable(MACA);

	BOUND_CHECK(p);
	
	if(!p) {  putstr("add_to_rx passed packet 0\r\n"); return; }
	p->offset = 1; /* first byte is the length */
	if(rx_head == 0) {
		/* start a new queue if empty */
		rx_end = p;
		rx_end->left = 0; rx_end->right = 0;
		rx_head = rx_end; 
	} else {
		rx_end->left = p;
		p->right = rx_end;
		rx_end = p; rx_end->left = 0;
	}
	
//	print_packets("add to rx");
	irq_restore();
    if(bit_is_set(*NIPEND, INT_NUM_MACA)) { *INTFRC = (1 << INT_NUM_MACA); }
	return;
}

void decode_status(void) {
	volatile uint32_t code;
	
	code = get_field(*MACA_STATUS,CODE);
	/* PRINTF("status code 0x%x\r\n",code); */
	
	switch(code)
	{
        case ABORTED:
            //putstr("abort\r\n");
            ResumeMACASync();
            break;
        case NOT_COMPLETED:
    //		putstr("maca: not completed\r\n");
            ResumeMACASync();
            break;
        case CODE_TIMEOUT:
            putstr("maca: timeout\r\n");
            ResumeMACASync();
            break;
        case LATE_START:
            putstr("late start\r\n");
            break;
        case NO_ACK:
            putstr("maca: no ack\r\n");
            ResumeMACASync();
            break;
        case EXT_TIMEOUT:
            ResumeMACASync();
            break;
        case EXT_PND_TIMEOUT:
            putstr("maca: ext pnd timeout\r\n");
            ResumeMACASync();
            break;
        case SUCCESS:
            //PRINTF("maca: success\r\n");
            //putstr("maca: success\r\n");
            ResumeMACASync();
            break;				
        default:
            putstr("unkstatus: ");
            put_hex32(*MACA_STATUS);
            putstr(NEWLINE);
            ResumeMACASync();
            break;
	}
}

void maca_isr(void) {
	maca_entry++;

	if (bit_is_set(*MACA_STATUS, maca_status_ovr)) { 
        putstr("maca overrun\r\n"); 
    }
	if (bit_is_set(*MACA_STATUS, maca_status_busy)) { 
        putstr("maca busy\r\n"); 
    } 
	if (bit_is_set(*MACA_STATUS, maca_status_crc)) { 
        putstr("maca crc error\r\n"); 
    }
	if (bit_is_set(*MACA_STATUS, maca_status_to)) { 
        putstr("maca timeout\r\n"); 
    }

    if (data_indication_irq()) {
        dma_rx->length = *MACA_GETRXLVL - 2; /* packet length does not include FCS */
        dma_rx->finishedtime = *MACA_TIMESTAMP;
        if(rand_needs_seed == 1) {
            *MACA_RANDOM = dma_rx->finishedtime;
            rand_needs_seed = 0;
        }
        add_to_rx(dma_rx);
        wakeup_rx((u_int32_t)rx_packet());
        dma_rx = 0;
        *MACA_CLRIRQ = (1 << maca_irq_di);
    }
    if (filter_failed_irq()) {
        putstr("maca filter failed\r\n");
        ResumeMACASync();
        *MACA_CLRIRQ = (1 << maca_irq_flt);
    }
    if (softclock_irq()) {
        //putstr("XXX softclock\r\n");
        *MACA_CLRIRQ = (1 << maca_irq_sftclk);
    }
    if (poll_irq()) {		
        putstr("XXX poll\r\n");
        *MACA_CLRIRQ = (1 << maca_irq_poll);
    }
    if(wu_irq()) {
        putstr("XXX wu\r\n");
        *MACA_CLRIRQ = (1 << maca_irq_wu);
    }
    if(rst_irq()) {
        putstr("XXX rst\r\n");
        *MACA_CLRIRQ = (1 << maca_irq_rst);
    }
    if(cm_irq()) {
        putstr("XXX cm\r\n");
        *MACA_CLRIRQ = (1 << maca_irq_cm);
    }
    if(lvl_irq()) {
        putstr("XXX lvl\r\n");
        *MACA_CLRIRQ = (1 << maca_irq_lvl);
    }
    unsigned int acpl = 0;
    if(action_complete_irq() || force_maca) {
        acpl = 1;
        //putstr("maca action complete 0x");
        //put_hex32(get_field(*MACA_CONTROL, SEQUENCE));
        //putstr(NEWLINE);
        if(last_post == TX_POST) {
            dma_tx = 0;
            rf_waiting = 0;
            if(*MACA_STATUS & 0xf) {
                DEBUG_RFIO("tx failure ");
                DEBUG32_RFIO((u_int32_t)*MACA_STATUS);
                DEBUG_RFIO(NEWLINE);
            }
            wakeup_tx();
            last_post = NO_POST;
        } else if(last_post == RX_POST) {
            rf_waiting = 0;
            wakeup_rx(0);
            last_post = NO_POST;
        } else {
            DEBUG_RFIO("unknown acpl\r\n");
        }
        ResumeMACASync();
        *MACA_CLRIRQ = (1 << maca_irq_acpl);		
    }
    if (checksum_failed_irq()) {
        putstr("maca checksum failed\r\n");
        ResumeMACASync();
        if(rf_waiting && last_post == RX_POST) {
            rf_waiting = 0;
            wakeup_rx(0);
            last_post = NO_POST;
        }
        *MACA_CLRIRQ = (1 << maca_irq_crc);
    }
    decode_status();

	if (*MACA_IRQ != 0) { 
        putstr("*MACA_IRQ ");
        put_hex32(*MACA_IRQ);
        putstr(NEWLINE);
    }
    if(tx_head != 0) {
        post_tx();
    } else {
        if(acpl && globals.now_fighting == 0) {
            u_int32_t curclk = *MACA_CLK;
            if((curclk + 2048) < rf_rx_end && (curclk + 2048) > curclk && curclk < rf_rx_end) {
                DEBUG_RFIO("restart\r\n");
                const u_int32_t start = curclk+1000;
                post_receive(start, rf_rx_end-start, 0);
            }
        } else if(globals.now_fighting) {
            post_receive(*MACA_CLK, 300000, 1);
        }
    }



}

void wakeup_rx(u_int32_t val) {
    if(tasks[TASK_RFIO].rfreceive_waiting) {
        task_wakeup_rfreceive(TASK_RFIO, val);
    }
}

static uint8_t ram_values[4];

void init_phy(void)
{
//  *MACA_TMREN = (1 << maca_tmren_strt) | (1 << maca_tmren_cpl);
	*MACA_CLKDIV = MACA_CLOCK_DIV;
	*MACA_WARMUP = 0x00180012;
	*MACA_EOFDELAY = 0x00000004;
	*MACA_CCADELAY = 0x001a0022;
	*MACA_TXCCADELAY = 0x00000025;
	*MACA_FRAMESYNC0 = 0x000000A7;
	*MACA_CLK = 0x00000008;
	*MACA_MASKIRQ = ((1 << maca_irq_rst)    | 
			 (1 << maca_irq_wu)     | 
			 (1 << maca_irq_acpl)   | 
			 (1 << maca_irq_poll)   | 
			 (1 << maca_irq_cm)     |
			 (1 << maca_irq_flt)    |
			 (1 << maca_irq_flt)    | 
			 (1 << maca_irq_crc)    | 
			 (1 << maca_irq_di)     |
			 (1 << maca_irq_sftclk)
		);
	*MACA_SLOTOFFSET = 0x00350000;	
}

void reset_maca(void)
{
	volatile uint32_t cnt;
	*MACA_RESET = (1 << maca_reset_rst);
	for(cnt = 0; cnt < 100; cnt++) {};
	*MACA_RESET = (1 << maca_reset_clkon);
	*MACA_CONTROL = maca_ctrl_seq_nop;
	for(cnt = 0; cnt < 400000; cnt++) {};
	/* Clear all interrupts. */
	*MACA_CLRIRQ = 0xffff;
}


/*
	004030c4 <SMAC_InitFlybackSettings>:
	4030c4:       4806            ldr     r0, [pc, #24]   (4030e0 <SMAC_InitFlybackSettings+0x1c>) // r0 gets base 0x80009a00
		4030c6:       6881            ldr     r1, [r0, #8]                                             // r1 gets *(0x80009a08)
		4030c8:       4806            ldr     r0, [pc, #24]   (4030e4 <SMAC_InitFlybackSettings+0x20>) // r0 gets 0x0000f7df
		4030ca:       4308            orrs    r0, r1                                                   // or them, r0 has it
		4030cc:       4904            ldr     r1, [pc, #16]   (4030e0 <SMAC_InitFlybackSettings+0x1c>) // r1 gets base 0x80009a00
		4030ce:       6088            str     r0, [r1, #8]     // put r0 into 0x80009a08
		4030d0:       0008            lsls    r0, r1, #0       // r0 gets r1, r0 is the base now
		4030d2:       4905            ldr     r1, [pc, #20]   (4030e8 <SMAC_InitFlybackSettings+0x24>) // r1 gets 0x00ffffff
		4030d4:       60c1            str     r1, [r0, #12]   // put 0x00ffffff into base+12
		4030d6:       0b09            lsrs    r1, r1, #12     // r1 = 0x00ffffff >> 12
		4030d8:       6101            str     r1, [r0, #16]   // put r1 base+16
		4030da:       2110            movs    r1, #16         // r1 gets 16
		4030dc:       6001            str     r1, [r0, #0]    // put r1 in the base
		4030de:       4770            bx      lr              // return
		4030e0:       80009a00        .word   0x80009a00
		4030e4:       0000f7df        .word   0x0000f7df
		4030e8:       00ffffff        .word   0x00ffffff
*/

/* tested and is good */
#define RF_BASE 0x80009a00
void flyback_init(void) {
	uint32_t val8, or;
	
	val8 = *(volatile uint32_t *)(RF_BASE+8);
	or = val8 | 0x0000f7df;
	*(volatile uint32_t *)(RF_BASE+8) = or;
	*(volatile uint32_t *)(RF_BASE+12) = 0x00ffffff;
	*(volatile uint32_t *)(RF_BASE+16) = (((uint32_t)0x00ffffff)>>12);
	*(volatile uint32_t *)(RF_BASE) = 16;
	/* good luck and godspeed */
}

#define MAX_SEQ1 2
const uint32_t addr_seq1[MAX_SEQ1] = {
	0x80003048,      
	0x8000304c,
};

const uint32_t data_seq1[MAX_SEQ1] = {
	0x00000f78,     
	0x00607707,
};


#define MAX_SEQ2 2
const uint32_t addr_seq2[MAX_SEQ2] = {
	0x8000a050,      
	0x8000a054,      
};

const uint32_t data_seq2[MAX_SEQ2] = {
	0x0000047b,
	0x0000007b, 
};

#define MAX_CAL3_SEQ1 3
const uint32_t addr_cal3_seq1[MAX_CAL3_SEQ1] = { 0x80009400,0x80009a04,0x80009a00, };
const uint32_t data_cal3_seq1[MAX_CAL3_SEQ1] = {0x00020017,0x8185a0a4,0x8c900025, };

#define MAX_CAL3_SEQ2 2
const uint32_t addr_cal3_seq2[MAX_CAL3_SEQ2] = { 0x80009a00,0x80009a00,};
const uint32_t data_cal3_seq2[MAX_CAL3_SEQ2] = { 0x8c900021,0x8c900027,};

#define MAX_CAL3_SEQ3 1
const uint32_t addr_cal3_seq3[MAX_CAL3_SEQ3] = { 0x80009a00 };
const uint32_t data_cal3_seq3[MAX_CAL3_SEQ3] = { 0x8c900000 };

#define MAX_CAL5 4
const uint32_t addr_cal5[MAX_CAL5] = { 
	0x80009400,  
	0x8000a050,       
	0x8000a054,  
	0x80003048,
};
const uint32_t data_cal5[MAX_CAL5] = {
	0x00000017,
	0x00000000,            
	0x00000000,
	0x00000f00,
};

#define MAX_DATA 43
const uint32_t addr_reg_rep[MAX_DATA] = { 0x80004118,0x80009204,0x80009208,0x8000920c,0x80009210,0x80009300,0x80009304,0x80009308,0x8000930c,0x80009310,0x80009314,0x80009318,0x80009380,0x80009384,0x80009388,0x8000938c,0x80009390,0x80009394,0x8000a008,0x8000a018,0x8000a01c,0x80009424,0x80009434,0x80009438,0x8000943c,0x80009440,0x80009444,0x80009448,0x8000944c,0x80009450,0x80009460,0x80009464,0x8000947c,0x800094e0,0x800094e4,0x800094e8,0x800094ec,0x800094f0,0x800094f4,0x800094f8,0x80009470,0x8000981c,0x80009828 };

const uint32_t data_reg_rep[MAX_DATA] = { 0x00180012,0x00000605,0x00000504,0x00001111,0x0fc40000,0x20046000,0x4005580c,0x40075801,0x4005d801,0x5a45d800,0x4a45d800,0x40044000,0x00106000,0x00083806,0x00093807,0x0009b804,0x000db800,0x00093802,0x00000015,0x00000002,0x0000000f,0x0000aaa0,0x01002020,0x016800fe,0x8e578248,0x000000dd,0x00000946,0x0000035a,0x00100010,0x00000515,0x00397feb,0x00180358,0x00000455,0x00000001,0x00020003,0x00040014,0x00240034,0x00440144,0x02440344,0x04440544,0x0ee7fc00,0x00000082,0x0000002a };

void maca_off(void) {
	disable_irq(MACA);
	/* turn off the radio regulators */
	reg(0x80003048) =  0x00000f00;
	/* hold the maca in reset */
	maca_reset = (1 << maca_reset_rst);
}

void maca_on(void) {
	/* turn the radio regulators back on */
	reg(0x80003048) =  0x00000f78; 
	/* reinitialize the phy */
	reset_maca();
	init_phy();
	
	enable_irq(MACA);
	maca_isr(); 
}

/* initialized with 0x4c */
uint8_t ctov[16] = {
        0x0b,
        0x0b,
        0x0b,
        0x0a,
        0x0d,
        0x0d,
        0x0c,
        0x0c,
        0x0f,
        0x0e,
        0x0e,
        0x0e,
        0x11,
        0x10,
        0x10,
        0x0f,
};

/* get_ctov thanks to Umberto */

#define _INIT_CTOV_WORD_1       0x00dfbe77
#define _INIT_CTOV_WORD_2       0x023126e9
uint8_t get_ctov( uint32_t r0, uint32_t r1 )
{

        r0 = r0 * _INIT_CTOV_WORD_1;
        r0 += ( r1 << 22 );
        r0 += _INIT_CTOV_WORD_2;

        r0 = (uint32_t)(((int32_t)r0) >> 25);

        return (uint8_t)r0;
}


/* radio_init has been tested to be good */
void radio_init(void) {
	volatile uint32_t i;
	/* sequence 1 */
	for(i=0; i<MAX_SEQ1; i++) {
		*(volatile uint32_t *)(addr_seq1[i]) = data_seq1[i];
	}
	/* seq 1 delay */
	for(i=0; i<0x161a8; i++) { continue; }
	/* sequence 2 */
	for(i=0; i<MAX_SEQ2; i++) {
		*(volatile uint32_t *)(addr_seq2[i]) = data_seq2[i];
	}
	/* modem val */
	*(volatile uint32_t *)0x80009000 = 0x80050100;
	/* cal 3 seq 1*/
	for(i=0; i<MAX_CAL3_SEQ1; i++) {
		*(volatile uint32_t *)(addr_cal3_seq1[i]) = data_cal3_seq1[i];
	}
	/* cal 3 delay */
	for(i=0; i<0x11194; i++) { continue; }
	/* cal 3 seq 2*/
	for(i=0; i<MAX_CAL3_SEQ2; i++) {
		*(volatile uint32_t *)(addr_cal3_seq2[i]) = data_cal3_seq2[i];
	}
	/* cal 3 delay */
	for(i=0; i<0x11194; i++) { continue; }
	/* cal 3 seq 3*/
	for(i=0; i<MAX_CAL3_SEQ3; i++) {
		*(volatile uint32_t *)(addr_cal3_seq3[i]) = data_cal3_seq3[i];
	}
	/* cal 5 */
	for(i=0; i<MAX_CAL5; i++) {
		*(volatile uint32_t *)(addr_cal5[i]) = data_cal5[i];
	}
	/*reg replacment */
	for(i=0; i<MAX_DATA; i++) {
		*(volatile uint32_t *)(addr_reg_rep[i]) = data_reg_rep[i];
	}
	
	//putstr("initfromflash\r\n");

	*(volatile uint32_t *)(0x80003048) = 0x00000f04; /* bypass the buck */
	for(i=0; i<0x161a8; i++) { continue; } /* wait for the bypass to take */
//	while((((*(volatile uint32_t *)(0x80003018))>>17) & 1) !=1) { continue; } /* wait for the bypass to take */
	*(volatile uint32_t *)(0x80003048) = 0x00000fa4; /* start the regulators */
	for(i=0; i<0x161a8; i++) { continue; } /* wait for the bypass to take */

	init_from_flash(0x1F000);

	putstr("ram_values:\r\n");
	for(i=0; i<4; i++) {
        putstr("  0x");
        put_hex32(ram_values[i]);
        putstr(NEWLINE);
	}

        putstr("radio_init: ctov parameter 0x");
        put_hex32(ram_values[3]);
        putstr(NEWLINE);
        for(i=0; i<16; i++) {
                ctov[i] = get_ctov(i,ram_values[3]);
                putstr("radio_init: ctov[0x");
                put_hex32(i);
                putstr("] = 0x");
                put_hex32(ctov[i]);
                putstr(NEWLINE);
        }


}

const uint32_t PSMVAL[19] = {
	0x0000080f,
	0x0000080f,
	0x0000080f,
	0x0000080f,
	0x0000081f,
	0x0000081f,
	0x0000081f,
	0x0000080f,
	0x0000080f,
	0x0000080f,
	0x0000001f,
	0x0000000f,
	0x0000000f,
	0x00000816,
	0x0000001b,
	0x0000000b,
	0x00000802,
	0x00000817,
	0x00000003,
};

const uint32_t PAVAL[19] = {
	0x000022c0,
	0x000022c0,
	0x000022c0,
	0x00002280,
	0x00002303,
	0x000023c0,
	0x00002880,
	0x000029f0,
	0x000029f0,
	0x000029f0,
	0x000029c0,
	0x00002bf0,
	0x000029f0,
	0x000028a0,
	0x00002800,
	0x00002ac0,
	0x00002880,
	0x00002a00,
	0x00002b00,
};

const uint32_t AIMVAL[19] = {
	0x000123a0,
	0x000163a0,
	0x0001a3a0,
	0x0001e3a0,
	0x000223a0,
	0x000263a0,
	0x0002a3a0,
	0x0002e3a0,
	0x000323a0,
	0x000363a0,
	0x0003a3a0,
	0x0003a3a0,
	0x0003e3a0,
	0x000423a0,
	0x000523a0,
	0x000423a0,
	0x0004e3a0,
	0x0004e3a0,
	0x0004e3a0,
};

#define RF_REG 0x80009400
void set_demodulator_type(uint8_t demod) {
	uint32_t val = reg(RF_REG);
	if(demod == DEMOD_NCD) {
		val = (val & ~1);
	} else {
		val = (val | 1);
	}
	reg(RF_REG) = val;
}

/* tested and seems to be good */
#define ADDR_POW1 0x8000a014
#define ADDR_POW2 ADDR_POW1 + 12
#define ADDR_POW3 ADDR_POW1 + 64
void set_power(uint8_t power) {
	safe_irq_disable(MACA);

	reg(ADDR_POW1) = PSMVAL[power];

/* see http://devl.org/pipermail/mc1322x/2009-October/000065.html */
/*	reg(ADDR_POW2) = (ADDR_POW1>>18) | PAVAL[power]; */
#ifdef USE_PA
	reg(ADDR_POW2) = 0xffffdfff & PAVAL[power]; /* single port */
#else
	reg(ADDR_POW2) = 0x00002000 | PAVAL[power]; /* dual port */
#endif

	reg(ADDR_POW3) = AIMVAL[power];
	
	irq_restore();
}

const uint8_t VCODivI[16] = {
	0x2f,
	0x2f,
	0x2f,
	0x2f,
	0x2f,
	0x2f,
	0x2f,
	0x2f,
	0x30,
	0x30,
	0x30,
	0x2f,
	0x30,
	0x30,
	0x30,
	0x30,
};

const uint32_t VCODivF[16] = {
	0x00355555,
	0x006aaaaa,
	0x00a00000,
	0x00d55555,
	0x010aaaaa,
	0x01400000,
	0x01755555,
	0x01aaaaaa,
	0x01e00000,
	0x00155555,
	0x004aaaaa,
	0x00800000,
	0x00b55555,
	0x00eaaaaa,
	0x01200000,
	0x01555555,		
};

/* tested good */
#define ADDR_CHAN1 0x80009800
#define ADDR_CHAN2 (ADDR_CHAN1+12)
#define ADDR_CHAN3 (ADDR_CHAN1+16)
#define ADDR_CHAN4 (ADDR_CHAN1+48)
void set_channel(uint8_t chan) {
	volatile uint32_t tmp;
	safe_irq_disable(MACA);

	tmp = reg(ADDR_CHAN1);
	tmp = tmp & 0xbfffffff;
	reg(ADDR_CHAN1) = tmp;

	reg(ADDR_CHAN2) = VCODivI[chan];
	reg(ADDR_CHAN3) = VCODivF[chan];

	tmp = reg(ADDR_CHAN4);
	tmp = tmp | 2;
	reg(ADDR_CHAN4) = tmp;

	tmp = reg(ADDR_CHAN4);
	tmp = tmp | 4;
	reg(ADDR_CHAN4) = tmp;

	tmp = tmp & 0xffffe0ff;
	tmp = tmp | (((ctov[chan])<<8)&0x1F00);
	reg(ADDR_CHAN4) = tmp;
	/* duh! */
	irq_restore();
}

#define ROM_END 0x0013ffff
#define ENTRY_EOF 0x00000e0f
/* processes up to 4 words of initialization entries */
/* returns the number of words processed */
uint32_t exec_init_entry(volatile uint32_t *entries, uint8_t *valbuf) 
{
	volatile uint32_t i;
	if(entries[0] <= ROM_END) {
		if (entries[0] == 0) {
			/* do delay command*/
			putstr("init_entry: delay 0x");
            put_hex32(entries[1]);
            putstr(NEWLINE);
			for(i=0; i<entries[1]; i++) { continue; }
			return 2;
		} else if (entries[0] == 1) {
			/* do bit set/clear command*/
			putstr("init_entry: bit set clear 0x");
            put_hex32(entries[1]);
            putstr(" 0x");
            put_hex32(entries[2]);
            putstr(" 0x");
            put_hex32(entries[3]);
            putstr(NEWLINE);
			reg(entries[2]) = (reg(entries[2]) & ~entries[1]) | (entries[3] & entries[1]);
			return 4;
		} else if ((entries[0] >= 16) &&
			   (entries[0] < 0xfff1)) {
			/* store bytes in valbuf */
            putstr("init_entry: store in valbuf 0x");
            put_hex32(entries[1]);
            putstr(" position 0x");
            put_hex32((entries[0] >> 4)-1);
            putstr(NEWLINE);
			valbuf[(entries[0]>>4)-1] = entries[1];
			return 2;
		} else if (entries[0] == ENTRY_EOF) {
			putstr("init_entry: eof ");
			return 0;
		} else {
			/* invalid command code */
			putstr("init_entry: invaild code ");
            put_hex32(entries[0]);
            putstr(NEWLINE);
			return 0;
		}
	} else { /* address isn't in ROM space */   
		 /* do store value in address command  */
		putstr("init_entry: address value pair - *0x");
        put_hex32(entries[0]);
        putstr(" = 0x");
        put_hex32(entries[1]);
        putstr(NEWLINE);
		reg(entries[0]) = entries[1];
		return 2;
	}
}


#define FLASH_INIT_MAGIC 0x00000abc
uint32_t init_from_flash(uint32_t addr) {
	nvmType_t type=0;
	nvmErr_t err;	
	volatile uint32_t buf[8];
	volatile uint32_t len;
	volatile uint32_t i=0,j;

	err = nvm_detect(gNvmInternalInterface_c, &type);
	putstr("nvm_detect returned type 0x");
    put_hex32(type);
    putstr(" err 0x");
    put_hex32(err);
    putstr(NEWLINE);
		
	nvm_setsvar(0);
	err = nvm_read(gNvmInternalInterface_c, type, (uint8_t *)buf, addr, 8);
	i+=8;
	putstr("nvm_read returned: 0x");
    put_hex32(err);
	
	for(j=0; j<4; j++) {
        putstr("0x");
        put_hex32(buf[j]);
        putstr(NEWLINE);
	}

	if(buf[0] == FLASH_INIT_MAGIC) {
		len = buf[1] & 0x0000ffff;
		while(i < (len-4)) {
			err = nvm_read(gNvmInternalInterface_c, type, (uint8_t *)buf, addr+i, 32);
			i += 4*exec_init_entry(buf, ram_values);
		}
		return i;
	} else {
		return 0;
	}
 	
}

/* 
 * Do the ABORT-Wait-NOP-Wait sequence in order to prevent MACA malfunctioning.
 * This seqeunce is synchronous and no interrupts should be triggered when it is done.
 */
void ResumeMACASync(void)
{ 
  volatile uint32_t clk, TsmRxSteps, LastWarmupStep, LastWarmupData, LastWarmdownStep, LastWarmdownData;
//  bool_t tmpIsrStatus;
  volatile uint32_t i;
  safe_irq_disable(MACA);

//  ITC_DisableInterrupt(gMacaInt_c);  
//  AppInterrupts_ProtectFromMACAIrq(tmpIsrStatus); <- Original from MAC code, but not sure how is it implemented

  /* Manual TSM modem shutdown */

  /* read TSM_RX_STEPS */
  TsmRxSteps = (*((volatile uint32_t *)(0x80009204)));
 
  /* isolate the RX_WU_STEPS */
  /* shift left to align with 32-bit addressing */
  LastWarmupStep = (TsmRxSteps & 0x1f) << 2;
  /* Read "current" TSM step and save this value for later */
  LastWarmupData = (*((volatile uint32_t *)(0x80009300 + LastWarmupStep)));

  /* isolate the RX_WD_STEPS */
  /* right-shift bits down to bit 0 position */
  /* left-shift to align with 32-bit addressing */
  LastWarmdownStep = ((TsmRxSteps & 0x1f00) >> 8) << 2;
  /* write "last warmdown data" to current TSM step to shutdown rx */
  LastWarmdownData = (*((volatile uint32_t *)(0x80009300 + LastWarmdownStep)));
  (*((volatile uint32_t *)(0x80009300 + LastWarmupStep))) = LastWarmdownData;

  /* Abort */
  MACA_WRITE(maca_control, 1);
  
  /* Wait ~8us */
  for (clk = maca_clk, i = 0; maca_clk - clk < 3 && i < 300; i++)
    ;
 
  /* NOP */
  MACA_WRITE(maca_control, 0);  
  
  /* Wait ~8us */  
  for (clk = maca_clk, i = 0; maca_clk - clk < 3 && i < 300; i++)
    ;
   

  /* restore original "last warmup step" data to TSM (VERY IMPORTANT!!!) */
  (*((volatile uint32_t *)(0x80009300 + LastWarmupStep))) = LastWarmupData;

  /* Clear all MACA interrupts - we should have gotten the ABORT IRQ */
  *MACA_CLRIRQ = 0xffff;

//  AppInterrupts_UnprotectFromMACAIrq(tmpIsrStatus);  <- Original from MAC code, but not sure how is it implemented
//  ITC_EnableInterrupt(gMacaInt_c);
//  enable_irq(MACA);
  irq_restore();

}
