#include "CommandHandler.h"
#include "RelayControl.h"

uint32_t commandCounter = 0;

// Sets all 4 relays from one JSON state object. Called by WebPortal.
//
// Two hardware rules enforced regardless of what the JSON asked for:
//   1. power=false is a hard safety gate: everything is forced off, since
//      heat must never run without the fan.
//   2. heat2 requires heat1 (matches level3 = Fan+Heat1+Heat2 together —
//      there is no "heat2 alone" state), so heat2=on forces heat1=on even
//      if heat1 was omitted or explicitly "off".
void applyRelayState(bool power, bool wantHeat1, bool wantHeat2, bool wantSwing) {
  if (!power) {
    powerOff();
    Serial.println(F("[STATE] power=off -> everything off"));
    commandCounter++;
    return;
  }

  const bool heat1Final = wantHeat1 || wantHeat2;

  fan(true);
  heat1(heat1Final);
  heat2(wantHeat2);
  swing(wantSwing);
  commandCounter++;
}
