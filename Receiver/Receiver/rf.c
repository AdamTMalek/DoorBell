#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include "uart.h"

#include "rf.h"
/************************************************************************/
/*                        COMMON FUNCTIONS                              */
/************************************************************************/
//-------------------------INITIALIZATION-------------------------------//
void RF_Initialize(void)
{
	//	INTIALZATION OF PINS
	
	RF_DDR |= (1<<SDI) | (1<<SCK) | (1<<CS);
	RF_DDR &= ~(1<<SDO);
	
	RF_PORT |= (1<<CS);
	//SPCR = (1<<SPE) | (1<<MSTR); //SCK - Fosc/128

#if RF_UseIRQ == 1
	RF_PORT |= (1<<SDO);
#endif

	_delay_ms(100); //WAITING FOR MODULE TO RESET

#if RF_UseIRQ == 1
	RF_status.Rx = 0;
	RF_status.Tx = 0;
	RF_status.New = 0;

	RF_IRQDDR &= ~(1<<IRQ);
	//LOW STATE ON INT0 WILL GENERATE AN INTERRUPT
	EICRA &= ~(1<<ISC01);
	EICRA &= ~(1<<ISC00);

	EIMSK |= (1<<INT0);
#endif
}

//----------------------------TRANSMITION-------------------------------//
uint16_t RF_Transmit(uint16_t data)
{
	uint16_t ret_val = 0;
	RF_PORT &= ~(1<<CS);

	for (uint8_t i = 0; i < 16; i++)
	{
		if (data & 0x8000) 
			RF_PORT |= (1<<SDI);
		else 
			RF_PORT &= ~(1<<SDI);

		ret_val <<= 1;

		if (RF_PIN & (1<<SDO)) 
			ret_val |= 1;
		
		RF_PORT |= (1<<SCK);
		data <<= 1;
		asm("nop");
		asm("nop");
		RF_PORT &= ~(1<<SCK);
	}

	RF_PORT |= (1<<CS);
	return ret_val;
}

// Calculate CRC16 checksum
uint16_t UpdateChecksum(uint16_t checksum, uint8_t data)
{
	uint16_t tmp = (data<<8);
	for(int i = 0; i < 8; i++)
	{
		if ((checksum ^ tmp) & 0x8000)
			checksum = (checksum<<1) ^ 0x1021;
		else
			checksum = (checksum<<1);
		tmp <<= 1;
	}
	return checksum;
}

void RF_SetRange(enum RANGE range)
{
	RF_Transmit(0x80C7 | range); //ENABLE FIFO, SET RANGE
}

void RF_SetBandwith(uint8_t bandwidth, uint8_t gain, uint8_t drssi)
{
	RF_Transmit(0x9000 | ((bandwidth & 7) << 5) | ((gain & 3) << 3) | (drssi & 7));
}

void RF_SetFrequency(uint16_t frequency)
{
	if(frequency < 96) // Below minimum
		frequency = 96;
	else if (frequency > 3903) // Above maximum
		frequency = 3903;

	RF_Transmit(0xA000 | frequency);
}

void RF_SetBaudRate(uint16_t baud)
{
	if (baud < 663)
		return;
	if (baud < 5400)
		RF_Transmit(0xC680 | ((43104 / baud) - 1));
	else
		RF_Transmit(0xC600 | ((344828UL / baud) - 1));		
}

void RF_DisableWakeUpTimer(void)
{
	RF_Transmit(0xE000);
}

void RF_SetPower(uint8_t power, uint8_t mod)
{
	RF_Transmit(0x9800 | (power & 7) | ((mod & 15) <<4));
}

/************************************************************************/
/*                        INTERRUPT FUNCTIONS                           */
/************************************************************************/
#if RF_UseIRQ == 1

volatile RF_STAT RF_status;	//RF STATUS STRUCTURE
volatile uint8_t RF_Index = 0;
uint8_t RF_Data[(RF_DataLength + 10)]; // +10 FOR THE REST OF THE FRAME

