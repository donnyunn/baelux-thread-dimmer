/*
 * app_works.c
 *
 *  Created on: Jan 8, 2025
 *      Author: donny
 */

#include "app_works.h"

#include "dbg_trace.h"
#include "openthread_api_wb.h"
#include "stm_logging.h"
#include "app_conf.h"
#include "stm32_lpm.h"
#include "stm32_seq.h"
#include "shci.h"
#include "app_status.h"

#include "adc.h"

#define GROUP_ID 1

typedef struct
{
    uint8_t deviceType;
    uint8_t groupId;
    uint8_t uniqueId[8];
} GroupJoinRequest;

typedef struct
{
    uint8_t groupId;
    uint8_t uniqueId[8];
    uint8_t command[10];
} CommandRequest;

GroupJoinRequest joinRequest;
uint8_t switchId[8];
bool changingParent = false;

uint8_t led_timer_id;
void led_timer(void)
{
    LED_OFF(1);
    LED_OFF(2);
    LED_OFF(3);
}

// CoAP 리소스 초기화 함수 (필요한 경우 추가 요청 처리 가능)
void InitCoap(otInstance *aInstance)
{
    otCoapStart(aInstance, OT_DEFAULT_COAP_PORT);
    APP_DBG("CoAP initialized for GroupJoinRequest.");
}

// CoAP 응답 처리 핸들러
void HandleCoapResponse(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError)
{
    if (aError != OT_ERROR_NONE)
    {
        APP_DBG("Failed to receive CoAP response: %d", aError);
        /* Allow the 800_15_4 IP to enter in low power mode */
        SHCI_C2_RADIO_AllowLowPower(THREAD_IP,TRUE);
        return;
    }

    char buffer[128];
    int length = otMessageRead(aMessage, otMessageGetOffset(aMessage), buffer, sizeof(buffer) - 1);
    if (length >= 0)
    {
        buffer[length] = '\0'; // NULL 종료
        // APP_DBG("Received CoAP response: %s", buffer);
        APP_DBG("Received CoAP response: %d", length );
        for (int i = 0; i < length; i++) {
            APP_DBG("%02x", buffer[i]);
        }
    }
    else
    {
        APP_DBG("Failed to read CoAP response.");
    }
    /* Allow the 800_15_4 IP to enter in low power mode */
    SHCI_C2_RADIO_AllowLowPower(THREAD_IP,TRUE);
}

