#define F_CPU 16000000UL		// CPU Speed for delay
#define FOSC 16000000			// Clock speed
#define BAUD 115200				// Desire baud rate
#define MYUBRR FOSC/8/BAUD-1	// Formula to set the baud rate [Double Transmission]

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "nrf24l01-mnemonics.h"
#include "nrf24l01.h"

nRF24L01 *setup_rf(void);
void process_message(char *message, uint8_t len);
void UART_TX(char *data);

volatile bool rf_interrupt = false;

int main(void)
{
	// NRF Settings
	uint8_t address[5] = {0x02, 0x04, 0x06, 0x08, 0x0A};
	//prepare_led_pin();
	sei();
	nRF24L01 *rf = setup_rf();
	nRF24L01_listen(rf, 0, address);
	uint8_t addr[5];
	nRF24L01_read_register(rf, CONFIG, addr, 1);
	
	// USART Settings
	UBRR0H = ((MYUBRR) >> 8);	// Set baud rate for UPPER Register
	UBRR0L = MYUBRR;			// Set baud rate for LOWER Register
	UCSR0A |= (1 << U2X0);		// Double UART transmission speed
	UCSR0B |= (1 << TXEN0);		// Enable transmitter
	UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);	// Frame: 8-bit Data and 1 Stop bit
	
	while (1)
	{
		if(rf_interrupt)
		{
			rf_interrupt = false;
			while(nRF24L01_data_received(rf))
			{
				nRF24L01Message msg;
				nRF24L01_read_received_data(rf, &msg);
				process_message((char *)msg.data, msg.length);
			}
			nRF24L01_listen(rf, 0, address);
		}
	}
	return 0;
}

nRF24L01 *setup_rf(void)
{
	nRF24L01 *rf = nRF24L01_init();
	rf->ss.port = &PORTB;
	rf->ss.pin = PB2;
	rf->ce.port = &PORTB;
	rf->ce.pin = PB1;
	rf->sck.port = &PORTB;
	rf->sck.pin = PB5;
	rf->mosi.port = &PORTB;
	rf->mosi.pin = PB3;
	rf->miso.port = &PORTB;
	rf->miso.pin = PB4;
	EICRA |= _BV(ISC01);
	EIMSK |= _BV(INT0);
	nRF24L01_begin(rf);
	return rf;
}

void process_message(char *message, uint8_t len)
{
	uint8_t i = 0;		// Iterative variable
	char LF = '\n';		// Line Feed
	
	// Process the temperature and display to UART
	for(i = 0; i < len; i++)
	{
		UART_TX(&message[i]);
	}
	UART_TX(&LF);
}

ISR(INT0_vect)
{
	rf_interrupt = true;
}

// UART transmission function
void UART_TX(char *data)
{
	while(!(UCSR0A & (1 << UDRE0)));	// Wait for UART to be available
	UDR0 = *data;						// Send the data
}