/*-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * This is the code I am using for a one transmitter with multiple recievers controller based on adafruit feathers. Specifically Adafruit Feather M0 RFM69HCW Packet Radio 
 * It is a work in progress based on John Parks Remote effects Trigger Box here https://learn.adafruit.com/remote-effects-trigger/programing-the-remote
 * This is all adafruit hardware so use them to hget all the library's setup, follow the trigger box instructions,skip the OLED and Trellis then go to the learning page for the 3.5 inch TFT
 */


#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "Adafruit_HX8357.h"
#include <Adafruit_STMPE610.h>
#include <RH_RF69.h>
#include <Wire.h>
//#include <Adafruit_SSD1306.h>
#include <RHReliableDatagram.h>
#include <RH_ASK.h>
#include <Encoder.h>

//********* Encoder Setup ***************
#define PIN_ENCODER_SWITCH 13   // pin for the pushbutton switch in the encoder, pin 13 is the led, but I am not using the led, and the feather is pin short. I am also not using the button, but it is here. 
Encoder knob(12, 11);   //pins for the encoder input
uint8_t activeRow = 0;
long pos = -999;
long newpos;
int prevButtonState = HIGH;
bool needsRefresh = true;
bool advanced = false;
unsigned long startTime;


//-------------------------------------------TFT Screen-------------------------
//the tft screen is made to be attached to the feather, we cant do that, so using the tft pinout guide, solder jumpers in place with the controller open and laying flat. 

#define STMPE_CS 6
#define TFT_CS   9
#define TFT_DC   10
#define SD_CS    5
#define TFT_RST -1
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);


//----------------------------------------------Radio ---------------------------
  #define RF69_FREQ 919.0
  #define RFM69_CS      8
  #define RFM69_INT     3
  #define RFM69_RST     4
  
// Where to send packets to! It is possible to send out commands to specific radios, and then have them all check in, it added complexity, and the recieve failures were so low, I quit using it. 255 is send to all. 
#define DEST_ADDRESS 255


// change addresses for each client board, any number :)
#define MY_ADDRESS 1
struct dataStruct{  // defines the radio packet n1 and n2 are for screen number, n3 is the pushbutton, this will limit you to 99 screens of 8 choices per, for a total of 792 individual commands. 
byte n1;
byte n2;
byte n3;
}RadioPacket;

 byte buf[sizeof(RadioPacket)] = {0};

 // Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram rf69_manager(rf69, MY_ADDRESS);

//int16_t packetnum = 0;  // packet counter, we increment per xmission
//----------------------------------------------constants---------------
//assign the board pins to the buttons 
const int Button1 = 20;
const int Button2 = 14;
const int Button3 = 16;
const int Button4 = 17;
const int Button5 = 18;
const int Button6 = 19;
const int Button7 = 0;
const int Button8 = 1;
const int Button9 = 15;


//variable for reading the pushbutton status 
int Button1state = 0;
int Button2state = 0;
int Button3state = 0;
int Button4state = 0;
int Button5state = 0;
int Button6state = 0;
int Button7state = 0;
int Button8state = 0;
int Button9state = 0;



//int Send1;
byte val1;
byte val2;
byte val3;

int SwitchIntHist = 1;

String menuTitle;
String menu1;
String menu2;
String menu3;
String menu4;
String menu5;
String menu6;
String menu7;
String menu8;

