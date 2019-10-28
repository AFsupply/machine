/*  #include <ArduinoJson.h>
 #include <FS.h>
 #include <Stepper.h>
 #include <ESP8266HTTPClient.h>
 #include <ESPAsyncWebServer.h>
 #include <ESPAsyncTCP.h>
 #include <AsyncJson.h> */

 
 #define ssid1      "Livebox"      // WiFi SSID
 #define password1  ""  // WiFi password
 #define ssid     "ESP_recycleBot"      // WiFi SSID
 #define password ""  // WiFi password
 //#define HISTORY_FILE "/history.json"
 #define TASK_FILE "/task.json"
 #define CURRENTPOS_FILE "/currentPos.json"
 #define M_PI 3.14159265358979323846 
     
 String connectionMethod = "default";  // WiFi password

   // Distance execute by X motor per revolution
 double rayRoueX = 12.5;
 double distPerRevX = 2 * M_PI * rayRoueX;
 const int stepsPerRevolutionX = 32*64;  
 double distPerStepX = distPerRevX/stepsPerRevolutionX ;
 
 // Distance execute by Y motor per revolution
 int distPerRevY = 18;
 const int stepsPerRevolutionY = 32*64;  
 int distPerStepY = distPerRevY/stepsPerRevolutionY ;
 int originYContactSensor = 16;

  // Distance execute by Z motor per revolution
 int distPerRevZ = 18;
 const int stepsPerRevolutionZ = 32*64;  
 int distPerStepZ = distPerRevZ/stepsPerRevolutionZ ;
 int originZContactSensor = 16;

  // Angle execute by C motor per revolution
 int anglePerRevC = 18;
 const int stepsPerRevolutionC = 32*64;  
 int anglePerStepC = anglePerRevC/stepsPerRevolutionC ;
 int openPosC = 12;
 
 Stepper myXStepper(stepsPerRevolutionX, D5, D6, D7, D8);
 //Stepper myYStepper(stepsPerRevolutionX, 6, 7, 8, 9);
 //Stepper myZStepper(stepsPerRevolutionX, 10, 11, 12, 13);
 //Stepper myCStepper(stepsPerRevolutionX, 17, 16, 15, 14);
 WiFiClient espClient;
  
 AsyncWebServer server ( 80 );

 //Task variable
 String kindTask;
 int line;
 int stage;
 int x;
 int y;
 int z;
 int diam;
 String elevator;
 bool done;
 String from;
 bool taskDone;
 bool haveTaskRecorded;

 //Distance between elevator and area
 int distBtElevatorX = 50;

 //Distance to grow area
 bool inGrowArea;
 
 //Distance to load pot area
 int loadPotAreaLine = 5;
 int loadPotAreaFloor = 6;
 bool inLoadPotArea;
 int xPosToLoadPot = 300;
 int yPosToLoadPot = 2;
 int zPosToLoadPot = 2;

 //Distance to drop area
 int dropAreaLine = 3;
 int dropAreaFloor = 4;
 bool inDropArea;
 int xPosToDrop = 400;
 int yPosToDrop = 2;
 int zPosToDrop = 2;

 //Distance to power charge area
 int powerAreaLine = 1;
 int powerAreaFloor = 2;
 bool inPowerArea;
 int xPosToPower = 200;
 int yPosToPower = 2;
 int zPosToPower = 2; 
 
  //Variable to store position data
 bool haveCurrentPosRecorded;
 int currentLine; 
 int currentFloor;
 int currentXPos;
 int currentYPos;
 int currentZPos;
 int currentCPos;

 //variable use to trigger in main loop
 bool goOnReceive = false;
 bool goInReceive = false;
 bool goOutReceive = false;
 bool potPicked = false;

//Go to origine for each motor
void goToYOrigin(){
  while(!digitalRead(originYContactSensor)==HIGH){
    //myYStepper.step(step--);
  }
}

//Go to origine for each motor
void goToZOrigin(){
  while(!digitalRead(originZContactSensor)==HIGH){
    //myZStepper.step(step--);
  }
}

