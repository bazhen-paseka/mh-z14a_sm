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
/*
**************************************************************************
*							LOCAL DEFINES
**************************************************************************
*/
	#define 	CO2_BUFFER_SIZE 	256
	#define 	MH_Z14A_UART 		&huart2
/*
**************************************************************************
*							LOCAL CONSTANTS
**************************************************************************
*/
			RingBuffer_DMA 			rx_buffer2;
	extern 	UART_HandleTypeDef 		huart2			;
	extern 	DMA_HandleTypeDef 		hdma_usart2_rx	;
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
			char 		UartBuff[CO2_BUFFER_SIZE];
			char 		cmd_co2_char[512];
			uint8_t 	co2_circular_buffer[CO2_BUFFER_SIZE];
	static	uint8_t 	time_flag_u8=0;
			uint32_t 	icmd_co2_u32 = 0;
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

void MH_Z14A_Init( void	)	{
	RingBuffer_DMA_Init( &rx_buffer2, &hdma_usart2_rx, co2_circular_buffer, CO2_BUFFER_SIZE);
	HAL_UART_Receive_DMA(MH_Z14A_UART, co2_circular_buffer, CO2_BUFFER_SIZE);

	sprintf(UartBuff,"\t CO2-meter on MH-Z14A sensor\r\n\t RingBuffer on UART2 for CO2 sensor started.\r\n");
	One_pin_debug_print( (uint8_t *)UartBuff, strlen(UartBuff) ) ;
}
//************************************************************************

uint32_t MH_Z14A_Main( void	) {
	uint8_t CO2_Packet_Write[9] = { 0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79 } ;
	HAL_UART_Transmit(MH_Z14A_UART, CO2_Packet_Write, 9, 1000);
	HAL_Delay( 500 ) ;
	uint32_t current_CO2_u32 = CO2_Read_u32( );
	return current_CO2_u32 ;
}
//************************************************************************

uint32_t CO2_Read_u32( void ) {
	uint32_t co2_u32 = 0;
	// if something go wrong with DMA - just restart receiving
	// 	if (hdma_usart2_rx.State == HAL_DMA_STATE_ERROR) {
	// 		HAL_UART_DMAStop(&huart3);
	// 		HAL_UART_Receive_DMA(&huart3, rx_circular_buffer, RX_BUFFER_SIZE);
	// 	}
	// how many bytes in buffer
	uint32_t rx_count = RingBuffer_DMA_Count(&rx_buffer2);

	sprintf(UartBuff,"uart_recieve %d byte\r\n",(int)rx_count);
	One_pin_debug_print( (uint8_t *)UartBuff, strlen(UartBuff) ) ;

	while (rx_count--) {
		uint8_t c = RingBuffer_DMA_GetByte(&rx_buffer2);
		sprintf(UartBuff,"%02X ", (int)c );
		One_pin_debug_print( (uint8_t *)UartBuff, strlen(UartBuff) ) ;
		cmd_co2_char[icmd_co2_u32++] = c;
	}
	co2_u32 = cmd_co2_char[2] * 0xFF + cmd_co2_char[3];

	uint8_t CheckSum = getCheckSum( cmd_co2_char );

	if ( cmd_co2_char[8] == CheckSum ) {
		sprintf( UartBuff, "; CheckSum Ok.\r\n" ) ;
	} else {
		sprintf( UartBuff, "; CheckSum Error: Result well be 333.\r\n" ) ;
		co2_u32 = 333 ;
	}
	One_pin_debug_print ( (uint8_t *)UartBuff, strlen(UartBuff) );

	cmd_co2_char[icmd_co2_u32++] = 0; // we received whole command, setting end of string
	icmd_co2_u32 = 0;

	return co2_u32;
}
//************************************************************************

void SetTimeFlag(uint8_t _set_time_flag_u8) {
	time_flag_u8 = _set_time_flag_u8;
}
//************************************************************************

uint8_t GetTimeFlag(void) {
	return time_flag_u8;
}

/*
**************************************************************************
*                           LOCAL FUNCTIONS
**************************************************************************
*/
uint8_t getCheckSum(char *packet) {
	if (packet == _NULL) {
		return 66;
	}

	uint8_t checksum = 0x00;
	for( uint8_t i = 1; i < 8; i++) {
		checksum += packet[i];
		}
	checksum = 0xff - checksum;
	checksum += 1;
	return checksum;
}

/*
**************************************************************************
*                          			END
**************************************************************************
*/
