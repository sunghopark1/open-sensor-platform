/* Open Sensor Platform Project
 * https://github.com/sensorplatforms/open-sensor-platform
 *
 * Copyright (C) 2015 Audience Inc.
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
#include "common.h" 
#include "ConfigManager.h"
#include "osp-api.h"
#include <string.h> //for sprintf, strncpy, memset
#include "BatchManager.h"
#include "BatchState.h"

/*-------------------------------------------------------------------------------------------------*\
 |    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/
OSP_STATUS_t Algorithm_SubscribeSensor( ASensorType_t sensor);
OSP_STATUS_t Algorithm_UnsubscribeSensor( ASensorType_t sensor);

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/

#define MY_FID  FID_CONFIG_MANAGER_C

#define ILLEGAL_SENSOR_INIT_TABLE_INDEX    0xFF

#define RESPONSE_COUNTER_LIMIT_MASK        0x3FFFFFFF

#define SH_SPRINTF   sprintf
#define SH_STRNCPY   strncpy
#define SH_MEMSET    memset

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   T Y P E   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

#define SDTOFFSETOF(field)  ((int16_t) offsetof( SensorDescriptor_t, field ))
#define SDT_ILLEGAL         ((int16_t) (-1))
#define SDT_NO_DESCRIPTOR   ((int16_t) (-2))
#define SDT_UNDEFINED       ((int16_t) (-3))
#define SDT_FURTHER_ACTION  ((int16_t) (-4))

//static const uint8_t controlRequestNeedsSensorDescriptor[N_PARAM_ID] =
static const int16_t sensorDescriptorOffsets[N_PARAM_ID] = 
{
    [PARAM_ID_ERROR_CODE_IN_DATA]    =      SDT_ILLEGAL,                               //  0x00
    [PARAM_ID_ENABLE]                =      SDT_NO_DESCRIPTOR,                         //  0x01
    [PARAM_ID_BATCH]                 =      SDT_NO_DESCRIPTOR,                         //  0x02
    [PARAM_ID_FLUSH]                 =      SDT_NO_DESCRIPTOR,                         //  0x03
    [PARAM_ID_RANGE_RESOLUTION]      =    SDT_UNDEFINED,                               //  0x04  TODO
    [PARAM_ID_POWER]                 =    SDT_UNDEFINED,                               //  0x05  TODO
    [PARAM_ID_MINMAX_DELAY]          =    SDT_UNDEFINED,                               //  0x06  TODO
    [PARAM_ID_FIFO_EVT_CNT]          =      SDT_NO_DESCRIPTOR,                         //  0x07
    [PARAM_ID_AXIS_MAPPING]          =  SDTOFFSETOF( AxisMapping                   ),  //  0x08
    [PARAM_ID_CONVERSION_OFFSET]     =  SDTOFFSETOF( ConversionOffset              ),  //  0x09
    [PARAM_ID_CONVERSION_SCALE]      =  SDTOFFSETOF( ConversionScale               ),  //  0x0A
    [PARAM_ID_SENSOR_NOISE]          =  SDTOFFSETOF( Noise                         ),  //  0x0B
    [PARAM_ID_TIMESTAMP_OFFSET]      =  SDT_UNDEFINED,                                 //  0x0C
    [PARAM_ID_ONTIME_WAKETIME]       =  SDT_UNDEFINED,                                 //  0x0D
    [PARAM_ID_HPF_LPF_CUTOFF]        =  SDT_UNDEFINED,                                 //  0x0E
    [PARAM_ID_SENSOR_NAME]           =  SDTOFFSETOF( SensorName                    ),  //  0x0F
    [PARAM_ID_XYZ_OFFSET]            =  SDT_UNDEFINED,                                 //  0x10
    [PARAM_ID_F_SKOR_MATRIX]         =  SDTOFFSETOF( factoryskr ),                     //  0x11
    [PARAM_ID_F_CAL_OFFSET]          =  SDTOFFSETOF( factoryoffset ),                  //  0x12
    [PARAM_ID_F_NONLINEAR_EFFECTS]   =  SDT_UNDEFINED,                                 //  0x13
    [PARAM_ID_BIAS_STABILITY]        =  SDTOFFSETOF( biasStability                 ),  //  0x14
    [PARAM_ID_REPEATABILITY]         =  SDTOFFSETOF( repeatability                 ),  //  0x15
    [PARAM_ID_TEMP_COEFF]            =  SDTOFFSETOF( tempco                        ),  //  0x16
    [PARAM_ID_SHAKE_SUSCEPTIBILITY]  =  SDTOFFSETOF( shake                         ),  //  0x17
    [PARAM_ID_EXPECTED_NORM]         =  SDTOFFSETOF( expectednorm                  ),  //  0x18
    [PARAM_ID_VERSION]               =      SDT_NO_DESCRIPTOR,                         //  0x19
    [PARAM_ID_DYNAMIC_CAL_SCALE]     =    SDT_FURTHER_ACTION,                          //  0x1A  TODO
    [PARAM_ID_DYNAMIC_CAL_SKEW]      =    SDT_FURTHER_ACTION,                          //  0x1B  TODO
    [PARAM_ID_DYNAMIC_CAL_OFFSET]    =    SDT_FURTHER_ACTION,                          //  0x1C  TODO
    [PARAM_ID_DYNAMIC_CAL_ROTATION]  =    SDT_FURTHER_ACTION,                          //  0x1D  TODO
    [PARAM_ID_DYNAMIC_CAL_QUALITY]   =    SDT_FURTHER_ACTION,                          //  0x1E  TODO
    [PARAM_ID_DYNAMIC_CAL_SOURCE]    =    SDT_FURTHER_ACTION,                          //  0x1F  TODO
    [PARAM_ID_CONFIG_DONE]           =      SDT_NO_DESCRIPTOR                          //  0x20  
          //   N_PARAM_ID                                                                  0x21
};

/*-------------------------------------------------------------------------------------------------*\
 |    S T A T I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
static const Q_Type_t queuesToFlushInOrder[NUM_QUEUE_TYPE] = 
{ 
    QUEUE_CONTROL_RESPONSE_TYPE, QUEUE_WAKEUP_TYPE, QUEUE_NONWAKEUP_TYPE 
};

static const int32_t queueFlushErrors[NUM_QUEUE_TYPE] = 
{
    OSP_STATUS_FLUSH_CTRL_RSP_Q_ERR, OSP_STATUS_FLUSH_WAKEUP_Q_ERR, OSP_STATUS_FLUSH_NONWAKEUP_Q_ERR 
};

/*-------------------------------------------------------------------------------------------------*\
 |    F O R W A R D   F U N C T I O N   D E C L A R A T I O N S
\*-------------------------------------------------------------------------------------------------*/
static int16_t GetSensorDescriptor( SensorDescriptor_t **ppSensorDescriptor, uint32_t sensorType );

