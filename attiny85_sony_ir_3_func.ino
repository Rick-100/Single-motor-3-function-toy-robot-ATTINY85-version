/*
 * Created: 3/10/2020 8:02:40 PM
 * Author : Rick100
 * cam switch on PB1 (leg 6) high when switch is depressed
 * transmit at 2400 baud on PB0 (leg 5)
 * L9110s controlA  on PB3 (leg 2)
 * L9110s controlB  on PB4(leg 3)
 * receive 12 bit Sony codes on PB2 (leg 7)
 * universal remote with Sony TV code 00000 sends
 * up arrow = 244
 * down arrow = 245
 * left arrow = 180
 * right arrow = 179
 * middle button = 229
 * 1 button = 128
 * 2 button = 129
 * 3 button = 130
 * 4 button = 131
 * 5 button = 132
 * 6 button = 133
 * 7 button = 134
 * 8 button = 135
 * 9 button = 136
 * 0 button = 137
 
 * direction 0 = forward
 * direction 1 = right
 * direction 2 = left

  */ 



/*
 IR remote control (Sony) detection for Arduino, M. Burnette
 Binary sketch size: 2,794 bytes (of a 8,192 byte maximum)

 ?  20130103 MRB Modified for interface to Mega2560
     Europa codebase for menu system
     
 ?  20121230 MRB Modified for Tiny85 Google Tiny library
     Tiny85 Internal RC 16MHz

 ?  20121230 MRB modifications to adapt to numeric input, avoid dupes,
     and to generally "behave" consistently
 ?  Used with Electronic Goldmine IR detector MIM 5383H4
     http://www.goldmine-elec-products.com/prodinfo.asp?number=G16737
 ?  IR detector:
     Pin 1: To pin D4 on Arduino ATtiny85
     Pin 2: GND
     Pin 3: 5V through 33 Ohm resistor
 
 ?  This is based on pmalmsten's code found on the Arduino forum from 2007:
     http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1176098434/0

     Modified by Rick100 for IR motor controller
     My version uses the Attiny core from SpenceKonde

*/

//#define SERIALDEBUG

//universal remote with Sony TV code sends
const int UPARROW = 244;
const int DOWNARROW = 245;
const int LEFTARROW = 180;
const int RIGHTARROW = 179;
const int ENTERBUTTON = 229;



// leg 5 (PB0) used for Software serial TX

#define CAM_SWITCH PIN_PB1  //NO switch tied to ground on leg 6
#define IR_PIN PIN_PB2    //IR receiver leg 7
#define CONTROL_A PIN_PB3   //Attiny85 leg 2 to L9110s IA or input A pin (leg 6 of L9110S)
#define CONTROL_B PIN_PB4  //Attiny85 leg 3 to L9110s IB or input B pin (leg 7 of L9110S)

//#define NUMOFINDEXES 8  // 8 directions
#define NUMOFINDEXES 3  // 3 directions


int start_bit = 2200;    //Start bit threshold (Microseconds)
int bin_1     = 1000;    //Binary 1 threshold (Microseconds)
int bin_0     = 400;     //Binary 0 threshold (Microseconds)

uint8_t motor_state = 0;  //0 = motor stopped / 1 = motor running forward / 2 = motor running reverse
uint8_t currentDirection; //direction the car is currently pointed
uint8_t speed = 255;  //full speed

void motorForward()
{
  digitalWrite(CONTROL_A,HIGH);
  //digitalWrite(CONTROL_B,LOW);
  analogWrite(CONTROL_B, 255 - speed);
  motor_state = 1;
}
void motorReverse()
{
  digitalWrite(CONTROL_A,LOW);
  //digitalWrite(CONTROL_B,HIGH);
  analogWrite(CONTROL_B,255);   //high
  motor_state = 2;
}

void motorStop()
{
  digitalWrite(CONTROL_A,HIGH);
  //digitalWrite(CONTROL_B,HIGH);
  analogWrite(CONTROL_B,255); //high
  motor_state = 0;
}


void motorIndex()
{
  uint16_t debounce;
  uint8_t debounce_flag;
       //start the motor in reverse and wait for the switch to be hi for 10 passes then low for 10 passes
       motorReverse();
       debounce = 0;
       debounce_flag = 1;
       while(debounce_flag)
       {
         if(digitalRead(CAM_SWITCH) != 0)
         { //wait for hi
           debounce++;
           if(debounce >= 10)
           {
             debounce_flag = 0;
           }
           }else
           {
           debounce = 0;
         }
         delay(5);
       }

       debounce = 0;
       debounce_flag = 1;
       while(debounce_flag)
       {
         if((PINB & (1 << CAM_SWITCH)) == 0)
         {  //wait for low
           debounce++;
           if(debounce >= 10)
           {
             debounce_flag = 0;
           }
           }else
           {
           debounce = 0;
         }
         delay(5);
       }
       
       motorStop();

}

