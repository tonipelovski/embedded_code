//the program is combining rfid, adc and uart

#include "msp430g2553.h"
#include<string.h>
#define TXD BIT2
#define RXD BIT1

char string[] = { "AT+CIPSTART=\"TCP\",\"78.130.176.59\",5000\r\nAT+CIPSEND=7\r\n2002200\r\n" }; //AT Command

char result[300];
unsigned int index = 0;
unsigned int i;
char last;

//functions for easier work with GPIOs
void set_p1_dir(int, int);
void set_p1_state(int, int);
void toggle_p1(int);
int get_p1_state(int);
void set_p2_dir(int, int);
void set_p2_state(int, int);
void toggle_p2(int);
int get_p2_state(int);

void configRFid();
int checkShock();
int checkRFid();
void configShock();

char c;
int k;
int j;
int limit = 40;
int read_phase = 0;
unsigned char byte = 0x00;
unsigned int shift = 0x01;
int byte_pos = 0;
char arr[10];

int rfid_auth = 0;
int shock = 0;
char last;

unsigned int ADC_value=0;

int car_on = 0;
int rfid_checked = 0;

// Function prototypes
void ConfigureAdc(void);


int main(void)
{
    WDTCTL = WDTPW + WDTHOLD; // Stop the Watch dog

    if (CALBC1_1MHZ==0xFF)   // If calibration constant erased
    {
      while(1);          // do not load, trap CPU!!
    }
    DCOCTL  = 0;             // Select lowest DCOx and MODx settings
    BCSCTL1 = CALBC1_1MHZ;   // Set range
    DCOCTL  = CALDCO_1MHZ;   // Set DCO step + modulation

    P1SEL  |=  BIT1 + BIT2;  // P1.1 UCA0RXD input
    P1SEL2 |=  BIT1 + BIT2;  // P1.2 UCA0TXD output

    //configuring the UART
    UCA0CTL1 |= UCSSEL_2; // SMCLK
    UCA0BR0 = 0x08; // 1MHz 115200
    UCA0BR1 = 0x00; // 1MHz 115200
    UCA0MCTL = UCBRS2 + UCBRS0; // Modulation UCBRSx = 5
    UCA0CTL1 &= ~UCSWRST; // **Initialize USCI state machine**
    UC0IE |= UCA0RXIE; // Enable USCI_A0 RX interrupt

    IE2 |= UCA0TXIE; // Enable the Transmit interrupt
    IE2 |= UCA0RXIE; // Enable the Receive  interrupt
    _BIS_SR(GIE); // Enable the global interrupt

   ConfigureAdc();                 // ADC set-up function call

   configShock(); // set up shock sensor GPIOs

   configRFid(); // set up RFid - init phase, charge phase

   ADC10CTL0 |= ENC + ADC10SC;         // Sampling and conversion start

   __enable_interrupt();           // Enable interrupts.

   int to_send = 0;
   int shock_delay = 0;
   while(1){

       //check rfid if car is started
       if(car_on && !rfid_auth){
           __disable_interrupt(); //disable uart and adc interrupts in order to have correct timing with TMS3705
           rfid_auth = checkRFid();
           __enable_interrupt();
       }

       //check shock sensor for 10 seconds
       while(shock_delay < 1000){
           int temp_shock = checkShock();
           if(temp_shock > shock){
               shock = temp_shock;
           }
           __delay_cycles(1000);
           shock_delay++;
       }
       shock_delay = 0;

       if(to_send >= 10){ // send every 10 cycles
           k = 40;
           limit = 54;

           string[59] = shock + '0';
           string[60] = rfid_auth + '0';
           limit = sizeof string - 1;
           __delay_cycles(20000);
           IE2 |= UCA0TXIE; //enable interrupt to send data
           to_send = 0;
           shock = 0;
       }
       to_send++;
   }
}

#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
{
   UCA0TXBUF = string[k++]; // TX next character
   if (k >= limit) // TX over?
      UC0IE &= ~UCA0TXIE; // Disable USCI_A0 TX interrupt
}

  #pragma vector = USCIAB0RX_VECTOR
  __interrupt void USCI0RX_ISR(void)
  {
    if(i > 199){
        i = 0;
    }
    result[i++] = UCA0RXBUF;

    IFG2 &= ~UCA0RXIFG; // Clear RX flag
  }


#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR (void)
{
    __bic_SR_register_on_exit(CPUOFF);        // Return to active mode
}


void ConfigureAdc(void)
{

    ADC10CTL1 = INCH_4 + ADC10DIV_3;
    ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON + ADC10IE;  // Vcc & Vss as reference, Sample and hold for 64 Clock cycles, ADC on, ADC interrupt enable
    ADC10AE0 |= BIT4;                         // ADC input enable P1.4
}


void configShock(){
    P1SEL |= BIT4; //ADC Input pin P1.4
    P1REN |= BIT4;
    P1OUT &= ~BIT4;
}

void configRFid(){
    set_p1_dir(5,0); //read SCIO

    set_p1_dir(6,1); //write TXCT

    set_p1_state(6,1); //start with high TXCT

    //init phase
    for (j=0;j<2200;j++){        // TXCT high for ~2.2ms
    __no_operation();           // TXCT high for ~2.2ms
    }

    set_p1_state(6,0); //charge phase with default mode control register
    //wait 2s for charge phase
    for (j=0;j<200;j++){        // TXCT low for ~2s
      __delay_cycles(10000);
    }


}

int checkRFid(){
    set_p1_state(6,0); //charge phase with default mode control register
    //wait 2s for charge phase
     for (j=0;j<200;j++){        // TXCT low for ~2s
         __delay_cycles(10000);
     }


    set_p1_state(6,1); //start reading phase

    //check for SCIO high, first bit is always high
    for (j=0;j<20000;j++){        // TXCT high for ~20ms
          if(get_p2_state(2)){
              read_phase = 1;
              break;
          }
     }

     while(read_phase){

         //repeat reading for one byte until end byte
         for (j=0; j<8; j++) // one byte
         {
             //NRZ bit duration on SCIO Asynchronous mode 64us

             if (get_p2_state(5)){  // SCIO high
                 byte = byte | shift; // set current bit
             }
             shift = shift << 1; // shift to next bit
             int t = 0;
             for (t=0;t<64;t++){            // TXCT high for ~64us
               __no_operation();           // TXCT high for ~64us

             }
         }
         arr[byte_pos] = byte;
         byte_pos++;
         for (j=0;j<3000;j++){ //wait for ~3ms for SCIO start bit
              if(get_p2_state(5)){ //check if there is input on SCIO
                  read_phase = 1; //read_phase reading next byte
                  break;
              }else{
                  arr[byte_pos] = '\0';
                  read_phase = 0; //stop read_phase
              }
          }
     }

     int auth = 1;
     //check bytes to see if authorization is correct
     for(j = 0; j < byte_pos; j++){
         if(arr[j] != 1){
             auth = 0;
             break;
         }
     }

     return auth;
}

int checkShock(){

    ADC10CTL0 |= ENC + ADC10SC;         // Sampling and conversion start

    ADC_value = ADC10MEM;// Assigns the value held in ADC10MEM to the integer called ADC_value

    if(ADC_value >= 5){
      //start LED if shock sensor value over limit
      return 1;

    }else{
      // stop LED if shock sensor value is in limit
      return 0;
    }
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