//Send task
void sendTask(){
  const size_t capacity = JSON_OBJECT_SIZE(10);
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& task = jsonBuffer.createObject();
  task["kindTask"] = kindTask;
  task["line"] = line;
  task["floor"] = stage;
  task["x"] = x;
  task["y"] = y;
  task["z"] = z;
  task["diam"] = diam;
  task["elevator"] = elevator;
  task["done"] = done;
  task["from"] = from;
  Serial.println("in send task");
  String url = from;
  url += "/receiveTask";
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  String taskString;
  task.printTo(taskString);
  http.POST(taskString);
  http.writeToStream(&Serial);
  http.end();
  int httpCode = http.GET();
  if(httpCode > 0) {
   // HTTP header has been send and Server response header has been handled
   Serial.printf("[HTTP] GET... code: %d\n" , httpCode);
   // file found at server
   if(httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println(payload);
   }
  }else{
    Serial.printf("[HTTP] GET... failed, error: %s\n" , http.errorToString(httpCode).c_str());
   }
  }

void saveTaskWithJson(JsonObject& task){
  File taskFile = SPIFFS.open(TASK_FILE, "w");
  task.prettyPrintTo(Serial);
  task.printTo(taskFile); // Exporte et enregsitre le JSON dans la zone SPIFFS - Export and save JSON object to SPIFFS area
  taskFile.close();  
}

void saveTask(){
  const size_t capacity = JSON_OBJECT_SIZE(10);
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& task = jsonBuffer.createObject();
  task["kindTask"] = kindTask;
  task["line"] = line;
  task["floor"] = stage;
  task["x"] = x;
  task["y"] = y;
  task["z"] = z;
  task["diam"] = diam;
  task["elevator"] = elevator;
  task["done"] = done;
  task["from"] = from;
  File taskFile = SPIFFS.open(TASK_FILE, "w");
  task.prettyPrintTo(Serial);
  task.printTo(taskFile); // Exporte et enregsitre le JSON dans la zone SPIFFS - Export and save JSON object to SPIFFS area
  taskFile.close();  
}

//Delete history of task (task.json)
void deleteTask(){
   SPIFFS.remove("/task.json");
   haveTaskRecorded = false;
}


////Load history of task from SPIFFS
//JsonObject& loadTask2(){
//  File file = SPIFFS.open(TASK_FILE, "r");
//  if (!file){
//    Serial.println();
//    Serial.println("Aucun historique de tâche existe - No Task History Exist");
//    haveTaskRecorded = false;
//  } else {
//    size_t size = file.size();
//    if ( size == 0 ) {
//      Serial.println();
//      Serial.println("Fichier historique de tâche vide - Task History file empty !");
//      haveTaskRecorded = false;
//    } else {
//        const size_t capacity = JSON_OBJECT_SIZE(8) + 90;
//        DynamicJsonBuffer jsonBuffer(capacity);        
//        JsonObject& task = jsonBuffer.parse(file);
//        if (!task.success()) {
//          Serial.println();
//          Serial.println("Impossible de lire le JSON - Impossible to read JSON file");
//          haveTaskRecorded = false;
//        } else {
//          Serial.println();
//          Serial.println("Historique de tâche charge - Task History loaded");
//          Serial.println("task in spiff function");
//          task.prettyPrintTo(Serial);
//          haveTaskRecorded = true;
//          return task;
//          }
//      }
//    file.close();
//  }
//}

JsonObject& loadTask(){
  File file = SPIFFS.open(TASK_FILE, "r");
  if (!file){
    Serial.println();
    Serial.println("Aucun historique de tâche existe - No Task History Exist");
    haveTaskRecorded = false;
  } else {
    size_t size = file.size();
    if ( size == 0 ) {
      Serial.println();
      Serial.println("Fichier historique de tâche vide - Task History file empty !");
      haveTaskRecorded = false;
    } else {
        const size_t capacity = JSON_OBJECT_SIZE(10) + 120;
        DynamicJsonBuffer jsonBuffer(capacity);        
        JsonObject& task = jsonBuffer.parse(file);
        if (!task.success()) {
          Serial.println();
          Serial.println("Impossible de lire le JSON - Impossible to read JSON file");
          haveTaskRecorded = false;
        } else {
          Serial.println();
          Serial.println("Historique de tâche charge - Task History loaded");
          Serial.println("task in spiff function");
          kindTask = task["kindTask"].as<String>();
          line = task["line"].as<int>();
          stage = task["floor"];
          x = task["x"];
          y = task["y"];
          z = task["z"];
          diam = task["diam"];
          elevator = task["elevator"].as<String>();
          done = task["done"];
          from = task["from"].as<String>();
          Serial.println(stage);
          task.prettyPrintTo(Serial);
          haveTaskRecorded = true;
          return task;
          }
      }
    file.close();
  }
}

