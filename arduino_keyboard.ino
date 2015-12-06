// Programmer:  Joshua T. Frey
// Project:     arduino_keyboard
// Purpose:     provides a pass through interface for an AT protocol keyboard
//              to be used with a USB-only computer.
//              Please see the README for known issues and description of use
//
// Liscense:    GNU 3. Please see the liscense in the git repo for details

//NOTE:The keyboard I tested buffered 17 bytes
#include <Keyboard.h>
const int  Clock                     = 10; //pin to receive kbd clock
const int  Data                      = 16; //pin to receive kbd data
const int  Ground_Clock              = 14; //pin to send clock to ground
const int  Ground_Data               = 15;
//const int  Load_Data                 = A0; //alias, but going to have to write explicitly for now
const int  FRAME_SIZE                = 11;

const int  byteBufferSize            = 22;
int        byteBuffer[byteBufferSize]= {0};
int        byteBufferRead            = 0;
int        byteBufferWrite           = 0;

// * denotes unknown code, used for debugging purposes or @todo resend requests
// ! denotes release
// @ denotes multi-byte code  
// # denotes unprintable code                       0      1      2      3      4      5      6      7  
const char lookupTable[256]          = { /*0*/     '&',   '*',   '*',   '*',   '*',   '*',   '@',   '*',   
                                                   '*',   '*',   '*',   '*',   '*','\352',   '!',   '*', 
                                         /*16*/    '*',   '*',   '*',   '*',   '*',   '*',   '*',   '*',   
                                         /*24*/    193,   '*',   '*',   '*',   204,   '*',   '*',   196,
                                         /*32*/    '*',   'o',   '*',   'e',   '*',   '*',   '*',   128,   
                                                   '*',   '[',   '*',   'g',   '*','\346',   197,   '*', 
                                         /*48*/    ';',   '*',   't',   '*','\347',   '*',   'a',   '*',   
                                                   '*',   '*',   'u',   '*','\335',   '*',   '*',   '*', 
                                         /*64*/    '*',   'k',   '*',   'x',   '*',   '*',   '*',   129,   
                                                   '*',  '\'',   '*',   'b',   '*','\342',   '*',   201, 
                                         /*80*/    '*',   '/',   '*',   'v',   '*',   '*',   '*',   'z',   
                                                   '*',   176,   '*',   'm',   '*','\343',   195,   '*', 
                                         /*96*/    '9',   '*',   '3',   '*',   178,   '*',   '1',   '*',   
                                                   '*',   '*',   '6',   '*',   177,   '*',   '`',   '*', 
                                         /*112*/   '-',   '*',   '5',   '*',   '*',   '*',   '2',   '*',   
                                                   '*',   '*',   '8',   '*',   207,   '*',   '*',   202, 
                                         /*128*/   '*',   ',',   '*',   'c',   '*',   131,   '*',   130,   
                                                   '*',   '*',   '*',   'n',   '*','\353',   '*',   203, 
                                         /*144*/   '*',   '.',   '*',   ' ',   '*','\341',   '*',   '*',   
                                                   '*',   133,   '*',   '*',   '*','\337',   194,   '*',
                                         /*160*/   '0',   '*',   '4',   '*',   '*',   '*',   'q',   '*',   
                                                   '=',   '*',   'y',   '*','\350',   '#',   179,   '*', 
                                         /*176*/   'p',   '*',   'r',   '*',   '*',   '*',   'w',   '*', 
                                                   '\\',   '*',   '7',   '*','\351',   '*',   198,   200, 
                                         /*192*/   'i',   '*',   'd',   '*',   '*',   '*',   '*',   '*',  
                                                   '*',   '*',   'h',   '*','\345',   '*',   199,   '*', 
                                         /*208*/   'l',   '*',   'f',   '*','\344',   '*',   's',   '*',   
                                                   ']',   '*',   'j',   '*','\336',   205,   '*',   '*', 
                                         /*224*/   '*',   '*',   '*',   '*',   '*',   '*',   '*',   '*',   
                                                   '*',   '*',   '*',   219,   '*',   '*',   '*',   '*', 
                                         /*240*/   '*',   '*',   '*',   '*',   '*',   '*',   '*',   '*',   
                                                   '*',   '*',   '*',   '*',   '*',   '*',   '*',   '*' };


