/* begin.cpp	Tue Nov  1 2016 05:25:12 tmm */

/*

Module:  begin.cpp

Function:
	Arduino_LoRaWAN::begin();

Version:
	V0.2.0	Tue Nov  1 2016 05:25:12 tmm	Edit level 2

Copyright notice:
	This file copyright (C) 2016 by

		MCCI Corporation
		3520 Krums Corners Road
                Ithaca, NY  14850

	An unpublished work.  All rights reserved.
	
	This file is proprietary information, and may not be disclosed or
	copied without the prior permission of MCCI Corporation.
 
Author:
	Terry Moore, MCCI Corporation	October 2016

Revision history:
   0.1.0  Tue Oct 25 2016 03:38:36  tmm
	Module created.

*/

#include <Arduino_LoRaWAN.h>
#include <Arduino_LoRaWAN_lmic.h>
#include <mcciadk_baselib.h>
#include <hal/hal.h>

/* the global instance pointer */
Arduino_LoRaWAN *Arduino_LoRaWAN::pLoRaWAN = NULL;

/* we have to provide the LMIC's instance of the lmic_pins structure */
static struct ::lmic_pinmap s_lmic_pins;

bool Arduino_LoRaWAN::begin()
    {
    // record self in a static so that we can dispatch events
    ASSERT(Arduino_LoRaWAN::pLoRaWAN == this ||
           Arduino_LoRaWAN::pLoRaWAN == NULL);

    Arduino_LoRaWAN::pLoRaWAN = this;

    // set up the LMIC pinmap.
    s_lmic_pins.nss = this->m_lmic_pins.nss;
    s_lmic_pins.rxtx = this->m_lmic_pins.rxtx;
    s_lmic_pins.rst = this->m_lmic_pins.rst;
    memcpy(s_lmic_pins.dio, this->m_lmic_pins.dio, sizeof(s_lmic_pins.dio));

    // LMIC init
    if (! os_init_ex(&s_lmic_pins))
        return false;

    // Reset the MAC state. Session and pending data transfers will be 
    // discarded.
    LMIC_reset();

    return this->NetBegin();
    }

/****************************************************************************\
|
|	The event dispatcher
|
\****************************************************************************/

void onEvent(ev_t ev) 
    {
    Arduino_LoRaWAN * const pLoRaWAN = Arduino_LoRaWAN::GetInstance();

    ASSERT(pLoRaWAN != NULL);

    pLoRaWAN->DispatchEvent(ev);
    }

void Arduino_LoRaWAN::DispatchEvent(
    uint32_t ev
    )
    {
    ARDUINO_LORAWAN_PRINTF(
        LogVerbose, 
        "EV_%s\n", 
        cLMIC::GetEventName(ev)
        );

    // do the usual work in another function, for clarity.
    this->StandardEventProcessor(ev);

    // dispatch to the registered clients
    for (unsigned i = 0; i < this->m_nRegisteredListeners; ++i)
        {
        this->m_RegisteredListeners[i].ReportEvent(ev);
        }
    }

bool Arduino_LoRaWAN::RegisterListener(
    ARDUINO_LORAWAN_EVENT_FN *pEventFn,
    void *pContext
    )
    {
    if (this->m_nRegisteredListeners < 
                MCCIADK_LENOF(this->m_RegisteredListeners))
        {
        Arduino_LoRaWAN::Listener * const pListener = 
                &this->m_RegisteredListeners[this->m_nRegisteredListeners];
        ++this->m_nRegisteredListeners;

        pListener->pEventFn = pEventFn;
        pListener->pContext = pContext;
        return true;
        }
    else
        {
        return false;
        }
    }