//Save the current position in SPIFFS
void saveCurrentPos(){
  File currentPosFile = SPIFFS.open(CURRENTPOS_FILE, "w");
  DynamicJsonBuffer jsonBuffer;
  JsonObject& currentPos = jsonBuffer.createObject();
  currentPos["currentLine"] = currentLine; 
  currentPos["currentFloor"] = currentFloor;
  currentPos["currentXPos"] = currentXPos;
  currentPos["currentYPos"] = currentYPos;
  currentPos["currentZPos"] = currentZPos;
  currentPos["currentCPos"] = currentCPos;
  currentPos["inPowerArea"] = inPowerArea;
  currentPos["inLoadPotArea"] = inLoadPotArea;
  currentPos["inGrowArea"] = inGrowArea;
  currentPos["inDropArea"] = inDropArea;
  currentPos.prettyPrintTo(Serial);
  currentPos.printTo(currentPosFile); // Exporte et enregsitre le JSON dans la zone SPIFFS - Export and save JSON object to SPIFFS area
  currentPosFile.close();  
}

//Delete history of current position
void deleteCurrentPos(){
     SPIFFS.remove("/currentPos.json");
     haveCurrentPosRecorded = false;
}

//Load last know position. It must be the current one if everythings is ok
JsonObject& loadCurrentPos(){
  File file = SPIFFS.open(CURRENTPOS_FILE, "r");
  if (!file){
    Serial.println();
    Serial.println("No History Exist to CURRENTPOS");
    haveCurrentPosRecorded = false;
  } else {
    size_t size = file.size();
    if ( size == 0 ) {
      Serial.println();
      Serial.println("Fichier historique vide - History file empty !");
      haveCurrentPosRecorded = false;
    } else {
        const size_t capacity = JSON_OBJECT_SIZE(8) + 90;
        DynamicJsonBuffer jsonBuffer(capacity);        
        JsonObject& currentPos = jsonBuffer.parse(file);
        if (!currentPos.success()) {
          Serial.println();
          Serial.println("Impossible de lire le JSON - Impossible to read JSON file");
          haveCurrentPosRecorded = false;
        } else {
          Serial.println();
          Serial.println("Current position loaded");
          Serial.println("Position in SPIFF");
          currentPos.prettyPrintTo(Serial);
          haveCurrentPosRecorded = true;
          //set global variable
          currentLine = currentPos["currentLine"]; 
          currentFloor = currentPos["currentFloor"];
          currentXPos =currentPos["currentXPos"];
          currentYPos = currentPos["currentYPos"];
          currentZPos = currentPos["currentZPos"];
          currentCPos = currentPos["currentCPos"];
          inPowerArea = currentPos["inPowerArea"];
          inLoadPotArea = currentPos["inLoadPotArea"];
          inGrowArea = currentPos["inGrowArea"];
          inDropArea = currentPos["inDropArea"];
          return currentPos;
          }
      }
    file.close();
  }
}

