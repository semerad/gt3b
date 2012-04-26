/*
    vector - interrupt vector table
    Copyright (C) 2011 Pavel Semerad

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/




typedef void @far (*intr_handler_t)(void);

struct intr_vector {
	unsigned char intr_inst;
	intr_handler_t intr_handler;
};
#define INTR_OP  0x82
#define INTR_VEC(func) { INTR_OP, (intr_handler_t)func }

@interrupt void DefaultInterrupt (void) {
	return;
}
#define INTR_DEFAULT INTR_VEC(DefaultInterrupt)


extern void _stext();     /* startup routine */
extern void ppm_interrupt(void);
extern void timer_interrupt(void);


struct intr_vector const _vectab[] = {
	INTR_VEC(_stext),	/* reset */
	INTR_DEFAULT,		/* trap  */
	INTR_DEFAULT,		/* 0  TLI - External top level interrupt */
	INTR_DEFAULT,		/* 1  AWU - Auto wakeup from halt */
	INTR_DEFAULT,		/* 2  CLK - Clock controller */
	INTR_DEFAULT,		/* 3  EXTI0 - Port A external interrupts */
	INTR_DEFAULT,		/* 4  EXTI1 - Port B external interrupts */
	INTR_DEFAULT,		/* 5  EXTI2 - Port C external interrupts */
	INTR_DEFAULT,		/* 6  EXTI3 - Port D external interrupts */
	INTR_DEFAULT,		/* 7  EXTI4 - Port E external interrupts */
	INTR_DEFAULT,		/* 8  */
	INTR_DEFAULT,		/* 9  */
	INTR_DEFAULT,		/* 10 SPI - End of transfer */
	INTR_DEFAULT,		/* 11 TIM1 - update/overflow/underflow/trigger/break */
	INTR_DEFAULT,		/* 12 TIM1 - capture/compare */
	INTR_VEC(timer_interrupt),/* 13 TIM2 - update/overflow */
	INTR_DEFAULT,		/* 14 TIM2 - capture/compare */
	INTR_VEC(ppm_interrupt),/* 15 TIM3 - update/overflow */
	INTR_DEFAULT,		/* 16 TIM3 - capture/compare */
	INTR_DEFAULT,		/* 17 */
	INTR_DEFAULT,		/* 18 */
	INTR_DEFAULT,		/* 19 I2C - interrupt */
	INTR_DEFAULT,		/* 20 UART2 - Tx complete */
	INTR_DEFAULT,		/* 21 UART2 - Receiver register DATA FULL */
	INTR_DEFAULT,		/* 22 ADC1 - end of conversion */
	INTR_DEFAULT,		/* 23 TIM4 - update/overflow */
	INTR_DEFAULT,		/* 24 Flash - EOP/WR_PG_DIS */
	INTR_DEFAULT,		/* 25 */
	INTR_DEFAULT,		/* 26 */
	INTR_DEFAULT,		/* 27 */
	INTR_DEFAULT,		/* 28 */
	INTR_DEFAULT,		/* 29 */
};