//----------------------------------------------------------------------//
//-------------------------------ISR------------------------------------//
ISR(INT0_vect)
{	
	if (RF_status.Rx)
	{
		if (RF_Index < RF_DataLength)
		{
			RF_Data[RF_Index++] = RF_Transmit(0xB000) & 0x00FF;
		}
		else
		{
			RF_Transmit(0x8208);
			RF_status.Rx = 0;
			RF_status.New = 1; //FRAME NOT RIGHT
			return;
		}
		if (RF_Index >= (RF_Data[0] + 3))
		{
			RF_Transmit(0x8208);
			RF_status.Rx = 0;
			RF_status.New = 1; //FRAME IS RIGHT
		}
	}
	else if (RF_status.Tx)
	{
		RF_Transmit(0xB800 | RF_Data[RF_Index]);
		if (!RF_Index)
		{
			RF_status.Tx = 0;
			RF_Transmit(0x8208); //TX OFF
		}
		else
		{	
			RF_Index--;
		}
	}
}
//-----------------------------DATA RECEIVE-----------------------------//
uint8_t RF_RxStart(void)
{
	if (RF_status.New)
		return 1;		//BUFFER NOT EMPTY YET
	if (RF_status.Tx)
		return 2;		//TRANSMITION IN PROGRESS
	if (RF_status.Rx)
		return 3;		//RECEIVING IN PROGRESS

	RF_Transmit(0x82C8); //RX ON

	//FIFO RESET
	RF_Transmit(0xCA81);
	RF_Transmit(0xCA83);

	RF_Index = 0;
	RF_status.Rx = 1;

	return 0;			//EVERYTING OK
}

/*	FUNCTION CHECKING AND FINISHING RECEIVING OF FULL DATA FRAME
 *
 *	*data - buffer for the frame
 *
 *	Value return will be:
 *	Size of the frame
 *	OR (ERRORS)
 *	255 - receiving in progress
 *	254 - previous frame was not read
*/
uint8_t RF_RxFinish(char* data)
{
	uint16_t crc;
	uint16_t crc_checksum = 0;
	uint8_t size = RF_Data[0];

	if (RF_status.Rx)
		return 255;
	if (!RF_status.New)
		return 254;

	if (size > RF_DataLength)
	{
		data[0] = 0;
		RF_status.New = 0;
		return 0; //FRAME SIZE NOT RIGHT
	}

	for (uint8_t i = 0; i < (size + 1); i++)
	{
		crc_checksum = UpdateChecksum(crc_checksum, RF_Data[i]);
	}

	crc = RF_Data[i++];
	crc |= RF_Data[i] << 8;
	RF_status.New = 0;
	
	if (crc != crc_checksum)
	{
		return 0; //DATA NOT RIGHT - DIFFERENT CHECKSUMS
	}
	else
	{
		for (int i = 0; i < size; i++)
		{
			data[i] = RF_Data[i + 1];
		}

		data[size] = 0; //ENDING FRAME WITH 0
		return size; //SIZE OF RECEIVED FRAME IN BYTES
	}
}
/*	FUNCTION INITIALIZING START OF DATA TRANSMITION
 *	*data - data buffer
 *	size = 0 when ASCII string is transmitted
 *	size > 0 when transmitting other data (binary)
 *
 *	Returned values:
 *	0 - transmition initialized properly
 *	1 - receiving data frame in progress
 *	3 - data frame is to big to transmit
*/

