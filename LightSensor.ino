// Define Blynk Template ID and Name (from your Blynk Console)
#define BLYNK_TEMPLATE_ID "TMPL67HU7CaMa"
#define BLYNK_TEMPLATE_NAME "Light Sensor Crop Protection System"

// Include necessary libraries
#include <ESP8266WiFi.h>          // For WiFi connectivity
#include <BlynkSimpleEsp8266.h>   // For Blynk communication
#include <Servo.h>                // For servo motor control

// WiFi credentials
char ssid[] = "GAU GAU";
char pass[] = "123456799";

// Blynk Auth Token (from your Blynk Console)
char auth[] = "LCC3fDHpK1SzrCkdBzPQ9sTlVuCCrmSH";

// Pin definitions
const int ldrPin = A0;      // LDR connected to A0 (Analog input)
const int servoPin = D4;    // Servo signal pin connected to D4 (PWM)

// Servo object
Servo servo;

// Variables
float ThresholdVoltage = 3.0;  // Default threshold voltage (adjustable via Blynk)
bool isAutoMode = true;        // Default to auto mode (true = auto, false = manual)

// Blynk virtual pin handlers
// V3: Auto/Manual mode switch (1 = auto, 0 = manual)
BLYNK_WRITE(V3) {
  isAutoMode = param.asInt();  // Update mode based on user input
  Blynk.virtualWrite(V3, isAutoMode);  // Sync the mode state to the app
}

// V4: Manual barrier control (1 = close, 0 = open)
BLYNK_WRITE(V4) {
  if (!isAutoMode) {  // Only allow manual control in manual mode
    int state = param.asInt();
    if (state == 1) {
      servo.write(180);  // Close barrier
      Blynk.virtualWrite(V2, "Closed");  // Update state on app
    } else {
      servo.write(0);    // Open barrier
      Blynk.virtualWrite(V2, "Open");    // Update state on app
    }
  }
}

// V5: Threshold voltage adjustment (0–5V)
BLYNK_WRITE(V5) {
  ThresholdVoltage = param.asFloat();  // Update threshold from slider
  Serial.print("New Threshold Voltage: ");
  Serial.println(ThresholdVoltage);
}

void setup() {
  // Initialize Serial Monitor for debugging
  Serial.begin(115200);

  // Attach servo to the defined pin and set initial position
  servo.attach(servoPin);
  servo.write(0);  // Start with barrier open

  // Connect to WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  // Initialize Blynk without authentication (using only auth token)
  Blynk.config(auth);
  Blynk.connect();

  // Set initial values on Blynk App
  Blynk.virtualWrite(V2, "Open");  // Initial state
  Blynk.virtualWrite(V3, 1);       // Default to auto mode
}

void loop() {
  // Run Blynk to handle communication with the app
  Blynk.run();

  // Read LDR value (0–1023) and convert to voltage (0–3.3V)
  int ldrValue = analogRead(ldrPin);
  float voltage = ldrValue * (3.3 / 1023.0);  // ESP8266 ADC range: 0–3.3V

  // Determine light level based on voltage (reversed logic for this module)
  String lightLevel;
  if (voltage < 1.5) {
    lightLevel = "High";  // High light when voltage is low (strong light)
  } else if (voltage < 3.0) {
    lightLevel = "Medium";
  } else {
    lightLevel = "Low";   // Low light when voltage is high (dark)
  }

  // Send data to Blynk App
  Blynk.virtualWrite(V0, voltage);      // V0: Voltage value
  Blynk.virtualWrite(V1, lightLevel);   // V1: Light level (Low/Medium/High)

  // Debug output to Serial Monitor
  Serial.print("Voltage: ");
  Serial.print(voltage);
  Serial.print("V, Light Level: ");
  Serial.println(lightLevel);

  // Control logic based on mode (reversed logic for this module)
  if (isAutoMode) {
    // Auto mode: Control barrier based on light threshold
    if (voltage < ThresholdVoltage) {  // Voltage low means strong light
      servo.write(180);  // Close barrier
      Blynk.virtualWrite(V2, "Closed");  // Update state on app
      Blynk.logEvent("high_light_alert", "High Light Detected! Barrier Closed.");  // Send event notification
    } else {  // Voltage high means low light
      servo.write(0);    // Open barrier
      Blynk.virtualWrite(V2, "Open");    // Update state on app
    }
  }

  // Delay for 3 seconds (3000 milliseconds) to avoid overwhelming the ESP8266
  delay(3000);
}