#include "wm.h"

static bool coin_valid(uint16_t c)
{
    return (c == 10u) || (c == 20u) || (c == 50u);
}

static void update_outputs(wm_t *w)
{
    switch (w->state) {
    case ST_IDLE:    w->rled = true;  w->bled = false;
                     w->motor = false; break;
    case ST_READY:   w->rled = false; w->bled = true;
                     w->motor = false; break;
    case ST_RUNNING: w->rled = false; w->bled = w->blink_on;
                     w->motor = true;  break;
    case ST_PAUSED:  w->rled = false; w->bled = true;
                     w->motor = false; break;
    case ST_ERROR:   w->rled = w->blink_on; w->bled = false;
                     w->motor = false; break;
    }
}

static void enter(wm_t *w, wm_state_t s)
{
    w->state = s;
    w->stop_cnt = 0u;
    w->stop_win = 0u;
    w->blink_cnt = 0u;
    w->blink_on = true;
    update_outputs(w);
}

static void terminate(wm_t *w)
{
    w->remain = 0u;
    w->credit = 0u;
    enter(w, ST_IDLE);
}

static void stop_pressed(wm_t *w)
{
    w->stop_cnt++;
    if (w->stop_cnt >= 2u) {
        terminate(w);                 /* forced stop */
    } else {
        w->stop_win = WM_STOP_WINDOW_TICKS;
    }
}

void wm_init(wm_t *w)
{
    w->credit = 0u;
    w->remain = 0u;
    enter(w, ST_IDLE);
}

void wm_event(wm_t *w, wm_event_t ev, uint16_t coin_cents)
{
    switch (w->state) {
    case ST_IDLE:
    case ST_READY:
        if (ev == EV_COIN) {
            if (!coin_valid(coin_cents)) {
                enter(w, ST_ERROR);
            } else {
                if (w->credit <= WM_CREDIT_MAX - coin_cents) {
                    w->credit += coin_cents;
                }
                if (w->state == ST_IDLE &&
                    w->credit >= WM_PRICE_CENTS) {
                    enter(w, ST_READY);
                }
            }
        } else if (ev == EV_RUN && w->state == ST_READY) {
            w->credit = 0u;               /* no change returned */
            w->remain = WM_CYCLE_TICKS;   /* start 30-min timer */
            enter(w, ST_RUNNING);
        }
        break;

    case ST_RUNNING:
        if (ev == EV_PAUSE)     { enter(w, ST_PAUSED); }
        else if (ev == EV_STOP) { stop_pressed(w); }
        break;

    case ST_PAUSED:
        if (ev == EV_RUN)       { enter(w, ST_RUNNING); }
        else if (ev == EV_STOP) { stop_pressed(w); }
        break;

    case ST_ERROR:
        if (ev == EV_STOP)      { terminate(w); }
        break;
    }
}

void wm_tick(wm_t *w)
{
    if (w->state == ST_RUNNING || w->state == ST_PAUSED) {
        w->remain--;                      /* timer keeps counting */
        if (w->remain == 0u) {
            terminate(w);                 /* cycle finished */
            return;
        }
        if (w->stop_win > 0u) {
            w->stop_win--;
            if (w->stop_win == 0u) { w->stop_cnt = 0u; }
        }
    }
    if (w->state == ST_RUNNING || w->state == ST_ERROR) {
        w->blink_cnt++;
        if (w->blink_cnt >= WM_BLINK_TICKS) {
            w->blink_cnt = 0u;
            w->blink_on = !w->blink_on;
        }
    }
    update_outputs(w);
}
