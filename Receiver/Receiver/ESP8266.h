#ifndef ESP8266_H_
#define ESP8266_H_

/************************************************************************/
/*                             ECHO                                     */
/************************************************************************/
/* Echoes any data received by UART
 * ESP_ECHO 0 - no echo
 * ESP_ECHO 1 - echo
*/
#define ESP_ECHO 1

/************************************************************************/
/*                      UART CONFIGURATION                              */
/************************************************************************/
/*
 * ESP_DATABITS 5 - 5 bits
 * ESP_DATABITS 6 - 6 bits
 * ESP_DATABITS 7 - 7 bits
 * ESP_DATABITS 8 - 8 bits
*/
#define ESP_DATABITS 8
/*
 * ESP_STOPBITS 1 - 1 stop bit
 * ESP_STOPBITS 2 - 1.5 stop bit
 * ESP_STOPBITS 3 - 2 stop bits
*/
#define ESP_STOPBITS 1
/*
 * ESP_PARITY 0 - none
 * ESP_PARITY 1 - Odd
 * ESP_PARITY 2 - Even
*/
#define ESP_PARITY 0
/*
 * ESP_FLOWCONTROL 0 - Disabled
 * ESP_FLOWCONTROL 1 - RTS Enabled
 * ESP_FLOWCONTROL 2 - CTS Enabled
 * ESP_FLOWCONTROL 3 - RTS and CTS Enabled
*/
#define ESP_FLOWCONTROL 3 

/************************************************************************/
/*                         ESP8266 SETTINGS                             */
/************************************************************************/
/* Sleep mode can only be used in station mode.
 * ESP_SLEEPMODE 0 - Sleep mode disabled
 * ESP_SLEEPMODE 1 - Light sleep mode
 * ESP_SLEEPMODE 2 - Modem sleep mode
*/
#define ESP_SLEEPMODE 2

/************************************************************************/
/*                          WIFI SETTINGS                               */
/************************************************************************/
/*
 * ESP_WIFIMODE 1 - Station mode
 * ESP_WIFIMODE 2 - softAP mode
 * ESP_WIFIMODE 3 - softAP + station
*/
#define ESP_WIFIMODE 1

#if ESP_WIFIMODE == 1 || ESP_WIFIMODE == 3
	/* This setting is for existing access point
	 * For any special characters in access point in name or password
	 * put a / before the character. For example:
	 * ESP_APNAME "ab/c" will become "ab//c"
	 * ESP_APNAME "ab,c" will become "ab/,c"
	 * ESP_APNAME "ab"c" will become "ab/"c"
	*/
#endif

#if ESP_WIFIMODE == 2 || ESP_WIFIMODE == 3
	/* This setting is for access point made by ESP
	 * For any special characters in access point in name or password
	 * put a / before the character
	 * ESP_APNAME "ab/c" will become "ab//c"
	 * ESP_APNAME "ab,c" will become "ab/,c"
	 * ESP_APNAME "ab"c" will become "ab/"c"
	*/
	#define ESP_SSID "ESP8266-AP"
	#define ESP_PASSWORD "2z5lo23c0"
	#define ESP_CHANNELID "5"
	/*
	 * ESP_ENCRYPTION 0 - Open
	 * ESP_ENCRYPTION 2 - WPA_PSK
	 * ESP_ENCRYPTION 3 - WPA2_PSK
	 * ESP_ENCRYPTION 4 - WPA_WPA2_PSK
	*/
	#define ESP_ENCRYPTION "4"
	/* IP address of ESP access point */
	#define ESP_APIP "192.168.1.80"
	/* MAC address of ESP access point */
	#define ESP_APMAC "1A:FE:34:2E:FD:D1"
#endif /* ESP_WIFIMODE 1 || ESP_WIFIMODE 2 */

/*
 * ESP_DHCP 0 - Disabled
 * ESP_DHCP 1 - Enabled
*/
#define ESP_DHCP 1

/*
 * ESP_AUTOCONNECT 0 - Do not auto connect to Access Point
 * ESP_AUTOCONNECT 1 - Auto-Connect to Access Point when powered on
*/
#define ESP_AUTOCONNECT 1

/* MAC ADDRESS OF THE ESP */
#define ESP_MAC "18:fe:35:98:d3:7b"

/************************************************************************/
/*                          TCP/UDP SETTINGS                            */
/************************************************************************/
#define ESP_IP "192.168.1.255"
#define ESP_PORT "1010"
/*
 * ESP_CONNECTIONTYPE 1 - "UDP"
 * ESP_CNNECTIONTYPE 2 - "TCP"
*/
#define ESP_CONNECTIONTYPE 1

/*
 * ESP_MULTIPLECONNECTIONS 0 - Single connection
 * ESP_MULTIPLECONNECTIONS 1 - Multiple connections
*/
#define ESP_MULTIPLECONNECTIONS 0

/************************************************************************/
/*                              RESPONSES                               */
/************************************************************************/
typedef struct
{
	char OK;
	char ERROR;
	char INPUT;
	char CONNECTED;
}ESP_RESP;

extern volatile ESP_RESP ESP_Response;
/************************************************************************/
/*                                 TCP                                  */
/************************************************************************/
uint8_t ESP_Connections;
/************************************************************************/
/*                           GLOBAL COMMANDS                            */
/************************************************************************/
/* Initializes the module
 * Return values are:
 * 0 - everything is OK
 * 1 - Error occurred
 * 2 - Couldn't connect to Access Point
 * 3 - Couldn't setup an Access Point
*/
uint8_t ESP_Initialize(void);
void ESP_AT(void); //Sends AT command. 1 if OK
void ESP_RESTART(void); //Restarts the module
uint8_t ESP_Send(char* data, char* connectionID); //0 if OK, 1 if ERROR
void ResetResponse(void);
void WaitForResponse(void);

/************************************************************************/
/*                            TCP COMMANDS                              */
/************************************************************************/
#if ESP_CONNECTIONTYPE == 2

void ESP_StartTCPServer(uint16_t port);
void ESP_StopTCPServer(void);
#endif /* ESP_CONNECTIONTYPE == 2 */
#endif /* ESP8266_H_ */