static void CopyStringField( char *pDest, const char *pSrc, uint16_t nCount );

static int32_t TakeWriteAction(               LocalPacketTypes_t *pLocalPacket );
static int32_t TakeReadAction(                LocalPacketTypes_t *pLocalPacket );

static int32_t ActionEnableDisable(           ASensorType_t sType, 
                                              uint8_t isEnable                 );
static int32_t ActionBatch(                   ASensorType_t sType, 
                                              uint64_t SamplingRate, 
                                              uint64_t ReportLatency           );
static int32_t ActionFlush(void);
static int32_t ActionReadVersion( LocalPacketTypes_t *pLocalPacket );
static int32_t ActionConfigDone(void);


/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

#if 0 //TODO - Pending new driver interface porting
/****************************************************************************************************
 * @fn      SensorInitTableIndex
 *          Returns the table index corresponding to the sensorID
 *
 ***************************************************************************************************/
static uint8_t SensorInitTableIndex( uint32_t sensorID )
{
    const uint32_t sensorIndex = AndroidSensorClearPrivateBase( (ASensorType_t)sensorID );
    uint8_t initTableIndex = 0;

    switch( sensorIndex )
    {
    case SENSOR_ACCELEROMETER:
        //initTableIndex = AiM[ACCEL_INPUT_SENSOR];
        break;

    case SENSOR_MAGNETIC_FIELD:
        //initTableIndex = AiM[MAG_INPUT_SENSOR];
        break;

    case SENSOR_GYROSCOPE:
        //initTableIndex = AiM[GYRO_INPUT_SENSOR];
        break;

    default:
        initTableIndex = ILLEGAL_SENSOR_INIT_TABLE_INDEX;
        break;
    }

    return initTableIndex;

}  //  SensorInitTableIndex()
#endif