//Send a position to the bridge
void setPosition(AsyncWebServerRequest *request, uint8_t *datas) {
  Serial.printf("[REQUEST]\t%s\r\n", (const char*)datas);
  DynamicJsonBuffer jsonBuffer;
  JsonObject& startPosition = jsonBuffer.parseObject((const char*)datas); 
  if (!startPosition.success()){
    request->send(200, "text/plain", "No JSON receive");
  }else{
    if (!startPosition.containsKey("inLoadArea") && !startPosition.containsKey("inDropArea") && !startPosition.containsKey("inPowerArea") && !startPosition.containsKey("inGrowArea")){
      request->send(200, "text/plain", "I don't know the area you suggested.");
    }else{
      if(startPosition["inPowerArea"] != true && startPosition["inLoadPotArea"] != true && startPosition["inGrowArea"] != true && startPosition["inDropArea"] == true){
        request->send(200, "text/plain", "I wanna know where i'm, not where i'm not");
      }
      if (startPosition["inPowerArea"] == true){
       inPowerArea = true;
       currentLine = powerAreaLine;
       currentFloor = powerAreaFloor;
       currentXPos = xPosToPower;
       request->send(200, "text/plain", "I start in power area");
      }
      if (startPosition["inLoadPotArea"] == true){
       inLoadPotArea = true;
       currentLine = loadPotAreaLine;
       currentFloor = loadPotAreaFloor;
       currentXPos = xPosToLoadPot;
       request->send(200, "text/plain", "I start in load pot area");
      }
      if (startPosition["inDropArea"] == true){
       inDropArea = true;
       currentLine = dropAreaLine;
       currentFloor = dropAreaFloor;
       currentXPos = xPosToDrop;
       request->send(200, "text/plain", "I start in drop area");
      }
      if (startPosition["inGrowArea"] == true){
       if(!startPosition.containsKey("line") || !startPosition.containsKey("floor")){
        request->send(200, "text/plain", "You must provide to me a line and a floor of where you put me");
       }else{
        currentXPos = distBtElevatorX;
        currentLine = startPosition["line"].as<signed int>();
        currentFloor = startPosition["floor"].as<signed int>();
        String currentLineString = (String) currentLine;
        request->send(200, "text/plain", "I start in grow area at line" + currentLineString + "floor" + currentFloor); 
       }
      }
      currentCPos = openPosC;
      saveCurrentPos();
      haveCurrentPosRecorded = true;
    }
  }
}


void forcedNewTask(AsyncWebServerRequest *request, uint8_t *datas) {

  Serial.printf("[REQUEST]\t%s\r\n", (const char*)datas);
  DynamicJsonBuffer jsonBuffer;
  JsonObject& task = jsonBuffer.parseObject((const char*)datas); 
  if (!task.success()){
    request->send(200, "text/plain", "No JSON receive");
  }else{
    if (!task.containsKey("kindTask")){
      request->send(200, "text/plain", "No key kindTask in JSON receive");
    }
    if (!task.containsKey("line")){
      request->send(200, "text/plain", "No key line in JSON receive");
    }
    if (!task.containsKey("floor")){
      request->send(200, "text/plain", "No key floor in JSON receive");
    }
    if (!task.containsKey("x")){
      request->send(200, "text/plain", "No key x in JSON receive");
    }
    if (!task.containsKey("y")){
      request->send(200, "text/plain", "No key y in JSON receive");
    }
    if (!task.containsKey("z")){
      request->send(200, "text/plain", "No key z in JSON receive");
    }
    if (!task.containsKey("diam")){
      request->send(200, "text/plain", "No key diam in JSON receive");
    }
    if (!task.containsKey("elevator")){
      request->send(200, "text/plain", "No key elevator in JSON receive");
    }
    Serial.println(request->client()->remoteIP());
    task.set("from",request->client()->remoteIP().toString());
    from = request->client()->remoteIP().toString();
    saveTaskWithJson(task);
    haveTaskRecorded = true;
    request->send(200, "text/plain", "This task is record");
   }
}

void newTask(AsyncWebServerRequest *request, uint8_t *datas) {

  Serial.printf("[REQUEST]\t%s\r\n", (const char*)datas);
  if(haveTaskRecorded == true){
    request->send(200, "text/plain", "i'm already busy, try later");
  }
  DynamicJsonBuffer jsonBuffer;
  JsonObject& task = jsonBuffer.parseObject((const char*)datas); 
  if (!task.success()){
    request->send(200, "text/plain", "No JSON receive");
  }else{
    if (!task.containsKey("kindTask")){
      request->send(200, "text/plain", "No key kindTask in JSON receive");
    }
    if (!task.containsKey("line")){
      request->send(200, "text/plain", "No key line in JSON receive");
    }
    if (!task.containsKey("floor")){
      request->send(200, "text/plain", "No key floor in JSON receive");
    }
    if (!task.containsKey("x")){
      request->send(200, "text/plain", "No key x in JSON receive");
    }
    if (!task.containsKey("y")){
      request->send(200, "text/plain", "No key y in JSON receive");
    }
    if (!task.containsKey("z")){
      request->send(200, "text/plain", "No key z in JSON receive");
    }
    if (!task.containsKey("diam")){
      request->send(200, "text/plain", "No key diam in JSON receive");
    }
    if (!task.containsKey("elevator")){
      request->send(200, "text/plain", "No key elevator in JSON receive");
    }
    Serial.println(request->client()->remoteIP());
    task.set("from",request->client()->remoteIP().toString());
    saveTaskWithJson(task);
    haveTaskRecorded = true;
    request->send(200, "text/plain", "This task is record");
    loadTask();
   }
}

