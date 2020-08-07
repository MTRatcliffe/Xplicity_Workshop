/* Example Code For Xplicity workshop 2020
  by MTR <www.mike@Michaelratcliffe.com>
  This example code is in the public domain.


-Basic_Sketch: Just tests the actuator and servo work as expected
-Basic_Sketch_L1: Fixing comunications issues with controller and limiting the ability of the motor to foul the housing
                  We can have a more stable comunication if we reduce the serial speed, reduce the motor current draw by slowing it down, adding a delay between motor movment and serial ussage
                  Change baud rate to 9600, limit motor movment speed, add half a second dealy before serial output, add min/max limits to servo possition

-Basic_Sketch_L2: Now that we all have a working unit that is moving the servo, reading the air quality sensor and trasmiting dat via serial
                  We will calibrate the servo possition for open and closed


                 
*/

//*****************************************************************************
// Libs, variables and info pertaining to the Servo controlling windows
// Note: Bad manufacturing of housing means that at full possition (180) servo fouls housing
//       Limit max servo angle to 150 to fix this with software
///////////////////////////////////////////////////////////////////////////////
#include <Servo.h>
Servo Window_Servo;  // create servo object to control a servo

int Servo_Max_Speed =20;  // How quick the motor turns in degree's per second, reduce this if you have serial conection problems
int Servo_Pos_Closed = 90; // Servo possition coresponding to a closed window
int Servo_Pos_Open = 150;  //Servo possition coresponding to Open window

int Max_Possition =150;   // Variable to limit the max angle the servo can travel
int Min_Possition =30;    // Variable to limit the min angle the servo can travel
int Servo_Possition=20;   //Variable used for motor speed control

//*****************************************************************************
// Libs, variables and info pertaining to the temperature and humidity sensor
///////////////////////////////////////////////////////////////////////////////
#include <Wire.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 bme; // I2C
bool BME_Status=0; // Flag, used o check if there is a error reading sensor


//**********************************************************************************************************************************************************
// Setup Loop, this will run once following power up of the MCU
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  // put your setup code here, to run once:
  delay(10000);// To stop brownouts/bad code locking user out offlashing new code
  
  Serial.begin(9600); //Setting up serial output from board
  Serial.println(F("BME280 Setup"));
  Wire.begin(D6,D5);// 0=sda, 2=scl
  Serial.println(F("BME280 Test"));
  Read_BME();
  delay(100);

  Serial.println(F("Setting up Servo"));
  Window_Servo.attach(D8);  // attaches the servo on GIO2 to the servo object
  Test_Servo(); // When system is powered on, run the servo to indicate to user its working
  delay(500);
}

//**********************************************************************************************************************************************************
// Main Loop, this loop will run forever untill a error occors or power is cycled
// put your main code here, to run repeatedly:
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  
Serial.println(F("up-Time:"));
Serial.println(millis());

Read_BME();
Calibrate_Servo();
Test_Servo_Calibration();

}


//**********************************************************************************************************************************************************
//  Small routine that will sweep the servo to show it is working
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Test_Servo(){
Serial.println(F("Testing the Window Servo"));
 Move_Servo(10);  // Sending a desired servo possition to the move servo function that controls speed
 delay(2000);
 Move_Servo(150);
}


//**********************************************************************************************************************************************************
//  If the motor moves too fast it will create power issues and serial will not work
//  This function checks the servo possition is reasonable and then limits the motor speed
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Move_Servo(int new_position){

//angle sanity check
if(new_position>Max_Possition) new_position=Max_Possition;
if(new_position<Min_Possition) new_position=Min_Possition;



// Incrimenting the servo possition one degree at a time with a pause inbetween
while(Servo_Possition!=new_position){


    if(Servo_Possition>new_position){
      Servo_Possition--;
      Window_Servo.write(Servo_Possition);   
                                    }
                                    
    if(Servo_Possition<new_position){
      Servo_Possition++;
      Window_Servo.write(Servo_Possition);
                                      }
delay(1000/Servo_Max_Speed); //   
  
}
delay(500); // when the motor stops moving it can cause voltage spikes, this delay stops it interfering with serial coms

  
}

//**********************************************************************************************************************************************************
//  Rouine that will slowly incriment servo possition and outputpo to serial
//  Using this we write down the servo position when the window is just closed and then fully open
// For the one on my desk this coresponds to 90 and 150 degrees
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Calibrate_Servo() {
int pos;
  for (pos = Min_Possition; pos <= Max_Possition; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    Move_Servo(pos);              // tell servo to go to position in variable 'pos'
    Serial.print("Servo_ Position = ");
    Serial.println(pos);
    
  }
}

void Test_Servo_Calibration(){
    Move_Servo(Servo_Pos_Closed);              // tell servo to go to position where the window is closed
    Serial.println("Window Closed ");
    delay(2000);
    Move_Servo(Servo_Pos_Open);              // tell servo to go to position where the window is open
    Serial.println("Window Open ");
    delay(2000);

     
}

//**********************************************************************************************************************************************************
//  Small routine thatreads the air quality sensor and outputs the dat to the serial console
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Read_BME() {
Serial.println(F("Reading Air Quality Sensor"));

BME_Status= bme.begin(0x76);

if (BME_Status==1){   
    Serial.print("Temperature = ");
    Serial.print(bme.readTemperature());
    Serial.println(" *C");

    Serial.print("Pressure = ");

    Serial.print(bme.readPressure() / 100.0F);
    Serial.println(" hPa");

    Serial.print("Humidity = ");
    Serial.print(bme.readHumidity());
    Serial.println(" %");

    Serial.println();
}
 else Serial.println("BME Error");
 //bme.writeMode(smSleep);   
 delay(100); 
}
