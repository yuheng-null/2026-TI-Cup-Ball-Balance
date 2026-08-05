#ifndef __INCONTROL_H
#define __INCONTROL_H

void IRremote_Init(void);
uint8_t IRremote_Counttime(void);

extern uint32_t IR_Receivecode;
extern uint8_t  IR_Receiveflag;

#endif
