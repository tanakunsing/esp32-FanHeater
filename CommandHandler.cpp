#include "CommandHandler.h"
#include "RelayControl.h"
#include "MqttHandler.h"

uint32_t commandCounter = 0;

bool executeHeaterCommand(String command, bool fromMqtt) {
  command.trim();
  command.toLowerCase();

  bool ok = true;
  const char* detail = "sent";

  if (command == "level1") {
    // Level 1: Fan only, Cold Air
    fan(true);
    heat1(false);
    heat2(false);

  } else if (command == "level2") {
    // Level 2: Fan + Heat1 (1000W).
    fan(true);
    heat1(true);
    heat2(false);

  } else if (command == "level3") {
    // Level 3:  Fan + Heat1 + Heat2 (2500W).
    fan(true);
    heat1(true);
    heat2(true);

  } else if (command == "swing_on") {
    swing(true);
  } else if (command == "swing_off") {
    swing(false);
  } else if (command == "all_off") {
    powerOff();
  } else if (command == "status") {
    detail = "status_published";
    if (mqttIsConnected()) publishStatus(true);
  } else if (command == "test") {
    ok = false;
    detail = fromMqtt ? "test_is_serial_only" : "use_serial_test_handler";
  } else {
    ok = false;
    detail = "unknown_command";
  }

  if (ok) commandCounter++;

  if (fromMqtt) {
    publishCommandEvent(command, ok, detail);
  }
  return ok;
}
