#ifndef __RF
#define __RF

/************************************************************************/
/*                     PIN CONFIGURATION                                */
/************************************************************************/
#define RF_PORT		PORTB
#define RF_DDR		DDRB
#define RF_PIN		PINB
#define SCK			PB5
#define SDO			PB4
#define SDI			PB3
#define CS			PB2

#define RF_IRQDDR	DDRD
#define RF_IRQPIN	PIND
#define IRQ			PD2

/************************************************************************/
/*                      INTERRUPT REQUEST                               */
/************************************************************************/
/*
 *	RF_UseIRQ = 1 - interrupts enabled
 *	RF_UseIRQ = 0 - interrupt disabled
*/
#define RF_UseIRQ 1

#if RF_UseIRQ == 1
	#if IRQ == PD2
		#define INT_PIN INT0
		#define ISC_INT0 ISC00
		#define ISC_INT1 ISC01
		#define INT_VECT INT0_vect
	#elif IRQ == PD3
		#define INT_PIN INT1
		#define ISC_INT0 ISC10
		#define ISC_INT1 ISC11
		#define INT_VECT INT1_vect
	#else
		# warning "Pin defined by IRQ is not supported."
	#endif
#endif

/************************************************************************/
/*                       WAIT UNTIL SENT                                */
/************************************************************************/
// 1 - ENABLED | 0 - DISABLED
#define WAIT_UNTIL_SENT 0

/*
 *	If interrupts will be used, buffer needs to be specified
 *	The maximum value is 243
*/
#if RF_UseIRQ == 1
	#define RF_DataLength 100
#endif

/************************************************************************/
/*                          RANGES                                      */
/************************************************************************/
enum RANGE
{
	/*
	 *	RANGE 433MHz:
	 *	CHANGE:				2.5kHz
	 *	MINIMAL FREQUENCY:	430.24MHz
	 *	MAXIMAL FREQUENCY:	439.75MHz
	*/
	RANGE_433MHZ = 16,
	/*
	 *	RANGE 868MHz:
	 *	CHANGE:				5.0kHz
	 *	MINIMAL FREQUENCY:	860.48MHz
	 *	MAXIMAL FREQUENCY:	879.51MHz
	*/
	RANGE_866MHZ = 32,
	/*
	 *	RANGE 915MHz:
	 *	CHANGE:				7.5kHz
	 *	MINIMAL FREQUENCY:	900.72MHz
	 *	MAXIMAL FREQUENCY:	929.37MHz
	*/
	RANGE_915MHZ = 48
};

/************************************************************************/
/*                        BANDWIDTHS                                    */
/************************************************************************/
#define RxBW400		1
#define RxBW340		2
#define RxBW270		3
#define RxBW200		4
#define RxBW134		5
#define RxBW67		6

#define TxBW15		0
#define TxBW30		1
#define TxBW45		2
#define TxBW60		3
#define TxBW75		4
#define TxBW90		5
#define TxBW105		6
#define TxBW120		7

#define LNA_0		0
#define LNA_6		1
#define LNA_14		2
#define LNA_20		3

#define RSSI_103	0
#define RSSI_97		1
#define RSSI_91		2
#define RSSI_85		3
#define RSSI_79		4
#define RSSI_73		5
#define RSSI_67		6
#define RSSI_61		7

#define PWRdB_0		0
#define PWRdb_3		1
#define PWRdB_6		2
#define PWRdB_9		3
#define PWRdB_12	4
#define PWRdB_15	5
#define PWRdB_18	6
#define PWRdB_21	7

/************************************************************************/
/*                            MACROS                                    */
/************************************************************************/
#define RFFREQ433(freq)		((freq-430.24)/0.0025)
#define RFFREQ868(freq)		((freq-860.48)/0.0050)
#define RFFREQ915(freq)		((freq-900.72)/0.0075)

/************************************************************************/
/*                           RF STATUS                                  */
/************************************************************************/
typedef union
{
	uint8_t status;
	struct 
	{
		uint8_t Rx:1;
		uint8_t Tx:1;
		uint8_t New:1;
	};
} RF_STAT;

extern volatile RF_STAT RF_status;

/************************************************************************/
/*                           FUNCTIIONS                                 */
/************************************************************************/
uint16_t RF_Transmit(uint16_t data); //TRASNMIITTING DATA TO RF
void RF_Initialize(void); //INITIALISING MODULE
void RF_SetRange(enum RANGE range); //SETTING RANGE OF MODULE: 433MHz, 868MHz or 915MHz
void RF_DisableWakeUpTimer(void); //DISABLING WAKEUP-TIMER 
void RF_SetFrequency(uint16_t frequency); //SETTING FREQUENCY
void RF_SetBaudRate(uint16_t baud); //SETTING SPEED OF DATA TRANSMITTION
void RF_SetPower(uint8_t power, uint8_t mod); //SETTING POWER OF TRANSMITTER AND FSK
void RF_SetBandwith(uint8_t bandwidth, uint8_t gain, uint8_t drssi); //SETTINGS OF RECEIVER

#if RF_UseIRQ == 0
void RF_TxData(char* data, uint8_t size); //TRANSMITTING DATA FRAME FROM BUFFER AND SIZE OF IT
uint8_t RF_RxData(char* data); //RECEIVING DATA FRAME TO BUFFER
void RF_Ready(void); //WAIT UNTIL FIFO IS READY TO RECEIVE OR TRANSMIT
#endif

#if RF_UseIRQ == 1
uint8_t RF_RxStart(void); //START RECEIVING DATA FRAME
uint8_t RF_RxFinish(char* data); //RECEIVING DATA TO BUFFER ONCE IT'S RECEIVED PROPERLY
uint8_t RF_TxStart(char *data, uint8_t size); //STARTING TRANSMITTING DATA FRAME FROM BUFFER
void RF_AllStop(void);
#endif

#endif /* --- __RF --- */