String Word1;
String Word2;
String Switches;
String Switch1;
String Switch2;
String Switch3;
String Switch4;
String Switch5;
String Buttons;
String Butt1;
String Butt2;
String Butt3;
String Butt4;
String Butt5;
String Butt6;
String Butt7;
String Butt8;
int SwitchInt;
int ButtonsInt;
int TxTimes = 0;
int diff;
//*********************************************************Timeing*******************************************
/*Based on the blink without delay sketch. these timeings can be used to fine toon your sketch. This is the data used by the time triggers in the loop section. 
 * For instance, if the serial display as going so fast you cant read it, set it to only refresh every so often. this works like this.
 *1. the very first line in loop is "currentMillis = millis();"  this capures the current time in milliseconds and names it Millis()
 *2. each thing you wish to control the time of gets a placeholder these are the unsigned long variables , our serial screen is previousDisplayMillis
 *3. how long the delay you wish is stored below in the const int these are also milliseconds so 500 is 1/2 second. 
so we do not have to scroll back and forth, I borrowed this from loop

if (millis() - previousDisplayMillis >= DisplayTime){
previousDisplayMillis = currentMillis;  
PrintValues();
}

this is the time check function the first line is asking if the current time in ms, minus the last stored value is greater or equal to the time desired. 
if the answer to that math problem is NO the program jumps over the the bracketed section, if the answer is YES then the program runs the bracket section.it does two things. 
1. makes previousDisplayMillis equal to the time right now.
2. Runs the subroutine called PrintValues();

This is such a useful tool. Arduinos are fast, but we dont need or want the display to update 200 times each second. by having it done just twice a second we reduced the load on the arduino. so it can do other things.
pretty much you want to use this and if you can help it, eliminate the use of delay in your sketches. 

*/
const int SensorTime = 500;
const int DisplayTime = 500;
const int LoopTime = 500;
const int SendTime = 200;
unsigned long currentMillis = millis();      // stores the value of millis() in each iteration of loop()
unsigned long previousSensorMillis = millis();
unsigned long previousDisplayMillis = millis();
unsigned long previousDisplay2Millis = millis();
unsigned long previousSwitchMillis = millis();
unsigned long previousAddMillis = millis();
unsigned long previousSwitch1Millis = millis();
unsigned long previousSendMillis = millis();

//*************************************************Encoder Setup*******************************************


int menuList[11]={1,2,3,4,5,6,7,8,9,10,11}; //for rotary encoder choices
int m = 1; //variable to increment through menu list


//-------------------------------------------------Setup______________________________
void setup() {

  Serial.begin(115200);
  delay(10);
  

 pinMode(Button1, INPUT_PULLUP);
 pinMode(Button2, INPUT_PULLUP);
 pinMode(Button3, INPUT_PULLUP);
 pinMode(Button4, INPUT_PULLUP);
 pinMode(Button5, INPUT_PULLUP);
 pinMode(Button6, INPUT_PULLUP);
 pinMode(Button7, INPUT_PULLUP);
 pinMode(Button8, INPUT_PULLUP);

  
//---------------------------------------------Radio------------------------  
  
  
 
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  
  if (!rf69.init()) {
    Serial.println("RFM69 radio init failed");
    while (1);
  }
  Serial.println("RFM69 radio init OK!");
  
  
  if (!rf69.setFrequency(RF69_FREQ)) {
    Serial.println("setFrequency failed");
  }

  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(20, true);

  // The encryption key has to be the same as the one in the server.      BUT DIFFERENT FROM OTHER DROIDS IN THE ROOM
  uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  rf69.setEncryptionKey(key);
  
//  pinMode(LED, OUTPUT);

  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");
  
 // give the radio packet a temp value 
RadioPacket.n1 = 0;
RadioPacket.n2 = 0;
RadioPacket.n3 = 0;


 delay(500); //yea, I know, a delay, but it is in the setup section, Just runs once. 

//---------------------------------------Display--------------------------------
 if (!ts.begin()) {
    Serial.println("Couldn't start touchscreen controller");
    while (1);
  }
  Serial.println("Touchscreen started");
  
 //tft.begin();
 tft.begin(HX8357D);
Serial.println("HX8357D Test!");
 tft.setRotation(1);

  tft.fillScreen(HX8357_BLACK);
  tft.setCursor(0,0);
  tft.setTextSize(2);
  tft.println("RFM69 @ ");
  tft.print((int)RF69_FREQ);
  tft.println(" MHz");
  //tft.display();
  delay(1200); //pause to let freq message be read by a human
  
//this displays your name and phone number at start up. I still think that most of the world is honest, thay just need the oportunity
  tft.fillScreen(HX8357_BLACK);
  tft.setCursor(0,0);
  tft.setTextSize(3);
  tft.println(" Your Name ");
   tft.println("  ");
  tft.println(" ");
   tft.println("  ");
  tft.println(" Your Phone Number  ");
  delay(3000); //pause to let message be read by a human
  tft.fillScreen(HX8357_BLACK);


  
}

// Dont put this on the stack:
//uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
uint8_t data[] = "  OK";
//*******************************************************Read Buttons**************************
/*
 * this section is reading the buttons the first bit is the encoder m is the encoder pos and i am using this to turn that position into a 2 digit value
 */
