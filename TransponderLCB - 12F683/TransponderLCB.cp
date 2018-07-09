#line 1 "C:/Users/Gustavo/Documents/BKP/Projetos/TransponderLCB - 12F683/TransponderLCB.c"





unsigned char transponderId;

void sendLCB(unsigned char id);
void sendIdLCB(unsigned char byte);
void sendBitLCB(int x);

void main()
{
 transponderId = EEPROM_Read(0x00);

 TRISIO = 0;
 ADCON0 = 0;
 CMCON0 = 7;
 CMCON1 = 0;
GPIO.F2=1;
 T2CON = 0b00000100;
 PR2 = 0b00011001;
 CCPR1L = 0b00001101;
 CCP1CON = 0b00011100;
 CCP1CON = 0x0;

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
 Delay_us( 450 );
 TMR2 = 0x00;
 CCP1CON = 0xC;
 Delay_us( 320 );
 CCP1CON = 0x0;
 GPIO.F2=1;
 }
 else if (x == 0) {
 Delay_us( 220 );
 TMR2 = 0x00;
 CCP1CON = 0xC;
 Delay_us( 580 );
 CCP1CON = 0x0;
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
