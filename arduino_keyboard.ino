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
const char lookupTable[256]          = { /*0*/     '&', '*', '*', '*', '*', '*', '@', '*', '*', '*', '*', '*', '*', '#', '!', '*', 
                                         /*16*/    '*', '*', '*', '*', '*', '*', '*', '*', '#', '*', '*', '*', '#', '*', '*', '#',
                                         /*32*/    '*', 'o', '*', 'e', '*', '*', '*', '*', '*', '[', '*', 'g', '*', '#', '#', '*', 
                                         /*48*/    ';', '*', 't', '*', '#', '*', 'a', '*', '*', '*', 'u', '*', '#', '*', '*', '*', 
                                         /*64*/    '*', 'k', '*', 'x', '*', '*', '*', '#', '*', '\'', '*', 'b', '*', '#', '*', '#', 
                                         /*80*/    '*', '/', '*', 'v', '*', '*', '*', 'z', '*', '#', '*', 'm', '*', '#', '#', '*', 
                                         /*96*/    '9', '*', '3', '*', '#', '*', '1', '*', '*', '*', '6', '*', '#', '*', '`', '*', 
                                         /*112*/   '-', '*', '5', '*', '*', '*', '2', '*', '*', '*', '8', '*', '#', '*', '*', '#', 
                                         /*128*/   '*', ',', '*', 'c', '*', '#', '*', '*', '*', '*', '*', 'n', '*', '#', '*', '#', 
                                         /*144*/   '*', '.', '*', ' ', '*', '#', '*', '*', '*', '#', '*', '*', '*', '#', '#', '*',
                                         /*160*/   '0', '*', '4', '*', '*', '*', 'q', '*', '=', '*', 'y', '*', '#', '#', '#', '*', 
                                         /*176*/   'p', '*', 'r', '*', '*', '*', 'w', '*', '\\', '*', '7', '*', '#', '*', '#', '#', 
                                         /*192*/   'i', '*', 'd', '*', '*', '*', '*', '*', '*', '*', 'h', '*', '#', '*', '#', '*', 
                                         /*208*/   'l', '*', 'f', '*', '#', '*', 's', '*', ']', '*', 'j', '*', '#', '#', '*', '*', 
                                         /*224*/   '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*', '#', '*', '*', '*', '*', 
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
  Serial.print("disambiguate");
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

//This huge switch statement handles all of the single byte codes that aren'd differentiable
// to a single character
void unprintable(int &keycode, bool isRelease) {
  //switch statement to identify functions etc.
  Serial.print("unprintable "); Serial.print(keycode);
  switch(keycode) {
    //these should possibly be rearranged to order of most frequent use
    case 13:  if(isRelease == true) Keyboard.release(   '\352'); // keypad 7
              else                  Keyboard.press(     '\352');
              break;
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
    //LEFT CTRL         
    case 39:  if(isRelease == true) Keyboard.release(   KEY_LEFT_CTRL);
              else                  Keyboard.press(     KEY_LEFT_CTRL);
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
    // F8
    case 79:  if(isRelease == true) Keyboard.release(   KEY_F8);
              else                  Keyboard.press(     KEY_F8);
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
    //F2          
    case 94:  if(isRelease == true) Keyboard.release(    KEY_F2);
              else                  Keyboard.press(      KEY_F2);
              break;
    //BACK SPACE
    case 100: if(isRelease == true) Keyboard.release(    KEY_BACKSPACE);
              else                  Keyboard.press(      KEY_BACKSPACE);
              break;
    // ESC
    case 108: if(isRelease == true) Keyboard.release(    KEY_ESC);
              else                  Keyboard.press(      KEY_ESC);
              break;
    // Scroll lock
    case 124: if(isRelease == true) Keyboard.release(    207); 
              else                  Keyboard.press(      207);
              break;
    // F9
    case 127: if(isRelease == true) Keyboard.release(    KEY_F9);
              else                  Keyboard.press(      KEY_F9);
              break;
     // LEFT GUI (only gui key mapped
    case 133: if(isRelease == true) Keyboard.release(    KEY_LEFT_GUI);
              else                  Keyboard.press(      KEY_LEFT_GUI);
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
    // keypad 9
    case 188: if(isRelease == true) Keyboard.release(   '\351');
              else                  Keyboard.press(     '\351');
              break;
    // F5
    case 190: if(isRelease == true) Keyboard.release(   KEY_F5);
              else                  Keyboard.press(     KEY_F5);
              break;
    // F7
    case 191: if(isRelease == true) Keyboard.release(   KEY_F7);
              else                  Keyboard.press(     KEY_F7);
              break;
    // keypad 5
    case 204: if(isRelease == true) Keyboard.release(  '\345');
              else                  Keyboard.press(    '\345');
              break;
    // F6          
    case 206: if(isRelease == true) Keyboard.release(   KEY_F6);
              else                  Keyboard.press(     KEY_F6);
              break;
    //keypad 4
    case 212: if(isRelease == true) Keyboard.release(   '\344'); 
              else                  Keyboard.press(     '\344');
              break;
    // keypad -
    case 220: if(isRelease == true) Keyboard.release(   '\336'); 
              else                  Keyboard.press(     '\336');
              break;
    // F12
    case 221: if(isRelease == true)  Keyboard.release(  KEY_F12);
              else                   Keyboard.press(    KEY_F12);
              break;
     //num lock
    case 235: if(isRelease == true) Keyboard.release(   219);
              else                  Keyboard.press(     219);
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
  Serial.print(returner);
  Serial.print('\n');
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
bool runonce = false;
void loop(){
  getData();
  if(runonce == true) {
    runonce = false;
    digitalWrite(Ground_Clock, HIGH);
    Serial.print("5 Seconds \n");
    delay(2000);
    Serial.print("3 Seconds \n");
    delay(1000);
    Serial.print("2 Seconds \n");
    delay(1000);
    Serial.print("1 Second \n");
    delay(1000);
    Serial.print("buffer should print now: \n"); 
    digitalWrite(Ground_Clock, LOW); 
  }
  
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