void ReadButtons(){
     if (m==0){
        val1 = 0;
        val2 = 1;
          }
      if (m==1){
        val1 = 0;
        val2 = 2;
       }
      if (m==2){
        val1 = 0;
        val2 = 3;
       }
      if (m==3){
        val1 = 0;
        val2 = 4;
       }
      if (m==4){
       val1 = 0;
       val2 = 5;
       }
      if (m==5){
       val1 = 0;
       val2 = 6;
       }
      if (m==6){
       val1 = 0;
       val2 = 7;
       }
      if (m==7){
        val1 = 0;
        val2 = 8;
       }
       if (m==8){
       val1 = 0;
       val2 = 9;
       }
       if (m==9){
       val1 = 1;
       val2 = 0;
       }
       if (m==10){
       val1 = 1;
       val2 = 1;
       }
      
       // * here we read the buttons
        
Button1state = digitalRead(Button1);
Button2state = digitalRead(Button2);
Button3state = digitalRead(Button3);
Button4state = digitalRead(Button4);
Button5state = digitalRead(Button5);
Button6state = digitalRead(Button6);
Button7state = digitalRead(Button7);
Button8state = digitalRead(Button8);

//turn the readings into strings

Butt1 = String(Button1state);
Butt2 = String(Button2state);
Butt3 = String(Button3state);
Butt4 = String(Button4state);
Butt5 = String(Button5state);
Butt6 = String(Button6state);
Butt7 = String(Button7state);
Butt8 = String(Button8state);

//strings because then they can be strung together and then turned into a single integer. 
//now we have all 8 buttons expressed as a single integer. then by using switch case I also eliminate the possibility of multiple button presses. 

Buttons = Butt1 + Butt2 + Butt3 + Butt4 + Butt5 + Butt6 + Butt7 + Butt8;
ButtonsInt=(Buttons.toInt());
switch (ButtonsInt){
  case 1111111:
  val3 = 1;
  break;
  case 10111111:
  val3 = 2;
  break;
  case 11011111:
  val3 = 3;
  break;
  case 11101111:
  val3 = 4;
  break;
  case 11110111:
  val3 = 5;
  break;
  case 11111011:
  val3 = 6;
  break;
  case 11111101:
  val3 = 7;
  break;
  case 11111110:
  val3 = 8;
  break;
  default:
  val3 = 0;
  break;
}
// and finally with one more set of strings  we create a single intiger that represents the encoder position and button press in one number
// because it is an integer, the leading 0 is dropped. so 010 becomes 10.
Switch1 = String(val1);
Switch2 = String(val2);
Switch3 = String(val3);

Switches = Switch1 + Switch2 + Switch3;
SwitchInt=(Switches.toInt());

}



//*********************************************************Print Values*************************
// run this to help troubleshoot. serail print the switch values.
void PrintValues() {

  Serial.print (" Rotary: ");
  Serial.print (val1);
  Serial.print (val2);
  Serial.print (" Buttons: ");
  Serial.println (val3);

 
}

//*************************************************************Display*********************
// in the display subroutine we have the seperate menu pages (later) This next section. and it is long, is the data that is dislayed when a button is pressed. 

