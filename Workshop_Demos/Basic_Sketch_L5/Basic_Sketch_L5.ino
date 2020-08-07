/* Example Code For Xplicity workshop 2020
  by MTR <www.mike@Michaelratcliffe.com>
  This example code is in the public domain.


-Basic_Sketch: Just tests the actuator and servo work as expected
-Basic_Sketch_L1: Fixing comunications issues with controller and limiting the ability of the motor to foul the housing
                  We can have a more stable comunication if we reduce the serial speed, reduce the motor current draw by slowing it down, adding a delay between motor movment and serial ussage
                  Change baud rate to 9600, limit motor movment speed, add half a second dealy before serial output, add min/max limits to servo possition

-Basic_Sketch_L2: Now that we all have a working unit that is moving the servo, reading the air quality sensor and trasmiting dat via serial
                  We will calibrate the servo possition for open and closed

-Basic_Sketch_L3: Now we are sarting to impliment thisngs that the client cares about Automaton of temp control
                  Passing sensor readings into variables not just serial 
                  Implimenting a basic hysteresis control for temp, too hot open the window

-Basic_Sketch_L4: Air temp and humidity is niceto know, but a good greenhouse controls the vapor pressure deficit, a way to quantify how well the plant is breathing       

-Basic_Sketch_L5: The esp8266 has iot capabilities lets have a web based readout

  
*/

//*****************************************************************************
// IOT Variables
///////////////////////////////////////////////////////////////////////////////
#include <ESP8266WiFi.h>
const char* ssid = "Farm-B";//type your ssid
const char* password = "";//type your password

WiFiServer server(80);//Service Port
//*****************************************************************************
// Enviromental variables
///////////////////////////////////////////////////////////////////////////////
float Set_Temp = 22; // How hot we want the max temp of the greenhouse
float Bandgap =0.5;


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


float Air_temp =-444;  // Variable to store air temp in degree C
float Humidity =-444;  // Variable to store Humidity in Percent
float Pressure =-444; 
float VPD =-444;
float VPsat = -444; //Variable to calculate VDP
float VPactual = -444;  //Variable to calculate VDP





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

  Serial.println(F("Setting up IOT"));
  IOT_Setup();
  delay(100);

  Serial.println(F("Setting up Servo"));
  Window_Servo.attach(D8);  // attaches the servo on GIO2 to the servo object
  Test_Servo(); // When system is powered on, run the servo to indicate to user its working
  delay(2000);

  Serial.println(F("Setting up IOT"));
  IOT_Setup();
  delay(100);
}

//**********************************************************************************************************************************************************
// Main Loop, this loop will run forever untill a error occors or power is cycled
// put your main code here, to run repeatedly:
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  
Serial.println(F("up-Time:"));
Serial.println(millis());

Serial.println(F("up-Time:"));
Serial.println(millis());
Read_BME();
Control_Temp_V1();
Check_IOT();
Serial.print("Use this URL to connect: ");
Serial.print("http://");
Serial.print(WiFi.localIP());
Serial.println("/");

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


BME_Status= bme.begin(0x76);

if (BME_Status==1){   

//Setting values to distinct numbers so corrupt data can be seen easily  
Air_temp =-444;  
Humidity =-444;  
Pressure =-444; 

  Air_temp= bme.readTemperature();
  Humidity= bme.readHumidity();
  Pressure= bme.readPressure();

//Calculating VDP or how well a plant is breathing
VPsat = 610.7 * pow(10, (7.5 * Air_temp / (237.3 + Air_temp)));// Saturation vapor pressure in Pascals
VPactual = (Humidity * VPsat) / 100.0;  // Actual vapor pressure in Pascals
VPD = ((100.0 - Humidity) /100.0) * VPsat;  // Vapor Pressure Deficit in Pascals
VPD=VPD/1000.0; // Greenhouse folk like to use the unit of kPa
  
    Serial.print("Temperature = ");
    Serial.print(Air_temp);
    Serial.println(" *C");

    Serial.print("Pressure = ");
    Serial.print(Pressure);
    Serial.println(" hPa");

    Serial.print("Humidity = ");
    Serial.print(Humidity);
    Serial.println(" %");

    
    Serial.print("VPD = ");
    Serial.print(VPD);

    Serial.println();
}
 else Serial.println("BME Error");
 //bme.writeMode(smSleep);    
}



//**********************************************************************************************************************************************************
//  Very basic hysteresis temp control system
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Control_Temp_V1(){

 Serial.println("Checking Temperature ");

if(Air_temp> (Set_Temp + Bandgap)){
   Move_Servo(Servo_Pos_Open);               // tell servo to go to position where the window is open
    Serial.println("Window Open Too hot ");
    
  
}
else if (Air_temp < (Set_Temp - Bandgap)) 
{
     Move_Servo(Servo_Pos_Closed);              // tell servo to go to position where the window is open
    Serial.println("Window Closed Too Cold ");
}
  
}

//**********************************************************************************************************************************************************
//  Very basic IOT 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void IOT_Setup(){
Serial.println();
Serial.println();
Serial.print("Connecting to ");
Serial.println(ssid);

if(sizeof(password)>0)WiFi.begin(ssid,password);
else WiFi.begin(ssid);

delay(2000);// give the router time to setup;
if (WiFi.status() != WL_CONNECTED) Serial.println("WiFi Problem");
else Serial.println("WiFi connected");

// Start the server
server.begin();
delay(100);
Serial.println("Server started");

// Print the IP address
Serial.print("Use this URL to connect: ");
Serial.print("http://");
Serial.print(WiFi.localIP());
Serial.println("/");
}

//**********************************************************************************************************************************************************
//  Very basic IOT, reconnecting if connection lost and sending user some data 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Check_IOT(){
  
if(sizeof(password)>0)WiFi.begin(ssid,password);
else WiFi.begin(ssid);

delay(2000);// give the router time to setup;
if (WiFi.status() != WL_CONNECTED){
if(sizeof(password)>0)WiFi.begin(ssid,password);
else WiFi.begin(ssid);
server.begin();
}
else Serial.println("WiFi connected");


// Print the IP address
Serial.print("Use this URL to connect: ");
Serial.print("http://");
Serial.print(WiFi.localIP());
Serial.println("/");
// Check if a client has connected
WiFiClient client = server.available();
if (client) {

Serial.println("new client");



// Return the response
client.println("HTTP/1.1 200 OK");
client.println("Content-Type: text/html");
client.println(""); //  do not forget this one
client.println("<!DOCTYPE HTML>");
client.println("<html>");

    client.print("Temperature = ");
    client.print(Air_temp);
    client.println(" *C");

client.println("<br><br>");

    client.print("Pressure = ");
    client.print(Pressure);
    client.println(" hPa");

client.println("<br><br>");

    client.print("Humidity = ");
    client.print(Humidity);
    client.println(" %");

client.println("<br><br>");
    
    client.print("VPD = ");
    client.print(VPD);
    client.println("kPa");


client.println("<br><br>");
client.println("</html>");

}
}