/****************************************************************************************************
 * @fn      GetSensorDescriptor
 *          Returns Sensor Descriptor address for the given sensor, or NULL if not available
 *
 ***************************************************************************************************/
static int16_t GetSensorDescriptor( SensorDescriptor_t **ppSensorDescriptor, uint32_t sensorType )
{
#if 0 //TODO - Pending new driver interface porting
    SensorInit_t  *pSensorInitTable; // = gSysDesc.pSensorInitTable;
    int32_t errorCode = OSP_STATUS_INVALID_PARAMETER;
    uint32_t sensorIndex;

    if ( pSensorInitTable == NULL || ppSensorDescriptor == NULL )
    {
        if ( ppSensorDescriptor != NULL )
        {
            *ppSensorDescriptor = NULL;  // (SensorDescriptor_t *) 
        }
    }
    else 
    {
        sensorIndex = SensorInitTableIndex( sensorType );
    }

    if ( sensorIndex >= NUM_INPUT_SENSORS )
    {
        errorCode = OSP_STATUS_INVALID_PARAMETER;
    }
    else if ( pSensorInitTable[sensorIndex].pDescriptor == NULL )
    {
        errorCode = OSP_STATUS_INVALID_PARAMETER;
    }
    else
    {
        *ppSensorDescriptor = pSensorInitTable[sensorIndex].pDescriptor;
        errorCode = OSP_STATUS_OK;
    }


    if ( errorCode != OSP_STATUS_OK && ppSensorDescriptor != NULL )
    {
        *ppSensorDescriptor = NULL;   // clear target pointer 
    }
    
    return SET_ERROR( errorCode );
#else
    return SET_ERROR( OSP_STATUS_UNSUPPORTED_FEATURE );
#endif
} //  end GetSensorDescriptor()


/****************************************************************************************************
 * @fn      CopyStringField
 *          Copies up to nMax, fills remainder with nulls up to nMax, ensures null character at end
 *
 ***************************************************************************************************/
static void CopyStringField( char *pDest, const char *pSrc, uint16_t nCount )
{
    const char sNull[] = "\0";
    const char *pReadFrom = ( pSrc == NULL ) ? &(sNull[0]) : pSrc;
    int gotNull = 0;
    int i;

    for (i = 0; i < nCount - 1; i++)
    {
        gotNull = (!gotNull && pReadFrom[i] == '\0' );
        pDest[i] = (!gotNull) ? pReadFrom[i] : '\0';
    }

    pDest[ nCount - 1 ] = '\0';

}  //  end CopyStringField()


/****************************************************************************************************
 * @fn      CleanString
 *          Writes null chars to string after first terminating null found
 *
 ***************************************************************************************************/
void CleanString( uint8_t *pDest, int16_t nCount ) 
{
    uint8_t gotNull = 0;
    int16_t i;

    for ( i = 0; i < nCount; i++)
    {
        if (!gotNull)
        {
                gotNull = ( pDest[i] != '\0' );
        }
        else
        {
            pDest[i] = '\0';
        }
    }

}  //  end CleanString()


/****************************************************************************************************
 * @fn      CopyPayloadVsSensorDescriptor
 *          Copies payload data from/to local packet depending on read or write operation
 *
 ***************************************************************************************************/