// Call an elevator
void callElevator(){
  Serial.println();
  Serial.println("call elevator");
  Serial.println(line);
  Serial.println(stage);
  String url = elevator;
  url += "/call?";
  url += "callFromLine=";
  url += currentLine;
  url += "&callFromFloor=";
  url += currentFloor;
  url += "&goToLine=";
  if(inGrowArea == true && done == true){
   url += powerAreaLine;
   url += "&goToFloor=";
   url += powerAreaFloor;
   goToElevator();
  }
  if(inGrowArea == true && done == false && kindTask == "sow"){ 
   url += loadPotAreaLine;
   url += "&goToFloor=";
   url += loadPotAreaFloor;
   //goToElevator();
  }
  if(inGrowArea == true && done == false && kindTask == "harvest" && currentCPos == openPosC ){ 
   if(currentLine != line && currentFloor != stage){
   url += String(stage);
   url += "&goToFloor=";
   url += String(stage);
   goToElevator();
   }else{
    takeInGrowArea();
    }
  }
  if(inGrowArea == true && done == false && kindTask == "harvest" && currentCPos != openPosC){ 
   url += dropAreaLine;
   url += "&goToFloor=";
   url += dropAreaFloor;
   potPicked = true;
   goToElevator();
  }
  if(inLoadPotArea == true){
   url += String(line);
   url += "&goToFloor=";
   url += String(stage); 
  }
  if(inDropArea == true){
   url += powerAreaLine;
   url += "&goToFloor=";
   url += powerAreaFloor;
   goToElevator();
  }
  if(inPowerArea == true && done == false && kindTask == "sow"){
   url += loadPotAreaLine;
   url += "&goToFloor=";
   url += loadPotAreaFloor;
  }
  if(inPowerArea == true && done == false && kindTask == "harvest"){
   url += line;
   url += "&goToFloor=";
   url += stage;
  }
  Serial.println();
  Serial.println(url);
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  if(httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.println();
    Serial.printf("[HTTP] GET... code: %d\n" , httpCode);
    // file found at server
    if(httpCode == HTTP_CODE_OK) {
     String payload = http.getString();
     Serial.println();
     Serial.println(payload);
    }
   }else{
    Serial.println();
    Serial.printf("[HTTP] GET... failed, error: %s\n" , http.errorToString(httpCode).c_str());
    }  
}

// Go to elevator waiting possition
 void goToElevator(){
  if(inPowerArea == true || inDropArea == true || inLoadPotArea == true){
   Serial.println();
   Serial.println("I'm moving to ");
   Serial.println(currentXPos/distPerStepX,15);
   Serial.println("steps");
   Serial.println("soit ");
   Serial.println(currentXPos);
   Serial.println("mm");
   //myXStepper.step(currentXPos/distPerStepX);
   currentXPos = distBtElevatorX;
  }else{
   Serial.println();
   Serial.println("I'm moving to ");
   Serial.println(-currentXPos/distPerStepX,15);
   Serial.println("steps");
   Serial.println("soit ");
   Serial.println(-currentXPos);
   Serial.println("mm");
   //myXStepper.step(-currentXPos/distPerStepX);
   currentXPos = distBtElevatorX;
  }
  int i=0;
  while(goInReceive != true){
   if(i<1){
    Serial.println();
    Serial.println("i wait elevator");
    Serial.println();
   }
   i = i+1;
   yield();
  }
  goIn();
 }