void Display(){
  
if ((SwitchInt)!= (SwitchIntHist)){     // this will only let the screen refresh if the switch has changed. 
 tft.fillScreen(HX8357_BLACK);
} 

// this prints the page number up in the top left corner
 tft.setTextSize(2);
     tft.setCursor(10, 10);
    tft.setTextColor(HX8357_WHITE);
     tft.println(SwitchInt);




 switch (SwitchInt) {   //use switchInt to pick the screen display
  case 11:
Word1 = "Random";       // data for subroutine Select1()
Word2 = "Whistle";      // data for subroutine Select1()
Select1();              // Run Select1() 
SwitchIntHist = 11;     // store that 11 was the last switch press
delay(200);             // needed
   break;               // exit out of switch case after finding match.
case 12:
Word1 = "Random";
Word2 = "Sad";
Select2();
SwitchIntHist = 12;
delay(200);  
   break;
case 13:
Word1 = "Random";
Word2 = "Chat";
Select3();
SwitchIntHist = 13;
delay(200);  
   break;
case 14:
Word1 = "Random";
Word2 = "Ack";
Select4();
SwitchIntHist = 14;
delay(200); 
   break;
case 15:
Word1 = "Random";
Word2 = "Razz";
Select5();
SwitchIntHist = 15;
delay(200);  
   break;
case 16:
Word1 = "Random";
Word2 = "Scream";
Select6();
SwitchIntHist = 16;
delay(200);  
   break;
case 17:
Word1 = "Random";
Word2 = "Alarm";
Select7();
SwitchIntHist = 17;
delay(200);  
   break;
case 18:
Word1 = "Random";
Word2 = "Hum";
Select8();
SwitchIntHist = 18;
delay(200);  
   break;
 

  case 21:
Word1 = " Open ";
Word2 = "All";
Select1();
SwitchIntHist = 21;
delay(200);   
   break;
case 22:
Word1 = "Wave";
Word2 = "1";
Select2();
SwitchIntHist = 22;
delay(200);  
   break;
case 23:
Word1 = "Wave";
Word2 = "3";
Select3();
SwitchIntHist = 23;
delay(200);  
   break;
case 24:
Word1 = "Alt";
Word2 = "1";
Select4();
SwitchIntHist = 24;
delay(200); 
   break;
case 25:
Word1 = "Close";
Word2 = "All";
Select5();
SwitchIntHist = 25;
delay(200);  
   break;
case 26:
Word1 = "Wave";
Word2 = "2";
Select6();
SwitchIntHist = 26;
delay(200);  
   break;
case 27:
Word1 = "Wave";
Word2 = "4";
Select7();
SwitchIntHist = 27;
delay(200);  
   break;
case 28:
Word1 = "Alt";
Word2 = "2";
Select8();
SwitchIntHist = 28;
delay(200);  
   break;

  case 31:
Word1 = " Knight ";
Word2 = "RIDER 1";
Select1();
SwitchIntHist = 31;
delay(200);   
   break;
case 32:
Word1 = "Rainbow";
Word2 = " ";
Select2();
SwitchIntHist = 32;
delay(200);  
   break;
case 33:
Word1 = "Dual";
Word2 = "Bounce";
Select3();
SwitchIntHist = 33;
delay(200);  
   break;
case 34:
Word1 = "Auto";
Word2 = "ON";
Select4();
SwitchIntHist = 34;
delay(200); 
   break;
case 35:
Word1 = "Knight";
Word2 = "Rider 2";
Select5();
SwitchIntHist = 35;
delay(200);  
   break;
case 36:
Word1 = "Short";
Word2 = "Circuit";
Select6();
SwitchIntHist = 36;
delay(200);  
   break;
case 37:
Word1 = "Zig";
Word2 = "Zag";
Select7();
SwitchIntHist = 37;
delay(200);  
   break;
case 38:
Word1 = "Auto";
Word2 = "OFF";
Select8();
SwitchIntHist = 38;
delay(200);  
   break;


case 41:
Word1 = "Two";
Word2 = "Legs";
Select1();
SwitchIntHist = 41;
delay(200);   
   break;
case 42:
 Word1 = "Three";
Word2 = "Legs";
Select2();
SwitchIntHist = 42;
   break;
case 43:
Word1 = "Vex";
Word2 = "Only";
Select3();
SwitchIntHist = 43;
   break;
case 44:
Word1 = " ";
Word2 = " ";
Select4();
SwitchIntHist = 44;
   break;
case 45:
Word1 = "Look Up";
Word2 = "Max";
Select5();
SwitchIntHist = 45;
   break;
case 46:
Word1 = "Look";
Word2 = "Up";
Select6();
SwitchIntHist = 46;
   break;
case 47:
Word1 = "Look";
Word2 = " Down";
Select7();
SwitchIntHist = 47;
   break;
case 48:
Word1 = "Look Down Max";
Word2 = "";
Select8();
SwitchIntHist = 48;
   break;
case 51:
Word1 = "Rockets";
Word2 = "Open";
Select1();
SwitchIntHist = 51;
    break;
case 52:
 Word1 = "Rocket Light";
Word2 = "on";
Select2();
SwitchIntHist = 52;
   break;
case 53:
Word1 = "CPU";
Word2 = "Working";
Select3();
SwitchIntHist = 53;
   break;
case 54:
Word1 = "CPU";
Word2 = "Open ";
Select4();
SwitchIntHist = 54;
   break;
case 55:
Word1 = "Rocket";
Word2 = "Closed";
Select5();
SwitchIntHist = 55;
   break;
case 56:
Word1 = "Light";
Word2 = "off";
Select6();
SwitchIntHist = 56;
   break;
case 57:
Word1 = "Hyperdrive";
Word2 = " Arm";
Select7();
SwitchIntHist = 57;
   break;
case 58:
Word1 = "CPU";
Word2 = "Closed";
Select8();
SwitchIntHist = 58;
   break; 
  case 61:
Word1 = "Periscope";
Word2 = "Up";
Select1();
SwitchIntHist = 61;
delay(200);   
   break;
case 62:
Word1 = "Periscope";
Word2 = "Random";
Select2();
SwitchIntHist = 62;
delay(200);  
   break;
case 63:
Word1 = "Speaker";
Word2 = "Up";
Select3();
SwitchIntHist = 63;
delay(200);  
   break;
case 64:
Word1 = "Saber";
Word2 = "Up";
Select4();
SwitchIntHist = 64;
delay(200); 
   break;
case 65:
Word1 = "Peri";
Word2 = "Down";
Select5();
SwitchIntHist = 65;
delay(200);  
   break;
case 66:
Word1 = "Saber";
Word2 = "Light";
Select6();
SwitchIntHist = 66;
delay(200);  
   break;
case 67:
Word1 = "Speaker";
Word2 = "Down";
Select7();
SwitchIntHist = 67;
delay(200);  
   break;
case 68:
Word1 = "Saber";
Word2 = "Down";
Select8();
SwitchIntHist = 68;
delay(200);  
   break;
 case 71:
Word1 = "Rocket";
Word2 = "Man";
Select1();
SwitchIntHist = 71;
delay(200);   
   break;
case 72:
 Word1 = "Leia";
Word2 = "Holo";
Select2();
SwitchIntHist = 72;
   break;
case 73:
Word1 = "Zapper";
Word2 = "Zapping";
Select3();
SwitchIntHist = 73;
   break;
case 74:
Word1 = "Zapper ";
Word2 = "Open";
Select4();
SwitchIntHist = 74;
   break;
case 75:
Word1 = "Favorite";
Word2 = "Things";
Select5();
SwitchIntHist = 75;
   break;
case 76:
Word1 = "Matcho";
Word2 = "Man";
Select6();
SwitchIntHist = 76;
   break;
case 77:
Word1 = "Harlem";
Word2 = " shuffle";
Select7();
SwitchIntHist = 77;
   break;
case 78:
Word1 = "Zapper";
Word2 = "Closed";
Select8();
SwitchIntHist = 78;
   break;       
case 81:
Word1 = "Main";
Word2 = "Theme";
Select1();
SwitchIntHist = 81;
break;
case 82:
 Word1 = "Vaders";
Word2 = "Theme";
Select2();
SwitchIntHist = 82;
   break;
case 83:
Word1 = "Leias";
Word2 = "Theme";
Select3();
SwitchIntHist = 83;
   break;
case 84:
Word1 = "Cantina ";
Word2 = " ";
Select4();
SwitchIntHist = 84;
   break;
case 85:
Word1 = "Staying";
Word2 = "Alive";
Select5();
SwitchIntHist = 85;
   break;
case 86:
Word1 = "R2 ";
Word2 = "Rocks";
Select6();
SwitchIntHist = 86;
   break;
case 87:
Word1 = "Gangam";
Word2 = " Style";
Select7();
SwitchIntHist = 87;
   break;
case 88:
Word1 = "Disco";
Word2 = "Star Wars";
Select8();
SwitchIntHist = 88;
   break;



  case 91:
Word1 = "Mana";
Word2 = "Mana";
Select1();
SwitchIntHist = 91;
delay(200);   
   break;
case 92:
Word1 = "PBJ";
Word2 = "Time ";
Select2();
SwitchIntHist = 92;
delay(200);  
   break;
case 93:
Word1 = "Low ";
Word2 = "Rider ";
Select3();
SwitchIntHist = 93;
delay(200);  
   break;
case 94:
Word1 = "Rocket ";
Word2 = "Man ";
Select4();
SwitchIntHist = 94;
delay(200);
  break;
case 95:
Word1 = "Happy";
Word2 = "B-Day";
Select5();
SwitchIntHist = 95;
delay(200);
   break;
case 96:
Word1 = "Matcho";
Word2 = "Man";
Select6();
SwitchIntHist = 96;
delay(200);  
   break;
case 97:
Word1 = "Harlem";
Word2 = "Shuffle";
Select7();
SwitchIntHist = 97;
delay(200);  
   break;
case 98:
Word1 = "Reset";
Word2 = "";
Select8();
SwitchIntHist = 98;
delay(200);  
   break;

  case 101:
Word1 = "a";
Word2 = "";
Select1();
SwitchIntHist = 101;
delay(200);   
   break;
case 102:
Word1 = "b";
Word2 = "";
Select2();
SwitchIntHist = 102;
delay(200);  
   break;
case 103:
Word1 = "c ";
Word2 = " ";
Select3();
SwitchIntHist = 103;
delay(200);  
   break;
case 104:
Word1 = "d ";
Word2 = " ";
Select4();
SwitchIntHist = 104;
delay(200);
  break;
case 105:
Word1 = "e";
Word2 = "";
Select5();
SwitchIntHist = 105;
delay(200);
   break;
case 106:
Word1 = "f";
Word2 = "";
Select6();
SwitchIntHist = 106;
delay(200);  
   break;
case 107:
Word1 = "g";
Word2 = "";
Select7();
SwitchIntHist = 107;
delay(200);  
   break;
case 108:
Word1 = "h";
Word2 = "";
Select8();
SwitchIntHist = 108;
delay(200);  
   break;

  case 111:
Word1 = "a";
Word2 = "";
Select1();
SwitchIntHist = 111;
delay(200);   
   break;
case 112:
Word1 = "b";
Word2 = "";
Select2();
SwitchIntHist = 112;
delay(200);  
   break;
case 113:
Word1 = "c ";
Word2 = " ";
Select3();
SwitchIntHist = 113;
delay(200);  
   break;
case 114:
Word1 = "d ";
Word2 = " ";
Select4();
SwitchIntHist = 114;
delay(200);
  break;
case 115:
Word1 = "e";
Word2 = "";
Select5();
SwitchIntHist = 115;
delay(200);
   break;
case 116:
Word1 = "f";
Word2 = "";
Select6();
SwitchIntHist = 116;
delay(200);  
   break;
case 117:
Word1 = "g";
Word2 = "";
Select7();
SwitchIntHist = 117;
delay(200);  
   break;
case 118:
Word1 = "h";
Word2 = "";
Select8();
SwitchIntHist = 118;
delay(200);  
   break;

//_______________________________________________________________________________Page menu Data__________________________________________________________
// by using the switch integer, any time that a button is not being pressed the end of that number is 0.
//when no button is pressed, than we want the menu to show. 
   
case 20:                          //starts with 20 so at the end, 10 can be set as a default
menuTitle = ("Front Body");       //Menu title
menu1 = ("Open All");             //data for Menu subroutine  
menu2 = ("Wave 1");               //data for Menu subroutine       
menu3 = ("Wave 3");               //data for Menu subroutine                
menu4 = ("Alt 1  ");              //data for Menu subroutine  
menu5 = ("Close All");            //data for Menu subroutine  
menu6 = ("Wave 2");               //data for Menu subroutine  
menu7 = ("Wave 4 ");              //data for Menu subroutine  
menu8 = "Alt 2 ";                 //data for Menu subroutine  
Menu();                            //Run Menu();
SwitchIntHist = 20;               // store last SwitchInt value
break;                            // exit switch case after finding match

case 30:
menuTitle = ("Lights");
menu1 = ("Knight R1");
menu2 = ("Rainbow");
menu3 = ("Dual Bnc");
menu4 = ("Auto Off  ");
menu5 = ("Knight R2");
menu6 = ("Short Cir");
menu7 = ("Zig Zag ");
menu8 = "Auto On ";
Menu();
SwitchIntHist = 30;
break;  

case 40:
menuTitle = ("Change Stance");
menu1 = ("Two Legs");
menu2 = ("Three Legs");
menu3 = ("Vex Only");
menu4 = ("  ");
menu5 = ("Look Up Max");
menu6 = ("Look Up");
menu7 = ("Look Down ");
menu8 = "Look Down Max ";
Menu();
SwitchIntHist = 40;
break;  
case 50:
menuTitle = ("Toys");
menu1 = ("Rockets Open");
menu2 = ("Rocket Light on");
menu3 = ("CPU Tip");
menu4 = ("CPU Open");
menu5 = ("   Closed");
menu6 = ("      off");
menu7 = ("  ");
menu8 = "CPU Closed ";
Menu();
SwitchIntHist = 50;
break;                
case 60:
menuTitle = ("  Lifters  ");
menu1 = ("Peri Up ");
menu2 = ("Peri Random ");
menu3 = ("Speaker Up ");
menu4 = ("Saber Up ");
menu5 = ("Peri Down ");
menu6 = ("Saber Light ");
menu7 = ("Speaker Down ");
menu8 = " Saber Down ";
Menu();
SwitchIntHist = 60;
break;
case 70:
menuTitle = ("Shows");
menu1 = ("Rocket Man ");
menu2 = ("Leia Holo");
menu3 = ("Zap ");
menu4 = ("Open Zapper ");
menu5 = ("Fav Things");
menu6 = ("??? ");
menu7 = ("??? ");
menu8 = " Close Zapper";
Menu();
SwitchIntHist = 70;
break;
case 80:
menuTitle = ("Songs and Speaches");
menu1 = (" MainTheme");
menu2 = ("Vaders");
menu3 = ("Leias Th");
menu4 = ("Cantina");
menu5 = ("StayinAlive");
menu6 = ("R2 Rockit");
menu7 = ("Gangam ");
menu8 = "Disco SW";
Menu();
SwitchIntHist = 80;
break;
case 90:
menuTitle = ("More Songs");
menu1 = ("Mana Mana ");
menu2 = ("PBJ Time ");
menu3 = ("Low Rider ");
menu4 = ("RocketMan ");
menu5 = ("Happy B-day");
menu6 = ("Matcho Man");
menu7 = ("Harlem Shuf");
menu8 = "Reset ";
Menu();
SwitchIntHist = 90;
break;
case 100:
menuTitle = ("A");
menu1 = ("B ");
menu2 = ("C ");
menu3 = ("D ");
menu4 = ("E ");
menu5 = ("F");
menu6 = ("G");
menu7 = ("H");
menu8 = ("I");
Menu();
SwitchIntHist = 100;
break;
case 110:
menuTitle = ("A");
menu1 = ("B ");
menu2 = ("C ");
menu3 = ("D ");
menu4 = ("E ");
menu5 = ("F");
menu6 = ("G");
menu7 = ("H");
menu8 = ("I");
Menu();
SwitchIntHist = 110;
break;
default:
case 10:
menuTitle = ("Random Sounds");
menu1 = ("R Whistle");
menu2 = ("R Sad");
menu3 = ("R Chat");
menu4 = ("R Ack");
menu5 = ("R Razz");
menu6 = ("R Scream");
menu7 = ("R Alarm");
menu8 = "R Hum";
Menu();
SwitchIntHist = 10;
break;

 }

}

