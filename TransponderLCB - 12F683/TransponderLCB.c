#define on1 320
#define off1 450

#define on0 580
#define off0 220
unsigned char transponderId;

void sendLCB(unsigned char id);
void sendIdLCB(unsigned char byte);
void sendBitLCB(int x);

void main()
{
  transponderId = EEPROM_Read(0x00);
  //transponderId = 147;
  TRISIO = 0;         // All output
  ADCON0 = 0;         // Turn off ADC
  CMCON0 = 7;         // Turn off the comparators
  CMCON1 = 0;
  GPIO.F2=1;
  T2CON = 0b00000100; // prescaler + turn on TMR2;
  PR2 = 0b00011001;
  CCPR1L = 0b00001101;  // set duty MSB
  CCP1CON = 0b00011100; // duty lowest bits + PWM mode
  CCP1CON = 0x0;  // Desliga PWM

  while(1)
  {
    sendLCB(transponderId);
    delay_ms(50);
  }
}
void sendLCB(unsigned char id) {
        sendBitLCB(1);
        sendIdLCB(id);
        sendBitLCB(0);
        sendBitLCB(1);
        sendBitLCB(0);
        sendBitLCB(1);
}
void sendBitLCB(int x) {
     if (x == 1) {
                    Delay_us(off1);  // 440uS
                    TMR2 = 0x00;
                    CCP1CON = 0xC;  // liga PWM
                    Delay_us(on1);  // 330uS
                    CCP1CON = 0x0;  // Desliga PWM
                    GPIO.F2=1;
     }
     else if (x == 0) {
                    Delay_us(off0);  // 170uS
                    TMR2 = 0x00;
                    CCP1CON = 0xC;  // liga PWM
                    Delay_us(on0);  // 610uS
                    CCP1CON = 0x0;  // Desliga PWM
                    GPIO.F2=1;
     }
}

void sendIdLCB(unsigned char byte)
{
        unsigned char i;
        for(i=8 ;i>0;i--)
        {
                if(byte & 0x80)
                {
                    sendBitLCB(1);
                }
                else
                {
                    sendBitLCB(0);
                }
                byte = byte <<1;
        }
}