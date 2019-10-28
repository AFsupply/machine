/*
 * ESP8266 + DHT22 + BMP180 + BOOTSTRAP + SPIFFS + GOOGLE CHARTS
 * Copyright (C) 2017 http://www.projetsdiy.fr - http://www.diyprojects.io
 * 
 * Part 2 : Interaction between Arduino code and HTML interface, data exchange in JSON
 * - How to prepare a JSON object to send measurements to the HTML interface from Arduino code
 * - Add a Bootstrap-table table (http://bootstrap-table.wenzhixin.net.cn/) and refresh the display automatically
 * - Driving the ESP8266 GPIO from the HTML interface
 * - Full tutorial here http://www.diyprojects.io/esp8266-ftp-server-spiffs-file-exchange-rapid-development-web-server/
 *
 * Partie 2 (Web Serveur ESP8266) : Interaction entre le code Arduino et l'interface HTML, échange de données en JSON
 * - Comment préparer un objet JSON pour envoyer les mesures à l'interface HTML depuis le code Arduino
 * - Ajouter une table Bootstrap-table (http://bootstrap-table.wenzhixin.net.cn/) et actualiser l'affichage automatiquement
 * - Piloter le GPIO de l'ESP8266 depuis l'interface HTML
 * - Tutoriel complet http://www.projetsdiy.fr/esp8266-web-serveur-partie2-interaction-arduino-interface-html/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 //Library
 #include <ESP8266WiFi.h>
 #include <ESP8266WebServer.h>
 #include <ESP8266HTTPClient.h>
 #include <TimeLib.h>
 #include <NtpClientLib.h>
 #include <ArduinoJson.h>
 #include <FS.h>
 #include <PubSubClient.h>
 #include <Stepper.h>
// #include "Plastickind.h"

 // Motor variable
 #define STEPPER_PIN_1 9
 #define STEPPER_PIN_2 10
 #define STEPPER_PIN_3 11
 #define STEPPER_PIN_4 12
 int step_number = 0;

 //Wifi variable
 #define ssid1      "Livebox-"      // WiFi SSID
 #define password1  ""  // WiFi password
 #define ssid     "ESP_recycleBot"      // WiFi SSID
 #define password ""  // WiFi password
 #define HISTORY_FILE "/history.json"
 char json[10000];  
 String connectionMethod = "default"; 
 const char* mqtt_server = "broker.mqttdashboard.com";
 int nbTour = 2048;
 const int stepsPerRevolution = 32*64;  // change this to fit the number of steps per revolution
 // initialize the stepper library on D1,D2,D5,D6

 //Infra sensor variable
 int floorCountInfraSensor = 16;
 //int InInfraSensor = 10;
 //int WaitInfraSensor = 10;
 
  //Variable to adjust parameters for the first time
 bool firstAdjustMade = false;

 //Variable to store position data
 int currentPos=0;
 //Variable to count flore
 int countFloor;
 
 Stepper myStepper(stepsPerRevolution, D5, D6, D7, D8);
 WiFiClient espClient;
 PubSubClient client(espClient);

  
 ESP8266WebServer server ( 80 );



 void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Command from MQTT broker is : [");
  Serial.print(topic);
  int p =(char)payload[0]-'0';
  // step one revolution  in one direction:
  if(p==1) 
  {
    myStepper.step(stepsPerRevolution);
    Serial.print("  clockwise" );
   }
  // step one revolution in the other direction:
  else if(p==2)
  {
    myStepper.step(-stepsPerRevolution);
    Serial.print("  counterclockwise" );
   }
   Serial.println();
}
 
//  Serial.println();
 //end callback

 void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if you MQTT broker has clientID,username and password
    //please change following line to    if (client.connect(clientId,userName,passWord))
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
     //once connected to MQTT broker, subscribe command if any
      client.subscribe("OsoyooCommand");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 6 seconds before retrying
      delay(6000);
    }
  }
}

//Elevator tell to the bridge "i'm here"
void SendGoIn(String ip ){
  //request http to the bridge on /goIn
  
  Serial.println("coucou3");
  Serial.println(ip);
  HTTPClient http;
  http.begin(ip + "/goIn");
  int httpCode = http.GET();
  if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] GET... code: %d\n" , httpCode);
            

            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                Serial.println(payload);
            }
        } else {
            Serial.printf("[HTTP] GET... failed, error: %s\n" , http.errorToString(httpCode).c_str());
            
        }
  
  }

//Elevator recive demand's signal
 void call() {
  String message="";
  WiFiClient client1; 
  if(server.arg("callFrom") == "" || server.arg("goTo") == ""){
    message = "Je ne sais pas ou tu es ou ou tu vas";
   }else{
    Serial.println("coucou");
     int floorToDo = abs(currentPos-server.arg("callFrom").toInt());
     if(currentPos-server.arg("callFrom").toInt()==0){
      //dire au demandeur que l'asseseur et arrivé
      SendGoIn(server.client().remoteIP().toString());
      Serial.println("i'am already here");  
     }else{
        countFloor=0;
        while(countFloor!=floorToDo){
          if(!digitalRead(floorCountInfraSensor)==HIGH){
           countFloor++;
           myStepper.step(2048);
           delay(10);
           Serial.println("I detected you on floor");
           Serial.println(countFloor);
          }
          if(currentPos-server.arg("callFrom").toInt()<0){
          //motor clockwise (monte)
          myStepper.step(1);
          }else{
          //motor counterwise (descend)
          myStepper.step(-1);
          }
          //Serial.println("coucou4");
          yield();
        }
        Serial.println("You're arrived !");
        currentPos= server.arg("callFrom").toInt();
        //dire au demandeur que l'asseseur et arrivé
      }
      //while(!digitalRead(InInfraSensor)){
      //dire que j'attends
      //} 
     floorToDo = abs(server.arg("goTo").toInt()-server.arg("callFrom").toInt());
     if(server.arg("goTo").toInt()-server.arg("callFrom").toInt()==0){
      //dire au demandeur que l'asseseur et arrivé  
     }else{
        while(countFloor!=floorToDo){
        if(server.arg("goTo").toInt()-server.arg("callFrom").toInt()<0){
          //motor clockwise (monte)
          currentPos= server.arg("goTo").toInt();
        }else{
          //motor counterwise (descend)
          currentPos= server.arg("goTo").toInt();
          }
        }
        //dire au demandeur que l'asseseur et arrivé
      }
     
     message="nous allons au :";
     message+=server.arg("goTo");
     int tour = server.arg("callFrom").toInt()-server.arg("goTo").toInt();
     message+= "soit";
     message+= tour;
     message+= "tour";    
    }
  server.send(200,"text/plain",message);
 }
//                         VOID SETUP

  void setup() {

  
  Serial.begin ( 115200 );
  Serial.println("scan start");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      if (WiFi.SSID(i) == ssid1) {
        connectionMethod = "home";
        Serial.println ( connectionMethod );
      }else {      
        };
      delay(10);
    }
  }

  
   

   if (connectionMethod == "home"){
    Serial.print ( "Connecting to " ); 
    Serial.println ( ssid1 );
    WiFi.begin ( ssid1, password1 );
  // Attente de la connexion au réseau WiFi / Wait for connection
    while ( WiFi.status() != WL_CONNECTED ) {
      delay ( 500 ); Serial.print ( "." );
    }
  // Connexion WiFi établie / WiFi connexion is OK
    Serial.println ( "" );
    Serial.print ( "Connected to " ); Serial.println ( ssid1 );
    Serial.print ( "IP address: " ); Serial.println ( WiFi.localIP() );
   }else{
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP( ssid, password );
    Serial.print ( "Connected to " ); 
    Serial.println ( ssid );
    Serial.print ( "IP address: " ); 
    Serial.println ( WiFi.softAPIP());
    
   }
   delay(10);

   NTP.begin("pool.ntp.org", 1, true);
   NTP.setInterval(60);
   Serial.println(NTP.getTime());
   NTP.onNTPSyncEvent([](NTPSyncEvent_t error) {
    if (error) {
     Serial.print("Time Sync error: ");
      if (error == noResponse)
        Serial.println("NTP server not reachable");
      else if (error == invalidAddress)
        Serial.println("Invalid NTP server address");
    }
    else {
      Serial.print("Got NTP time: ");
      Serial.println(NTP.getTimeDateString(NTP.getLastNTPSync()));
    }
  });

   if (!SPIFFS.begin()){
     Serial.println("SPIFFS Mount failed");
    } else {
     Serial.println("SPIFFS Mount succesfull");
    }


//   server.on("/gpio", updateGpio);
   server.on("/call", call);
//  server.on("/plasticList.json", sendPlasticList);
// server.on("/savePlasticList", savePlasticList);
   /*HTTP_POST, []() {
     updateGpio();
   });
   */
   server.serveStatic("/js", SPIFFS, "/js");
   server.serveStatic("/css", SPIFFS, "/css");
   server.serveStatic("/img", SPIFFS, "/img");
   server.serveStatic("/", SPIFFS, "/index.html");
 
   server.begin();
   Serial.println();
   Serial.println ( "HTTP server started" );
   myStepper.setSpeed(12);
   pinMode(floorCountInfraSensor, INPUT);
   digitalWrite(floorCountInfraSensor, LOW);
   
 }
 
 void loop() {
   // put your main code here, to run repeatedly:
   server.handleClient();
   delay(100);
  
  //myStepper.step(nbTour);
  
  //myStepper.step(-nbTour);
   delay(10);
//  Serial.println("rootStrAfterLoad");
//  Serial.println(rootStrAfterLoad);
//  Serial.println("json");
//  Serial.println(json);
//  Serial.println();
//  Serial.println();
//  Serial.println("root");
//  root.prettyPrintTo(Serial);
//  Serial.println();
//  Serial.println("root2");
//  root2.prettyPrintTo(Serial);
//  Serial.println();
//   addPlastic();
////  
//  savePlastic();
//  loadPlastic();
  

    

    
   
   
   
 }
