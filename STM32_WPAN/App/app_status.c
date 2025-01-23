/*
 * app_status.c
 *
 *  Created on: Jan 10, 2025
 *      Author: donny
 */

#include "app_status.h"

#define BL_N_STX 		    0xF0
#define BL_N_ETX 		    0xF7
#define BL_N_HEADER_REQ     0x10
#define BL_N_HEADER_RES     0x11
#define BL_N_DATA_LENGTH    5

uint8_t status[10] = {0};

uint8_t* StatusChange(status_index_e index, uint8_t event)
{
    switch (index) {
        case STATUS_POWER:
            if (status[STATUS_POWER] != 0) {
                status[STATUS_POWER] = 0;
            } else {
                status[STATUS_POWER] = 1;
            }
            status[STATUS_NIGHT] = 0;
        break;
        case STATUS_ENCODER:
            if (event != 0) {// step up
                if (status[STATUS_ENCODER] != 23) {
                    status[STATUS_ENCODER]++;
                }
            } else {// step down
                if (status[STATUS_ENCODER] > 1) {
                    status[STATUS_ENCODER]--;
                }
            }
            status[STATUS_POWER] = 1;
            status[STATUS_NIGHT] = 0;
        break;
        case STATUS_AUTO:
        break;
        case STATUS_NIGHT:
            if (status[STATUS_NIGHT] == 0) {
                status[STATUS_NIGHT] = 1;//morning
                status[STATUS_POWER] = 1;
            } else if (status[STATUS_NIGHT] == 1) {
                status[STATUS_NIGHT] = 2;//night
            } else if (status[STATUS_NIGHT] == 2) {
                status[STATUS_NIGHT] = 1;//morning
                status[STATUS_POWER] = 1;
            }
        break;
        case STATUS_SOURCE:
        break;
    }

    uint8_t chksum = 0;
    chksum = chksum^status[3];
    chksum = chksum^status[4];
    chksum = chksum^status[5];
    chksum = chksum^status[6];
    chksum = chksum^status[7];
    status[8] = chksum;
    return status;
}

void StatusInit(void)
{
    uint8_t chksum = 0;
    status[0] = BL_N_STX;
    status[1] = BL_N_HEADER_REQ;
    status[2] = BL_N_DATA_LENGTH;
    status[3] = 0;
    status[4] = 23;
    status[5] = 0;
    status[6] = 0;
    status[7] = 0;
    chksum = chksum^status[3];
    chksum = chksum^status[4];
    chksum = chksum^status[5];
    chksum = chksum^status[6];
    chksum = chksum^status[7];
    status[8] = chksum;
    status[9] = BL_N_ETX;
}