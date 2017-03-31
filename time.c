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

#include    "mipsasm.h"
#include    "mips.h"
#include    "irq.h"
#include    "ic.h"
#include    "time.h"
#include    <limits.h>

/* TEMPORARY */
#define cpu_get_frequency() (200000000)

static unsigned int time_clock_adjust_current;
static unsigned int clock_time_multiplier;
static unsigned int current_clock;
unsigned long long initial_wall_time;
volatile unsigned long long current_wall_time;

static struct timer timer_array[MAX_NUM_GUEST + 1];
static struct timer timerQ = {
    .next = &timerQ,
};

unsigned long long time_convert_clocks(unsigned int clock)
{
	return ((unsigned long long)clock) * clock_time_multiplier;
}

static unsigned int time_get_clock(void)
{
	unsigned int ie;
	unsigned int clock;

	im_up(ie);
	clock  = read_cp0_count();
	clock -= time_clock_adjust_current;
	im_down(ie);

	return clock;
}

static unsigned int time_set_timer(struct timer *timer)
{
	unsigned int ie;
	unsigned int cnt;
	unsigned int clock;

	im_up(ie);
	clock = timer->clock + time_clock_adjust_current;
	cnt    = read_cp0_count();
	if ((int)(clock - cnt) < 50)
		clock = cnt + 50;
	write_cp0_compare(clock);
	ehb();
	im_down(ie);

	return clock;
}

static void time_update_wall_time(void)
{
	unsigned int clock;
	unsigned int clock_delta;
	/* minimum CPU frequency is around 20MHz to avoid exceeding 32bits */
	unsigned long long time_delta;

	clock = time_get_clock();
	clock_delta = clock - current_clock;
	current_clock = clock;
	time_delta = (unsigned long long)clock_delta * clock_time_multiplier;
	current_wall_time = current_wall_time + time_delta;
}

static void time_insert_timer(struct timer *timer)
{
	struct timer *tmr;

	if (timer->flag & TIMER_FLAG_ACTIVE ) {
		timer->prev->next = timer->next;
		timer->next->prev = timer->prev;
	}

	timer->clock = (timer->time + initial_wall_time + clock_time_multiplier - 1) / clock_time_multiplier;

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
		if (++timer->count >= TIMER_TICKS_SECOND) {
			timer->count = 0;
			/* call to scheduler */
			reschedule(exfr);
		}
	} else {
		if (timer->flag & TIMER_FLAG_FUNC) {
			func = (void *)timer->count;
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

	timer->time = current_wall_time + delay;
	if (func) {
		timer->flag |= TIMER_FLAG_FUNC;
		timer->count = (unsigned int)func;
	}
	im_up(ie);
	time_insert_timer(timer);
	time_set_timer(timerQ.next);
	im_down(ie);
}

void timer_g_irq_reschedule(unsigned int timerno, unsigned int clock)
{
	struct timer *timer;
	unsigned long long delay;

	timer = &timer_array[timerno];
	timer->flag |= TIMER_FLAG_TIMER_IRQ;
	time_update_wall_time();
	clock = clock - time_clock_adjust_current - current_clock;
	// clock now a delta in internal counts
	delay = (unsigned long long)clock * clock_time_multiplier;
	timer->time = current_wall_time + delay;
	time_insert_timer(timer);
	time_set_timer(timerQ.next);
}

int time_irq(struct exception_frame *exfr, unsigned int irq)
{
	struct timer *timer;
int many = 0;
	if (irq != _TIMER_IRQ)
		return 0;

	irq_ack(irq);
	time_update_wall_time();
	for (timer = timerQ.next; timer != &timerQ; timer = timer->next) {
		if (timer->time > current_wall_time) {
if (!many) {
printf("time_irq: timer->time=%llx current_wall_time=%llx\n",timer->time,current_wall_time);
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
				timer->count++;
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

	freq = cpu_get_frequency();

	__asm__ __volatile__("rdhwr %0, $3": "=&r"(multiplier):);

	clock_time_multiplier = TIME_SECOND / (freq / multiplier);

	timer = &timer_array[0];
	current_clock = time_get_clock();
	initial_wall_time = (unsigned long long)current_clock * clock_time_multiplier;

	timer->time = TIMER_TICK;
	timer->flag |= TIMER_FLAG_TICK;
	timerQ.next = &timerQ;
	time_insert_timer(timer);
	clear_cp0_cause(CP0_CAUSE_DC);
	time_set_timer(timerQ.next);

	irq_set_prio_and_unmask(_TIMER_IRQ, _TIMER_PRIO);

	return 1;
}
