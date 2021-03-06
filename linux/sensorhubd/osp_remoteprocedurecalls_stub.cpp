/* Open Sensor Platform Project
 * https://github.com/sensorplatforms/open-sensor-platform
 *
 * Copyright (C) 2013 Sensor Platforms Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*-------------------------------------------------------------------------------------------------*\
 |    I N C L U D E   F I L E S
\*-------------------------------------------------------------------------------------------------*/
#include "osp_remoteprocedurecalls.h"
#include <cstring>
#include <cstdlib>
#include "osp_debuglogging.h"



/*-------------------------------------------------------------------------------------------------*\
 |    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   T Y P E   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    S T A T I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
static OSPD_ResultDataCallback_t _resultReadyCallbacks[RESULT_ENUM_COUNT] = {0};

/*-------------------------------------------------------------------------------------------------*\
 |    F O R W A R D   F U N C T I O N   D E C L A R A T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/****************************************************************************************************
 * @fn      OSPD_Initialize
 *          Initialize remote procedure call for the daemon
 *
 ***************************************************************************************************/
osp_status_t OSPD_Initialize(void) {
    osp_status_t result = OSP_STATUS_OK;
    LOGT("%s\r\n", __FUNCTION__);

    return result;
}

/****************************************************************************************************
 * @fn      OSPD_GetVersion
 *          Helper routine for getting daemon version information
 *
 ***************************************************************************************************/
osp_status_t OSPD_GetVersion(char* versionString, int bufSize) {
    osp_status_t result = OSP_STATUS_OK;

    LOGT("%s\r\n", __FUNCTION__);

    return result;
}

/****************************************************************************************************
 * @fn      OSPD_SubscribeResult
 *          Enables subscription for results
 *
 ***************************************************************************************************/
osp_status_t OSPD_SubscribeResult(uint32_t sensorType, OSPD_ResultDataCallback_t dataReadyCallback ) {
    osp_status_t result = OSP_STATUS_OK;

    LOGT("%s\r\n", __FUNCTION__);

    return result;
}

/****************************************************************************************************
 * @fn      OSPD_UnsubscribeResult
 *          Unsubscribe from sensor results
 *
 ***************************************************************************************************/
osp_status_t OSPD_UnsubscribeResult(uint32_t sensorType) {
    osp_status_t result = OSP_STATUS_OK;

    LOGT("%s\r\n", __FUNCTION__);

    return result;
}


/****************************************************************************************************
 * @fn      OSPD_Deinitialize
 *          Tear down RPC interface function
 *
 ***************************************************************************************************/
osp_status_t OSPD_Deinitialize(void) {
    osp_status_t result = OSP_STATUS_OK;

    LOGT("%s\r\n", __FUNCTION__);

    return result;
}


/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/
