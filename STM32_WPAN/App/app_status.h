/*
 * app_status.h
 *
 *  Created on: Jan 10, 2025
 *      Author: donny
 */

#ifndef APP_APP_STATUS_H_
#define APP_APP_STATUS_H_

#include "main.h"

typedef enum {
    STATUS_POWER = 3,
    STATUS_ENCODER = 4,
    STATUS_AUTO = 5,
    STATUS_NIGHT = 6,
    STATUS_SOURCE = 7,
} status_index_e;

uint8_t* StatusChange(status_index_e index, uint8_t event);
void StatusInit(void);

#endif /* APP_APP_STATUS_H_ */
