#include "wm.h"
#include "hal.h"

typedef struct { bool stable; uint8_t cnt; } deb_t;

/* returns true once, on a debounced press (30 ms stable) */
static bool pressed_edge(deb_t *d, bool raw)
{
    bool edge = false;
    if (raw == d->stable) {
        d->cnt = 0u;
    } else if (++d->cnt >= 3u) {
        d->stable = raw;
        d->cnt = 0u;
        edge = raw;
    }
    return edge;
}

int main(void)
{
    wm_t wm;
    deb_t b_run = {false, 0u}, b_pause = {false, 0u};
    deb_t b_stop = {false, 0u};
    uint16_t coin;

    hal_init();
    wm_init(&wm);
    for (;;) {
        hal_wait_tick_10ms();
        if (pressed_edge(&b_run, hal_btn_run()))
            wm_event(&wm, EV_RUN, 0u);
        if (pressed_edge(&b_pause, hal_btn_pause()))
            wm_event(&wm, EV_PAUSE, 0u);
        if (pressed_edge(&b_stop, hal_btn_stop()))
            wm_event(&wm, EV_STOP, 0u);
        coin = hal_coin_poll();
        if (coin != 0u)
            wm_event(&wm, EV_COIN, coin);
        wm_tick(&wm);
        hal_set_rled(wm.rled);
        hal_set_bled(wm.bled);
        hal_set_motor(wm.motor);
    }
}
