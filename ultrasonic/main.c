#include <msp430.h> 
#include "driverlib.h"
#include <stdio.h>

void lcdInit();

const unsigned char lcd_num[10] = {
    0xFC,        // 0
    0x60,        // 1
    0xDB,        // 2
    0xF3,        // 3
    0x67,        // 4
    0xB7,        // 5
    0xBF,        // 6
    0xE4,        // 7
    0xFF,        // 8
    0xF7,        // 9
};

char buff[100];
uint8_t compareDone = 0,reset;
uint16_t negEdge,posEdge,distance,diff;
const uint16_t onecycle = 30;//1/32768

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    lcdInit();

    P9DIR |= BIT7;
    P9OUT &= ~BIT7;

    P3DIR |= BIT3;
    P3OUT &= ~BIT3;


    P1DIR &= ~BIT7;
    P1SEL0 |= BIT7;
    P1SEL1 |= BIT7;

    PM5CTL0 &= ~LOCKLPM5;

      // Timer0_A3 Setup
      TA0CCTL2 = CM_3 | CCIS_0 | SCS | CAP | CCIE;
                                                // Capture rising edge,
                                                // Use CCI2B=ACLK,
                                                // Synchronous capture,
                                                // Enable capture mode,
                                                // Enable capture interrupt

      P3OUT |= BIT3;
      __delay_cycles(20);
      P3OUT &= ~BIT3;
      TA0CTL = TASSEL__ACLK | MC__CONTINUOUS;  // Use SMCLK as clock source,
                                                // Start timer in continuous mode
      __bis_SR_register(GIE);

      while(1)
      {
          if(compareDone == 0x02)
          {
              __disable_interrupt();
              TA0CTL = TACLR;
              diff = negEdge - posEdge;
              distance = (onecycle * diff) / 58;
              if(distance < 30)
              {
                  // Display "3" on " on Segment 28,29
                  LCDM15 = lcd_num[distance / 10];
                  LCDM8 = lcd_num[distance % 10];
              }
//              else
//              {
//                  LCDM15 = lcd_num[0];
//                  LCDM8 = lcd_num[0];
//              }
              compareDone = 0x00;
              __enable_interrupt();
              P3OUT |= BIT3;
              __delay_cycles(20);
              P3OUT &= ~BIT3;
              TA0CTL = TASSEL__ACLK | MC__CONTINUOUS;
              reset = 0;
          }
            __delay_cycles(800000);
            reset++;
          if(reset == 10)
          {
              __disable_interrupt();
              TA0CTL = TACLR;
              compareDone = 0x00;
              __enable_interrupt();
              P3OUT |= BIT3;
              __delay_cycles(20);
              P3OUT &= ~BIT3;
              TA0CTL = TASSEL__ACLK | MC__CONTINUOUS;
              reset = 0x00;
          }
      }
    return 0;
}

// Timer0_A3 CC1-4, TA Interrupt Handler
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = TIMER0_A1_VECTOR
__interrupt void Timer0_A1_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_A1_VECTOR))) Timer0_A1_ISR (void)
#else
#error Compiler not supported!
#endif
{
  switch (__even_in_range(TA0IV, TA0IV_TAIFG)) {
    case TA0IV_TA0CCR1:
      break;
    case TA0IV_TA0CCR2:
        if(compareDone == 0x00)
        {
            posEdge = TA0CCR2;
            compareDone = 0x01;
        }
        else if(compareDone == 0x01)
        {
            negEdge = TA0CCR2;
            compareDone = 0x02;
        }

      break;
    case TA0IV_TA0IFG:
      break;
    default:
      break;
  }
}


void lcdInit()
{
    PJSEL0 = BIT4 | BIT5;                   // For LFXT

    // Initialize LCD segments 0 - 21; 26 - 43
    LCDCPCTL0 = 0xFFFF;
    LCDCPCTL1 = 0xFC3F;
    LCDCPCTL2 = 0x0FFF;

    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;

    // Configure LFXT 32kHz crystal
    CSCTL0_H = CSKEY >> 8;                  // Unlock CS registers
    CSCTL4 &= ~LFXTOFF;                     // Enable LFXT
    do
    {
      CSCTL5 &= ~LFXTOFFG;                  // Clear LFXT fault flag
      SFRIFG1 &= ~OFIFG;
    }while (SFRIFG1 & OFIFG);               // Test oscillator fault flag
    CSCTL0_H = 0;                           // Lock CS registers

    // Initialize LCD_C
    // ACLK, Divider = 1, Pre-divider = 16; 4-pin MUX
    LCDCCTL0 = LCDDIV__1 | LCDPRE__16 | LCD4MUX | LCDLP;

    // VLCD generated internally,
    // V2-V4 generated internally, v5 to ground
    // Set VLCD voltage to 2.60v
    // Enable charge pump and select internal reference for it
    LCDCVCTL = VLCD_1 | VLCDREF_0 | LCDCPEN;

    LCDCCPCTL = LCDCPCLKSYNC;               // Clock synchronization enabled

    LCDCMEMCTL = LCDCLRM;                   // Clear LCD memory

    //Turn LCD on
    LCDCCTL0 |= LCDON;

}