// Elevator give signal to the bridge to continue motion
 void goIn() {
  // motor clokwise
  if(inPowerArea == true || inDropArea == true || inLoadPotArea == true){
   Serial.println();
   Serial.println("I'm moving to ");
   Serial.println(currentXPos/distPerStepX,15);
   Serial.println("steps");
   Serial.println("soit ");
   Serial.println(currentXPos);
   Serial.println("mm");
   //myXStepper.step(currentXPos/distPerStepX);
  }else{
   Serial.println();
   Serial.println("I'm moving to ");
   Serial.println(currentXPos/distPerStepX,15);
   Serial.println("steps");
   Serial.println("soit ");
   Serial.println(currentXPos);
   Serial.println("mm");
   Serial.print("soit " + (-currentXPos));
   //myXStepper.step(-currentXPos/distPerStepX);
  }
  goInReceive = false;
  inLoadPotArea = false;
  inGrowArea = false;
  inPowerArea = false;
  inDropArea = false;
  currentXPos = 0;
  int i=0;
  while(goOutReceive != true){
   if(i<1){
    Serial.println();
    Serial.println("I'm in the elevator, tell me when we arrived");
    Serial.println();
   }
   i = i+1;
   yield();
  }
  goOut();
 }
 
//Receive "arrived" signal from load area
 void loadPot(AsyncWebServerRequest *request){
  // put convertion bt diam and angle of motor
  // move z
  // move y
  currentCPos = 18;
  callElevator();
  goToElevator();
  }

//Send who load pot to Pi
 void sendWhoLoadPot(){
  JsonObject& task = loadTask();
  String url = task["from"];
  url += "/loadPot";
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  String taskString;
  task.printTo(taskString);
  http.POST(taskString);
  http.writeToStream(&Serial);
  http.end();
  int httpCode = http.GET();
  if(httpCode > 0) {
   // HTTP header has been send and Server response header has been handled
   Serial.printf("[HTTP] GET... code: %d\n" , httpCode);
   // file found at server
   if(httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println(payload);
   }
  }else{
    Serial.printf("[HTTP] GET... failed, error: %s\n" , http.errorToString(httpCode).c_str());
   }
 }
//Cycle of drop pot
 void dropInGrowArea(){
  Serial.println();
  Serial.println("I'm moving to ");
  Serial.println(x/distPerStepX,15);
  Serial.println("steps");
  Serial.println("soit ");
  Serial.println(x);
  Serial.println("mm");
  myXStepper.step(x/distPerStepX);
  currentXPos = x; 
  //myYStepper.step(task["y"].as<signed int>()/distPerStepY);
  //myZStepper.step(task["z"].as<signed int>()/distPerStepZ);
  //myCStepper.step(openPosC);
  currentCPos = openPosC;
  //myZStepper.step(-task["z"].as<signed int>()/distPerStepZ);
  //myYStepper.step(-task["y"].as<signed int>()/distPerStepY);
  done = true;
  saveTask();
  sendTask();
  //delay to let pi send a new task
//  unsigned long interval=1000; // the time we need to wait
//  unsigned long previousMillis=0;
//  unsigned long currentMillis = millis();
//  if ((unsigned long)(currentMillis - previousMillis) >= interval) {
//   callElevator();
//   previousMillis = millis();
//  }
  callElevator();
 }

//Load pot in load pot area
 void loadInLoadPotArea(){
  Serial.println();
  Serial.println("I'm moving to ");
  Serial.println(-xPosToLoadPot/distPerStepX,15);
  Serial.println("steps");
  Serial.println("soit ");
  Serial.println(-xPosToLoadPot);
  Serial.println("mm");
  myXStepper.step(-xPosToLoadPot);
  currentXPos = -xPosToLoadPot;
  sendWhoLoadPot();
 }

//Cycle of harvest
 void takeInGrowArea(){
  //JsonObject& task = loadTask();
  Serial.println();
  Serial.println("I'm moving to ");
  Serial.println(x/distPerStepX,15);
  Serial.println("steps");
  Serial.println("soit ");
  Serial.println(x);
  Serial.println("mm");
  //myXStepper.step(task["x"].as<signed int>()/distPerStepX);
  //myXStepper.step(30/distPerStepX);
  currentXPos = x;
  //myCStepper.step(openPosC); 
  //myYStepper.step(task["y"].as<signed int>()/distPerStepY);
  //myZStepper.step(task["z"].as<signed int>()/distPerStepZ);
  //myXStepper.step(30/distPerStepX);
  //myCStepper.step(18); // put function to convert diam to degree
  currentCPos = 18;
  //myZStepper.step(-task["z"].as<signed int>()/distPerStepZ);
  //myYStepper.step(-task["y"].as<signed int>()/distPerStepY);
  //callElevator();
  }

