#pragma once
#include <stdint.h>

void ws_start(void);

void ws_send_raw_begin(void);
void ws_send_raw_packet(int16_t *samples, int count);
void ws_send_raw_finish(void);
