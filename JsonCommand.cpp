#include "JsonCommand.h"
#include <ArduinoJson.h>

static bool fieldIsOn(JsonDocument& doc, const char* key) {
  if (!doc[key].is<const char*>()) return false;
  String value = doc[key].as<const char*>();
  value.toLowerCase();
  return value == "on";
}

bool parseRelayStateJson(const String& line, bool& power, bool& heat1, bool& heat2, bool& swing) {
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, line);
  if (error) {
    Serial.print(F("[JSON] Parse error: "));
    Serial.println(error.c_str());
    return false;
  }

  power = fieldIsOn(doc, "power");
  heat1 = fieldIsOn(doc, "heat1");
  heat2 = fieldIsOn(doc, "heat2");
  swing = fieldIsOn(doc, "swing");
  return true;
}