static int32_t CopyPayloadVsSensorDescriptor( uint8_t isWrite, LocalPacketTypes_t *pLocalPacket )
{
    const uint8_t parameterID = pLocalPacket->SCP.CRP.ParameterID;
    const int16_t packetPayloadOffset = pLocalPacket->PayloadOffset;
    const int16_t packetPayloadSize = pLocalPacket->PayloadSize;
    const int16_t sdOffset = sensorDescriptorOffsets[ parameterID ];
    int32_t        errorCode;

    SensorDescriptor_t *pSensorDescriptor;
    uint8_t *pSensorTablePayload;
    uint8_t *pPacketPayload;
    uint8_t *pDest;
    uint8_t *pSrc;

    if ( pLocalPacket == NULL )
    {
        return SET_ERROR( OSP_STATUS_INVALID_PARAMETER );
    }

    if ( !ValidParameterID( parameterID ) || packetPayloadOffset < 0 || packetPayloadSize < 0 || sdOffset < 0 )
    {
        return SET_ERROR( OSP_STATUS_INVALID_PARAMETER );
    }

    errorCode = GetSensorDescriptor( &(pSensorDescriptor), pLocalPacket->SType );

    if ( errorCode < 0 )
    {
        return SET_ERROR( errorCode );
    }

    pSensorTablePayload = sdOffset + (uint8_t *) pSensorDescriptor;
    pPacketPayload = pLocalPacket->PayloadOffset + (uint8_t *) pLocalPacket;

    if ( isWrite )
    {
        pDest = pSensorTablePayload;
        pSrc  = pPacketPayload;
    }
    else
    {
        pDest = pPacketPayload;
        pSrc  = pSensorTablePayload;
    }

    SH_MEMCPY( pDest, pSrc, packetPayloadSize );

    return SET_ERROR( OSP_STATUS_OK );

}  //  end CopyPayloadVsSensorDescriptor()


/****************************************************************************************************
 * @fn      TakeWriteAction
 *          Write Action Handler
 *
 ***************************************************************************************************/
static int32_t TakeWriteAction( LocalPacketTypes_t *pLocalPacket )
{
    const uint8_t isWrite = TRUE;
    int32_t errorCode;

    if ( pLocalPacket == NULL )
    {
        return SET_ERROR( OSP_STATUS_INVALID_PARAMETER );
    }

    switch( pLocalPacket->SCP.CRP.ParameterID )
    {
    case PARAM_ID_ENABLE:                   //  0x01
        errorCode = ActionEnableDisable( pLocalPacket->SType, pLocalPacket->SCP.CRP.PL.Enable.DataU8 );
        break;

    case PARAM_ID_BATCH:                    //  0x02
        errorCode = ActionBatch( pLocalPacket->SType, pLocalPacket->SCP.CRP.PL.Batch.DataU64x2[0],
                                 pLocalPacket->SCP.CRP.PL.Batch.DataU64x2[1] );
        break;

    case PARAM_ID_FLUSH:                    //  0x03
        errorCode = ActionFlush();
        break;

    case PARAM_ID_RANGE_RESOLUTION:         //  0x04
    case PARAM_ID_POWER:                    //  0x05
    case PARAM_ID_MINMAX_DELAY:             //  0x06
        errorCode = SET_ERROR( OSP_STATUS_UNSUPPORTED_FEATURE );
        break;

    case PARAM_ID_FIFO_EVT_CNT:             //  0x07
        errorCode = SET_ERROR( OSP_STATUS_INVALID_PARAMETER );  ;  //  because Read-Only
        break;

    case PARAM_ID_AXIS_MAPPING:             //  0x08
    case PARAM_ID_CONVERSION_OFFSET:        //  0x09
    case PARAM_ID_CONVERSION_SCALE:         //  0x0A
    case PARAM_ID_SENSOR_NOISE:             //  0x0B
    case PARAM_ID_TIMESTAMP_OFFSET:         //  0x0C
    case PARAM_ID_ONTIME_WAKETIME:          //  0x0D
    case PARAM_ID_HPF_LPF_CUTOFF:           //  0x0E
    case PARAM_ID_SENSOR_NAME:              //  0x0F
    case PARAM_ID_XYZ_OFFSET:               //  0x10
    case PARAM_ID_F_SKOR_MATRIX:            //  0x11
    case PARAM_ID_F_CAL_OFFSET:             //  0x12
    case PARAM_ID_F_NONLINEAR_EFFECTS:      //  0x13
    case PARAM_ID_BIAS_STABILITY:           //  0x14
    case PARAM_ID_REPEATABILITY:            //  0x15
    case PARAM_ID_TEMP_COEFF:               //  0x16
    case PARAM_ID_SHAKE_SUSCEPTIBILITY:     //  0x17
    case PARAM_ID_EXPECTED_NORM:            //  0x18
        errorCode = CopyPayloadVsSensorDescriptor( isWrite, pLocalPacket );
        break;

    case PARAM_ID_VERSION:                  //  0x19
        errorCode = SET_ERROR( OSP_STATUS_INVALID_PARAMETER );  //  because Read-Only
        break;

    case PARAM_ID_DYNAMIC_CAL_SCALE:        //  0x1A
    case PARAM_ID_DYNAMIC_CAL_SKEW:         //  0x1B
    case PARAM_ID_DYNAMIC_CAL_OFFSET:       //  0x1C
    case PARAM_ID_DYNAMIC_CAL_ROTATION:     //  0x1D
    case PARAM_ID_DYNAMIC_CAL_QUALITY:      //  0x1E
    case PARAM_ID_DYNAMIC_CAL_SOURCE:       //  0x1F
        errorCode = SET_ERROR( OSP_STATUS_UNSUPPORTED_FEATURE );
        break;

    case PARAM_ID_CONFIG_DONE:              //  0x20  
        ActionConfigDone();
        break;

    default:  // 0x00 or out-of-bounds is an error.
        errorCode = SET_ERROR( OSP_STATUS_INVALID_PARAMETER );
        break;
    }

    return errorCode;

}  //  end TakeWriteAction()


