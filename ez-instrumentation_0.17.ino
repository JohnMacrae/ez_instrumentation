#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ArduinoOTA.h>
//#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <FS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Ticker.h>

Ticker secondTick;

const char* vers = "0.17";
const char* ssid = "RiverRover";
const char* password = "beroroha";
const char* host = "Default";
const char* MQ_client = "Default";       // your network password
const char* MQ_user = "";       // your network password
const char* MQ_pass = "";       // your network password
char server[]="192.168.43.1";
long ADCfactor = 16;
const unsigned int MAX_INPUT = 50;

extern "C" long DS_loop();
int fmode = 14;
int Watchdogcount = 0;

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 0
#define TEMPERATURE_PRECISION 9

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
//DeviceAddress insideThermometer, outsideThermometer;
DeviceAddress deviceAddress;

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

struct SettingsStruct
{
  char          WifiSSID[32];
  char          WifiKey[64];
  char          WifiAPKey[64];
  char          ControllerUser[26];
  char          ControllerPassword[64];
  char          Host[26];
  char          Broker[4];
  char          BrokerIP[16];
  char          Version[16];
} Settings;

boolean ok;
unsigned long start = 0;
unsigned long timer60 = 0;
unsigned int loop_delay = 2000;

volatile int watchdogCount = 0;

void ISRwatchdog() {
  watchdogCount++;
  if ( watchdogCount == 5 ) {
     // Only print to serial when debugging
     ESP.reset();
  }
}

void process_data (const char * data)
  {
  // for now just display it
  // (but you could compare it to some value, convert to an integer, etc.)
  Serial.println (data);
  char topic[0];
  Controller(topic, data);
  }  // end of process_data
  
void processIncomingByte (const byte inByte)
  {
  static char input_line [MAX_INPUT];
  static unsigned int input_pos = 0;

  switch (inByte)
    {

    case '\n':   // end of text
      input_line [input_pos] = 0;  // terminating null byte
      
      // terminator reached! process input_line here ...
      process_data (input_line);
      
      // reset buffer for next time
      input_pos = 0;  
      break;

    case '\r':   // discard carriage return
      break;

    default:
      // keep adding if not full ... allow for terminating null byte
      if (input_pos < (MAX_INPUT - 1))
        input_line [input_pos++] = inByte;
     //Serial.print(input_line);
      break;

    }  // end of switch
   
  } // end of processIncomingByte  

void callback(char* topic, byte* payload, unsigned int length) {
  int pinstate = 1;
  char buf[80];
  // handle message arrived
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  int i = 0;
  for ( i=0;i<length;i++) {
   buf[i] = payload[i];
  }
   buf[i] = '\0';
  Controller(topic, buf);
}


//  strcpy(server,Settings.BrokerIP);
  WiFiClient ethClient;
  PubSubClient client(server, 1883, callback, ethClient);


/********************************************************************************************\
  Save data into config file on SPIFFS
  \*********************************************************************************************/
void SaveToFile(char* fname, int index, byte* memAddress, int datasize)
{
  File f = SPIFFS.open(fname, "r+");
  if (f)
  {
    f.seek(index, SeekSet);
    byte *pointerToByteToSave = memAddress;
    for (int x = 0; x < datasize ; x++)
    {
      f.write(*pointerToByteToSave);
      pointerToByteToSave++;
    }
    f.close();
    String log = F("FILE : File saved");
//    addLog(LOG_LEVEL_INFO, log);
  }
}

/********************************************************************************************\
  Load data from config file on SPIFFS
  \*********************************************************************************************/
void LoadFromFile(char* fname, int index, byte* memAddress, int datasize)
{
  File f = SPIFFS.open(fname, "r+");
  if (f)
  {
    f.seek(index, SeekSet);
    byte *pointerToByteToRead = memAddress;
    for (int x = 0; x < datasize; x++)
    {
      *pointerToByteToRead = f.read();
      pointerToByteToRead++;// next byte
    }
    f.close();
  }
}

  /********************************************************************************************\
  Save settings to SPIFFS
  \*********************************************************************************************/
void SaveSettings(void)
{
  SaveToFile((char*)"config.txt", 0, (byte*)&Settings, sizeof(struct SettingsStruct));
  //SaveToFile((char*)"security.txt", 0, (byte*)&SecuritySettings, sizeof(struct SecurityStruct));
Serial.print("Setting Saved");
}


/********************************************************************************************\
  Load settings from SPIFFS
  \*********************************************************************************************/
