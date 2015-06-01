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
// # denotes unprintable code                       0    1    2    3    4    5    6    7    8    9    10   11   12   13   14   15  
const char lookupTable[256]          = { /*0*/     '&', '*', '*', '*', '*', '*', '@', '*', '*', '*', '*', '*', '*', '\352', '!', '*', 
                                         /*16*/    '*', '*', '*', '*', '*', '*', '*', '*', '#', '*', '*', '*', '#', '*', '*', '#',
                                         /*32*/    '*', 'o', '*', 'e', '*', '*', '*', 128, '*', '[', '*', 'g', '*', '#', '#', '*', 
                                         /*48*/    ';', '*', 't', '*', '#', '*', 'a', '*', '*', '*', 'u', '*', '#', '*', '*', '*', 
                                         /*64*/    '*', 'k', '*', 'x', '*', '*', '*', '#', '*', '\'', '*', 'b', '*', '#', '*', 201, 
                                         /*80*/    '*', '/', '*', 'v', '*', '*', '*', 'z', '*', '#', '*', 'm', '*', '#', 195, '*', 
                                         /*96*/    '9', '*', '3', '*', 178, '*', '1', '*', '*', '*', '6', '*', 177, '*', '`', '*', 
                                         /*112*/   '-', '*', '5', '*', '*', '*', '2', '*', '*', '*', '8', '*', 207, '*', '*', 202, 
                                         /*128*/   '*', ',', '*', 'c', '*', 131, '*', '*', '*', '*', '*', 'n', '*', '#', '*', '#', 
                                         /*144*/   '*', '.', '*', ' ', '*', '#', '*', '*', '*', '#', '*', '*', '*', '#', '#', '*',
                                         /*160*/   '0', '*', '4', '*', '*', '*', 'q', '*', '=', '*', 'y', '*', '#', '#', '#', '*', 
                                         /*176*/   'p', '*', 'r', '*', '*', '*', 'w', '*', '\\', '*', '7', '*', '\351', '*', '#', '#', 
                                         /*192*/   'i', '*', 'd', '*', '*', '*', '*', '*', '*', '*', 'h', '*', '\345', '*', '#', '*', 
                                         /*208*/   'l', '*', 'f', '*', '\344', '*', 's', '*', ']', '*', 'j', '*', '\336', 205, '*', '*', 
                                         /*224*/   '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', 219, '*', '*', '*', '*', 
                                         /*240*/   '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*' };
                                         
// Write all data to bitBuffer and deal with extra bits in read
// (quicker to ignore than to decide not to write)
void writeBitBuffer(bool writeIn) {
  bitBuffer[bitBufferWrite++] = writeIn;
  if(bitBufferWrite >= bitBuffer_SIZE)
    bitBufferWrite = 0;
}