/****************************************************************************************************
 * @fn      TakeReadAction
 *          Read Action Handler
 *
 ***************************************************************************************************/
static int32_t TakeReadAction( LocalPacketTypes_t *pLocalPacket )
{
    const uint8_t isWrite = FALSE;
    int32_t errorCode = SET_ERROR( OSP_STATUS_UNSPECIFIED_ERROR );

    if ( pLocalPacket == NULL )
    {
        return SET_ERROR( OSP_STATUS_INVALID_PARAMETER );
    }

    switch( pLocalPacket->SCP.CRP.ParameterID )
    {
    case PARAM_ID_ENABLE:                   //  0x01
    case PARAM_ID_BATCH:                    //  0x02
    case PARAM_ID_FLUSH:                    //  0x03
        errorCode = SET_ERROR( OSP_STATUS_INVALID_PARAMETER );  //  because Write-Only.
        break;

    case PARAM_ID_RANGE_RESOLUTION:         //  0x04
    case PARAM_ID_POWER:                    //  0x05
    case PARAM_ID_MINMAX_DELAY:             //  0x06
        errorCode = SET_ERROR( OSP_STATUS_UNSUPPORTED_FEATURE );
        break;

    case PARAM_ID_FIFO_EVT_CNT:             //  0x07
        errorCode = SET_ERROR( OSP_STATUS_UNSUPPORTED_FEATURE );  // TODO: TBD
        break;

    case PARAM_ID_AXIS_MAPPING:             //  0x08
    case PARAM_ID_CONVERSION_OFFSET:        //  0x09
    case PARAM_ID_CONVERSION_SCALE:         //  0x0A
    case PARAM_ID_SENSOR_NOISE:             //  0x0B
    case PARAM_ID_TIMESTAMP_OFFSET:         //  0x0C
    case PARAM_ID_ONTIME_WAKETIME:          //  0x0D
    case PARAM_ID_HPF_LPF_CUTOFF:           //  0x0E
    case PARAM_ID_SENSOR_NAME:              //  0x0F
    case PARAM_ID_XYZ_OFFSET:               //  0x10
    case PARAM_ID_F_SKOR_MATRIX:            //  0x11
    case PARAM_ID_F_CAL_OFFSET:             //  0x12
    case PARAM_ID_F_NONLINEAR_EFFECTS:      //  0x13
    case PARAM_ID_BIAS_STABILITY:           //  0x14
    case PARAM_ID_REPEATABILITY:            //  0x15
    case PARAM_ID_TEMP_COEFF:               //  0x16
    case PARAM_ID_SHAKE_SUSCEPTIBILITY:     //  0x17
    case PARAM_ID_EXPECTED_NORM:            //  0x18

        errorCode = CopyPayloadVsSensorDescriptor( isWrite, pLocalPacket );

        //  for SENSOR_NAME, ensure no non-null chars after terminating null char.
        if ( pLocalPacket->SCP.CRP.ParameterID == PARAM_ID_SENSOR_NAME )
        {
            uint8_t *pPayload  = pLocalPacket->PayloadOffset + (uint8_t *) pLocalPacket;
            int16_t  payloadSize = pLocalPacket->PayloadSize;

            CleanString( pPayload, payloadSize );
        }
        break;

    case PARAM_ID_VERSION:                  //  0x19
        errorCode = ActionReadVersion( pLocalPacket );
        break;

    case PARAM_ID_DYNAMIC_CAL_SCALE:        //  0x1A
    case PARAM_ID_DYNAMIC_CAL_SKEW:         //  0x1B
    case PARAM_ID_DYNAMIC_CAL_OFFSET:       //  0x1C
    case PARAM_ID_DYNAMIC_CAL_ROTATION:     //  0x1D
    case PARAM_ID_DYNAMIC_CAL_QUALITY:      //  0x1E
    case PARAM_ID_DYNAMIC_CAL_SOURCE:       //  0x1F
        errorCode = SET_ERROR( OSP_STATUS_UNSUPPORTED_FEATURE );
        break;

    case PARAM_ID_CONFIG_DONE:              //  0x20  
        errorCode = SET_ERROR( OSP_STATUS_INVALID_PARAMETER );
        break;

    default:  // 0x00 or out-of-bounds is an error.
        errorCode = SET_ERROR( OSP_STATUS_INVALID_PARAMETER );
        break;
    }

    return errorCode;

}  //  end TakeReadAction()