// the color,  text size and spacing of the menu pages is set here
unsigned long Menu() {
  
  //  tft.fillScreen(HX8357_BLACK);
    tft.setTextSize(3);
     tft.setCursor(100, 40);
    tft.setTextColor(HX8357_WHITE);
     tft.println(menuTitle);
     tft.setCursor(10, 80);
    tft.setTextColor(HX8357_BLUE);
     tft.println(menu1);
     tft.setCursor(10, 130);
    tft.setTextColor(HX8357_RED);
     tft.println(menu2);
     tft.setCursor(10, 180);
    tft.setTextColor(HX8357_GREEN);
     tft.println(menu3);
     tft.setCursor(10, 230);
    tft.setTextColor(HX8357_YELLOW);
     tft.println(menu4);
     tft.setCursor(250, 80);
    tft.setTextColor(HX8357_BLUE);
     tft.println(menu5);
     tft.setCursor(250, 130);
    tft.setTextColor(HX8357_RED);   
     tft.println(menu6);
     tft.setCursor(250, 180);
    tft.setTextColor(HX8357_GREEN);
     tft.println(menu7);
      tft.setCursor(250, 230);
    tft.setTextColor(HX8357_YELLOW);
     tft.println(menu8);
   
  
}
//  here is select 1 through 8 so that the chosen button is diplayed in the proper color.

