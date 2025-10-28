/*
 * GccApplication3.cpp
 *
 * Created: 21/10/2025 10:55:57
 * Author : stein
 */ 
#define F_CPU 8000000UL // Setter CPU klokke frekvens til 8 Mhz
#include <avr/io.h> // I/O register
#include <util/delay.h> // Tilater bruken av "_delay_ms"
#include <stdint.h> // Tilater bruken av "uint8_t" data type

void PWM_init(void) {
	DDRB |= (1 << PB1); // Setter PB1/OC1A(pin15) som ut pin

	// Setter verdier til Fast PWM. WGM10=1, WGM12=1
	TCCR1A = (1 << WGM10);
	TCCR1B = (1 << WGM12);

	// Non-inverting mode: COM1A1=1, COM1A0=0. Duty cycle = ORC1A/255
	TCCR1A |= (1 << COM1A1);

	// Prescaler = 64: CS11=1, CS10=1  //Setter PWM signal frekvensen ~488.03 Hz ved 8 MHz
	TCCR1B |= (1 << CS11) | (1 << CS10);

	// Starter med 0% duty som utgangspunkt
	OCR1A = 0; //
}

void set_PWM_duty(uint8_t duty_cycle) {
	OCR1A = duty_cycle*duty_cycle/255; //Setter duty cycle*duty cycle for å få uliner dimming (uten hopp i bunn)
}

int main(void) {
	PWM_init();

	while (1) {
		// Fade opp
		for (uint16_t i = 0; i < 256; i++) {
			set_PWM_duty((uint8_t)i);
			_delay_ms(1);
		}
		// Fade ned
		for (uint16_t i = 0; i < 256; i++) {
			set_PWM_duty((uint8_t)(255 - i));
			_delay_ms(1);
		}
	}
}
