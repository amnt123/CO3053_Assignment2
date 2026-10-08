#ifndef WM_H
#define WM_H

#include <stdint.h>
#include <stdbool.h>

#define WM_TICK_MS            10u
#define WM_PRICE_CENTS        50u
#define WM_CREDIT_MAX         9999u
#define WM_CYCLE_TICKS        180000UL  /* 30 min = 1800 s / 10 ms  */
#define WM_BLINK_TICKS        50u       /* 500 ms half period (1 Hz) */
#define WM_STOP_WINDOW_TICKS  300u      /* 3 s between two STOP presses */

typedef enum { ST_IDLE, ST_READY, ST_RUNNING,
               ST_PAUSED, ST_ERROR } wm_state_t;
typedef enum { EV_RUN, EV_PAUSE, EV_STOP, EV_COIN } wm_event_t;

typedef struct {
    wm_state_t state;
    uint16_t   credit;     /* money inserted, in cents          */
    uint32_t   remain;     /* ticks left in the washing cycle   */
    uint8_t    stop_cnt;   /* STOP presses seen in this window  */
    uint16_t   stop_win;   /* ticks left in the STOP window     */
    uint8_t    blink_cnt;
    bool       blink_on;
    bool       rled, bled, motor;   /* outputs */
} wm_t;

void wm_init(wm_t *w);
void wm_event(wm_t *w, wm_event_t ev, uint16_t coin_cents);
void wm_tick(wm_t *w);   /* call every WM_TICK_MS */

#endif