unsigned long Select1() {
   tft.setTextColor(HX8357_BLUE);
   tft.setTextSize(4);
   tft.setCursor(100, 100);
   tft.println(Word1) ;
   tft.setCursor(100, 140);
   tft.println(Word2);
  }

unsigned long Select2() {
   tft.setTextColor(HX8357_RED);
   tft.setTextSize(4);
   tft.setCursor(100, 100);
   tft.println(Word1);
   tft.setCursor(100, 140);
   tft.println(Word2);
  }
unsigned long Select3() {
   tft.setTextColor(HX8357_GREEN);
   tft.setTextSize(4);
   tft.setCursor(100, 100);
   tft.println(Word1);
   tft.setCursor(100, 140);
   tft.println(Word2);
}
unsigned long Select4() {
   tft.setTextColor(HX8357_YELLOW);
   tft.setTextSize(4);
   tft.setCursor(100, 100);
   tft.println(Word1);
   tft.setCursor(100, 140);
   tft.println(Word2);
}
unsigned long Select5() {
   tft.setTextColor(HX8357_BLUE);
   tft.setTextSize(4);
   tft.setCursor(100, 100);
   tft.println(Word1) ;
   tft.setCursor(100, 140);
   tft.println(Word2);
}

unsigned long Select6() {
   tft.setTextColor(HX8357_RED);
   tft.setTextSize(4);
   tft.setCursor(100, 100);
   tft.println(Word1);
   tft.setCursor(100, 140);
   tft.println(Word2);
}
unsigned long Select7() {
   tft.setTextColor(HX8357_GREEN);
   tft.setTextSize(4);
   tft.setCursor(100, 100);
   tft.println(Word1);
   tft.setCursor(100, 140);
   tft.println(Word2);
}
unsigned long Select8() {
   tft.setTextColor(HX8357_YELLOW);
   tft.setTextSize(4);
   tft.setCursor(100, 100);
   tft.println(Word1);
   tft.setCursor(100, 140);
   tft.println(Word2);
}