//Power in power area
void powerInPowerArea(){
 Serial.println();
 Serial.println("I'm moving to ");
 Serial.println(-xPosToPower/distPerStepX,15);
 Serial.println("steps");
 Serial.println("soit ");
 Serial.println(-xPosToPower);
 Serial.println("mm");
 myXStepper.step(-xPosToPower/distPerStepX);
 currentXPos = -xPosToPower;
 SPIFFS.remove("/task.json");
 //to testing
 JsonObject& task = loadTask();
 haveTaskRecorded = false;
}

//Drop harvest vegetable in drop area
void dropInDropArea(){
  //JsonObject& task = loadTask();
  Serial.println();
  Serial.println("I'm moving to drop position");
  Serial.println(-xPosToDrop/distPerStepX,15);
  Serial.println("steps");
  Serial.println("soit ");
  Serial.println(-xPosToDrop);
  Serial.println("mm");
  myXStepper.step(-xPosToDrop/distPerStepX);
  currentXPos = -xPosToDrop; 
  //myYStepper.step(yPosToDrop/distPerStepY);
  //myZStepper.step(zPosToDrop/distPerStepZ);
  //myCStepper.step(openPosC);
  currentCPos = openPosC;
  //myZStepper.step(zPosToDrop/distPerStepZ);
  //myYStepper.step(yPosToDrop/distPerStepY);
  done = true;
  saveTask();
  sendTask();
  //delay to let pi send a new task
//  unsigned long interval=1000; // the time we need to wait
//  unsigned long previousMillis=0;
//  unsigned long currentMillis = millis();
//  if ((unsigned long)(currentMillis - previousMillis) >= interval) {
//   callElevator();
//   previousMillis = millis();
//  }
  callElevator();
 }
  
