/*
 *     Sistema de cronometragem de RCÅ› via IR
 *
 *     VersÃ£o 0.2
 *
 *     Este sistema consistem em um uC Atmega328P , com atÃ© 4 sensores IR , buzzer e led de indicaÃ§Ã£o de power, conectado e corrida em execuÃ§Ã£o
 *     O software de gerenciamento de corrdas Ã© o Zround www.zround.com , onde o protocolo utilizado foi dso RiCino
 *
 *    Luiz Gustavo L. Fernandes + Ederson R. Carnaúba
 *
 *    02/01/2017
 *
 */

/*
 * Alterações
 * 
 * 12/2/2017
 * Adicionado o envio do cronometro para sincronizacao do rx com o pc
 * Adicionada dupla verificacao, necessario 2 recepcoes identicas seguidas para considerar como uma volta valida
 * Alteracao para o protocolo de comunicacao do LapZ
 * Minimizado o numero de escritas na porta serial, fazendo um buffer da string completa antes de enviar ao pc
 * 
 * 
 * 9/2/2017
 * Remocao do suporte a multiplos sensores, utilizando apenas um pino para diversos em paralelo
 * Remocao de alguns codigos duplicados e verificacoes
 * Remocao do leds e buzzer
 * Adicionada funcoes para limpara buffers
 * 
 * 
 * TODO:
 * Decodificacao do IR por Interrupcao
 * Testar a funcao pinRead(PINA,2)
 * 
 */

/*
Serial-protocol for LapZ

General:

• The COM-port must be configured to 19200,8,n,1.
• The data is sent as ”normal text strings”, i.e. ascii, not binary.
• Each command and reply starts with a '#' and ends with a '$'.
• All data/numbers are given in hex.
• The length of the replies are fixed. E.g. if car 1 passes after 10 milliseconds, the result will be:
#P0100000A$
• ID 0 is an invalid ID. All ID’s must be between 1 and 255.

Commands from PC to LapZ:

#S$ Start race.
#F$ Stop race
#Ixx$ Write new ID into the car placed under the bridge. xx is a number between 1 and
255 (hex). E.g.: #I0B$
#G$ Get a single ID from the car placed under the bridge.

Commands from LapZ to PC:

During a race

#Pxxyyyyyy$ A car has passed. xx = The cars ID, yyyyyy = number of milliseconds since the start (hex). E.g.: #P1300023D$
#Uyyyyyy$ Clock update. Comes approximately each 7 seconds. yyyyyy = number of milliseconds since the start (hex). E.g.: #U00032B$

After programming new ID (#Ixx$)

#Oxx$ New ID was programmed successfully. xx = The ID that was programmed (this can typically be used to verify that the correct ID was programmed).
#N$ New ID was NOT programmed. This comes from a timeout after 200 ms.

After requesting a single ID (#G$)

#Hxx$ The ID of the car under the bridge.
#J$ No response from any cars. This comes from a timeout 200 ms after #G$ was sent.

*/

/*#######################################################
##                       Defines                       ##
####################################################### */


#define BAUDRATE      19200  //19200 para Zround

#define NUMBITS          12  //8 bitis de dados + 0101 de final de mensagem
#define DATABITS         8  //8 Databits -> 0-255

#define CRITTIME       400  //ÂµS , tempo de decisÃ£o entre 0 e 1
#define LOWBORDER      100  //ÂµS
#define TOPBORDER      900  //ÂµS

#define ANALYSEDELAY   150  //ÂµS , atraso entre o dado capturado e dado analisado

// essa funcao teoricamente le a porta muito mais rapido, testar!
#define pinRead(x,y)  (x & (1 << y))  //e.g. read state of D2 : pinRead(PIND, 2)


/*#######################################################
##                      Variaveis                      ##
#######################################################*/

static const uint8_t sensorPin = 2;   // Pino de entrada dos sensores de IR


long unsigned int millisInicio;       // millis() do inicio da corrida
boolean dataReady;                    // Buffer cheio e pronto para leitura
boolean state;                        // Estado do sensor de IR
boolean lastState;                    // Ultimo estado do sensor deIR
uint32_t lastChange;                  // Ultimo tempo lido do sensor
uint16_t buf[NUMBITS + 1];            // Salva o tempo de buffer
uint8_t counter;
uint32_t dataReadyTime;               //Coleta de analise completa
uint8_t message[NUMBITS];             // bit do cÃ³digo traduzido da decodificaÃ§Ã£o do ID
uint8_t data;                         //Leitura do ID
uint32_t actTime;

char bufferx[10];                     // Buffer newlap
int contaZero = 0;


int lastDet = 0; //ultimo transponder detectado

int cronoiniciado = 0;
unsigned long tiempo;
unsigned long ultimoTimer;

//Ajustes para o Zround
uint8_t connect = 1;
uint8_t message_zround[3];

