#include <msp430.h> 

void set_p1_dir(int, int);
void set_p1_state(int, int);
void toggle_p1(int);
int get_p1_state(int);
void set_p2_dir(int, int);
void set_p2_state(int, int);
void toggle_p2(int);
int get_p2_state(int);
char arr[10];
int i;
int k;
void main(void)
{

       WDTCTL = WDTPW + WDTHOLD; // Stop WDT

       //set the msp clock at 8MHz
       BCSCTL1 = CALBC1_8MHZ;
       DCOCTL = CALDCO_8MHZ;
       BCSCTL2 &= ~(DIVS_3); // SMCLK = DCO = 1MHz

       set_p1_dir(5,0); //read SCIO

       set_p1_dir(6,1); //write TXCT
       set_p1_state(6,1); //write TXCT

       int j = 0;

       for (j=0;j<20050;j++){ //INIT PHASE
        }
       
       int read_phase = 0;
       unsigned char byte = 0x00;
       unsigned int shift = 0x01;
       int byte_pos = 0;

       while(1){
          
           set_p1_state(6,0); //charge phase with default mode control register
           //wait 2s for charge phase

           for(j=0; j < 20000; j++){
               __delay_cycles(800);
           }
           set_p1_state(6,1); //start reading phase

           for(j=0; j < 20000; j++){
               if(get_p1_state(5)){
                   read_phase = 1;
                   break;
               }
          }
            int counter = 0;
            while(read_phase){
                    counter++;
                //repeat reading for one byte until end byte
                for (j=0; j<8; j++) // one byte
                {
                    //NRZ bit duration on SCIO Asynchronous mode 64us

                    if (get_p1_state(5)){  // SCIO high
                        byte = byte | shift; // set current bit

                    }
                    shift = shift << 1; // shift to next bit
                    int t = 0;
                    for (t=0;t<512;t++){            // TXCT high for ~64us
                      __no_operation();           // TXCT high for ~64us

                    }
                }
                arr[byte_pos] = byte;
                byte_pos++;
                for (j=0;j<24000;j++){ //wait for ~3ms for SCIO start bit
                     if(get_p1_state(5)){ //check if there is input on SCIO
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
