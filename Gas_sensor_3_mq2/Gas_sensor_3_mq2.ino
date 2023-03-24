

//Include the library
#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#include <FirebaseESP32.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#elif defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <FirebaseESP8266.h>
#endif

#include <MQUnifiedsensor.h>
#include "time.h"

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>

/************************Hardware Related Macros************************************/
#define         Board                   ("ESP32")
#define         Pin33                     (33)  //Analog input 3 of your arduino
/***********************Software Related Macros************************************/
#define         Type                    ("MQ-2") //MQ2
#define         Voltage_Resolution      (3.3)
#define         ADC_Bit_Resolution      (12) // For arduino UNO/MEGA/NANO
#define         RatioMQ2CleanAir        (9.83) //RS / R0 = 9.83 ppm 

/* 1. Define the WiFi credentials */
#define WIFI_SSID ""                                  
#define WIFI_PASSWORD "" 

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

#define FORMAT_SPIFFS_IF_FAILED true

/*****************************Globals***********************************************/
MQUnifiedsensor MQ2(Board, Voltage_Resolution, ADC_Bit_Resolution, Pin33, Type);
/*****************************Globals***********************************************/


// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  else{
    Serial.println("SPIFFS mounted successfully");
  }
}

String readGasH2(){
   MQ2.update();
   MQ2.setA(987.99); MQ2.setB(-2.162);
   float H2 = MQ2.readSensor();
   if (isnan(H2)){
   Serial.println("Failed to read from MQ2 H2 gas");
   return "";
   }else{
    Serial.println(H2);
    return String(H2);
   }
}

String readGasLPG(){
   MQ2.update();
   MQ2.setA(574.25); MQ2.setB(-2.222);
   float LPG = MQ2.readSensor();
   if (isnan(LPG)){
   Serial.println("Failed to read from MQ2 LPG gas");
   return "";
   }else{
    Serial.println(LPG);
    return String(LPG);
   }
}

String readGasCO(){
   MQ2.update();
   MQ2.setA(36974); MQ2.setB(-3.109);
   float CO = MQ2.readSensor();
   if (isnan(CO)){
   Serial.println("Failed to read from MQ2 CO gas");
   return "";
   }else{
    Serial.println(CO);
    return String(CO);
   }
}

String readGasAlcohol(){
   MQ2.update();
   MQ2.setA(3616.1); MQ2.setB(-2.675);
   float Alcohol = MQ2.readSensor();
   if (isnan(Alcohol)){
   Serial.println("Failed to read from MQ2 Alcohol gas");
   return "";
   }else{
    Serial.println(Alcohol);
    return String(Alcohol);
   }
}

String readGasPropane(){
   MQ2.update();
   MQ2.setA(658.71); MQ2.setB(-2.168);
   float Propane = MQ2.readSensor();
   if (isnan(Propane)){
   Serial.println("Failed to read from MQ2 Propane gas");
   return "";
   }else{
    Serial.println(Propane);
    return String(Propane);
   }
}



String getSensorReadings_MQ2_1(){
  readings["sensor1"] = String(readGasH2());
  readings["sensor2"] = String(readGasLPG());
  readings["sensor3"] = String(readGasCO());
  readings["sensor4"] = String(readGasAlcohol());
  readings["sensor5"] = String(readGasPropane());
  String jsonString = JSON.stringify(readings);
  return jsonString;
}


String getSensorReadings_MQ2_2(){
  readings["sensor1"] = String(readGasH2());
  readings["sensor2"] = String(readGasLPG());
  readings["sensor3"] = String(readGasCO());
  readings["sensor4"] = String(readGasAlcohol());
  readings["sensor5"] = String(readGasPropane());
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

String getSensorReadings_MQ2_3(){
  readings["sensor1"] = String(readGasH2());
  readings["sensor2"] = String(readGasLPG());
  readings["sensor3"] = String(readGasCO());
  readings["sensor4"] = String(readGasAlcohol());
  readings["sensor5"] = String(readGasPropane());
  String jsonString = JSON.stringify(readings);
  return jsonString;
}



void setup() {
  //Init the serial port communication - to debug the library
  Serial.begin(115200); //Init serial port

  initWiFi();
  initSPIFFS();
  MQ2.init(); 
  MQ2.setRegressionMethod(1); //_PPM =  a*ratio^b
//  MQ2.setRL(4.7);

  /*****************************  MQ CAlibration ********************************************/ 
  // Explanation: 
   // In this routine the sensor will measure the resistance of the sensor supposedly before being pre-heated
  // and on clean air (Calibration conditions), setting up R0 value.
  // We recomend executing this routine only on setup in laboratory conditions.
  // This routine does not need to be executed on each restart, you can load your R0 value from eeprom.
  // Acknowledgements: https://jayconsystems.com/blog/understanding-a-gas-sensor
  Serial.print("Calibrating please wait.");
  float calcR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ2.update(); // Update data, the arduino will read the voltage from the analog pin
    calcR0 += MQ2.calibrate(RatioMQ2CleanAir);
    Serial.print(".");
  }
  MQ2.setR0(calcR0/10);
  Serial.println("  done!.");
  
  if(isinf(calcR0)) {Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(calcR0 == 0){Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); while(1);}
  /*****************************  MQ CAlibration ********************************************/ 
  
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });
  
  server.serveStatic("/", SPIFFS, "/");

  
   server.on("/readings_MQ2_1", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getSensorReadings_MQ2_1();
    request->send(200, "application/json", json);
    json = String();
  });

    server.on("/readings_MQ2_2", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getSensorReadings_MQ2_2();
    request->send(200, "application/json", json);
    json = String();
  });

  server.on("/readings_MQ2_3", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getSensorReadings_MQ2_3();
    request->send(200, "application/json", json);
    json = String();
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  
  // Start server
  server.begin();


}

void loop() {
    if ((millis() - lastTime) > timerDelay) {
    // Send Events to the client with the Sensor Readings Every 10 seconds
    events.send("ping",NULL,millis());
    events.send(getSensorReadings_MQ2_1().c_str(),"new_readings_MQ2_1" ,millis());
    events.send(getSensorReadings_MQ2_2().c_str(),"new_readings_MQ2_2" ,millis());
    events.send(getSensorReadings_MQ2_3().c_str(),"new_readings_MQ2_3" ,millis());
    lastTime = millis();
  }

}
