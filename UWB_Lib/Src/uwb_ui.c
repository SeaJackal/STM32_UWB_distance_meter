#include "uwb_ui.h"

#include "deca_device_api.h"
#include "deca_regs.h"
#include "sleep.h"
#include "dw1000.h"

#include "EventRecorder.h"

#include <stdio.h>

UWB_status UWB_Init(uint32_t rx_timeout)
{
		dwt_config_t config = {
    2,               /* Channel number. */
		//DWT_PRF_16M,
    DWT_PRF_64M,     /* Pulse repetition frequency. */
    DWT_PLEN_1024,   /* Preamble length. Used in TX only. */
    DWT_PAC32,       /* Preamble acquisition chunk size. Used in RX only. */
    9,               /* TX preamble code. Used in TX only. */
    9,               /* RX preamble code. Used in RX only. */
    1,               /* 0 to use standard SFD, 1 to use non-standard SFD. */
    DWT_BR_110K,     /* Data rate. */
    DWT_PHRMODE_STD, /* PHY header mode. */
    (1025 + 64 - 32) /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
		};
		
		reset_DW1000();
    spi_set_rate_low();
    if (dwt_initialise(DWT_LOADUCODE) == DWT_ERROR)
    {
			EventRecord2(1, 0, 0);
        return UWB_ERROR;
    }
    dwt_configure(&config);
		dwt_setrxtimeout(rx_timeout);
		spi_set_rate_high();
		return UWB_OK;
}

UWB_status UWB_SendMessage(const void* message, uint16_t length, uint8_t wait_responce_flag)
{
		//dwt_forcetrxoff();
		dwt_writetxdata(length+2, (uint8_t*)message, 0);
		dwt_writetxfctrl(length+2, 0, 0);
		uint8_t mode;
		if(wait_responce_flag)
			mode = DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED;
		else
			mode = DWT_START_TX_IMMEDIATE;
		if(dwt_starttx(mode)==DWT_SUCCESS)
			return UWB_OK;
		else
		{
			EventRecord2(2, 0, 0);
			return UWB_ERROR;
		}
}

UWB_status UWB_ActivateRX()
{
		if(dwt_rxenable(DWT_START_RX_IMMEDIATE)==DWT_ERROR)
			return UWB_ERROR;
		else
			return UWB_OK;
}

UWB_status UWB_WaitForMessage(void* message, uint32_t length)
{
		uint32_t status_reg_;
		while (!((status_reg_ = dwt_read32bitreg(SYS_STATUS_ID)) & SYS_STATUS_RXFCG))
		{
				if(status_reg_ & SYS_STATUS_ALL_RX_ERR)
				{
					EventRecord2(3, status_reg_ & SYS_STATUS_ALL_RX_ERR, 0);
					return UWB_ERROR;
				}
				if(status_reg_ & SYS_STATUS_RXRFTO)
				{
					dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXRFTO);
					return UWB_TIMEOUT;
				}
		}
		if(!(status_reg_ & SYS_STATUS_RXDFR))
		{
			EventRecord2(4, 0, 0);
				return UWB_ERROR;
		}
		dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);
		//uint32_t frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFL_MASK_1023;
		dwt_readrxdata(message, length, 0);
		dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);
		return UWB_OK;
}

uint64_t UWB_GetTxTimestamp64()
{
	uint32_t status_reg_;
	while(!((status_reg_ = dwt_read32bitreg(SYS_STATUS_ID)) & SYS_STATUS_TXFRS))
	{
		if(status_reg_ & SYS_STATUS_ALL_RX_ERR)
		{
			EventRecord2(5, 0, 0);
				return UWB_ERR_TIME;
		}
	}
	uint64_t tx_time = 0;
	dwt_readtxtimestamp((uint8_t*)&tx_time);
	return tx_time;
}
uint64_t UWB_GetRxTimestamp64()
{
	uint32_t status_reg_;
	while(!((status_reg_ = dwt_read32bitreg(SYS_STATUS_ID)) & SYS_STATUS_LDEDONE))
	{
		if(status_reg_ & SYS_STATUS_ALL_RX_ERR)
		{
			EventRecord2(6, 0, 0);
				return UWB_ERR_TIME;
		}
	}
	uint64_t rx_time = 0;
	dwt_readrxtimestamp((uint8_t*)&rx_time);
	return rx_time;
}

UWB_status UWB_checkMessage()
{
	uint32_t status_reg_ = dwt_read32bitreg(SYS_STATUS_ID);
	if(status_reg_ & SYS_STATUS_RXFCG)
	{
		if(!(status_reg_ & SYS_STATUS_RXDFR))
				return UWB_ERROR;
		return UWB_READY;
	}
	if(status_reg_ & SYS_STATUS_ALL_RX_ERR)
		return UWB_ERROR;
	if(status_reg_ & SYS_STATUS_RXRFTO)
	{
		dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXRFTO);
		return UWB_TIMEOUT;
	}
	return UWB_OK;
}

uint32_t UWB_readMessage(uint8_t* message)
{
		dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);
		uint32_t frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFL_MASK_1023;
		dwt_readrxdata(message, frame_len, 0);
		dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);
		return frame_len;
}