/*#######################################################
##                Function Declaration                 ##
#######################################################*/

void setup();                                   //initialize everything
void loop();                                   //check the sensors and check for new round
void connect_Zround();

/*#######################################################
##                        Setup                        ##
#######################################################*/

void setup()
{
    pinMode(sensorPin, INPUT);
    dataReady = false;
    Serial.begin(BAUDRATE);
    connect_Zround();
}

/*#######################################################
##                        Loop                         ##
#######################################################*/

void loop()
{
   while (Serial.available() > 0) { // While RX Serial
    for (int i = 0; i < 3; i++) {
      message_zround[i] = Serial.read();
      delay(50);
    }
   }
   
  // Verifica se a corrida ou treino terminou
  if ((message_zround[0] == '#') && (message_zround[1] == 'F') && (message_zround[2] == '$')) {
    connect = 1;
    cronoiniciado = 0;
    connect_Zround();
    message_zround[0] = ('$');
  }
  
  //
  // Relógio
  //      
  if (cronoiniciado == 1){
    tiempo = (unsigned long)(millis()-millisInicio);
    if ((millis()-ultimoTimer) > 5000){
      sprintf(bufferx,"#U%06lX$",tiempo);
      Serial.println(bufferx);
      ultimoTimer = millis();
    }
  }
  //Leitura das entradas digitais
  state = digitalRead(sensorPin);
  
  //Compare state against lastState
  if(!dataReady)  //If data isn't analysed don't collect new data
  {
    if(state != lastState)  //New pulse/pause
    {
      lastState = state;
      actTime = micros(); //save time for upcoming calculations
      uint32_t pulseLength = actTime - lastChange;
      lastChange = actTime;
      
      if(pulseLength >= LOWBORDER)
      {
        if(pulseLength <= TOPBORDER)  //Same Signal
        {
          //SÃ³ vai ler o tempo de pulso se o pulso for 1
          if(!state){
            buf[counter] = pulseLength;  //salva a largura do pulso
            counter++;
          }

          if(counter == NUMBITS)  //All bits read -> ready for check
          {
            counter = 0;
            dataReady = true;
            dataReadyTime = actTime;
          }
        }//if(pulseLength <= TOPBORDER)
        else    //New Signal
        {
          //Error occurred. New message but last message wasn't completed
          if(counter != 0) { resetAll(); }
          counter = 0;
        }
      }//if(pulseLength >= LOWBORDER)
      else { 
        resetAll(); 
      }
    
    }
  }
  
  //calculate message if reading is completet
  if(dataReady && micros() >= (dataReadyTime + ANALYSEDELAY))  //New message is ready
  {
    dataReady = false;

    for(int8_t a = 0; a < NUMBITS ; a++)  //Calculate bit message
    {
      //if time < CRITTIME it's a zero, if it's bigger its a one
      message[a] = (buf[a] <= CRITTIME) ? 0 : 1;
      if (message[a] == 0) { contaZero++; } else { contaZero = 0; }
      if (contaZero >= 4) { a = NUMBITS; resetAll(); }
    }

    if(message[8] == 0 && message[9] == 1 && message[10] == 0 && message[11] == 1){
      data = ((message[0] << 7) + (message[1] << 6) + (message[2] << 5) + (message[3] << 4) + (message[4] << 3) + (message[5] << 2) + (message[6] << 1) + (message[7]));
      if (data == lastDet) {
        sprintf(bufferx,"#P%02X%06lX$",data,tiempo);
        Serial.println(bufferx);
        lastDet = 0;
        resetAll();
      }
      else { lastDet = data; }
      resetAll(); 
    }
    else { 
      resetAll(); 
    }
  }
}

/*#######################################################
##                      Functions                      ##
#######################################################*/

//ConexÃ£o com o Zround
void connect_Zround() {
  while (connect == 1) {
    // Read Message from Serial Port
    while (Serial.available() > 0) {
      for (int i = 0; i < 3; i++) {
        message_zround[i] = Serial.read();
        delay(50);
      }
    } // While Serial.available()

    if ((message_zround[0] == '#') && (message_zround[2] == '$')) {
      if (message_zround[1] == 'G') {    
        Serial.write("#J$");
        message_zround[0] = ('$');
      } 
   
      // Start Practice or Race
      if (message_zround[1] == 'S') {
        connect = 0;
        millisInicio = millis();
        cronoiniciado = 1;
        message_zround[0] = ('$');
      }
    }
    delay(50);
  } // While connect = 1 
}// fim connect Zround

void resetAll() {
  counter = 0; 
  dataReady = false;
  dataReadyTime = 0;
  data = 0;
  contaZero = 0;
  for (int zx = NUMBITS + 1; zx >= 0; zx--) {
    buf[zx] = 0;
  }
  for (int x = NUMBITS; x >= 0; x--) {
    message[x] = 0;
  }
}

