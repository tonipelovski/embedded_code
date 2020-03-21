#include <msp430.h>

void set_p1_dir(int, int);
void set_p1_state(int, int);
void toggle_p1(int);
int get_p1_state(int);
void set_p2_dir(int, int);
void set_p2_state(int, int);
void toggle_p2(int);
int get_p2_state(int);


int i = 0;
unsigned int ADC_value=0;

// Function prototypes
void ConfigureAdc(void);

void main(void)
{
       WDTCTL = WDTPW + WDTHOLD; // Stop WDT
       BCSCTL1 = CALBC1_1MHZ; // Set range
       DCOCTL = CALDCO_1MHZ;
       BCSCTL2 &= ~(DIVS_3); // SMCLK = DCO = 1MHz
       P1SEL |= BIT3; //ADC Input pin P1.3
       set_p1_dir(2,1);
       set_p1_state(2,0);

       set_p1_dir(1,1);
       set_p1_state(1,0);

        ConfigureAdc();                 // ADC set-up function call
        __enable_interrupt();           // Enable interrupts.

        while(1)
        {
            __delay_cycles(1000);               // Wait for ADC Ref to settle
            ADC10CTL0 |= ENC + ADC10SC;         // Sampling and conversion start
            __bis_SR_register(CPUOFF + GIE);    // Low Power Mode 0 with interrupts enabled
            ADC_value = ADC10MEM;// Assigns the value held in ADC10MEM to the integer called ADC_value

            if(ADC_value >= 512){
                set_p1_state(1,1); //start LED if shock sensor value over limit
            }else{
                set_p1_state(1,0); // stop LED if shock sensor value is in limit
            }

        }

}

// ADC10 interrupt service routine
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR (void)
{
    __bic_SR_register_on_exit(CPUOFF);        // Return to active mode }
}

// Function containing ADC set-up
void ConfigureAdc(void)
{

    ADC10CTL1 = INCH_3 + ADC10DIV_3 ;         // Channel 3, ADC10CLK/3
    ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON + ADC10IE;  // Vcc & Vss as reference, Sample and hold for 64 Clock cycles, ADC on, ADC interrupt enable
    ADC10AE0 |= BIT3;                         // ADC input enable P1.3
}

void set_p1_dir(int bit, int dir){
    if(dir == 1){
        P1DIR |= (1 << bit);
    }else{
        P1DIR &= ~(1 << bit);

    }
}

void set_p1_state(int bit, int state){
    if(state == 1){
        P1OUT |= (1 << bit);
    }else{
        P1OUT &= ~(1 << bit);

    }
}

void toggle_p1(int bit){
    P1OUT ^= (1 << bit);
}

int get_p1_state(int bit){
    return P1IN &= (1 << bit);
}



void set_p2_dir(int bit, int dir){
    if(dir == 1){
        P2DIR |= (1 << bit);
    }else{
        P2DIR &= ~(1 << bit);

    }
}

void set_p2_state(int bit, int state){
    if(state == 1){
        P2OUT |= (1 << bit);
    }else{
        P2OUT &= ~(1 << bit);

    }
}

void toggle_p2(int bit){
    P2OUT ^= (1 << bit);
}

int get_p2_state(int bit){
    return P2IN &= (1 << bit);
}
