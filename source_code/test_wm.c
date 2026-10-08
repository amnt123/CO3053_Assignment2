#include <stdio.h>
#include <string.h>
#include "wm.h"

static int fails = 0, total = 0;
static const char *SN[] = {"IDLE","READY","RUNNING","PAUSED","ERROR"};

static void run_ms(wm_t *w, unsigned ms)
{ for (unsigned i = 0; i < ms / WM_TICK_MS; i++) wm_tick(w); }

static void report(const char *id, int ok, const char *fmt, ...);
#include <stdarg.h>
static void report(const char *id, int ok, const char *fmt, ...)
{
    char buf[200]; va_list a; va_start(a, fmt);
    vsnprintf(buf, sizeof buf, fmt, a); va_end(a);
    total++; if (!ok) fails++;
    printf("%s|%s|%s\n", id, buf, ok ? "PASS" : "FAIL");
}

int main(void)
{
    wm_t w;
    /* TC01 power-on */
    wm_init(&w);
    report("TC01", w.state==ST_IDLE && w.rled && !w.bled && w.credit==0,
      "state=%s RLED=%d BLED=%d credit=%u", SN[w.state], w.rled, w.bled, w.credit);

    /* TC02 less than 50 cents: RUN refused */
    wm_event(&w, EV_COIN, 10); wm_event(&w, EV_COIN, 20);
    wm_event(&w, EV_RUN, 0);
    report("TC02", w.state==ST_IDLE && w.credit==30 && w.rled,
      "state=%s credit=%u RLED=%d", SN[w.state], w.credit, w.rled);

    /* TC03 reaching 50 cents */
    wm_event(&w, EV_COIN, 20);
    report("TC03", w.state==ST_READY && w.credit==50 && w.bled && !w.rled,
      "state=%s credit=%u BLED=%d RLED=%d", SN[w.state], w.credit, w.bled, w.rled);

    /* TC04 surplus coins accepted while READY, then RUN clears all money */
    wm_event(&w, EV_COIN, 50);
    unsigned c_before = w.credit;
    wm_event(&w, EV_RUN, 0);
    report("TC04", c_before==100 && w.state==ST_RUNNING && w.credit==0 && w.remain==WM_CYCLE_TICKS,
      "credit before RUN=%u, after=%u, state=%s, timer=%lu ticks", c_before, w.credit, SN[w.state], (unsigned long)w.remain);

    /* TC05 BLED blinks at 1 Hz while running, motor on, RLED off */
    int toggles = 0; bool last = w.bled;
    for (int i = 0; i < 300; i++) { wm_tick(&w); if (w.bled != last) { toggles++; last = w.bled; } }
    report("TC05", toggles==6 && !w.rled && w.motor,
      "BLED toggles in 3 s=%d, RLED=%d, motor=%d", toggles, w.rled, w.motor);

    /* TC06 coin ignored while running */
    wm_event(&w, EV_COIN, 50);
    report("TC06", w.credit==0 && w.state==ST_RUNNING, "credit=%u state=%s", w.credit, SN[w.state]);

    /* TC07 PAUSE: timer keeps counting, BLED steady on */
    unsigned long r0 = w.remain;
    wm_event(&w, EV_PAUSE, 0);
    run_ms(&w, 60000);
    report("TC07", w.state==ST_PAUSED && w.bled && !w.motor && (r0 - w.remain)==6000,
      "state=%s BLED=%d motor=%d, ticks counted in 60 s of pause=%lu", SN[w.state], w.bled, w.motor, r0 - w.remain);

    /* TC08 RUN resumes */
    wm_event(&w, EV_RUN, 0);
    report("TC08", w.state==ST_RUNNING && w.motor, "state=%s motor=%d", SN[w.state], w.motor);

    /* TC09 automatic termination after exactly 30 min (counted from RUN) */
    {
        wm_t x; wm_init(&x); wm_event(&x, EV_COIN, 50); wm_event(&x, EV_RUN, 0);
        run_ms(&x, 30u*60u*1000u - 10u);
        int still = (x.state == ST_RUNNING);
        wm_tick(&x);
        report("TC09", still && x.state==ST_IDLE && x.rled && !x.bled && !x.motor,
          "running at 29:59.99=%d, after 30:00 state=%s RLED=%d motor=%d", still, SN[x.state], x.rled, x.motor);
    }

    /* TC10 timer expires while paused */
    {
        wm_t x; wm_init(&x); wm_event(&x, EV_COIN, 50); wm_event(&x, EV_RUN, 0);
        run_ms(&x, 10u*60u*1000u); wm_event(&x, EV_PAUSE, 0);
        run_ms(&x, 20u*60u*1000u);
        report("TC10", x.state==ST_IDLE && x.rled, "10 min run + 20 min pause -> state=%s RLED=%d", SN[x.state], x.rled);
    }

    /* TC11 STOP twice within window terminates */
    {
        wm_t x; wm_init(&x); wm_event(&x, EV_COIN, 50); wm_event(&x, EV_RUN, 0);
        run_ms(&x, 5000);
        wm_event(&x, EV_STOP, 0); int after1 = x.state==ST_RUNNING;
        run_ms(&x, 1000);
        wm_event(&x, EV_STOP, 0);
        report("TC11", after1 && x.state==ST_IDLE && x.rled && !x.motor,
          "after 1st STOP running=%d, after 2nd STOP state=%s motor=%d", after1, SN[x.state], x.motor);
    }

    /* TC12 single STOP does not stop the machine; window expires */
    {
        wm_t x; wm_init(&x); wm_event(&x, EV_COIN, 50); wm_event(&x, EV_RUN, 0);
        wm_event(&x, EV_STOP, 0); run_ms(&x, 4000);
        wm_event(&x, EV_STOP, 0);
        report("TC12", x.state==ST_RUNNING, "STOP, 4 s gap, STOP -> state=%s", SN[x.state]);
    }

    /* TC13 STOP twice while paused */
    {
        wm_t x; wm_init(&x); wm_event(&x, EV_COIN, 50); wm_event(&x, EV_RUN, 0);
        wm_event(&x, EV_PAUSE, 0); wm_event(&x, EV_STOP, 0); wm_event(&x, EV_STOP, 0);
        report("TC13", x.state==ST_IDLE, "PAUSED + 2xSTOP -> state=%s", SN[x.state]);
    }

    /* TC14 invalid coin -> error, RLED blinks, STOP clears */
    {
        wm_t x; wm_init(&x); wm_event(&x, EV_COIN, 20);
        wm_event(&x, EV_COIN, 100);
        int toggles = 0; bool last = x.rled;
        for (int i = 0; i < 300; i++) { wm_tick(&x); if (x.rled != last) { toggles++; last = x.rled; } }
        int err = x.state==ST_ERROR && !x.bled;
        wm_event(&x, EV_RUN, 0); int runIgn = x.state==ST_ERROR;
        wm_event(&x, EV_STOP, 0);
        report("TC14", err && toggles==6 && runIgn && x.state==ST_IDLE && x.credit==0 && x.rled,
          "ERROR=%d RLED toggles in 3 s=%d, RUN ignored=%d, after STOP state=%s credit=%u",
          err, toggles, runIgn, SN[x.state], x.credit);
    }

    /* TC15 several coin combinations reach READY */
    {
        wm_t x; wm_init(&x);
        wm_event(&x,EV_COIN,10); wm_event(&x,EV_COIN,10); wm_event(&x,EV_COIN,10);
        wm_event(&x,EV_COIN,10); int s4 = x.state==ST_IDLE;
        wm_event(&x,EV_COIN,10);
        report("TC15", s4 && x.state==ST_READY && x.credit==50,
          "4x10c -> IDLE=%d, 5x10c -> state=%s credit=%u", s4, SN[x.state], x.credit);
    }

    /* TC16 new cycle after finishing needs new money */
    {
        wm_t x; wm_init(&x); wm_event(&x, EV_COIN, 50); wm_event(&x, EV_RUN, 0);
        run_ms(&x, 30u*60u*1000u);
        wm_event(&x, EV_RUN, 0);
        report("TC16", x.state==ST_IDLE && x.credit==0, "RUN without money after a cycle -> state=%s credit=%u", SN[x.state], x.credit);
    }

    printf("SUMMARY|%d of %d passed|%s\n", total - fails, total, fails ? "FAIL" : "PASS");
    return fails;
}
