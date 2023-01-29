/*
 * adc.h
 *
 *  Created on: 28.04.2019
 *      Author: andre
 */


#ifndef MEGA32U4_ADC_H_
#define MEGA32U4_ADC_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#ifndef OFF
#define OFF (0)
#endif

#ifndef ON
#define ON  (1)
#endif

namespace pliskin_adc
{

typedef enum e_voltage_ref
{
	e_vAREF = 0, 			// AREF pin (21); Nano: between the "3V3" and "A0" pin
	e_vAVCC = 1, 			// Supply Voltage, *should* be filtered with an external capacitor at AREF
	e_vInternal2_56 = 3		// internal 1.1 V reference voltage, *should* be filtered with an external capacitor at AREF
} e_voltage_ref;

typedef enum e_adc_prescaler {
	e_adcPRE2 = 1,
	e_adcPRE4,
	e_adcPRE8,
	e_adcPRE16,
	e_adcPRE32,
	e_adcPRE64,
	e_adcPRE128
} adc_prescaler;

typedef enum e_adc_trigger
{
	e_trigFreeRunning = 0,	// starts when adc_start() is called. if auto-trigger is disabled, only one conversion is done, otherwise free running
	e_trigACOMP,			// analog comparator
	e_trigINT0,				// external int0 request
	e_trigT0A,				// timer0 compare match A
	e_trigT0OVF,			// timer0 overflow
	e_trigT1B,				// timer1 compare match B
	e_trigT1OVF,			// timer1 overflow
	e_trigT1CAP,			// timer 1 capture event
#if defined (__AVR_ATmega32U4__)
	e_trigT4OVF,
	e_trigT4A,
	e_trigT4B,
	e_trigT4D
#endif
} e_adc_trigger;

// not all channels are available for both arduinos (nano/leonardo)
typedef enum e_adc_channel
{
	e_adc0 = 0, e_adc1, e_adc4 = 4, e_adc5, e_adc6, e_adc7,
#if defined (__AVR_ATmega328P__)
	e_adcBandGap = 0b1110, // the 1.1 internal voltage reference. ideal to check the supply voltage
	e_adcGND,
#elif defined (__AVR_ATmega32U4__)
	e_adcBandGap = 0b11110, // the 1.1 internal voltage reference. ideal to check the supply voltage
	e_adcGND,
	e_adc8, e_adc9, e_adc10, e_adc11, e_adc12, e_adc13,
	e_adcTEMP = 0b100111
#endif
} e_adc_channel;

void adc_init (e_voltage_ref ref, adc_prescaler pre);

/* used to start automatic adc conversion with the configured trigger signal
 * see the "timer" module for a time based trigger signal
 */
void adc_auto_trigger (uint8_t enable, e_adc_trigger trigger);

/* Configures the next ADC conversion to use the specified ADC channel
 * The provided function is called inside the ISR when conversion is completed
 */
void adc_config (e_adc_channel channel, void (*callback) (const uint16_t adc_value));

// starts a single conversion or free running
void adc_start (void);

// disables auto trigger
void adc_stop (void);

// calculates the actual supply voltage by comparing with the bandgap voltage
uint16_t adc_get_vcc_mV (void);

// enables or disables the ADC (useful with "sleep" since "idle" starts a conversion if ADC is enabled)
void adc_enable (const uint8_t on_off);

};
#endif /* MEGA32U4_ADC_H_ */
