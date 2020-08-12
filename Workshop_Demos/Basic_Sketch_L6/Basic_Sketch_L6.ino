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

-Basic_Sketch_L6: The esp8266 also has 4mb of internal Non-volatile memory, we can use this to store data locally in the form of a sensor data log and html files to make the gui more interactive

  
*/

//*****************************************************************************
// IOT Variables
///////////////////////////////////////////////////////////////////////////////
#include <ESP8266WiFi.h>
#include <FS.h>   
const char* ssid = "Farm-B";//type your ssid
const char* password = "";//type your password
String Filepath="/Log/data1.csv"; // Thids is the path to where we are storing our data
File Logfile;
File fsUploadFile;
String DataLog=""; // Just a string that we use to store data temp before upload


//WiFiServer server(80);//Service Port
#include <WebSocketsServer.h>
#include <WebSocketsClient.h>
WebSocketsServer webSocket = WebSocketsServer(81); //setting up websockets stuff for sending user setpoints
//WebSocketsClient webSocket;
int WebSocketsReadings=0;  //A Flag For When To Send Data T readings.html via websckets                           


#include <ESP8266WebServer.h>
ESP8266WebServer server(80);




String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  //Some extra stuff t handle the gzipped cntent [faster page load]
else if(filename.endsWith(".htm.gz")) return "text/html";
else if(filename.endsWith(".html.gz")) return "text/html";
else if(filename.endsWith(".css.gz")) return "text/css";
else if(filename.endsWith(".js.gz")) return "application/javascript";
else if(filename.endsWith(".png.gz")) return "image/png";
else if(filename.endsWith(".gif.gz")) return "image/gif";
else if(filename.endsWith(".jpg.gz")) return "image/jpeg";
else if(filename.endsWith(".ico.gz")) return "image/x-icon";
else if(filename.endsWith(".xml.gz")) return "text/xml";
else if(filename.endsWith(".pdf.gz")) return "application/x-pdf";
else if(filename.endsWith(".zip.gz")) return "application/x-zip";
else if(filename.endsWith(".gz")) return "application/x-gzip";


  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".csv")) return "text/csv";

//Extra Stuff to handle fnt types
else if(filename.endsWith(".woff")) return  "application/x-font-woff";
else if(filename.endsWith(".woff2")) return  "application/octet-stream";
else if(filename.endsWith(".ttf")) return   "application/font-ttf";
else if(filename.endsWith(".eot")) return   "application/vnd.ms-fontobject";
else if(filename.endsWith(".otf")) return   "application/font-otf";
 

  return "application/x-gzip";
 //return "text/plain";


 
}
bool handleFileRead(String path){
  if(path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileList() {
  if(!server.hasArg("dir")) {server.send(500, "text/plain", "BAD ARGS"); return;}
  
  String path = server.arg("dir");
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }
  
  output += "]";
  server.send(200, "text/json", output);
}


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

  Serial.println(F("Setting up Servo"));
  Window_Servo.attach(D8);  // attaches the servo on GIO2 to the servo object
  Test_Servo(); // When system is powered on, run the servo to indicate to user its working
  delay(2000);

  Serial.println(F("Setting up IOT"));
  IOT_Setup();
  delay(100);
  SetupWebSokets(); // how we are passing data between esp8266 and html webpage
  

  
    {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {    
    String fileName = dir.fileName();
     size_t fileSize = dir.fileSize();
    }
  }
  
  //SERVER INIT
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
   delay(5);
  //load editor
  server.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.html")) server.send(404, "text/plain", "FileNotFound");
  });
   delay(5);
  //create file

  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([](){
    if(!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });
   delay(5);

 
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
LogData();
Control_Temp_V1();
//Check_IOT();
Run_IOT();
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

SPIFFS.begin();// We need to boot up the internal memory
}

void SetupWebSokets(){
    webSocket.begin();
    delay(40);
    webSocket.onEvent(webSocketEvent);
    yield();
    delay(40);
    
 }

//**********************************************************************************************************************************************************
//  If a user connects to the IP of the controller, send them a html file from the internal memory and update their sensor readings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Run_IOT() {//Run IOT things multiple time to reduce lag?
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
for (int count=0; count <= 20; count++){
      delay(1);
       yield(); //Yield Is a ESP8266 Function, it gives processing time to the hidden functions needed to run IOT,Spiffs etc 
      server.handleClient(); //delivering webpage to user
      delay(1);
       yield();
    webSocket.loop();//checking if we have new data from website
      delay(1);
      yield();
}
}

