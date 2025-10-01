#include "default_script.h"

const char DEFAULT_SCRIPT[] PROGMEM = R"json(
{
  "name": "hello",
  "mode": "relative",
  "reference": { "w": 1080, "h": 2400 },
  "repeat": 1,
  "steps": [
    { "wait": { "ms": 500 } },
    { "type": { "text": "Hello_from_ESP32" } },
    { "key":  { "code": "ENTER" } }
  ]
}
)json";