/****************************************************************************************************
 * @fn      ActionEnableDisable
 *          Subscribe/unsubscribe to sensor, for PARAM_ID_ENABLE.
 *
 * @return  OSP_STATUS_OK or negative error code.
 *
 ***************************************************************************************************/
static int32_t ActionEnableDisable( ASensorType_t sType, uint8_t isEnable )
{
    /*  To enable sensor 
          BatchManagerSensorEnable()
          Subscribe to Sensor results from Algo

        To disable Sensor 
          BatchManagerSensorDeRegister()
          BatchManagerSensorDisable()
          UnSubscribe to Sensor results from Algo
    */

    int32_t  errorCode;

    if ( isEnable )
    {
        errorCode = (int32_t) BatchManagerSensorEnable( sType );

        if ( errorCode == OSP_STATUS_OK )
        {
            errorCode = (int32_t) Algorithm_SubscribeSensor( sType );
        }
   
    }
    else
    {
        errorCode = (int32_t) BatchManagerSensorDeRegister( sType );

        if ( errorCode == OSP_STATUS_OK )
        {
            errorCode = (int32_t) BatchManagerSensorDisable( sType );

            if ( errorCode == OSP_STATUS_OK )
            {
                errorCode = (int32_t) Algorithm_UnsubscribeSensor( sType );
            }

        }
    }

    return SET_ERROR( errorCode );

}  //  end ActionEnableDisable()



/****************************************************************************************************
 * @fn      ActionBatch
 *          Register sensor, for PARAM_ID_BATCH.
 *
 * @return  OSP_STATUS_OK or negative error code.
 *
 ***************************************************************************************************/
static int32_t ActionBatch( ASensorType_t sType, uint64_t SamplingRate,  uint64_t ReportLatency ) 
{
    int32_t errorCode;

    errorCode = (int32_t) BatchManagerSensorRegister( sType, SamplingRate, ReportLatency );

    return SET_ERROR( errorCode );

}  //  end ActionBatch()


