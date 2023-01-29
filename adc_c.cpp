/*
 * adc.c
 *
 *  Created on: 28.04.2019
 *      Author: andre
 */

#include "adc_c.h"

namespace pliskin_adc
{

static void (*callback_conversion_complete) (const uint16_t adc_value) = NULL;

void adc_init (e_voltage_ref ref, adc_prescaler pre)
{
	uint8_t temp_reg, pre_u8 = (uint8_t) pre;

	// voltage reference
	if ((ADMUX >> REFS0) != ref)
	{
		temp_reg = ADMUX & 0x3F;
		ADMUX = temp_reg | (ref << REFS0);
	}

	// prescaler
	if ((ADCSRA & 0x07) != pre_u8)
	{
		temp_reg = ADCSRA & 0xF8;
		ADCSRA = (1 << ADEN) | pre_u8;
	}
}

void adc_auto_trigger (uint8_t enable, e_adc_trigger trigger)
{
	uint8_t temp_reg;

	if (enable)
	{
		temp_reg = ADCSRB & 0xF0;		// clear current trigger
		ADCSRB = temp_reg | trigger;	// set new trigger

		ADCSRA |= (1 << ADATE);			// auto trigger enable
	}
	else
		ADCSRA &= ~(1 << ADATE);		// auto trigger disable
}

// changing the channel disables auto-trigger!
void adc_config (e_adc_channel channel, void (*callback) (const uint16_t adc_value))
{
	uint8_t temp_reg;
	static e_adc_channel last_channel = e_adc0;

	if (channel != last_channel)
	{
		adc_stop();
		while (ADCSRA & (1 << ADSC));	// wait for completion

		temp_reg = ADMUX & 0xE0;		// filter MUX[0:4]

		// set channel
		temp_reg |= (channel & 0x1F);	// set MUX[0:4]
		ADMUX     = temp_reg;

#if defined (__AVR_ATmega32U4__)
		temp_reg  = (ADCSRB  & ~(1 << MUX5));
		temp_reg |= (channel &  (1 << MUX5));	// set MUX5
		ADCSRB    = temp_reg;
#endif

		last_channel = channel;
	}

	// register callback
	callback_conversion_complete = callback;

	if (callback != NULL)
		ADCSRA |= (1 << ADIE);
	else
		ADCSRA &= ~(1 << ADIE);
}

void adc_start (void)
{
	ADCSRA |= (1 << ADSC);
}

void adc_stop (void)
{
	adc_auto_trigger(OFF, e_trigFreeRunning);	// second parameter is ignored if "enabled" = off
}

uint16_t adc_get_vcc_mV (void)
{
	uint16_t vcc;
	uint32_t adc2vcc;

	// save current status of all relevant registers
#if defined (__AVR_ATmega32U4__)
	e_adc_channel prev_channel = (ADMUX & 0x1F) | (ADCSRB | (1 << MUX5));
#elif defined (__AVR_ATmega328P__)
	e_adc_channel prev_channel = (e_adc_channel) (ADMUX & 0x1F);
#endif
	e_voltage_ref prev_ref	    = (e_voltage_ref) (ADMUX >> REFS0);
	uint8_t       prev_autoTrig = (ADCSRA & (1 << ADATE));
	uint8_t		  prev_preScal  = (ADCSRA & 0x07);

	adc_stop();
	while (ADCSRA & (1 << ADSC));	// wait for completion

	adc_init(e_vAVCC, e_adcPRE64);	// select supply voltage as reference, use a high prescaler for better accuracy
	adc_config(e_adcBandGap, callback_conversion_complete);	// measure bandgap voltage (approx. constant 1.1V, slightly temperature depended)
	ADCSRA &= ~(1 << ADIE);			// disable interrupt (this function waits for completion)
	ADCSRA |= (1 << ADIF);			// clear interrupt flag
	_delay_ms(10);

	adc_start();
	while (ADCSRA & (1 << ADSC));	// wait for completion


	// read adc value
	adc2vcc  = ADCL;
	adc2vcc |= (ADCH << 8);

	vcc      = (uint16_t) ((1100ul * 1023ul) / adc2vcc);	// assuming bandgap is always 1.1V

	// restore previous register values
	adc_init(prev_ref, (adc_prescaler) prev_preScal);
	adc_config(prev_channel, callback_conversion_complete);
	ADCSRA |= prev_autoTrig;

	return vcc;
}

void adc_enable (const uint8_t on_off)
{
	if (on_off == OFF)
		ADCSRA &= ~(1 << ADEN);
	else
		ADCSRA |= (1 << ADEN);
}

ISR(ADC_vect)
{
	uint16_t value = ADCL;	// make sure ADCH is read after ADCL
	value |= (ADCH << 8);

	// execute callback function
	if (callback_conversion_complete != NULL)
		callback_conversion_complete(value);
}

};