//Receive "arrived" signal from elevator
 int goOut(){
  goOutReceive = false;
//   inLoadPotArea = false;
//   inGrowArea = false;
//   inPowerArea = false;
//   inDropArea = false;
   if(done == false && kindTask == "sow" && openPosC == currentCPos){
    Serial.println();
    Serial.println("I'm moving to in load pot area ");
    Serial.println(-distBtElevatorX/distPerStepX,15);
    Serial.println("steps");
    Serial.println("soit ");
    Serial.println(-distBtElevatorX);
    Serial.println("mm");
    myXStepper.step(-distBtElevatorX/distPerStepX);
    inLoadPotArea = true;
    currentLine = loadPotAreaLine;
    currentFloor = loadPotAreaFloor;
    loadInLoadPotArea();
   }
   if(done == false && kindTask == "sow" && openPosC != currentCPos){
    Serial.println();
    Serial.println("I'm moving to grow area ");
    Serial.println(distBtElevatorX/distPerStepX,15);
    Serial.println("steps");
    Serial.println("soit ");
    Serial.println(distBtElevatorX);
    Serial.println("mm");
    myXStepper.step(distBtElevatorX/distPerStepX);
    inGrowArea = true;
    currentLine = line;
    currentFloor = stage;
    dropInGrowArea();
   }
   if(done == false && kindTask == "harvest" && openPosC == currentCPos){
    currentLine = line;
    currentFloor = stage;
    int xdd = round(distBtElevatorX/distPerStepX);
    myXStepper.step(xdd);
    Serial.println();
    Serial.println("I'm moving to grow area ");
    Serial.println(xdd);//distBtElevatorX/distPerStepX,15);
    Serial.println("steps");
    Serial.println("soit ");
    Serial.println(distBtElevatorX);
    Serial.println("mm");
    inGrowArea = true;
    takeInGrowArea();
    return 0;
   }
   if(done == false && kindTask == "harvest" && openPosC != currentCPos){
    Serial.println();
    Serial.println("I'm moving to drop area ");
    Serial.println(distBtElevatorX/distPerStepX,15);
    Serial.println("steps");
    Serial.println("soit ");
    Serial.println(distBtElevatorX);
    Serial.println("mm");
    myXStepper.step(distBtElevatorX/distPerStepX);
    inDropArea = true;
    currentLine = dropAreaLine;
    currentFloor = dropAreaFloor;
    dropInDropArea();
   }
   if(done == true){
    Serial.println();
    Serial.println("I'm moving to power area ");
    Serial.println(distBtElevatorX/distPerStepX,15);
    Serial.println("steps");
    Serial.println("soit ");
    Serial.println(distBtElevatorX);
    Serial.println("mm");
    myXStepper.step(distBtElevatorX/distPerRevX*stepsPerRevolutionX);
    inPowerArea = true;
    currentLine = powerAreaLine;
    currentFloor = powerAreaFloor;
    powerInPowerArea();
   }
 }

 //Give order to the brige to begin working
 void goOn(){
  goOnReceive = false;
  if(haveTaskRecorded == true){
   int i=0;
   if(haveCurrentPosRecorded == false){     
    //go to y origine
    //go to z origine
    Serial.println();
    Serial.println("i don't know where i am. Please Use url /setPosition to provide me a JSON");
   }else{
     callElevator();
//     unsigned long interval=1000; // the time we need to wait
//   unsigned long previousMillis=0;
//   unsigned long currentMillis = millis();
//   if ((unsigned long)(currentMillis - previousMillis) >= interval) {
     goToElevator();
//    previousMillis = millis();
    }
  }else{
    Serial.println();
    Serial.println("i don't have work to do. Please Use url /newTask to provide me a JSON");
  }
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

   //NTP.begin("pool.ntp.org", 1, true);
   //NTP.setInterval(60);
   //Serial.println(NTP.getTime());
   //NTP.onNTPSyncEvent([](NTPSyncEvent_t error) {
   // if (error) {
   //  Serial.print("Time Sync error: ");
   //   if (error == noResponse)
   //     Serial.println("NTP server not reachable");
   //   else if (error == invalidAddress)
   //     Serial.println("Invalid NTP server address");
   // }
   // else {
   //   Serial.print("Got NTP time: ");
   //   Serial.println(NTP.getTimeDateString(NTP.getLastNTPSync()));
   // }
  //});

   if (!SPIFFS.begin()){
     Serial.println("SPIFFS Mount failed");
    } else {
     Serial.println("SPIFFS Mount succesfull");
     loadTask();
     loadCurrentPos();
    }


  server.on("/goIn", HTTP_GET, [](AsyncWebServerRequest *request){
   request->send(200, "text/plain", "go In");
   goInReceive = true;
  });

  server.on("/goOut", HTTP_GET, [](AsyncWebServerRequest *request){
   request->send(200, "text/plain", "go out");
   goOutReceive = true;
  });

  server.on("/deleteCurrentPos", HTTP_GET, [](AsyncWebServerRequest *request){
   request->send(200, "text/plain", "record Current position deleted");
   deleteCurrentPos();
  });

  server.on("/deleteTask", HTTP_GET, [](AsyncWebServerRequest *request){
   request->send(200, "text/plain", "record Task deleted");
   deleteTask();
  });

//Use this url to start the bridge bot
  server.on("/goOn", HTTP_GET, [](AsyncWebServerRequest *request){
   request->send(200, "text/plain", "go on");
   goOnReceive = true;
  });
    
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
   if (request->url() == "/setPosition") {
    setPosition(request, data);
   }
   if (request->url() == "/newTask") {
    newTask(request, data);
   }
   if (request->url() == "/forcedNewTask") {
    forcedNewTask(request, data);
   }
   if (request->url() == "/loadPot") {
    loadPot(request);
   }
  });


   
   server.serveStatic("/js", SPIFFS, "/js");
   server.serveStatic("/css", SPIFFS, "/css");
   server.serveStatic("/img", SPIFFS, "/img");
   server.serveStatic("/", SPIFFS, "/index.html");
 
   server.begin();
   Serial.println();
   Serial.println ( "HTTP server started" );
   myXStepper.setSpeed(12);
   Serial.println();
   Serial.println ("Use url /goOn to give me the order to work");
   
 }
                                              //VOID LOOP

                                              
 void loop() {
  if(goOnReceive == true){
   Serial.println();
   Serial.println("I go to work");
   Serial.println();
   goOn();
  }
 }