/****************************************************************************************************
 * @fn      ActionFlush
 *          Register sensor, for PARAM_ID_FLUSH.
 *
 * @return  OSP_STATUS_OK or negative error code.
 *
 ***************************************************************************************************/
static int32_t ActionFlush() 
{
    int32_t errorCode;
    uint8_t i;

    for (i = 0; i < NUM_QUEUE_TYPE; i++)
    {
        errorCode = (int32_t) BatchManagerQueueFlush(queuesToFlushInOrder[i]);

        if ( errorCode != OSP_STATUS_OK )
        {
            errorCode = ((queueFlushErrors[i]));
            return SET_ERROR( errorCode );
        }
    }

    return SET_ERROR( errorCode );

}  //  end ActionFlush()


/****************************************************************************************************
 * @fn      ActionReadVersion
 *          Read the Batch library version information into the response payload, for PARAM_ID_VERSION.
 *
 * @return  OSP_STATUS_OK or negative error code.
 *
 ***************************************************************************************************/
static int32_t ActionReadVersion(  LocalPacketTypes_t *pLocalPacket )
{
    int32_t errorCode = OSP_STATUS_UNSPECIFIED_ERROR;

    osp_status_t status;
    OSP_Library_Version_t const *pLibraryVersion;

    status = OSP_GetLibraryVersion( &(pLibraryVersion) );

    if ( !status || pLibraryVersion == NULL || pLibraryVersion->VersionString == NULL )
    {
        errorCode = OSP_STATUS_UNSUPPORTED_FEATURE;
    }
    else {

        char *pDest = (char *) (pLocalPacket->PayloadOffset + (uint8_t *) pLocalPacket);
        const uint16_t lenVersionMax = pLocalPacket->PayloadSize;

        CopyStringField( pDest, pLibraryVersion->VersionString, lenVersionMax );

        errorCode = OSP_STATUS_OK;
    }

    return SET_ERROR( errorCode );

}  //  end ActionReadVersion()


/****************************************************************************************************
 * @fn      ActionConfigDone
 *          Change BatchState to BATCH_IDLE, for PARAM_ID_CONFIG_DONE.
 *
 * @return  OSP_STATUS_OK or negative error code.
 *
 ***************************************************************************************************/
static int32_t ActionConfigDone() 
{
    int32_t errorCode;

    errorCode = (int32_t) BatchStateSet( BATCH_IDLE );

    return SET_ERROR( errorCode );
}

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/****************************************************************************************************
 * @fn      SHConfigManager_ProcessControlRequest
 *          Control Request parser/handler/response-formatter.
 *
 *          Parse serialized Control Request packet, take action as necessary 
 *          (rejecting if action not allowed by current Batch state), make serialized 
 *          Control Response packet and send to BatchManager Enqueue, if indicated (if 
 *          Sequence Number field in Request packet is nonzero).  Returns 
 *          negative error code on non-respond-able error, otherwise zero if no response 
 *          requested, otherwise positive response packet length.
 *
 * @param   [IN]pRequestPacket - Packet buffer containing the serialized packet to parse and act on
 * @param   [IN]requestPacketBufferSize - Size of the request packet buffer
 *
 * @return  OSP_STATUS_OK or negative Error code enum corresponding to the error encountered
 *
 ***************************************************************************************************/

