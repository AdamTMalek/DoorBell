#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <util/delay.h>
#include <stdio.h>

#include "uart.h"
#include "ESP8266.h"

volatile ESP_RESP ESP_Response;

/************************************************************************/
/*                        INITIALIZATION                                */
/************************************************************************/
uint8_t ESP_Initialize(void)
{
	/*--- Checking communication between microcontroller and ESP ---*/
	ResetResponse();
	ESP_AT();
	WaitForResponse();
	ResetResponse();

	/*--- ECHO ---*/
#if ESP_ECHO == 0
	UART_SendString("ATE0\r\n");
#else
	UART_SendString("ATE1\r\n");
#endif
	WaitForResponse();
	ResetResponse();

#if ESP_DHCP == 1
	UART_SendString("AT+CWDHCP=2,1\r\n");
#else
	UART_SendString("AT+CWDHCP=2,0\r\n");
#endif
	WaitForResponse();
	ResetResponse();

	/*--- MODE ---*/
#if ESP_WIFIMODE == 1
	UART_SendString("AT+CWMODE=1\r\n"); //STATION MODE
#elif ESP_WIFIMODE == 2
	UART_SendString("AT+CWMODE=2\r\n"); //ACCESS POINT MODE
#elif ESP_WIFIMODE == 3
	UART_SendString("AT+CWMODE=3\r\n"); //STATON + ACCESS POINT MODE
#endif
	WaitForResponse();
	ResetResponse();

	/*--- CONNECTING TO WIFI ---*/
#if ESP_WIFIMODE == 1 || ESP_WIFIMODE  == 3
#ifdef ESP_APNAME
#ifdef ESP_APPASSWORD
	ESP_Response.ERROR = 0;
	
	UART_SendString("AT+CWJAP=\"");
	UART_SendString(ESP_APNAME);
	UART_SendString("\",\"");
	UART_SendString(ESP_APPASSWORD);
	UART_SendString("\"\r\n");
	WaitForResponse();

	if (ESP_Response.ERROR == 1)
	{
		ESP_Response.ERROR = 0;
		return 2;
	}
	else
	{
		ESP_Response.OK = 0;
	}
#else
	#error Undefined PASSWORD
#endif
#else
	#error Undefined SSID
#endif

#endif /* ESP_WIFIMODE == 1 || ESP_WIFIMODE == 3 */
	
	/*--- SETTING UP ACCESS POINT ---*/
#if ESP_WIFIMODE == 2 || ESP_WIFIMODE == 3
	UART_SendString("AT+CWSAP=\"");
	UART_SendString(ESP_SSID);
	UART_SendString("\",\"");
	UART_SendString(ESP_PASSWORD);
	UART_SendString("\",");
	UART_SendString(ESP_CHANNELID);
	UART_SendChar(',');
	UART_SendString(ESP_ENCRYPTION);
	UART_SendString("\r\n");
	WaitForResponse();

	if (ESP_Response.ERROR)
	{
		ESP_Response.ERROR = 0;
		return 3;
	}
	else
	{
		ESP_Response.OK = 0;
	}

	UART_SendString("AT+CIPAP=\"");
	UART_SendString(ESP_APIP);
	UART_SendString("\"\r\n");
	WaitForResponse();
	if (ESP_Response.ERROR)
	{
		ESP_Response.ERROR = 0;
		return 3;
	}
	else
	{
		ESP_Response.OK = 0;
	}

	#ifdef ESP_APMAC
	UART_SendString("AT+CIPAPMAC=\"");
	UART_SendString(ESP_APMAC);
	UART_SendString("\"\r\n");
	WaitForResponse();
	ResetResponse();
	#endif

#endif /* ESP_WIFIMODE == 2 || ESP_WIFIMODE == 3 */
#if ESP_CONNECTIONTYPE == 1 //UDP
	UART_SendString("AT+CIPMUX=0\r\n");
	WaitForResponse();
	ResetResponse();
	UART_SendString("AT+CIPSTART=\"UDP\",\"");
	UART_SendString(ESP_IP);
	UART_SendString("\",");
	UART_SendString(ESP_PORT);
	UART_SendString("\r\n");
	WaitForResponse();
	ResetResponse();
#else
	UART_SendString("AT+CIPDINFO=1\r\n");
	WaitForResponse();
	ResetResponse();
#endif
	return 0;
}
/************************************************************************/
/*                          GENERAL COMMANDS                            */
/************************************************************************/
void ESP_AT(void)
{
	UART_SendString("AT\r\n");
}
void ESP_RESTART(void)
{
	UART_SendString("AT+RST\r\n");
	_delay_ms(10);
	ESP_Response.OK = 0;
}
inline void ResetResponse(void)
{
	ESP_Response.OK = 0;
	ESP_Response.ERROR = 0;
	ESP_Response.INPUT = 0;
}
void WaitForResponse(void)
{
	while (ESP_Response.OK == 0 &&
		   ESP_Response.ERROR == 0 &&
		   ESP_Response.INPUT == 0);
}
/************************************************************************/
/*                               SENDING                                */
/************************************************************************/
uint8_t ESP_Send(char* data, char* connectionID)
{
#if ESP_CONNECTIONTYPE == 2
	if (ESP_Connections == 0) //If there are no connections...
		return 1; //Return 1 - error
#endif
	//Length of data
	uint16_t size = strlen(data);
	char length[4];
	sprintf(length, "%d", size);

	UART_SendString("AT+CIPSEND=");
#if ESP_CONNECTIONTYPE == 2
	UART_SendString(connectionID);
	UART_SendString(",");
#endif
	UART_SendString(length);
	UART_SendString("\r\n");
	
	while (!ESP_Response.INPUT && !ESP_Response.ERROR)
		;

	if (ESP_Response.INPUT == 1)
	{
		UART_SendString(data);
	}
	else if (ESP_Response.ERROR) 
	{
		ESP_Response.ERROR = 0;
		return 1;
	}

#if ESP_CONNECTIONTYPE == 2
	UART_SendString("AT+CIPCLOSE=0\r\n");
	WaitForResponse();
#endif
	ResetResponse();
	return 0;
}
/************************************************************************/
/*                            TCP COMMANDS                              */
/************************************************************************/
void ESP_StartTCPServer(uint16_t port)
{
	UART_SendString("AT+CIPMUX=1\r\n");
	WaitForResponse();
	_delay_ms(50);

	char tcpPort[5];
	sprintf(tcpPort, "%d", port);
	UART_SendString("AT+CIPSERVER=1,");
	UART_SendString(tcpPort);
	UART_SendString("\r\n");

	WaitForResponse();
	ResetResponse();
}
void ESP_StopTCPServer()
{
	UART_SendString("AT+CIPSERVER=0\r\n");

	WaitForResponse();
	ResetResponse();
}