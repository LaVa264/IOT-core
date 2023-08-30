#include <Arduino.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

void setup() {

    /* Configure log baudrate. */
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {

    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("LED ON"); // LED TURN ON
    delay(1000);

    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("LED OFF"); // LED TURN OFF
    delay(1000);
}
