/*
 * Copyright (c) 2016 Leonid Yegoshin
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include    "tinyVP.h"
#include    "mipsasm.h"
#include    "mips.h"
#include    "irq.h"
#include    "ic.h"
#include    "time.h"
#include    "thread.h"
#include    <limits.h>

/* TEMPORARY */
#define cpu_get_frequency() (200000000)

// static unsigned int time_clock_adjust_current;
static unsigned int clock_time_multiplier;
volatile unsigned long long current_lcount;
unsigned long long initial_wall_time;
volatile unsigned long long current_wall_time;

int need_time_update = 0;

static struct timer timer_array[MAX_NUM_THREAD + 1];
static struct timer timerQ = {
    .next = &timerQ,
};


static unsigned long long time_get_lcount(void)
{
	unsigned int count;

	count  = read_cp0_count();
//        count -= time_clock_adjust_current;

	return time_extend_count(current_lcount,count);
}

static void time_set_timer(struct timer *timer)
{
	unsigned long long lcnt;
	unsigned long long lcount;

	lcnt = time_get_lcount();
	lcount = timer->lcount;
	if ((long long)(lcount - lcnt) < 100)
		lcount = lcnt + 100;
	write_cp0_compare((unsigned int)lcount /* + time_clock_adjust_current */);
	ehb();
}

void time_update_wall_time(void)
{
	unsigned int ie;
	unsigned long long lclock;
	unsigned long long lclock_delta;
	/* minimum CPU frequency is around 20MHz to avoid exceeding 32bits */
	unsigned long long time_delta;

	im_up(ie);
	lclock = time_get_lcount();
	lclock_delta = lclock - current_lcount;
	current_lcount = lclock;
	time_delta = lclock_delta * clock_time_multiplier;
	current_wall_time = current_wall_time + time_delta;
	need_time_update = 0;
	im_down(ie);
}

static void time_insert_timer(struct timer *timer)
{
	struct timer *tmr;

	if (timer->flag & TIMER_FLAG_ACTIVE ) {
		timer->prev->next = timer->next;
		timer->next->prev = timer->prev;
	}

	if (timer->flag & TIMER_FLAG_COUNT) {
		timer->time = timer->lcount * clock_time_multiplier - initial_wall_time;
	} else
		timer->lcount = (timer->time + initial_wall_time + clock_time_multiplier - 1) / clock_time_multiplier;

	timer->flag |= TIMER_FLAG_ACTIVE;
	if (timerQ.next == &timerQ) {
		timerQ.next = timer;
		timerQ.prev = timer;
		timer->next = &timerQ;
		timer->prev = &timerQ;

		return;
	}

	for (tmr = timerQ.next; tmr != &timerQ; tmr = tmr->next) {
		if (tmr->time > timer->time) {
			timer->next = tmr;
			timer->prev = tmr->prev;
			timer->prev->next = timer;
			tmr->prev = timer;

			return;
		}
	}
	timer->next = tmr;
	timer->prev = tmr->prev;
	timer->prev->next = timer;
	tmr->prev = timer;
}

static void handle_event(struct exception_frame *exfr, struct timer *timer)
{
	void (*func)(void);

	if (timer->flag & TIMER_FLAG_TICK) {
		timer->cnt = 0;
		/* call to scheduler */
		reschedule(exfr);
	} else {
		if (timer->flag & TIMER_FLAG_FUNC) {
			func = (void *)timer->cnt;
			func();
			return;
		}
		if (timer->flag & TIMER_FLAG_TIMER_IRQ) {
			execute_timer_IRQ(exfr, timer - timer_array);
			return;
		}

		/* call to guest scheduler or whatever */
	}
}

void timer_request(struct timer *timer, unsigned long long delay,
		   void (*func)(void))
{
	unsigned int ie;

	if (need_time_update)
		time_update_wall_time();
	timer->time = current_wall_time + delay;
	if (func) {
		timer->flag |= TIMER_FLAG_FUNC;
		timer->flag &= ~TIMER_FLAG_COUNT;
		timer->cnt = (unsigned int)func;
	}
	im_up(ie);
	time_insert_timer(timer);
	time_set_timer(timerQ.next);
	im_down(ie);
}