//----------------------------------------------------------Send------------------

//here is where we send our three val's out to the world. The radio is constatnly transmitting and if no button is pressed the val3 will be zero.  this can be used in the recievers to trigger new actions. 
void Send(){
 RadioPacket.n1 = val1;
  RadioPacket.n2 = val2;
  RadioPacket.n3 = val3;
  
   byte zize=sizeof(RadioPacket);
  memcpy (buf, &RadioPacket, zize); 
 
   Serial.print("Sending "); Serial.println(SwitchInt);

   if (rf69_manager.sendtoWait(buf, zize, DEST_ADDRESS)) {
   //  if (rf69_manager.sendto(buf, zize, DEST_ADDRESS)) {
    }}
  

//---------------------------------------------------Debug Radio
// running this will help diagnose radio issues, normally comment it out in loop.
void DebugRadio(){
 
     Serial.print("RSSI: ");
      Serial.println(rf69.lastRssi(), DEC);
 Serial.print("Sending "); Serial.println(SwitchInt);
   
 byte zize=sizeof(RadioPacket);
    if (rf69_manager.sendto(buf, zize, DEST_ADDRESS)) {
      Serial.println("Message sent");   
    }
    else{
      Serial.println("sendto failed");
    }     
      
//      Serial.print("Got message from unit: ");
         
    
//      Serial.print(IncomingMsg);  
 //     Serial.print("Incoming Int   ");
 //     Serial.println(IncomingInt);
     
    
}
 //====================================================Debug Encoder
 // running this will help diagnose encoder issues, normally comment it out in loop.
