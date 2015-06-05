//NOTE: keyboard will only buffer 17 bytes..
//this may or may not need to be worked around depending on how fast you can make the reciever
//   you could also only deal with x amount of bytes at a time and buffer locally as many as you need


const int  Clock                     = 10; //pin to receive kbd clock
const int  Data                      = 16; //pin to receive kbd data
const int  Ground_Clock              = 14; //pin to send clock to ground
const int  FRAME_SIZE                = 11;
const int  bitBuffer_SIZE            = 550; // in bits, equates to 50 frames
bool       bitBuffer[bitBuffer_SIZE] = { 0 };
int        bitBufferRead             = 0;
int        bitBufferWrite            = 0;
int        bytesInQueue              = 0;
bool       RUN_ON_PRESS              = false;
// * denotes unknown code, used for debugging purposes or @todo resend requests
// ! denotes release
// @ denotes multi-byte code  
// # denotes unprintable code                       0      1      2      3      4      5      6      7      8      9      10     11     12     13     14     15  
const char lookupTable[256]          = { /*0*/     '&',   '*',   '*',   '*',   '*',   '*',   '@',   '*',   '*',   '*',   '*',   '*',   '*','\352',   '!',   '*', 
                                         /*16*/    '*',   '*',   '*',   '*',   '*',   '*',   '*',   '*',   193,   '*',   '*',   '*',   204,   '*',   '*',   196,
                                         /*32*/    '*',   'o',   '*',   'e',   '*',   '*',   '*',   128,   '*',   '[',   '*',   'g',   '*','\346',   197,   '*', 
                                         /*48*/    ';',   '*',   't',   '*','\347',   '*',   'a',   '*',   '*',   '*',   'u',   '*','\335',   '*',   '*',   '*', 
                                         /*64*/    '*',   'k',   '*',   'x',   '*',   '*',   '*',   129,   '*',  '\'',   '*',   'b',   '*','\342',   '*',   201, 
                                         /*80*/    '*',   '/',   '*',   'v',   '*',   '*',   '*',   'z',   '*',   176,   '*',   'm',   '*','\343',   195,   '*', 
                                         /*96*/    '9',   '*',   '3',   '*',   178,   '*',   '1',   '*',   '*',   '*',   '6',   '*',   177,   '*',   '`',   '*', 
                                         /*112*/   '-',   '*',   '5',   '*',   '*',   '*',   '2',   '*',   '*',   '*',   '8',   '*',   207,   '*',   '*',   202, 
                                         /*128*/   '*',   ',',   '*',   'c',   '*',   131,   '*',   130,   '*',   '*',   '*',   'n',   '*','\353',   '*',   203, 
                                         /*144*/   '*',   '.',   '*',   ' ',   '*','\341',   '*',   '*',   '*',   133,   '*',   '*',   '*','\337',   194,   '*',
                                         /*160*/   '0',   '*',   '4',   '*',   '*',   '*',   'q',   '*',   '=',   '*',   'y',   '*','\350',   '#',   179,   '*', 
                                         /*176*/   'p',   '*',   'r',   '*',   '*',   '*',   'w',   '*',  '\\',   '*',   '7',   '*','\351',   '*',   198,   200, 
                                         /*192*/   'i',   '*',   'd',   '*',   '*',   '*',   '*',   '*',   '*',   '*',   'h',   '*','\345',   '*',   199,   '*', 
                                         /*208*/   'l',   '*',   'f',   '*','\344',   '*',   's',   '*',   ']',   '*',   'j',   '*','\336',   205,   '*',   '*', 
                                         /*224*/   '*',   '*',   '*',   '*',   '*',   '*',   '*',   '*',   '*',   '*',   '*',   219,   '*',   '*',   '*',   '*', 
                                         /*240*/   '*',   '*',   '*',   '*',   '*',   '*',   '*',   '*',   '*',   '*',   '*',   '*',   '*',   '*',   '*',   '*' };

// writeBitBuffer: writes a single bit to the buffer and decides when to roll over                                         
// we write all data to bitBuffer and deal with extra bits in read
//  (there is an 11 bit frame for ever byte sent, it's
//   quicker to ignore known worthless bits than to decide not to write
//    certain bits)
void writeBitBuffer(bool writeIn) {
  bitBuffer[bitBufferWrite++] = writeIn;
  if(bitBufferWrite >= bitBuffer_SIZE)
    bitBufferWrite = 0;
}