int32_t SHConfigManager_ProcessControlRequest( const uint8_t *pRequestPacket, uint16_t requestPacketBufferSize )
{
    //  check that packet is a Request packet, and set flag if it is a Write packet
    const uint8_t packetID  = GetPacketID( pRequestPacket );
    uint8_t       isWriteRequest;

    LocalPacketTypes_t localPacket;
    int32_t errorCode;  
    uint16_t packetSize;

    if (DEBUG_SENSOR_PACKETS)
    {
        DPRINTF("ConfigManager - packet received:   ");

    }

    if ( packetID == PKID_CONTROL_REQ_WR )
    {
        isWriteRequest = 1;
        errorCode = OSP_STATUS_OK;
    }
    else if ( packetID == PKID_CONTROL_REQ_RD )
    {
        isWriteRequest = 0;
        errorCode = OSP_STATUS_OK;
    }
    else
    {
        /* We don't handle any other packet types here */
        errorCode = OSP_STATUS_INVALID_PARAMETER;
    }

    //  parse the request packet (only one request packet is expected at a time!)
    if ( errorCode == OSP_STATUS_OK )
    {
        errorCode = ParseHostInterfacePkt( &localPacket, pRequestPacket, &packetSize, requestPacketBufferSize ); //TODO FIXME
    }

    //  check that Control Request is valid for current Batch state, and advance state 
    //  to BATCH_CONFIG if:
    //    * This is a command valid to be executed under the current state, and 
    //    * This is a Write command, and
    //    * Old state was BATCH_STANDBY.
    //  
    if ( errorCode == OSP_STATUS_OK )
    {
        //TODO:  May be better to move this block of code into a routine in BATCH_State.c, 
        //TODO:  to encapsulate state machine handling.

        //TODO:  It seems likely that some commands would be disallowed in some states 
        //TODO:  and allowed in other states based on whether they were Write or Read commands, 
        //TODO:  but BATCH_State.c state matrices only distinguish commands based on Parameter ID, 
        //TODO:  not Write or Read.  For example, some commands could probably read parameters 
        //TODO:  after ConfigDone is sent, but state matrices do not permit that and state 
        //TODO:  diagram does not have a return to BATCH_STANDBY from BATCH_IDLE/BATCH_ACTIVE.  Maybe a 
        //TODO:  hub reset is needed to get back to there.

        const uint8_t parameterID = localPacket.SCP.CRP.ParameterID;

        errorCode          = (int32_t) BatchStateCommandValidate( (BatchCmdList_t)parameterID );
        if ( errorCode == OSP_STATUS_OK )
        {
            if ( IsWriteConfigCommand( packetID, parameterID ) )
            {
                BatchStateType_t  currentState;
    
                errorCode = (int32_t) BatchStateGet( &currentState );
    
                if ( errorCode == OSP_STATUS_OK )
                {
                    if ( currentState == BATCH_STANDBY )
                    {
                        errorCode = (int32_t) BatchStateSet( BATCH_CONFIG );

                    }
                }
            }
        }
    }

    //  take action on the request, if successful so far
    if ( errorCode == OSP_STATUS_OK )
    {
        if ( isWriteRequest )
        {
            errorCode = TakeWriteAction( &localPacket );
        }
        else
        {
            errorCode = TakeReadAction( &localPacket );
        }
    }

    DPRINTF( "ConfigManager errorCode post-action / pre-response: %08X\r\n", errorCode );

    // compose and enqueue response packet if so requested0
    if ( localPacket.SCP.CRP.SequenceNumber != 0 )
    {
        HostIFPackets_t hostResponsePacket;
        uint16_t hostPacketLength;

        // change local packet type to Control Response, for the reply to Host
        localPacket.PacketID = PKID_CONTROL_RESP;

        if ( errorCode < 0 )   // N.B.:  This is the error code returned to Host (not to routine caller).
        {
            SetResponsePacketToErrorPacket( &localPacket, errorCode );
        }

        errorCode = FormatControlPacket( &hostResponsePacket, &localPacket, &hostPacketLength );

        if ( errorCode != OSP_STATUS_OK )  // N.B.:  This is the error code returned to routine caller (not to Host)! (doc TODO?)
        {
            DPRINTF( "ConfigManager errorCode response post-format: %08X\r\n", errorCode );
        }

        if ( errorCode == OSP_STATUS_OK && hostPacketLength > 0 )
        {
            int32_t batchQueueErrorCode;
            
            batchQueueErrorCode = (int32_t) BatchManagerControlResponseEnQueue( &hostResponsePacket, hostPacketLength  );

            if (batchQueueErrorCode != OSP_STATUS_OK)
            {
                DPRINTF( "ConfigManager response batchQueueErrorCode post-queue: %08X\r\n", batchQueueErrorCode );

                errorCode = batchQueueErrorCode;
            }
        }
    }

    return SET_ERROR( errorCode );

}  //  end SHConfigManager_ProcessControlRequest()

/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/