void getData();
int  getNextByte();
const int ACK = 94;
const int RES = 125;

bool num    = false;
bool scroll = false;
bool caps   = false;
const bool echo[FRAME_SIZE] = { 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0 }; //final bit is a bit strange
                                                                   // always One...but then the first is always zero
const bool setLocks[FRAME_SIZE] = { 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0 }; //final bit is a bit strange
                                                                   // always One...but then the first is always zero
const bool zero[FRAME_SIZE] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 };

void setLights( ) {
  //get this out of the way now to save tive later
  int lockCount = 0;
  if(num)    lockCount++;
  if(scroll) lockCount++;
  if(caps)   lockCount++;
  bool par = false;
  if( lockCount % 2 == 0) par = true;
  bool locks[FRAME_SIZE] = {0, scroll, num, caps, 0, 0, 0, 0, 0, par, 0 };
    //signal to the keyboard that you have something to say;]
  bool eHere = setLocks[0];
  int  i     = 1;
  
  digitalWrite(Ground_Clock, HIGH);
  delayMicroseconds(60);
  digitalWrite(Ground_Data,  HIGH);
  delayMicroseconds(4);
  digitalWrite(Ground_Clock, LOW );

  digitalWrite( Ground_Data, !eHere );
  digitalWrite( A0,           eHere );  
  

  for( ; i < FRAME_SIZE; i++) {
     //00111000111 //suggests I'm not writing what I think I am    
    while( digitalRead(Clock) == 1) { /*wait for a falling edge*/ }
    eHere = setLocks[i];

    digitalWrite( Ground_Data, !eHere );
    digitalWrite( A0,           eHere );
    

    while( digitalRead(Clock) == 0) { /*wait again for a rising edge*/ }
  }

  
  //don't use the last bit, just put high on the edge

  digitalWrite(A0, LOW);
  digitalWrite(Ground_Data, LOW);
  
  while(digitalRead(Clock) == 1) { Serial.print( "L" ); }
  while(digitalRead(Clock) == 0) { }
  getData();
  int deca = getNextByte();
  Serial.print(deca);
  Serial.print("|");
  if( deca == ACK ) {
    Serial.print("FirstIff");
     deca = RES;
     while(deca == RES) {
        eHere = locks[0];
        i     = 1;
        digitalWrite(Ground_Clock, HIGH);
        delayMicroseconds(60);
        digitalWrite(Ground_Data,  HIGH);
        delayMicroseconds(4);
        digitalWrite(Ground_Clock, LOW );

        digitalWrite( Ground_Data, !eHere );
        digitalWrite( A0,           eHere );  
  

         for( ; i < FRAME_SIZE; i++) {
            //00111000111 //suggests I'm not writing what I think I am    
              while( digitalRead(Clock) == 1) { /*wait for a falling edge*/ }
              eHere = locks[i];

              digitalWrite( Ground_Data, !eHere );
              digitalWrite( A0,           eHere );
    

             while( digitalRead(Clock) == 0) { /*wait again for a rising edge*/ }
         }

         //don't use the last bit, just put high on the edge
         digitalWrite(A0, LOW);
         digitalWrite(Ground_Data, LOW);
  
         while(digitalRead(Clock) == 1) { Serial.print( "L" ); }
         while(digitalRead(Clock) == 0) { }
         getData();
         deca = getNextByte(); 
      }
    } else if(deca == RES) {
      setLights( );
    } else {
      Serial.print("Unable to decode in set lights.");
    }
}