//*********************** Websockets Code For Moving data too and from GUI ****************************//
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {

    switch(type) {
        case WStype_DISCONNECTED:
            
            break;
            //Sending some stuff to the webgui
        case WStype_CONNECTED:
         {  Serial.println(F("WebSockets Connected"));
          IPAddress ip = webSocket.remoteIP(num);
          //Sending each bit of data one at a time, maybe it would be cleaner to json this up at a later date 
         webSocket.sendTXT(num, "WeBSocketsConnected");
       
          }
         
       
            break;
        case WStype_TEXT:
        {
           String text = String((char *) &payload[0]); //Turning the websockets incoming into a string

//************************** IF Reading HTML Is Open **************************************************************//
if(text.startsWith("#ConnectedReadings#")){  //Html is sendig "Connected" when any index page is loaded
webSocket.sendTXT(num, "Connected To Reading WebSockets");                                
Serial.println("Readings WebSockets Initiated");
}
if(text.startsWith("#SendMeSomeData#")){  //Html wants some more data now and it has been 5 seconds or more since we last sent some
Read_BME();//Updating the Readings for the GUI
Serial.println("Readings WebSockets Ended");  
//Checking if the payload came from Setpoints GUI
Serial.println("Updated Readings html");
String t= {"#Time#"+String(millis())};
webSocket.sendTXT(num, t);
delay(20);
String a= {"#S1#"+String(Air_temp)};
webSocket.sendTXT(num, a);
delay(20);
String b= {"#S2#"+String(Humidity)};
webSocket.sendTXT(num, b);
delay(20);
String c= {"#S3#"+String(VPD)};
webSocket.sendTXT(num, c);
     

}

if(text.startsWith("#DeleetLog#")){  //Html is sendig "Connected" when any index page is loaded
                                 DeleetLog();
                                 webSocket.sendTXT(num, "#LogDeleeted#");
                                 Serial.println(F("Deleeted Log Files"));
                               }


            }
            
           Serial.println(F("Reached end of websockets call"));
           webSocket.sendTXT(num, payload, lenght);  //# Dont think we need this m.r
            webSocket.broadcastTXT(payload, lenght);  //# Dont think we need this m.r
           delay(2);
            break;

        //# Need to refresh my memory on what thi is actually doing? m.r
        case WStype_BIN:
     
            hexdump(payload, lenght);

            // echo data back to browser
            webSocket.sendBIN(num, payload, lenght);
            break;


            
    }


}


//**********************************************************************************************************************************************************
//  Logging data to the internal memory
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////     
/*Useful Stuff:

   r      Open text file for reading.  The stream is positioned at the
          beginning of the file.

   r+     Open for reading and writing.  The stream is positioned at the
          beginning of the file.

   w      Truncate file to zero length or create text file for writing.
          The stream is positioned at the beginning of the file.

   w+     Open for reading and writing.  The file is created if it does
          not exist, otherwise it is truncated.  The stream is
          positioned at the beginning of the file.

   a      Open for appending (writing at end of file).  The file is
          created if it does not exist.  The stream is positioned at the
          end of the file.

   a+     Open for reading and appending (writing at end of file).  The
          file is created if it does not exist.  The initial file
          position for reading is at the beginning of the file, but
          output is always appended to the end of the file.
*/

//** We are creatng a .csv format  "data"+","+"data2"+","+"data3"
void LogData(){
Serial.print(F("Logging Data to Spiffs Log File:"));  

if (Logfile.size()>=1000000){ // How Much memory we are allocatng to the logs in bytes [100,000 = 0.1mb])){ // Lets not over run our memory, strange things will happen
Serial.println(F("LogFile Full, not logging data"));
Logfile.close();
           
                                }

// If memory isnt full, lets fill it up with data readings
else {

Logfile = SPIFFS.open(Filepath, "a");// Open the log file in write mode

DataLog=Air_temp; //A 
DataLog+=","; //A 
DataLog+=Humidity; //A 
DataLog+=","; //A 
DataLog+=VPD; //A 
Serial.println(DataLog);
Logfile.println(DataLog); // commiting the new cvs data string to memory
delay(10);
Logfile.close();
 
  
}
}

void  DeleetLog(){
  Serial.println("User Selected to Deleet data log");
 File Logfile = SPIFFS.open("/Log/data1.csv", "w");    
   Logfile.println("Temp *C, Humidity %, VPD kPa");  //puttng some blank data in   
   Logfile.close();

  
}