void timer_g_irq_reschedule(unsigned int timerno, unsigned long long clock)
{
	unsigned int ie;
	struct timer *timer;

	if (need_time_update)
		time_update_wall_time();
	im_up(ie);
	timer = &timer_array[timerno];
	timer->flag |= TIMER_FLAG_TIMER_IRQ|TIMER_FLAG_COUNT;
	timer->lcount = clock /* - time_clock_adjust_current */;
	time_insert_timer(timer);
	time_set_timer(timerQ.next);
	im_down(ie);
}

extern struct timer tmp_timer;

int time_irq(struct exception_frame *exfr, unsigned int irq)
{
	struct timer *timer;
int many = 0;
	if (irq != _TIMER_IRQ)
		return 0;

	/* IRQ disabled here... */

	irq_ack(irq);
	time_update_wall_time();
	for (timer = timerQ.next; timer != &timerQ; timer = timer->next) {
		if (timer->time > current_wall_time) {
if (!many) {
// Abnormal situation: unexpected CP0 COMPARE IRQ.
// It usually happens on PIC32MZEF if guest plays with Guest CP0 COMPARE
// because SoC has a signal buffer for CP0 COMPARE IRQ vs a plain MIPS CPU.
// However, it happens with time keeping bugs too.
int i;
unsigned gcount = read_g_cp0_count();
unsigned gcompare = read_g_cp0_compare();;
printf("time_irq: [%d] timer->time=%llx lcount=%llx current_wall_time=%llx current_lcount=%llx\n",timer-timer_array,timer->time,timer->lcount,current_wall_time,current_lcount);
printf("\tCOUNT=%08x COMPARE=%08x timerQ=%08x next=%08x prev=%08x G.COUNT=%08x G.COMPARE=%08x\n",read_cp0_count(),read_cp0_compare(),(void *)&timerQ, timerQ.next, timerQ.prev,gcount,gcompare);
for (i=0; i <= MAX_NUM_THREAD; i++)
    printf("\t%08x: time=%llx lcount=%llx flag=%08x cnt=%d; next=%08x prev=%08x\n",
(void *)&timer_array[i],timer_array[i].time, timer_array[i].lcount, timer_array[i].flag, timer_array[i].cnt, timer_array[i].next, timer_array[i].prev);
    printf("\t%08x: time=%llx lcount=%llx flag=%08x cnt=%d; next=%08x prev=%08x\n",
(void *)&tmp_timer,tmp_timer.time, tmp_timer.lcount, tmp_timer.flag, tmp_timer.cnt, tmp_timer.next, tmp_timer.prev);
}
			break;
		}
many = 1;
		timer->prev->next = timer->next;
		timer->next->prev = timer->prev;
		timer->flag &= ~TIMER_FLAG_ACTIVE;
		if (timer->flag & TIMER_FLAG_TICK) {
			timer->time = timer->time + TIMER_TICK;
			while (timer->time <= (current_wall_time + 1)) {
				timer->time = timer->time + TIMER_TICK;
				timer->cnt++;
			}
			time_insert_timer(timer);
		}

		handle_event(exfr, timer);
	};
	if (timerQ.next->flag & TIMER_FLAG_ACTIVE)
		time_set_timer(timerQ.next);

	return 1;
}

int init_time()
{
	unsigned int freq;
	unsigned int multiplier;
	struct timer *timer;

	/* IRQ disabled here... */

	freq = cpu_get_frequency();

	__asm__ __volatile__("rdhwr %0, $3": "=&r"(multiplier):);

	clock_time_multiplier = TIME_SECOND / (freq / multiplier);

	timer = &timer_array[0];
	current_lcount = 0;
	current_lcount = time_get_lcount();
	initial_wall_time = current_lcount * clock_time_multiplier;

	timer->time = TIMER_TICK;
	timer->flag |= TIMER_FLAG_TICK;
	timerQ.next = &timerQ;
	time_insert_timer(timer);
	clear_cp0_cause(CP0_CAUSE_DC);
	time_set_timer(timerQ.next);

	irq_set_prio_and_unmask(_TIMER_IRQ, _TIMER_PRIO);

	return 1;
}

void write_time_values(void)
{
    char str[128];

    sprintf(str, "clock_time_multiplier=%x, initial_wall_time=%T current_lcount=%llx\n",
	clock_time_multiplier, initial_wall_time, current_lcount);
    uart_writeline(console_uart, str);
}