void DebugEncoder(){
   
     Serial.print("pos=");
     Serial.print(pos);
     Serial.print(", newpos=");
     Serial.println(newpos);
    
      Serial.print("Diff = ");
      Serial.print(diff);
      Serial.print("  pos= ");
      Serial.print(pos);
      Serial.print(", newpos=");
      Serial.println(newpos);
      Serial.println(menuList[m]);
     

       Serial.print("m is: ");
      Serial.println(m);


      
    
}
           
//--------------------------------------------Loop--------------------------
// I have found that the encoder loop likes living here better than in a subrouteen. 
// ReadButtons is here without any time delay
//

void loop(void) {

currentMillis = millis();

  //check the encoder knob, set the current position as origin
    long newpos = knob.read() / 4;//divide for encoder detents
    if(newpos != pos){
      int diff = newpos - pos;//check the different between old and new position
      if(diff>=1){
        m--; 
        m = (m+11) % 11;//modulo to roll over the m variable through the list size
       }
     if(diff==-1){ //rotating backwards
         m++;
         m = (m+11) % 11;
       }
      pos = newpos; 
    }


// ReadSwitches();
ReadButtons();
 


if (millis() - previousDisplayMillis >= DisplayTime){
previousDisplayMillis = currentMillis;  
PrintValues();
}
if (millis() - previousDisplay2Millis >= DisplayTime){
previousDisplay2Millis = currentMillis;   
Display();
//DebugRadio();
//DebugEncoder();
}
if (millis() - previousSendMillis >= SendTime){
previousSendMillis = currentMillis;
Send();
 
}
 
}
