#ifndef HAL_H
#define HAL_H

#include <stdint.h>
#include <stdbool.h>

void     hal_init(void);
void     hal_wait_tick_10ms(void);  /* blocks until timer flag set */
bool     hal_btn_run(void);         /* true while pressed          */
bool     hal_btn_pause(void);
bool     hal_btn_stop(void);
uint16_t hal_coin_poll(void);       /* 0 = none, else value (cents) */
void     hal_set_rled(bool on);
void     hal_set_bled(bool on);
void     hal_set_motor(bool on);

#endif