void getData() {
  digitalWrite(Ground_Clock, LOW); //make sure keyboard can transmit
  int counter = 0; //reset on bit
  int counter_MAX = 1000; //adjust to time out @todo 1000 works, see if you can go lower
  //wait for a falling edge to measure, acceptable because we can't do anything without data
  while(digitalRead(Clock) == 1) { }
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

void disambiguate(bool isRelease) {

  if(bitBufferRead == bitBufferWrite) getData(); // make sure there is data to read
  int decodedHere = integerDecode();
  if(lookupTable[decodedHere] == '!') disambiguate(true); //@todo still to slow
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
      case 77:  if(isRelease == true) Keyboard.release(   KEY_DOWN_ARROW);
                else                  Keyboard.press(     KEY_DOWN_ARROW);
                break;
      case 81:  if(isRelease == true) Keyboard.release(   '\334'); //  \ on num
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

//This huge switch statement handles all of the single byte codes that aren't differentiable
// to a single character
// This can be simplified by placing the integers associated with each of these into the character table?
void unprintable(int &keycode, bool isRelease) {

  switch(keycode) {

    //CAPS lock @todo, write led status change function
    case 24:  if(isRelease == true) Keyboard.release(   KEY_CAPS_LOCK);
              else                  Keyboard.press(     KEY_CAPS_LOCK);     
              break;
    //F11         
    case 28:  if(isRelease == true)  Keyboard.release(  KEY_F11);
              else                   Keyboard.press(    KEY_F11);
              break;
    //F3
    case 31:  if(isRelease == true) Keyboard.release(   KEY_F3);
              else                  Keyboard.press(     KEY_F3);
              break;
    //keypad 7         
    case 45:  if(isRelease == true) Keyboard.release(   '\346'); 
              else                  Keyboard.press(     '\346');
              break;
     // F4
    case 46:  if(isRelease == true) Keyboard.release(   KEY_F4);
              else                  Keyboard.press(     KEY_F4);
              break;
    // keypad 7
    case 52:  if(isRelease == true) Keyboard.release(  '\347'); 
              else                  Keyboard.press(    '\347');
              break;
    // keypad *         
    case 60:  if(isRelease == true) Keyboard.release(   '\335'); 
              else                  Keyboard.press(     '\335');
              break;
    //LEFT SHIFT      
    case 71:  if(isRelease == true) Keyboard.release(    KEY_LEFT_SHIFT);
              else                  Keyboard.press(      KEY_LEFT_SHIFT);
              break;
    //keypad 2          
    case 77:  if(isRelease == true) Keyboard.release(   '\342');
              else                  Keyboard.press(     '\342');
              break;
    // keypad '/'
    case 81:  if(isRelease == true) Keyboard.release(   '\334');  //keypad /
              else                  Keyboard.press(     '\334');
              break;
    //RETURN        
    case 89:  if(isRelease == true) Keyboard.release(    KEY_RETURN);
              else                  Keyboard.press(      KEY_RETURN);
              break;
    // keypad 3
    case 93:  if(isRelease == true) Keyboard.release(   '\343');  //keypad 3
              else                  Keyboard.press(     '\343');
              break;

    //LEFT ALT         
    case 135: if(isRelease == true) Keyboard.release(    KEY_LEFT_ALT);
              else                  Keyboard.press(      KEY_LEFT_ALT);
              break;
    // keypad ./del
    case 141: if(isRelease == true) Keyboard.release(   '\353');
              else                  Keyboard.press(     '\353');
              break;
    // F10          
    case 143: if(isRelease == true) Keyboard.release(   KEY_F10);
              else                   Keyboard.press(    KEY_F10);
              break;
    // keypad 1
    case 149: if(isRelease == true) Keyboard.release(  '\341');
              else                  Keyboard.press(    '\341');
              break;
    // RIGHT SHIFT
    case 153: if(isRelease == true) Keyboard.release(  KEY_RIGHT_SHIFT);
              else                  Keyboard.press(    KEY_RIGHT_SHIFT);
             break;
    // Something on the number pad         
    case 157: if(isRelease == true) Keyboard.release(  '\337');
              else                  Keyboard.press(    '\337');
              break; 
    //F1          
    case 158: if(isRelease == true) Keyboard.release(   KEY_F1);
              else                  Keyboard.press(     KEY_F1);
              break;
    // Something on the number pad
    case 172: if(isRelease == true) Keyboard.release(   '\350');
              else                  Keyboard.press(     '\350');
              break;
    //TAB 
    case 174: if(isRelease == true) Keyboard.release(   KEY_TAB);
              else                  Keyboard.press(     KEY_TAB);
              break;

    // F5
    case 190: if(isRelease == true) Keyboard.release(   KEY_F5);
              else                  Keyboard.press(     KEY_F5);
              break;
    // F7
    case 191: if(isRelease == true) Keyboard.release(   KEY_F7);
              else                  Keyboard.press(     KEY_F7);
              break;
    // F6          
    case 206: if(isRelease == true) Keyboard.release(   KEY_F6);
              else                  Keyboard.press(     KEY_F6);
              break;


    default:
      Serial.print("ERROR: unprintable called, but did not find decodable keypress");    
  }
}
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
  //Serial.print(returner);
  //Serial.print('\n');
  return returner;
}
 
void setup() {
   pinMode(Clock, INPUT);
   pinMode(Data,  INPUT);
   pinMode(Ground_Clock, OUTPUT);
   digitalWrite(Ground_Clock, LOW);
   Serial.begin(9600);
   Keyboard.begin();
}

// we alternate between getData (which will timeout on its own)
//   and letting the keyboard bitBuffer
int i = 0;

void loop(){
  getData();
  if( (bitBufferRead - bitBufferWrite) != 0) {
    while( (bitBufferRead - bitBufferWrite) != 0) {
      int decoded = integerDecode( ) ;
      if( lookupTable[decoded] == '!') { //super slow, may want to change to decoded == 28
        if(bitBufferRead == bitBufferWrite) getData(); //nothing to decode, so we read more
        int decodedHere = integerDecode();
        if(lookupTable[decodedHere] == '#') unprintable(decodedHere, true);
        else Keyboard.release(lookupTable[decodedHere]);
      }
      else if( lookupTable[decoded] == '@') { //multi-byte code
        disambiguate( false );
      }
      else {
        if(lookupTable[decoded] == '#') unprintable(decoded, false);
        else Keyboard.press( lookupTable[decoded] );
      }
    }
  }
}

