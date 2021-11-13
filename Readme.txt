This library will edit for ESP32. It will work only for softwareSerial library for ESP32. 
You can download SoftwareSerial library from below link. 
https://downloads.arduino.cc/libraries/github.com/plerup/EspSoftwareSerial-6.13.2.zip


I have added one more option for power and reset pin for LTE shield. For custom PCB, if the POWER and RESET pin are high enable, change the last parameter to 1.
LTE_Shield lte(POWER_PIN, RESET_PIN,_txPin,_rxPin ,1);


Note: This library will not work with HARDWARE Serial.