void whatHappensNext() {
  int counter = 0;
  while(true) {
    while(digitalRead(Clock) == 1) { Serial.print(counter++); }
    Serial.print("\n");
    while(digitalRead(Clock) == 0) { Serial.print(counter++); }
  
  }
}
//@input indicator tells the method which frame to write
void commToBoard(const bool *sendByte ) {
  //signal to the keyboard that you have something to say;]
  bool eHere = sendByte[0];
  int  i     = 1;
  digitalWrite(Ground_Clock, HIGH);
  delayMicroseconds(60);
  digitalWrite(Ground_Data,  HIGH);
  delayMicroseconds(4);
  digitalWrite(Ground_Clock, LOW );

  digitalWrite( Ground_Data, !eHere );
  digitalWrite( A0,           eHere );  
  

  for( ; i < FRAME_SIZE; i++) {
     //00111000111 //suggests I'm not writing what I think I am    
    while( digitalRead(Clock) == 1) { /*wait for a falling edge*/ }
    eHere = sendByte[i];

    digitalWrite( Ground_Data, !eHere );
    digitalWrite( A0,           eHere );
    

    while( digitalRead(Clock) == 0) { /*wait again for a rising edge*/ }
  }

  
  //don't use the last bit, just put high on the edge

  digitalWrite(A0, LOW);
  digitalWrite(Ground_Data, LOW);
  


  while(digitalRead(Clock) == 1) { Serial.print( "L" ); }
  while(digitalRead(Clock) == 0) { }
  getData();
  int deca = getNextByte();

  Serial.print("\n");
  Serial.print(deca);
  Serial.print("\n");

  if(sendByte[4]) { commToBoard(zero); }
} //END commToBoard


int getNextByte( ) {
  if(byteBufferRead == byteBufferWrite) getData();
  int returner = byteBuffer[ byteBufferRead++ ];
  if(byteBufferRead == byteBufferSize) byteBufferRead = 0;
  return returner;
}
int decodeFrame(bool* frameIn) {
  int returner = 0;
  for( int i = 8; i > 0; i-- ) {
    Serial.print(frameIn[9-i]);
    returner += (int)frameIn[9 - i] * pow(2, i - 1); 
  }
  return returner;
}

// getData reads a single frame from the keyboard. Refuses to do anything
//    until data comes in.
//  @todo - write nosend() to when host would like to write to keyboard
void getData() {
  digitalWrite(Ground_Clock, LOW); //make sure keyboard can transmit
  bool frame[11] = {0};
  int bitCounter = 0;
  int tCounter = 0; //reset on bit
  int tCounter_MAX = 1000;//timeout adjustmenst
  //wait for a falling edge to measure, acceptable because we can't do anything without data
  //  if we need to communicate from host to keyboard, we should do that immediately on a lock
  //  being pressed.
  Serial.print("I'm in get data waiting for clock to fall\n");
  while(digitalRead(Clock) == 1) { /*check those two booleans again*/ }
  //take in whatever data we can and wait to make sure
  //   there is no more
  while(tCounter < tCounter_MAX) {
    
    if(digitalRead(Clock) != 1) { //on clock's falling edge
      tCounter = 0; //reset if we got a bit
      
      
      frame[bitCounter++] =  digitalRead(Data);
      //check if we got an entire frame
      if(bitCounter == 11) {
        digitalWrite(Ground_Clock, HIGH);
        byteBuffer[byteBufferWrite++] = decodeFrame( frame );
        Serial.print(byteBuffer[byteBufferWrite-1]);
        Serial.print("|");
        if(byteBufferWrite == byteBufferSize) byteBufferWrite = 0;
        bitCounter = 0;
        digitalWrite(Ground_Clock, LOW);
      }
      while(digitalRead(Clock) != 1){} //do nothing till next rising edge
    }
    tCounter++; //increment to timeout clock pulses
  }
  digitalWrite(Ground_Clock, HIGH); //doing this to make sure keyboard buffers when we aren't expecting data
}