boolean LoadSettings()
{
  LoadFromFile((char*)"config.txt", 0, (byte*)&Settings, sizeof(struct SettingsStruct));
//  LoadFromFile((char*)"security.txt", 0, (byte*)&SecuritySettings, sizeof(struct SecurityStruct));
}

void ResetSettings()
  {
   
   strcpy_P ( Settings.WifiSSID, ssid);
   strcpy_P ( Settings.WifiKey, password);
   strcpy_P ( Settings.WifiAPKey, host);
   strcpy_P ( Settings.ControllerUser, MQ_user);
   strcpy_P ( Settings.ControllerPassword, MQ_pass);
   strcpy_P ( Settings.Host, MQ_client);

   strcpy_P ( Settings.BrokerIP , server);
   strcpy_P ( Settings.Version , vers);
  
  SaveSettings();
  SPIFFS.end();
  ESP.restart();
  }

void runEach60Seconds()
{
  start = micros();
  timer60 = millis() + 10000;
  
}
 
  
  void PrintSettings()
{
    Serial.println("Printing...");
    char pTopic[30]={0};
    strcat (pTopic,Settings.Host);
    strcat (pTopic,"/");
    strcat (pTopic,"outTopic");
    client.publish(pTopic,Settings.WifiSSID);
    client.publish(pTopic,Settings.ControllerUser);
    client.publish(pTopic,Settings.Host);

    char charBuf[50];
    client.publish(pTopic,Settings.BrokerIP);
    client.publish(pTopic,Settings.Version);  
}

  void Controller(const char *topic,const char *buf)
  {

  char * pch;
  int i=0;
  long pos = 0;
  int pos2 = 0;
  char gpio[20];
  char val[20];
  char command[20];
  long int lgpio = 0;
  Serial.printf ("Topic[%s]\n",topic);
  Serial.printf ("Looking for the ':' character in \"%s\"...\n",buf);
  pch=strchr(buf,':');
  Serial.printf ("found at %d\n",pch-buf+1);
  if(pch !=NULL){
    pos = pch-buf+1;
  
  pch=strchr(pch+1,':');
  
  if(pch !=NULL){
    pos2 = pch-buf+1;
    //Serial.printf("pos2 \"%d\"...\n",pos2);
   // Serial.printf("pos \"%d\"...\n",pos);
  }
  }else
  {
    pos = strlen(buf)+1;
  }
  
  for (i=0;i<pos-1;i++) {
   
    command[i] = buf[i];
 
   }
    command[i] = '\0';
     Serial.printf("Command \"%s\"\n",command);
    
    if((pos2-pos)>0){
      for ( i=pos;i<pos2-1;i++) {
    
        gpio[i-pos] = buf[i];
   
       }
      gpio[i-pos] = '\0';
     Serial.printf("gpio \"%s\"\n",gpio);
     lgpio=atol(gpio);
    }
       
    if(pos2>0){
      Serial.print("val = "); 
      for ( i=pos2;i<(unsigned)strlen(buf);i++) {
      
        val[i-pos2] = buf[i];

        }
      val[i-pos2] = '\0';
        }

    long int lval = atol(val);
    Serial.printf("value \"%d\"...\n",lval);

    boolean resetflag = false;
   
    if (strcmp (command ,"resetsettings")==0){
    Serial.printf("command \"%d\".....\n",command);
    ResetSettings();
    }
    else if(strcmp (command ,"BrokerIP")==0){
      //Serial.printf("%d",lval);
      strcpy(Settings.BrokerIP,val);
      //Serial.printf("%d .../n",Settings.BrokerIP1);
    }  
    else if(strcmp (command ,"WiFi")==0){
    strcpy(Settings.WifiSSID,gpio);
    strcpy(Settings.WifiKey,val);
    resetflag = 1;
    }
    else if(strcmp (command ,"Broker")==0){
    strcpy(Settings.ControllerUser,gpio);
    strcpy(Settings.ControllerPassword,val);
    resetflag = 1;
    }
    else if(strcmp (command , "Delay")==0){
      loop_delay = lval;
    }
    else if(strcmp (command , "Host")==0){
      strcpy_P (Settings.Host,val);
      resetflag = true;
    }
     else if(strcmp (command , "Reboot")==0){
     resetflag = true;
    }
     else if(strcmp (command , "Reset")==0){
     ResetSettings();
    }
    else if(strcmp (command , "PWM")==0){
    pinMode(lgpio, OUTPUT);
    analogWriteFreq(500);
    analogWrite(lgpio, lval);
    }     
    else if(strcmp (command , "Version")==0){
     char pTopic[30]={0};
     strcat (pTopic,Settings.Host);
     strcat (pTopic,"/");
     client.publish(pTopic,vers);
    }
    else if(strcmp (command , "GPIO")==0){
    pinMode(lgpio, OUTPUT);
    if (lval==1){
      Serial.printf("GPIO \"%d\"...\n",lgpio);
      digitalWrite(lgpio, HIGH);
      } else {
      digitalWrite(lgpio, LOW);
      Serial.printf("GPIO-off \"%d\"...\n",lgpio);
      }
    }
     else if(strcmp (command , "PrintSettings")==0){
     PrintSettings();
     }
      else if(strcmp (command , "PrintSensors")==0){
     PrintSensors();
     }
    SaveSettings();
    if(resetflag){
      ESP.restart();
    }
}