// getData reads a single frame from the keyboard. Refuses to do anything
//    until data comes in.
//  @todo - write nosend() to when host would like to write to keyboard
void getData() {
  digitalWrite(Ground_Clock, LOW); //make sure keyboard can transmit
  int counter = 0; //reset on bit
  int counter_MAX = 1000; //adjust to time out @todo 1000 works, see if you can go lower
  //wait for a falling edge to measure, acceptable because we can't do anything without data
  //  if we need to communicate from host to keyboard, we should do that immediately on a lock
  //  being pressed.
  while(digitalRead(Clock) == 1) { /*check those two booleans again*/ }
  //take in whatever data we can and wait to make sure
  //   there is no more
  while(counter < counter_MAX) {
    if(digitalRead(Clock) != 1) { //on clock's falling edge
      RUN_ON_PRESS = true;
      writeBitBuffer( digitalRead(Data) );
      counter = 0;
      while(digitalRead(Clock) != 1){} //do nothing till next rising edge
    }
    counter++; //increment to timeout clock pulses
  }
  digitalWrite(Ground_Clock, HIGH); //doing this to make sure keyboard buffers when we aren't expecting data
}

// disambiguate() is used to identify keystrokes that come from multibyte keys
// @isRelease - is the key being pressed or released. Called false initially, then recursively
//              called true once a release code is read.
void disambiguate(bool isRelease) {

  if(bitBufferRead == bitBufferWrite) getData(); // make sure there is data to read
  int decodedHere = integerDecode();
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
                   if(bitBufferRead == bitBufferWrite) getData();
                   if(bitBufferRead == bitBufferWrite) getData();
                   if(bitBufferRead == bitBufferWrite) getData();
                   integerDecode();
                   integerDecode();
                   integerDecode();
                   Keyboard.release(206);
                } else{/*should never see this block*/}
                break;
      case 71:  if(isRelease == true) {/*should never see this*/}
                else {
                   if(bitBufferRead == bitBufferWrite) getData();
                   if(bitBufferRead == bitBufferWrite) getData();
                   integerDecode();
                   integerDecode();
                   Keyboard.press(206);
                }
                break;   
      case 77:  if(isRelease == true) Keyboard.release(   KEY_DOWN_ARROW);
                else                  Keyboard.press(     KEY_DOWN_ARROW);
                break;
      case 81:  if(isRelease == true) Keyboard.release(   '\334'); //  / on num
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

//integerDecode: Checks a single frame of the buffer and returns an
//   integer that can be directly looked up in the table array.
//@ todo check parity bit and request resend
int integerDecode() {
  int returner = 0;
  //ignore first bit
  bitBufferRead++;
  for( int ender = bitBufferRead + 8; bitBufferRead < ender; bitBufferRead++ ) {
    //this inverts the significance of the bits
    returner += (int)bitBuffer[bitBufferRead] * pow(2, ender - bitBufferRead - 1); 
  }
  bitBufferRead += 2;
  if(bitBufferRead >= bitBuffer_SIZE) bitBufferRead = 0;
  Serial.print(returner); Serial.print("\n");
  return returner;
}
 
// Creates the output objects for the machine
void setup() {
   pinMode(Clock, INPUT);
   pinMode(Data,  INPUT);
   pinMode(Ground_Clock, OUTPUT);
   digitalWrite(Ground_Clock, LOW);
   Serial.begin(9600); //Can and should be removed when a stable version is made
   Keyboard.begin();
}

//loop arduino IDE compatible. Waits for key input then decodes it.
void loop(){
  getData();
  if( (bitBufferRead - bitBufferWrite) != 0) {
    while( (bitBufferRead - bitBufferWrite) != 0) {

      int decoded = integerDecode( ) ;
      
      if( decoded == 14 ) { //super slow, may want to change to decoded == 28
        if(bitBufferRead == bitBufferWrite) getData(); //nothing to decode, so we read more
        int decodedHere = integerDecode();
        Keyboard.release(lookupTable[decodedHere]);
      }
      else if( lookupTable[decoded] == '*') Keyboard.releaseAll();
      else if( lookupTable[decoded] == '@') disambiguate( false );
      else Keyboard.press( lookupTable[decoded] );
    
    } 
  }
}