// disambiguate() is used to identify keystrokes that come from multibyte keys
// @isRelease - is the key being pressed or released. Called false initially, then recursively
//              called true once a release code is read.
void disambiguate(bool isRelease) {

  int decodedHere = getNextByte();
  if(decodedHere == 14) disambiguate(true); //Found release character
  else {
    switch(decodedHere) {
      case 13:  if(isRelease == true) Keyboard.release(   KEY_INSERT);
                else                  Keyboard.press(     KEY_INSERT);
                break;      
      case 39:  if(isRelease == true) Keyboard.release(   KEY_RIGHT_CTRL);
                else                  Keyboard.press(     KEY_RIGHT_CTRL);
                break;
      case 45:  if(isRelease == true) Keyboard.release(   KEY_RIGHT_ARROW);
                else                  Keyboard.press(     KEY_RIGHT_ARROW);
                break;
      case 52:  if(isRelease == true) Keyboard.release(   KEY_HOME);
                else                  Keyboard.press(     KEY_HOME);
                break;
      // case 60 and 71 relate to print screen are not robust, but work
      case 60:  if(isRelease == true) {
                   getNextByte();
                   getNextByte();
                   getNextByte();
                   Keyboard.release(206);
                } else{/*should never see this block*/}
                break;
      case 71:  if(isRelease == true) {/*should never see this*/}
                else {
                   getNextByte();
                   getNextByte();
                   Keyboard.press(206);
                }
                break;   
      case 77:  if(isRelease == true) Keyboard.release(   KEY_DOWN_ARROW);
                else                  Keyboard.press(     KEY_DOWN_ARROW);
                break;
      case 81:  if(isRelease == true) Keyboard.release(   '\334'); //  '/' on num
                else                  Keyboard.press(     '\334');
                break;
      case 89:  if(isRelease == true) Keyboard.release(   '\340'); //NUM ENTER
                else                  Keyboard.press(     '\340');
                
                break;
      case 93:  if(isRelease == true) Keyboard.release(   KEY_PAGE_DOWN);
                else                  Keyboard.press(     KEY_PAGE_DOWN);
                break;
      case 135: if(isRelease == true) Keyboard.release(   KEY_RIGHT_ALT);
                else                  Keyboard.press(     KEY_RIGHT_ALT);
                break;
      case 141: if(isRelease == true) Keyboard.release(   KEY_DELETE);
                else                  Keyboard.press(     KEY_DELETE);
                break;
      case 149: if(isRelease == true) Keyboard.release(   KEY_END);
                else                  Keyboard.press(     KEY_END);
                break;          
      case 172: if(isRelease == true) Keyboard.release(   KEY_UP_ARROW);
                else                  Keyboard.press(     KEY_UP_ARROW);
                break;
      case 188: if(isRelease == true) Keyboard.release(   KEY_PAGE_UP);
                else                  Keyboard.press(     KEY_PAGE_UP);
                break;
      case 212: if(isRelease == true) Keyboard.release(   KEY_LEFT_ARROW);
                else                  Keyboard.press(     KEY_LEFT_ARROW);
                break;
                
      default:
        Serial.print("ERROR: disambiguate called without readable data");    
    }
  }
}

// Creates the output objects for the machine
void setup() {
   pinMode(Clock, INPUT);
   pinMode(Data,  INPUT);
   pinMode(Ground_Clock, OUTPUT);
   pinMode(Ground_Data,  OUTPUT);
   pinMode(A0, OUTPUT);
   digitalWrite(A0,           LOW);
   digitalWrite(Ground_Data,  LOW);
   digitalWrite(Ground_Clock, LOW);
   Serial.begin(9600); //Can and sho02uld be removed when a stable version is made
   Keyboard.begin();
}

//loop arduino IDE compatible. Waits for key input then decodes it.
void loop(){
  getData();
  if( (byteBufferRead - byteBufferWrite) != 0) {
    while( (byteBufferRead - byteBufferWrite) != 0) {

      int decoded = getNextByte();

      if( decoded == 14 ) { //super slow, may want to change to decoded == 28
        int decodedHere = getNextByte();
        Keyboard.release(lookupTable[decodedHere]);
      }
      else if( lookupTable[decoded] == '*') Keyboard.releaseAll();
      else if( lookupTable[decoded] == '@') disambiguate( false );
      else {
        /*
        * This block is coded for testing, make sure you remember to change
        * it as the tests change
         */
        
        if(decoded == 235) {
          num = !num;
          setLights(); 
          Keyboard.press(   lookupTable[235] );
          Keyboard.release( lookupTable[235] );
        } //numlock = 235
        else if(decoded == 124) {
          scroll = !scroll;
          setLights();
          Keyboard.press(   lookupTable[124] );
          Keyboard.release( lookupTable[124] );
        }
        else if(decoded == 24) {
          caps = !caps;
          setLights();
          Keyboard.press(   lookupTable[24]  );
          Keyboard.release( lookupTable[24]  );
        }
        else {Keyboard.press( lookupTable[decoded] );} // won't press numlock presently but will run the echo routine
      }
    } 
  }
  
}
