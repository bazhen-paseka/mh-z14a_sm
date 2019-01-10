/**
* \file
* \version 1.0
* \author bazhen.levkovets
** \date 2018
*
*************************************************************************************
* \copyright	Bazhen Levkovets
* \copyright	Brovary, Kyiv region
* \copyright	Ukraine
*
*************************************************************************************
*
* \brief
*
*/

/*
**************************************************************************
*							INCLUDE FILES
**************************************************************************
*/

	#include "mh-z14_sm.h"
	extern UART_HandleTypeDef huart1;
	extern UART_HandleTypeDef huart2;
	extern DMA_HandleTypeDef hdma_usart2_rx;

/*
**************************************************************************
*							LOCAL DEFINES
**************************************************************************
*/

	uint32_t icmd_co2_u32 = 0;
	char cmd_co2_char[512];

	RingBuffer_DMA rx_buffer2;
	#define CO2_BUFFER_SIZE 256
	uint8_t co2_circular_buffer[CO2_BUFFER_SIZE];
	static uint8_t time_flag_u8=0;
/*
**************************************************************************
*							LOCAL CONSTANTS
**************************************************************************
*/


/*
**************************************************************************
*						    LOCAL DATA TYPES
**************************************************************************
*/


/*
**************************************************************************
*							  LOCAL TABLES
**************************************************************************
*/

/*
**************************************************************************
*								 MACRO'S
**************************************************************************
*/


/*
**************************************************************************
*						    GLOBAL VARIABLES
**************************************************************************
*/

/*
**************************************************************************
*                        LOCAL FUNCTION PROTOTYPES
**************************************************************************
*/
	uint8_t getCheckSum(char *packet);
/*
**************************************************************************
*                           GLOBAL FUNCTIONS
**************************************************************************
*/

	void MH_Z14A_Init(void)
	{
		char UartBuff[100];
		sprintf(UartBuff,"\r\nCO2-meter on MH-Z14A sensor\r\nUART1 for debug Start\r\n");
		HAL_UART_Transmit(&huart1, (uint8_t *)UartBuff, strlen(UartBuff), 100);

		// RingBuffer variables



		/* Array for DMA to save Rx bytes */

		RingBuffer_DMA_Init(&rx_buffer2, &hdma_usart2_rx, co2_circular_buffer, CO2_BUFFER_SIZE);
		HAL_UART_Receive_DMA(&huart2, co2_circular_buffer, CO2_BUFFER_SIZE);

	}

	uint32_t MH_Z14A_Main(void)
	{
		char DataChar[100];
		sprintf(DataChar,"ack MH-Z14A... ");
		HAL_UART_Transmit(&huart1, (uint8_t *)DataChar, strlen(DataChar), 100);

		uint8_t CO2_Packet_Write[9] = {0xff,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};
		HAL_UART_Transmit(&huart2, CO2_Packet_Write, 9, 100);
		HAL_Delay(1000);
		uint32_t current_CO2_u32 = CO2_Read();

//		sprintf(DataChar,"CO2: %d ppm\r\n", (int)current_CO2_u32);
//		HAL_UART_Transmit(&huart1, (uint8_t *)DataChar, strlen(DataChar), 100);

		return current_CO2_u32;
	}


uint32_t CO2_Read(void)
{
	uint32_t co2_u32 = 0;
	char DataChar_UART[100];
	// if something go wrong with DMA - just restart receiving
	// 	if (hdma_usart2_rx.State == HAL_DMA_STATE_ERROR) {
	// 		HAL_UART_DMAStop(&huart3);
	// 		HAL_UART_Receive_DMA(&huart3, rx_circular_buffer, RX_BUFFER_SIZE);
	// 	}
	// how many bytes in buffer
	uint32_t rx_count = RingBuffer_DMA_Count(&rx_buffer2);

	sprintf(DataChar_UART,"uart_recieve %d byte\r\n",(int)rx_count);
		HAL_UART_Transmit(&huart1, (uint8_t *)DataChar_UART, strlen(DataChar_UART), 100);

	while (rx_count--)
		{
		uint8_t c = RingBuffer_DMA_GetByte(&rx_buffer2);

		sprintf(DataChar_UART,"%d ", (int)c );	HAL_UART_Transmit(&huart1, (uint8_t *)DataChar_UART, strlen(DataChar_UART), 100);

		cmd_co2_char[icmd_co2_u32++] = c;
		HAL_Delay(10);
		}// while (rx_count--)
	co2_u32 = cmd_co2_char[2] * 0xff +cmd_co2_char[3];

	uint8_t CheckSum = getCheckSum(cmd_co2_char);
	sprintf(DataChar_UART,"\r\ncheckSum: %d\r\n", (int)CheckSum );	HAL_UART_Transmit(&huart1, (uint8_t *)DataChar_UART, strlen(DataChar_UART), 100);

	if (cmd_co2_char[8] == CheckSum)
		sprintf(DataChar_UART,"checkSum Ok. Result: %d\r\n", (int)co2_u32);
	else
	{
		sprintf(DataChar_UART,"checkSum Error: CO2=333\r\n");
		co2_u32 = 333;
	}
	HAL_UART_Transmit(&huart1, (uint8_t *)DataChar_UART, strlen(DataChar_UART), 100);

	cmd_co2_char[icmd_co2_u32++] = 0; // we received whole command, setting end of string
	icmd_co2_u32 = 0;

	return co2_u32;
}
//************************************************************************

void SetTimeFlag(uint8_t _set_time_flag_u8)
{
	time_flag_u8 = _set_time_flag_u8;
}
//************************************************************************

uint8_t GetTimeFlag(void)
{
	return time_flag_u8;
}
//************************************************************************

/*
**************************************************************************
*                           LOCAL FUNCTIONS
**************************************************************************
*/
uint8_t getCheckSum(char *packet)
{
	if (packet == _NULL)		return 66;

	uint8_t checksum = 0x00;
	for( uint8_t i = 1; i < 8; i++)
		{
		checksum += packet[i];
		}
	checksum = 0xff - checksum;
	checksum += 1;
	return checksum;
}