void SendGroupJoinRequest(otInstance *aInstance)
{
    otError error;
    otMessage *message;
    otMessageInfo messageInfo;
    otExtAddress extAddr;
    otRouterInfo parentInfo;

    /* Forbid the 800_15_4 IP to enter in low power mode.
    *
    * Reason for that...
    * As soon as the M0 is receiving an ot cmd, it wake up the IP 802_15_4,
    * send the command and put back the IP 802_15_4 in sleep mode when possible.
    * If the application send too much ot_cmd commands sequentially, the IP 802_15_4
    * will wake up and sleep at a non reasonable speed. It is why it is advised
    * to control when the IP 802_15_4 radio is allowed to enter in low power.
    */
    SHCI_C2_RADIO_AllowLowPower(THREAD_IP,FALSE);

    // 부모 정보 가져오기
    error = otThreadGetParentInfo(aInstance, &parentInfo);
    if (error != OT_ERROR_NONE)
    {
        APP_DBG("Failed to get parent info: %d", error);
        return;
    } else {
        APP_DBG("New Parent: Rloc16=0x%04X, ExtAddr=%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
                parentInfo.mRloc16,
                parentInfo.mExtAddress.m8[0], parentInfo.mExtAddress.m8[1],
                parentInfo.mExtAddress.m8[2], parentInfo.mExtAddress.m8[3],
                parentInfo.mExtAddress.m8[4], parentInfo.mExtAddress.m8[5],
                parentInfo.mExtAddress.m8[6], parentInfo.mExtAddress.m8[7]);
    }
    // 부모의 링크 로컬 주소로 설정
    otIp6Address destAddr;
    otIp6AddressFromString("fe80::", &destAddr);
    destAddr.mFields.m8[8] = parentInfo.mExtAddress.m8[0] ^ 0x02;
    destAddr.mFields.m8[9] = parentInfo.mExtAddress.m8[1];
    destAddr.mFields.m8[10] = parentInfo.mExtAddress.m8[2];
    destAddr.mFields.m8[11] = parentInfo.mExtAddress.m8[3];
    destAddr.mFields.m8[12] = parentInfo.mExtAddress.m8[4];
    destAddr.mFields.m8[13] = parentInfo.mExtAddress.m8[5];
    destAddr.mFields.m8[14] = parentInfo.mExtAddress.m8[6];
    destAddr.mFields.m8[15] = parentInfo.mExtAddress.m8[7];
    // memcpy(&destAddr.mFields.m8[8], parentInfo.mExtAddress.m8, sizeof(parentInfo.mExtAddress.m8));

    // GroupJoinRequest 데이터 채우기
    otLinkGetFactoryAssignedIeeeEui64(aInstance, &extAddr);
    joinRequest.deviceType = 1; // 1 = 스위치
    joinRequest.groupId = GROUP_ID;    // 요청 그룹 ID
    memcpy(joinRequest.uniqueId, extAddr.m8, sizeof(extAddr.m8));
    memcpy(switchId, joinRequest.uniqueId, 8);

    // CoAP 메시지 생성
    message = otCoapNewMessage(aInstance, NULL);
    if (message == NULL)
    {
        APP_DBG("Failed to allocate CoAP message.");
        return;
    }

    // CoAP 메시지 설정 (Confirmable, PUT 요청)
    otCoapMessageInit(message, OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_PUT);
    otCoapMessageAppendUriPathOptions(message, "group/join");
    otCoapMessageSetPayloadMarker(message);

    // GroupJoinRequest 데이터를 페이로드로 추가
    error = otMessageAppend(message, &joinRequest, sizeof(GroupJoinRequest));
    if (error != OT_ERROR_NONE)
    {
        APP_DBG("Failed to append GroupJoinRequest to CoAP message: %d", error);
        otMessageFree(message);
        return;
    }

    // 메시지 정보 설정
    memset(&messageInfo, 0, sizeof(messageInfo));
    // memcpy(&messageInfo.mPeerAddr, &parentInfo.mExtAddress, sizeof(parentInfo.mExtAddress));
    messageInfo.mPeerAddr = destAddr;
    messageInfo.mPeerPort = OT_DEFAULT_COAP_PORT; // CoAP 기본 포트

    // APP_DBG("Peer Addr: %02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X", 
    //                     messageInfo.mPeerAddr.mFields.m8[0], messageInfo.mPeerAddr.mFields.m8[1],
    //                     messageInfo.mPeerAddr.mFields.m8[2], messageInfo.mPeerAddr.mFields.m8[3],
    //                     messageInfo.mPeerAddr.mFields.m8[4], messageInfo.mPeerAddr.mFields.m8[5],
    //                     messageInfo.mPeerAddr.mFields.m8[6], messageInfo.mPeerAddr.mFields.m8[7],
    //                     messageInfo.mPeerAddr.mFields.m8[8], messageInfo.mPeerAddr.mFields.m8[9],
    //                     messageInfo.mPeerAddr.mFields.m8[10], messageInfo.mPeerAddr.mFields.m8[11],
    //                     messageInfo.mPeerAddr.mFields.m8[12], messageInfo.mPeerAddr.mFields.m8[13],
    //                     messageInfo.mPeerAddr.mFields.m8[14], messageInfo.mPeerAddr.mFields.m8[15]);

    // CoAP 요청 전송
    error = otCoapSendRequest(aInstance, message, &messageInfo, HandleCoapResponse, NULL);
    if (error != OT_ERROR_NONE)
    {
        APP_DBG("Failed to send CoAP request: %d", error);
        otMessageFree(message);
        return;
    }

    APP_DBG("Group join request sent to parent.");
}

void SendGroupCommand(otInstance *aInstance, uint8_t groupId, uint8_t* command)
{
    CommandRequest commandRequest;
    commandRequest.groupId = groupId;
    memcpy(commandRequest.uniqueId, switchId, 8);
    memcpy(commandRequest.command, command, sizeof(commandRequest.command));


    otMessage *message = otCoapNewMessage(aInstance, NULL);
    if (message == NULL) {
        APP_DBG("Failed to allocate CoAP message");
        return;
    }

    // CoAP 메시지 초기화
    otCoapMessageInit(message, OT_COAP_TYPE_NON_CONFIRMABLE, OT_COAP_CODE_PUT);
    otCoapMessageAppendUriPathOptions(message, "group/command"); // 버튼 명령 URI Path
    otCoapMessageSetPayloadMarker(message);
    otMessageAppend(message, &commandRequest, sizeof(CommandRequest));

    otMessageInfo messageInfo = {0};
    otIp6AddressFromString("ff03::2", &messageInfo.mPeerAddr);
    messageInfo.mPeerPort = OT_DEFAULT_COAP_PORT;

    SHCI_C2_RADIO_AllowLowPower(THREAD_IP,FALSE);

    otError error = otCoapSendRequest(aInstance, message, &messageInfo, NULL, NULL);
    if (error != OT_ERROR_NONE) {
        APP_DBG("Failed to send Command: %d", error);
        otMessageFree(message);
    } else {
        APP_DBG("Command send: GroupId=%d, Command=%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", 
                groupId,
                command[0], command[1], command[2], command[3], command[4],
                command[5], command[6], command[7], command[8], command[9]);
    }
    
    SHCI_C2_RADIO_AllowLowPower(THREAD_IP,TRUE);
}

void WorkNightmodeCheck(uint8_t mode)
{
    switch (mode) {
        case 1: // morning
            LED_ON(1);
        break;
        case 2: // night
            LED_ON(2);
        break;
    }
    HW_TS_Start(led_timer_id, TIMER_INTERVAL_SEC(1));
}