void setup() {
  secondTick.attach(1, ISRwatchdog);
  
   Serial.begin(115200);
   fileSystemCheck();
   LoadSettings();
   Serial.println(vers);
   Serial.println(Settings.Version);
   if(strcmp(Settings.Version, vers) != 0)
  {
    ResetSettings();
  }
   
   strcpy(server,Settings.BrokerIP);
  
   Serial.println("Booting");
   WiFi.hostname(Settings.Host);
   WiFi.mode(WIFI_STA);
   Serial.println(Settings.WifiSSID);
   Serial.println(Settings.Host);
   WiFi.begin(Settings.WifiSSID, Settings.WifiKey);
  
     int count =0;
   while ((count<15)&&(WiFi.status() != WL_CONNECTED)){
     
     //WiFi.begin(Settings.WifiSSID, Settings.WifiKey);    
     Serial.print(WiFi.status());
     Serial.println(" Retrying connection...");
     delay(1000);
     count++;
     while (Serial.available () > 0)
     processIncomingByte (Serial.read ());
   }

  MDNS.begin(host);

  httpUpdater.setup(&httpServer);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", Settings.Host);
   
   Serial.print(Settings.BrokerIP);
    
   //if (client.connect("", Settings.ControllerUser , Settings.ControllerPassword)) {
   if (client.connect(Settings.Host)) {
   client.publish(strcat(Settings.Host,"/outTopic"),"hello world");
   //client.subscribe(strcat(Settings.Host,"system"));
   Serial.print("Connected ");
   Serial.println(Settings.BrokerIP);
   }
   
// Start up the library
  sensors.begin();

  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

   for (int i = 0; i < sensors.getDeviceCount(); i++)
   {
     if (!sensors.getAddress(deviceAddress, i)) Serial.printf("Unable to find address for Device %d",i); 
   }
  
 // show the addresses we found on the bus
  char cSensor[15]={0};
   for (int i = 0; i < sensors.getDeviceCount(); i++)
  {
    bool ok = sensors.getAddress(deviceAddress, i);
    Serial.printf("Device %d Address: ",i);
    printAddress(deviceAddress);
    Serial.println();
    Serial.printf("Device %d Resolution: ",i);
    Serial.print(sensors.getResolution(deviceAddress), DEC); 
    Serial.println();
    sensors.setResolution(deviceAddress, TEMPERATURE_PRECISION);
    convAddress(cSensor,deviceAddress);
    Serial.print(cSensor);
    Serial.println();
    //if (client.connect(Settings.Host, Settings.ControllerUser , Settings.ControllerPassword)) {
       if (client.connect(Settings.Host)) {
    client.publish("Default/outTopic",cSensor); 
    }
  }
  /* switch off led */

  ArduinoOTA.setHostname(Settings.Host);
  ArduinoOTA.onStart([]() { // switch off all the PWMs during upgrade
                       SPIFFS.end();
                    });

  ArduinoOTA.onEnd([]() { // do a fancy thing with our board led at end
                        
                        });

  ArduinoOTA.onError([](ota_error_t error) { ESP.restart(); });

   /* setup the OTA server */
   ArduinoOTA.begin();
   Serial.println("Ready");
   Serial.print("IP address: ");
   Serial.println(WiFi.localIP());

}