void gotoNewDirection(uint8_t newdir)
{
  while(currentDirection != newdir)
  {
    motorIndex();
    currentDirection++;
    if(currentDirection >= NUMOFINDEXES)  //wrap currentDirection back to 0
    {
      currentDirection = 0;
    }
  }
  motorForward(); 
}


void setup() {
 pinMode(CONTROL_A, OUTPUT);
 pinMode(CONTROL_B,OUTPUT);
 digitalWrite(CONTROL_A, HIGH); // motor off when both controlA and controlB are same level
 analogWrite(CONTROL_B, 255);
 pinMode(IR_PIN, INPUT);   //ir receiver input

#ifdef SERIALDEBUG
 Serial.begin(4800);  // can't uses 2400 with this Attiny core
 Serial.println("IR/Serial Initialized: ");
#endif

 
}


void loop() {
 int deviceCode, commandCode;
  
 int key = getIRKey();   //Fetch the key
 Serial.println(key);

 
 if(key != 0)            //Ignore keys that are zero
#ifdef SERIALDEBUG
  Serial.println(key);
  Serial.println(key, BIN);
  deviceCode = (key & 0b111110000000) >> 7;
  commandCode = (key & 0b1111111);
  Serial.println(deviceCode);
  Serial.println(commandCode);
#endif
 {
   switch(key)
     {
       case 244:  //forward
       if(motor_state != 0)
       {
        motorStop();
       }
       gotoNewDirection(0);
      break;

       case 180: //left
       if(motor_state != 0)
       {
         motorStop();
       }
       gotoNewDirection(2);    
       break;
 
       case 179: //right
       if(motor_state != 0)
       {
         motorStop();
       }
       gotoNewDirection(1);
       break;
      
//----------- speed selection
       case 128: //1 key
       speed = 50;
       if(motor_state == 1){
        analogWrite(CONTROL_B, 255 - speed);
       }
       break;

       case 129: //2 key
       speed = 75;
       if(motor_state == 1){
        analogWrite(CONTROL_B, 255 - speed);
       }
       break;
       
       case 130: //3 key
       speed = 100;
       if(motor_state == 1){
        analogWrite(CONTROL_B, 255 - speed);
       }
       break;

       case 131: //4 key
       speed = 125;
       if(motor_state == 1){
        analogWrite(CONTROL_B, 255 - speed);
       }
       break;
       
       case 132: //5 key
       speed = 150;
       if(motor_state == 1){
        analogWrite(CONTROL_B, 255 - speed);
       }
       break;
       
       case 133: //6 key
       speed = 175;
       if(motor_state == 1){
        analogWrite(CONTROL_B, 255 - speed);
       }
       break;

       case 134: //7 key
       speed = 200;
       if(motor_state == 1){
        analogWrite(CONTROL_B, 255 - speed);
       }
       break;

       case 135: //8 key
       speed = 225;
       if(motor_state == 1){
        analogWrite(CONTROL_B, 255 - speed);
       }
       break;

       case 136: //9 key
       speed = 255;
       if(motor_state == 1){
        analogWrite(CONTROL_B, 255 - speed);
       }
       break;
//--------------------------------------------------------


   
       case 229: //stop
         motorStop();
         break;

       //default: Serial.println(key); // for inspection of keycode
     }
   delay(400);    // avoid double key logging (adjustable)
 }
}

int getIRKey() {
 int data[12];
 int i;

 while(pulseIn(IR_PIN, LOW) < start_bit); //Wait for a start bit
 
 for(i = 0 ; i <= 11 ; i++)
   data[i] = pulseIn(IR_PIN, LOW);      //Start measuring bits, I only want low pulses
 
 for(i = 0 ; i <= 11 ; i++)             //Parse them
 {     
   if(data[i] > bin_1)                 //is it a 1?
     data[i] = 1;
   else if(data[i] > bin_0)            //is it a 0?
     data[i] = 0;
   else
     return -1;                        //Flag the data as invalid; I don't know what it is! Return -1 on invalid data
 }

 int result = 0;
 for(i = 0 ; i <= 11 ; i++)             //Convert data bits to integer
   if(data[i] == 1) result |= (1<<i);

 return result;                        //Return key number
}
