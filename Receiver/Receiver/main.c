#define F_CPU 14745600
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#define BELL PB1
#include "uart.h"
#include "rf.h"
#include "ESP8266.h"

#define BAUD 115200
#define UBRR F_CPU/16/BAUD-1

char data[120]; //Data received by RF12
char crcErrorMsg[] = "CRC(CHECKSUM) ERROR";
char conntestMsg[] = "conntest";
uint8_t connCounter; //This will be used to sent connection test message (conntest)

void RingBell(void);

int main(void)
{
	_delay_ms(1000); //Waiting until ESP is ready

	DDRB |= (1<<BELL);

	// --- TIMER --- //
	TCCR1B = (1<<CS12) | (1<<CS10); // clk/1024 prescaler
	TIMSK1 = (1<<TOIE1); //Overflow interrupt enable

	// --- RF12 --- //
	RF_Initialize();
	RF_SetRange(RANGE_433MHZ);
	RF_SetFrequency(RFFREQ433(432.74));
	RF_SetBaudRate(9600);
	RF_SetBandwith(RxBW200, LNA_6, RSSI_79);
	RF_SetPower(PWRdB_0, TxBW120);
	RF_DisableWakeUpTimer();
	RF_Transmit(0x0000);
	RF_Transmit(0xCC77);
	uint8_t ret = 0;

	sei();

	// --- UART --- //
	UART_Initlialise(UBRR);
	ESP_Initialize();

	// --- SLEEP AND POWER MANAGEMENT --- //
	PRR = (1<<PRTWI) | (1<<PRTIM2) | (1<<PRADC);

	ESP_Send(conntestMsg,0);
	while (1)
	{
		if (connCounter >= 13) //Approximately a minute
		{
			ESP_Send(conntestMsg,0);
			connCounter = 0;
		}

		// --- RF12 RECEIVING DATA --- //
#if RF_UseIRQ == 1
		if (!(RF_status.status & 0x07))
		{
			RF_RxStart();
		}
		if (RF_status.New)
		{
			ret = RF_RxFinish(data);

			if (data > 0 && ret < 254)
			{
				RingBell();
				ESP_Send(data,0);
				data[16] = 0;
			}
			else if (!ret)
			{
				ESP_Send(crcErrorMsg, 0);
			}
		}
#else
		ret = RF_RxData(data);
		if (ret)
		{
			RingBell();
			ESP_Send(data,0);
		}
		else
		{
			ESP_Send(crcErrorMsg, 0);
		}
#endif
	}
}

void RingBell(void)
{
	PORTB |= (1<<BELL);
	_delay_ms(1000);
	PORTB &= ~(1<<BELL);
}

// --- "ALIVE" LED INTERRUPT --- //
ISR(TIMER1_OVF_vect) //4.5s on 14MHz clock
{
	connCounter++;	
}