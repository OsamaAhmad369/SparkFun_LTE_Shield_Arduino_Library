#include <SparkFun_LTE_Shield_Arduino_Library.h>
#include <SoftwareSerial.h>

#define _rxPin      14
#define _txPin      13
#define POWER_PIN   27
#define RESET_PIN   26
#define baud        9600
#define RTS_S       12
SoftwareSerial mySerial;
LTE_Shield lte(POWER_PIN, RESET_PIN,_txPin,_rxPin ,0); //Set last parameter to 1 if your shield  power pin and reset pin are active high 
// Network operator can be set to either:
// MNO_SW_DEFAULT -- DEFAULT
// MNO_ATT -- AT&T 
// MNO_VERIZON -- Verizon
// MNO_TELSTRA -- Telstra
// MNO_TMO -- T-Mobile
const mobile_network_operator_t MOBILE_NETWORK_OPERATOR = MNO_SW_DEFAULT;
const String MOBILE_NETWORK_STRINGS[] = {"Default", "SIM_ICCD", "AT&T", "VERIZON", 
  "TELSTRA", "T-Mobile", "CT"};

// APN -- Access Point Name. Gateway between GPRS MNO
// and another computer network. E.g. "hologram
const String APN = "hologram";

// This defines the size of the ops struct array. Be careful making
// this much bigger than ~5 on an Arduino Uno. To narrow the operator
// list, set MOBILE_NETWORK_OPERATOR to AT&T, Verizeon etc. instead
// of MNO_SW_DEFAULT.
#define MAX_OPERATORS 5

#define DEBUG_PASSTHROUGH_ENABLED

void setup() {
  int opsAvailable;
  struct operator_stats ops[MAX_OPERATORS];
  String currentOperator = "";
  bool newConnection = true;
  pinMode(RTS_S,OUTPUT);
  digitalWrite(RTS_S,LOW);
  Serial.begin(9600);
  while (!Serial) ; // For boards with built-in USB
  Serial.println(F("Code Version_1.4.1"));
  Serial.println(F("Initializing the LTE Shield..."));
  Serial.println(F("...this may take ~25 seconds if the shield is off."));
  Serial.println(F("...it may take ~5 seconds if it just turned on."));

  if ( lte.begin(mySerial,baud) ) {
    Serial.println(F("LTE Shield connected!\r\n"));
  } else {
    Serial.println("Unable to initialize the shield.");
    while(1) ;
  }

  // First check to see if we're already connected to an operator:
  if (lte.getOperator(&currentOperator) == LTE_SHIELD_SUCCESS) {
    Serial.print(F("Already connected to: "));
    Serial.println(currentOperator);
    // If already connected provide the option to type y to connect to new operator
    Serial.println(F("Press y to connect to a new operator, or any other key to continue.\r\n"));
    while (!Serial.available()) ;
    if (Serial.read() != 'y') {
      newConnection = false;
    }
    while (Serial.available()) Serial.read();
  }

  if (newConnection) {
    // Set MNO to either Verizon, T-Mobile, AT&T, Telstra, etc.
    // This will narrow the operator options during our scan later
    Serial.println(F("Setting mobile-network operator"));
    if (lte.setNetwork(MOBILE_NETWORK_OPERATOR)) {
      Serial.print(F("Set mobile network operator to "));
      Serial.println(MOBILE_NETWORK_STRINGS[MOBILE_NETWORK_OPERATOR] + "\r\n");
    } else {
      Serial.println(F("Error setting MNO. Try cycling power to the shield/Arduino."));
      while (1) ;
    }
    
    // Set the APN -- Access Point Name -- e.g. "hologram"
    Serial.println(F("Setting APN..."));
    if (lte.setAPN(APN) == LTE_SHIELD_SUCCESS) {
      Serial.println(F("APN successfully set.\r\n"));
    } else {
      Serial.println(F("Error setting APN. Try cycling power to the shield/Arduino."));
      while (1) ;
    }

    // Wait for user to press button before initiating network scan.
    Serial.println(F("Press any key scan for networks.."));
    serialWait();

    Serial.println(F("Scanning for operators...this may take up to 3 minutes\r\n"));
    // lte.getOperators takes in a operator_stats struct pointer and max number of
    // structs to scan for, then fills up those objects with operator names and numbers
    opsAvailable = lte.getOperators(ops, MAX_OPERATORS); // This will block for up to 3 minutes

    if (opsAvailable > 0) {
      // Pretty-print operators we found:
      Serial.println("Found " + String(opsAvailable) + " operators:");
      printOperators(ops, opsAvailable);

      // Wait until the user presses a key to initiate an operator connection
      Serial.println("Press 1-" + String(opsAvailable) + " to select an operator.");
      char c = 0;
      bool selected = false;
      while (!selected) {
        while (!Serial.available()) ;
        c = Serial.read();
        int selection = c - '0';
        if ((selection >= 1) && (selection <= opsAvailable)) {
          selected = true;
          Serial.println("Connecting to option " + String(selection));
          if (lte.registerOperator(ops[selection - 1]) == LTE_SHIELD_SUCCESS) {
            Serial.println("Network " + ops[selection - 1].longOp + " registered\r\n");
          } else {
            Serial.println(F("Error connecting to operator. Reset and try again, or try another network."));
          }
        }
      }
    } else {
      Serial.println(F("Did not find an operator. Double-check SIM and antenna, reset and try again, or try another network."));
      while (1) ;
    }
  }

  // At the very end print connection information
  printInfo();
}

void loop() {
  // Loop won't do much besides provide a debugging interface.
  // Pass serial data from Arduino to shield and vice-versa
#ifdef DEBUG_PASSTHROUGH_ENABLED
  if (mySerial.available()) {
    Serial.write((char) mySerial.read());
  }
  if (Serial.available()) {
    mySerial.write((char) Serial.read());
  }
#endif
}

void printInfo(void) {
  String currentApn = "";
  IPAddress ip(0, 0, 0, 0);
  String currentOperator = "";

  Serial.println(F("Connection info:"));
  // APN Connection info: APN name and IP
  if (lte.getAPN(&currentApn, &ip) == LTE_SHIELD_SUCCESS) {
    Serial.println("APN: " + String(currentApn));
    Serial.print("IP: ");
    Serial.println(ip);
  }

  // Operator name or number
  if (lte.getOperator(&currentOperator) == LTE_SHIELD_SUCCESS) {
    Serial.print(F("Operator: "));
    Serial.println(currentOperator);
  }

  // Received signal strength
  Serial.println("RSSI: " + String(lte.rssi()));
  Serial.println();
}

void printOperators(struct operator_stats * ops, int operatorsAvailable) {
  for (int i = 0; i < operatorsAvailable; i++) {
    Serial.print(String(i + 1) + ": ");
    Serial.print(ops[i].longOp + " (" + String(ops[i].numOp) + ") - ");
    switch (ops[i].stat) {
    case 0:
      Serial.println(F("UNKNOWN"));
      break;
    case 1:
      Serial.println(F("AVAILABLE"));
      break;
    case 2:
      Serial.println(F("CURRENT"));
      break;
    case 3:
      Serial.println(F("FORBIDDEN"));
      break;
    }
  }
  Serial.println();
}

void serialWait() {
  while (!Serial.available()) ;
  while (Serial.available()) Serial.read();
}