uint8_t RF_TxStart(char *data, uint8_t size)
{
	if (!size)
		size = strlen(data);

	if (RF_status.Tx)
		return 1;
	if (RF_status.Rx)
		return 2;
	if (size > RF_DataLength)
		return 3;

	RF_status.Tx = 1;

	//	Increasing frame size by 10 for:
	//	Preamble of 3 bytes, 2 synchrobytes and one byte for the size of the frame
	//	CRC16 Checksum and last preamble bytes
	RF_Index = size + 9;
	uint8_t index = RF_Index;
	RF_Data[index--] = 0xAA;
	RF_Data[index--] = 0xAA;
	RF_Data[index--] = 0xAA;
	RF_Data[index--] = 0x2D;
	RF_Data[index--] = 0xD4;
	RF_Data[index--] = size;

	uint16_t checksum = UpdateChecksum(0, size);
	for (int i = 0; i < size; i++)
	{
		RF_Data[index--] = data[i];
		checksum = UpdateChecksum(checksum,data[i]);
	}

	RF_Data[index--] = (checksum & 0x00FF);
	RF_Data[index--] = (checksum >> 8);
	RF_Data[index--] = 0xAA;
	RF_Data[index--] = 0xAA;

	RF_Transmit(0x8238); //TX ON

#if WAIT_UNTIL_SENT == 1
	uint8_t timeout = 250;

	while (timeout-- && RF_status.Tx)
	{
		_delay_ms(1);
	}
#endif

	return 0;
}

// Stops transmission and reception
void RF_AllStop(void)
{
	RF_status.Rx = 0;
	RF_status.Tx = 0;
	RF_status.New = 0;
	RF_Transmit(0x8208); //Idle
	RF_Transmit(0x0000); 
}
#endif //END OF IF USEIRQ == 1

/************************************************************************/
/*                      NON-INTERRUPT FUNCTIONS                         */
/************************************************************************/
#if RF_UseIRQ == 0
void RF_Ready(void)
{
	RF_PORT &= ~(1<<CS);
	while (!(RF_PIN & (1 << SDO)))
		; //Do nothing until FIFO is ready
}

void RF_TxData(char* data, uint8_t size)
{
	RF_Transmit(0x8238); //TX ON

	if (!size)
		size = strlen(data);

	RF_Ready();
	RF_Transmit(0xB8AA);
	RF_Ready();
	RF_Transmit(0xB8AA);
	RF_Ready();
	RF_Transmit(0xB8AA);
	RF_Ready();
	RF_Transmit(0xB82D);
	RF_Ready();
	RF_Transmit(0xB8D4);
	RF_Ready();
	RF_Transmit(0xB800 | size);

	uint16_t checksum = UpdateChecksum(0, size);
	for (int i = 0; i < size; i++)
	{
		RF_Ready();
		RF_Transmit(0xB800 | data[i]);
		checksum = UpdateChecksum(checksum, data[i]);
	}
	RF_Ready();
	RF_Transmit(0xB800 | (checksum & 0x00FF));
	RF_Ready();
	RF_Transmit(0xB800 | (checksum >> 8));
	RF_Ready();
	RF_Transmit(0xB8AA);
	RF_Ready();
	RF_Transmit(0xB8AA);

	RF_Ready();
	RF_Transmit(0x8208); //TX OFF
}
uint8_t RF_RxData(char* data)
{
	RF_Transmit(0x82C8); //RX ON

	//FIFO RESET
	RF_Transmit(0xCA81);
	RF_Transmit(0xCA83);

	RF_Ready();
	uint8_t num = (RF_Transmit(0xB000) & 0x00FF);
	uint16_t crc_Checksum = UpdateChecksum(0, num);

	for (int i = 0; i < num; i++)
	{
		RF_Ready();
		data[i] = (char)(RF_Transmit(0xB000) & 0x00FF);
		crc_Checksum = UpdateChecksum(crc_Checksum, data[i]);
	}

	RF_Ready();
	uint16_t checksum = (RF_Transmit(0xB000) & 0x00FF);
	RF_Ready();
	checksum |= (RF_Transmit(0xB000) << 8);

	RF_Transmit(0x8208);

	if (checksum != crc_Checksum)
	{
		num = 0;
		data[0] = 0;
	}
	data[num] = 0;
	return num;
}
#endif //END OF IF USEIRQ == 0