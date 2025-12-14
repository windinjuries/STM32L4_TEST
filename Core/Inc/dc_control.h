/**
  * @file           : dc_control.h
  * @brief          : Header for dc_control.c
  */

#ifndef __DC_CONTROL_H
#define __DC_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void TC214B_Forward(uint8_t speed);
void TC214B_Stop(void);
void TC214B_Reverse(uint8_t speed);
void StartTC214BTask(void const * argument);

#ifdef __cplusplus
}
#endif

#endif /* __DC_CONTROL_H */