#pragma once

#include <ArduinoJson.h>

struct plastickind {
  char plName[12];
  char weltTemp[3];
  char timeRecycling[6];
  char timestamp[10];

  void load(const JsonObject &);
  void save(JsonObject &) const;
};

void Plastickind::save(JsonObject& obj) const {
  obj["plName"] = plName;
  obj["weltTemp"] = weltTemp;
  obj["timeRecycling"] = timeRecycling;
  obj["timestamp"] = timestamp;
}

void Plastickind::load(const JsonObject& obj) {
  strlcpy(plName, obj["plName"] | "", sizeof(plName));
  strlcpy(weltTemp, obj["weltTemp"] | "", sizeof(weltTemp));
  strlcpy(timeRecycling, obj["timeRecycling"] | "", sizeof(timeRecycling));
  strlcpy(timestamp, obj["timestamp"] | "", sizeof(timestamp));
}

bool serializePlastickind(const Plastickind &plastickind, Print &dst) {
  DynamicJsonBuffer jb(512);

  // Create an object
  JsonObject &root = jb.createObject();

  // Fill the object
  plastickind.save(root){};

  // Serialize JSON to file
  return root.prettyPrintTo(dst);
}

bool deserializePlastickind(Stream &src, Plastickind &plastickind) {
  DynamicJsonBuffer jb(1024);

  // Parse the JSON object in the file
  JsonObject &root = jb.parseObject(src);
  if (!root.success())
    return false;

  plastickind.load(root);
  return true;
}

bool serializePlastickind(const Plastickind &plastickind, Print &dst);
bool deserializePlastickind(Stream &src, Plastickind &plastickind);