void reconnect() {
  // Loop until we're reconnected
   int count = 0;
  
  while( (count<15)&&(WiFi.status() != WL_CONNECTED)){
     
     //WiFi.begin(Settings.WifiSSID, Settings.WifiKey);    
     Serial.print(WiFi.status());
     Serial.println(" Retrying connection...");
     delay(1000);
     count++;
     while (Serial.available () > 0)
     processIncomingByte (Serial.read ());
   }
   
   if(WiFi.status() != WL_CONNECTED);
  {
    count=0;
    while (!client.connected()&&(count < 5)) {
     Serial.print("Attempting MQTT connection...");
    // Attempt to connect
     Serial.print(Settings.BrokerIP);
    
//      if (client.connect("", Settings.ControllerUser , Settings.ControllerPassword)){ 
      if (client.connect(Settings.Host)){ 
        Serial.println("connected");
       client.publish(strcat(Settings.Host,"/outTopic"),"hello world");
      //client.subscribe(strcat(Settings.Host,"system"));
      } else  {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 1 second");
      // Wait a second before retrying
        delay(1000);
      }
    count = count+1;
    }
  }
}

void MQTT_Topic(char *Host, char *Topic, char *Value)
{
  char charBuf[10]={0};
  char pTopic[30]={0};
  strcat (pTopic,Host);
  strcat (pTopic,"/");
  strcat (pTopic,Topic);
  pTopic[strlen(pTopic)]='\0';
  client.publish(pTopic,Value);
  Serial.println();
  Serial.printf("%s : \"%s\"\n",pTopic, Value);
}

void fileSystemCheck()
{
  if (SPIFFS.begin())
  {
    Serial.println("SPIFFS Mount successful");
    //addLog(LOG_LEVEL_INFO, log);
    File f = SPIFFS.open("config.txt", "r");
    if (!f)
    {
      Serial.println("formatting...");
      //addLog(LOG_LEVEL_INFO, log);
      SPIFFS.format();
      Serial.println("format done!");
      //addLog(LOG_LEVEL_INFO, log);
      File f = SPIFFS.open("config.txt", "w");
      if (f)
      {
        for (int x = 0; x < 32768; x++)
          f.write(0);
        f.close();
        ResetSettings();
      }
    }
  }
  else
  {
    Serial.println("SPIFFS Mount failed");
    //addLog(LOG_LEVEL_INFO, log);
  }
}

void convAddress(char *ID, DeviceAddress deviceAddress)
{
  char id[4]={""};
  char buf[17]={""};
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    //if (deviceAddress[i] < 16) strcat(buf,"0");
    
    int n = snprintf(id,3,"%02X",deviceAddress[i]);
    strcat(buf,id);
    buf[17]='\0';
  }
  strncpy (ID,buf,17); 
 // Serial.printf("%s : %s",id ,ID);
}

  void PrintSensors()
{
    Serial.println("Printing...");
    char pTopic[30]={0};
    strcat (pTopic,Settings.Host);
    strcat (pTopic,"/");
    strcat (pTopic,"outTopic");

   for (int i = 0; i < sensors.getDeviceCount(); i++)
  {
  char cSensor[17]={0};
  bool ok = sensors.getAddress(deviceAddress, i);
  Serial.printf("Device %d Address: ",i);
  convAddress(cSensor,deviceAddress);
  client.publish(pTopic,cSensor);  
  }
  
}

void loop() {
   watchdogCount = 0;
   ArduinoOTA.handle();

  if (!client.connected()) {
    reconnect();
  }


 // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures();
  Serial.println("DONE");
     
      char cSensor[17]="XXXXXXXXXXXXXXXX";
      char temp[8];
      char ptemp[8];
      float tempC;
      char pTopic[60]={0};
      //Serial.print(sensors.getDeviceCount());
      // print the device information
    for (int i = 0; i < sensors.getDeviceCount(); i++)
      {
      
      char pTopic[60]={0};
      bool ok = sensors.getAddress(deviceAddress, i);
      convAddress(cSensor,deviceAddress);
      tempC = sensors.getTempC(deviceAddress);
      dtostrf(tempC,3,1,temp);
      
      strcat(pTopic,Settings.Host);
      strcat(pTopic,"/");
      strcat(pTopic,cSensor);
      Serial.print(pTopic);
      Serial.print(" : ");
      Serial.println(temp);
      client.publish(pTopic,temp);
     // MQTT_Topic(Settings.Host, cSensor, temp, 5);
      }

    delay(loop_delay);
 
  client.loop();
   //Serial.print("Try Byte: "); 
   while (Serial.available () > 0)
    //Serial.print("Byte: ");
    processIncomingByte (Serial.read ());
      httpServer.handleClient();
}