void WorkBatteryCheck(void)
{
    uint32_t value;
    uint32_t vout;

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    value = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    vout = 4915200 / value;
    APP_DBG("Battery: %dmV", vout);
    if (vout < 2500) {
        LED_ON(3);
        HW_TS_Start(led_timer_id, TIMER_INTERVAL_SEC(1));
    }
}

void WorkButtonCommand(otInstance *aInstance, button_e numButton)
{
    uint8_t* status;
    switch (numButton)
    {
        case BUTTON_STEPDOWN:
            status = StatusChange(STATUS_ENCODER, 0);
        break;
        case BUTTON_STEPUP:
            status = StatusChange(STATUS_ENCODER, 1);
        break;
        case BUTTON_MODE:
            status = StatusChange(STATUS_NIGHT, 0);
            WorkNightmodeCheck(status[STATUS_NIGHT]);
            UTIL_SEQ_SetTask(TASK_BATTERY_CHECK, CFG_SCH_PRIO_0);
        break;
        case BUTTON_ONOFF:
            status = StatusChange(STATUS_POWER, 0);
            UTIL_SEQ_SetTask(TASK_BATTERY_CHECK, CFG_SCH_PRIO_0);
        break;
    }
    SendGroupCommand(aInstance, GROUP_ID, status);
    // APP_DBG("%x %x %x %x %x %x %x %x %x %x", status[0], status[1], status[2], status[3], status[4], status[5], status[6], status[7], status[8], status[9]);
}

void WorkBecameChild(otInstance *aInstance)
{
    APP_DBG("%s", __func__);
    if (changingParent) {
        // InitCoap(aInstance);
        SendGroupJoinRequest(aInstance);
        changingParent = false;
    }
}

void WorkChangeParent(otInstance *aInstance)
{
    otError error;

    error = otThreadSetEnabled(aInstance, false);
    if (error != OT_ERROR_NONE) {
        APP_DBG("Failed to disable Thread network: %d", error);
        return;
    }
    APP_DBG("Thread network disabled");

    otLinkResetCounters(aInstance);

    error = otThreadSetEnabled(aInstance, true);
    if (error != OT_ERROR_NONE)
    {
        APP_DBG("Failed to re-enable Thread network: %d", error);
        return;
    }
    APP_DBG("Thread network re-enabled. Searching for a new parent...");
    changingParent = true;
}

void WorkPrintDeviceState(otInstance *aInstance)
{
  otDeviceRole role = otThreadGetDeviceRole(aInstance); // 디바이스 역할 가져오기
  uint16_t panId = otLinkGetPanId(aInstance);          // PAN ID 가져오기
  uint8_t channel = otLinkGetChannel(aInstance);       // 채널 가져오기
  otExtAddress extAddr;                                // 확장 주소 구조체
  otLinkGetFactoryAssignedIeeeEui64(aInstance, &extAddr);

  switch (role)
    {
    case OT_DEVICE_ROLE_DISABLED:
        APP_DBG("Device Role: Disabled");
        break;
    case OT_DEVICE_ROLE_DETACHED:
        APP_DBG("Device Role: Detached");
        break;
    case OT_DEVICE_ROLE_CHILD:
        APP_DBG("Device Role: Child");
        break;
    case OT_DEVICE_ROLE_ROUTER:
        APP_DBG("Device Role: Router");
        break;
    case OT_DEVICE_ROLE_LEADER:
        APP_DBG("Device Role: Leader");
        break;
    default:
        APP_DBG("Device Role: Unknown");
        break;
    }

    // PAN ID 및 채널 출력
    APP_DBG("PAN ID: 0x%04X", panId);
    APP_DBG("Channel: %d", channel);

    // 확장 주소 출력
    APP_DBG("Extended Address: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
            extAddr.m8[0], extAddr.m8[1], extAddr.m8[2], extAddr.m8[3],
            extAddr.m8[4], extAddr.m8[5], extAddr.m8[6], extAddr.m8[7]);

    // Thread 네트워크 상태 출력
    // bool isThreadEnabled = otThreadGetAutoStart(aInstance);
    // APP_DBG("Thread Auto-Start: %s", isThreadEnabled ? "Enabled" : "Disabled");

    // IPv6 상태 출력
    bool isIp6Enabled = otIp6IsEnabled(aInstance);
    APP_DBG("IPv6: %s", isIp6Enabled ? "Enabled" : "Disabled");
}

void WorkInit(otInstance *aInstance)
{
    otExtAddress extAddr;
    // GroupJoinRequest 데이터 채우기
    otLinkGetFactoryAssignedIeeeEui64(aInstance, &extAddr);
    joinRequest.deviceType = 1; // 1 = 스위치
    joinRequest.groupId = GROUP_ID;    // 요청 그룹 ID
    memcpy(joinRequest.uniqueId, extAddr.m8, sizeof(extAddr.m8));
    memcpy(switchId, joinRequest.uniqueId, 8);
    
    InitCoap(aInstance);
    StatusInit();

    UTIL_SEQ_RegTask( TASK_BATTERY_CHECK, UTIL_SEQ_RFU, WorkBatteryCheck);
    HW_TS_Create(CFG_TIM_USER_ID_ISR, &led_timer_id, hw_ts_SingleShot, led_timer);
}
