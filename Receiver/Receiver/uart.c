/*
 * uart.c
 *
 * Created: 11.07.2016 21:34:25
 *  Author: malek
 */ 
#include <avr/interrupt.h>
#include <avr/io.h>
#include "uart.h"
#include <stdlib.h>
#include "ESP8266.h"

typedef struct
{
	char characters[7]; //Currently the longest response that is interpreted is ERROR
	uint8_t* fullyReceived; //This will be used to change different statuses
}Message;

Message okMessage = {{'O', 'K', '\r', '\n'}, &ESP_Response.OK};
Message errorMessage = {{'E', 'R', 'R', 'O', 'R', '\r','\n'}, &ESP_Response.ERROR};
Message inputMessage = {{'>', ' '}, &ESP_Response.INPUT};

volatile unsigned char messageIndex;
volatile Message *message;

//volatile char buffer[100]; //Buffer for incoming data of UART
//volatile uint8_t bufferIndex = 0;

void UART_Initlialise(unsigned int ubrr)
{
	//SETTING BAUD RATE
	UBRR0H = (unsigned char) (ubrr>>8);
	UBRR0L = (unsigned char) ubrr;

	//ENABLING RECEIVER (RX) AND TRANSMITER (TX)
	UCSR0B = (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0);

	//SETTING FRAME FORMAT: 8-bit frame, 1 stop bit
	UCSR0C = (0<<USBS0) | (3<<UCSZ00);
}

/* --- SENDING --- */
void UART_SendChar(char data)
{
	while(!(UCSR0A & (1<<UDRE0))); //Waiting for empty transmit buffer

	UDR0 = data; //Putting data into buffer and sending the data
}
void UART_SendString(char *string)
{
	while(*string)
	{
		UART_SendChar(*string++);
	}
}

/* --- RECEIVING --- */
char UART_ReceiveChar(void)
{
	while(!(UCSR0A & (1<<RXC0))); //EMPTY LOOP - WAITING UNTIL DATA IS RECEIVED

	return UDR0;
}
ISR(USART_RX_vect)
{
	char character = UDR0;

	if(messageIndex == 0)
	{
		if(character == 'E')
		{
			message = &errorMessage;
			messageIndex++;
		}
		else if(character == 'O')
		{
			message = &okMessage;
			messageIndex++;
		}
		else if(character == '>')
		{
			message = &inputMessage;
			messageIndex++;
		}
	}
	else
	{
		if(character == message->characters[messageIndex])
		{
			messageIndex++;	
				
			if(message->characters[messageIndex] == '\0')
			{
				*message->fullyReceived = 1;
				messageIndex = 0;
			}
		}
		else
		{
			messageIndex = 0;
		}
	}

	//buffer[bufferIndex] = receivedByte;
	//if(receivedByte == '\n')
	//{
		//if(buffer[0] == 'O' && buffer[1] == 'K')
		//{
			//ESP_Response.OK = 1;
		//}
		//else if(buffer[0] == 'E' && buffer[1] == 'R' && buffer[4] == 'R')
		//{
			//ESP_Response.ERROR = 1;
		//}
		//else if(buffer[1] == ',' && buffer[3] == 'O')
		//{
			//ESP_Connections++;
		//}
		//else if(buffer[1] == ',' && buffer[3] == 'L')
		//{
			//ESP_Connections--;
		//}
		//else if(buffer[0] == 'S' && buffer[5] == 'O')
			//ESP_Response.OK;
		//bufferIndex = 0;
	//}
	//else if(receivedByte == ' ')
	//{
		//if(buffer[0] == '>')
		//{
			//ESP_Response.INPUT = 1;
			//bufferIndex = 0;
		//}	
	//}
	//else
	//{
		//bufferIndex++;
	//}
}