/*
 * app_works.h
 *
 *  Created on: Jan 8, 2025
 *      Author: donny
 */

#ifndef APP_APP_WORKS_H_
#define APP_APP_WORKS_H_

#include "main.h"

#include "openthread_api_wb.h"

typedef enum {
    BUTTON_STEPDOWN = 0,
    BUTTON_STEPUP,
    BUTTON_ONOFF,
    BUTTON_MODE,
} button_e;

typedef enum {
    PARENT_REQ_NOTHING = 0,
    PARENT_REQ_ATTACH,
    PARENT_REQ_DETACH,
} parent_req_e;

void WorkButtonCommand(otInstance *aInstance, button_e numButton);
void WorkBecameChild(otInstance *aInstance);
void WorkChangeParent(otInstance *aInstance, parent_req_e req);
void WorkPrintDeviceState(otInstance *aInstance);
void WorkInit(otInstance *aInstance);

#endif /* APP_APP_WORKS_H_ */