const char *
Arduino_LoRaWAN::cLMIC::GetEventName(uint32_t ev)
    {
    static const char szEventNames[] = ARDUINO_LORAWAN_LMIC_EV_NAMES_MZ__INIT;
    const char *p;

    if (ev < ARDUINO_LORAWAN_LMIC_EV_NAMES__BASE)
        return "<<unknown>>";
        
    p = McciAdkLib_MultiSzIndex(
        szEventNames, 
        ev - ARDUINO_LORAWAN_LMIC_EV_NAMES__BASE
        );

    if (*p == '\0')
        return "<<unknown>>";
    else
        return p;
    }

void Arduino_LoRaWAN::StandardEventProcessor(
    uint32_t ev
    )
    {
    switch(ev) 
        {
        case EV_SCAN_TIMEOUT:
            break;
        case EV_BEACON_FOUND:
            break;
        case EV_BEACON_MISSED:
            break;
        case EV_BEACON_TRACKED:
            break;
        case EV_JOINING:
            break;

        case EV_JOINED:
            // Disable link check validation (automatically enabled
            // during join, but not supported by TTN at this time).
            // TODO: move this to a TTN module; perhaps virtualize?
	    this->NetJoin();
            break;

        case EV_RFU1:
            break;

        case EV_JOIN_FAILED:
            break;

        case EV_REJOIN_FAILED:
            break;

        case EV_TXCOMPLETE:
            this->m_fTxPending = false;
            this->NetTxComplete();
            if (this->m_pSendBufferDoneFn)
                this->m_pSendBufferDoneFn(this->m_pSendBufferDoneCtx, true);
            break;

        case EV_LOST_TSYNC:
            break;

        case EV_RESET:
            break;

        case EV_RXCOMPLETE:
            // data received in ping slot
            // see TXCOMPLETE.
            this->NetRxComplete();
            break;

        case EV_LINK_DEAD:
            break;

        case EV_LINK_ALIVE:
            break;

        case EV_SCAN_FOUND:
            break;

        case EV_TXSTART:
            break;

	default:
	    break;
	}
    }


/****************************************************************************\
|
|	Getting provisioning info
|
\****************************************************************************/

void os_getArtEui(uint8_t* buf) 
    {
    Arduino_LoRaWAN * const pLoRaWAN = Arduino_LoRaWAN::GetInstance();

    ASSERT(pLoRaWAN != NULL);

    pLoRaWAN->GetAppEUI(buf);
    }

bool Arduino_LoRaWAN::GetAppEUI(
    uint8_t *pBuf
    )
    {
    OtaaProvisioningInfo otaaInfo;

    if (! this->GetOtaaProvisioningInfo(&otaaInfo))
        return false;

    memcpy(
        pBuf,
        otaaInfo.AppEUI,
        sizeof(otaaInfo.AppEUI)
        );

    return true;
    }

void os_getDevEui(uint8_t* buf) 
    {
    Arduino_LoRaWAN * const pLoRaWAN = Arduino_LoRaWAN::GetInstance();

    ASSERT(pLoRaWAN != NULL);

    pLoRaWAN->GetDevEUI(buf);
    }

bool Arduino_LoRaWAN::GetDevEUI(
    uint8_t *pBuf
    )
    {
    OtaaProvisioningInfo otaaInfo;

    if (! this->GetOtaaProvisioningInfo(&otaaInfo))
        return false;

    memcpy(
        pBuf, 
        otaaInfo.DevEUI,
        sizeof(otaaInfo.DevEUI)
        );

    return true;
    }



void os_getDevKey(uint8_t* buf) 
    {
    Arduino_LoRaWAN * const pLoRaWAN = Arduino_LoRaWAN::GetInstance();

    ASSERT(pLoRaWAN != NULL);

    pLoRaWAN->GetAppKey(buf);
    }

bool Arduino_LoRaWAN::GetAppKey(
    uint8_t *pBuf
    )
    {
    OtaaProvisioningInfo otaaInfo;

    if (! this->GetOtaaProvisioningInfo(&otaaInfo))
        return false;

    memcpy(
        pBuf, 
        otaaInfo.AppKey,
        sizeof(otaaInfo.AppKey)
        );

    return true;
    }

