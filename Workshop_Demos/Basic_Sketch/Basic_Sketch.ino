/* Example Code For Xplicity workshop 2020
  by MTR <www.mike@Michaelratcliffe.com>
  This example code is in the public domain.


-Basic_Sketch: Just tests the actuator and servo work as expected

                 
*/

//*****************************************************************************
// Libs, variables and info pertaining to the Servo controlling windows
// Note: Bad manufacturing of housing means that at full possition (180) servo fouls housing
//       Limit max servo angle to 160 to fix this with software
///////////////////////////////////////////////////////////////////////////////
#include <Servo.h>
Servo Window_Servo;  // create servo object to control a servo


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
  
  Serial.begin(115200); //Setting up serial output from board
  

  Serial.println(F("LED Test"));
// pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
// digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
  // but actually the LED is on; this is because
  // it is active low on the ESP-01)
 // delay(1000);                      // Wait for a second
//  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
 // delay(2000);      

   Serial.println(F("BME280 Setup"));
   Wire.begin(D6,D5);// 0=sda, 2=scl
   Serial.println(F("BME280 Test"));
   Read_BME();
   delay(100);

  Serial.println(F("Servo test"));
  Window_Servo.attach(D8);  // attaches the servo on GIO2 to the servo object
  
  Test_Servo(); // When system is powered on, run the servo to indicate to user its working
  delay(100);
}

//**********************************************************************************************************************************************************
// Main Loop, this loop will run forever untill a error occors or power is cycled
// put your main code here, to run repeatedly:
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  
Serial.println(F("up-Time:"));
Serial.println(millis());
Read_BME();
Test_Servo(); // When system is powered on, run the servo to indicate to user its working
//Test_Servo_New(); // When system is powered on, run the servo to indicate to user its working
delay(5000);

}


//**********************************************************************************************************************************************************
//  Small routine that will sweep the servo to show it is working
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Test_Servo(){

 Window_Servo.write(10);
 delay(2000);
 Window_Servo.write(150);
}

void Test_Servo_New(){


    int pos;

  for (pos = 10; pos <= 160; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    Window_Servo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(40);                       // waits 15ms for the servo to reach the position
  }
  for (pos = 160; pos >= 10; pos -= 1) { // goes from 180 degrees to 0 degrees
    Window_Servo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(40);                       // waits 15ms for the servo to reach the position
  }
}

//**********************************************************************************************************************************************************
//  Small routine that will sweep the servo to show it is working
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Read_BME() {


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
}
