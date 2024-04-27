#include <LiquidCrystal.h>

bool simulation = true;

// Voltage and Current Sensor Analog Pins
const int V_PIN = A4;
const int I_PIN = A0;
// Button Pin
const int BUTTON_PIN = 7;
// LCD pins and initialization
const int rs = 13, en = 12, d4 = 11, d5 = 10, d6 = 9, d7 = 8;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
// Button Variables
byte pressed = 0;
int presses = 0;

// Sensor Readings
double vCurrentAnalogValue=0;
double iCurrentAnalogValue=0;

// For Power Calculations
const long int processingRange = 50;
double Vsquared = 0;
double Isquared = 0;
double VIproduct = 0;
long int numSamples = 0;
double startPeriod = 0;
double endPeriod = 0;
double Energy = 0;

// For Voltage Sensor
double voltageSensorOffset = 0;
const double Voltage_Scaling_Factor = simulation ? 1.247 : 2.136;
double vMax = 0;
double vMaxTime = 0;

// For Current Sensor
double currentSensorOffset = 0;
const double Current_Scaling_Factor = 5000.0 / (66.0 * 1024.0);
double iMax = 0;
double iMaxTime = 0;

void calibrateOffset() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibrating!");

  double offset_V = 0;
  double offset_I = 0;
  
  int i = 0;
  int start = millis();
  while (true) {
    int now = millis();
    double reading_V = analogRead(V_PIN);
    double reading_I = analogRead(I_PIN);
    
    if (now - start >= 1000) {
      break;
    } else {
      i++;
      offset_V += reading_V;
      offset_I += reading_I;
    }
  }

  voltageSensorOffset = offset_V / i;
  currentSensorOffset = offset_I / i;

  Serial.print("V Offset = ");
  Serial.print(voltageSensorOffset);
  Serial.println();
  Serial.print("I Offset = ");
  Serial.print(currentSensorOffset);
  Serial.println();

  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("V Offset = ");
  lcd.print(voltageSensorOffset);
  lcd.setCursor(0, 1);
  lcd.print("I Offset = ");
  lcd.print(currentSensorOffset);
  delay(2000);
  startPeriod = 0;
}

void buttonCallback() {
  calibrateOffset();  
}

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(V_PIN, INPUT);
  pinMode(I_PIN, INPUT);
  
  calibrateOffset();
}

void loop() {
  // Button Logic
  byte buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == 1 && pressed == 0)
    pressed = 1;
  if (buttonState == 0 && pressed == 1) {
    pressed = 0;
    // Do Something for pressing this button
    buttonCallback();
  }

  // Get New Voltage and Current Values from Sensor
  vCurrentAnalogValue = analogRead(V_PIN);
  iCurrentAnalogValue = analogRead(I_PIN);
  
  vCurrentAnalogValue = (vCurrentAnalogValue - voltageSensorOffset) * Voltage_Scaling_Factor;
  iCurrentAnalogValue = (iCurrentAnalogValue - currentSensorOffset) * Current_Scaling_Factor;

  if (endPeriod - startPeriod <= 20000) {
    if (vCurrentAnalogValue >= vMax) {
      vMax = vCurrentAnalogValue;
      vMaxTime = micros();
    }
  
    if (iCurrentAnalogValue >= iMax) {
      iMax = iCurrentAnalogValue;
      iMaxTime = micros();
    }
  }

  // Test Current RMS
  if (startPeriod == 0) {
    startPeriod = micros();
    endPeriod = startPeriod;
    Isquared = 0;
    Vsquared = 0;
    VIproduct = 0;
    numSamples = 0;
  } else if (endPeriod - startPeriod >= processingRange * 20000) {
    double Irms = sqrt(Isquared / numSamples);
    double Vrms = sqrt(Vsquared / numSamples);
    double Power = VIproduct / numSamples;
    double phi = (iMaxTime - vMaxTime) / 10000 * 2 * 3.1415926 * 50 - acos(0.886);
    double PF = phi > 0 ? cos(acos(Power / (Vrms * Irms)) - acos(0.886)) : cos(- acos(Power / (Vrms * Irms)) - acos(0.886));
    Power = Vrms * Irms * PF;
    
    Serial.print("Irms = ");
    Serial.print(Irms);
    Serial.println();
    Serial.print("Vrms = ");
    Serial.print(Vrms);
    Serial.println();
    Serial.print("Power = ");
    Serial.print(Power);
    Serial.println();
    Serial.print("PF = ");
    Serial.print(PF);
    Serial.println();
    Serial.print("Numsamples = ");
    Serial.print(numSamples);
    Serial.println();
    Serial.print("Phase Shift = ");
    Serial.print(phi);
    Serial.println();
    delay(2000);

    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Vrms = ");
    lcd.print(Vrms, 3);
    lcd.setCursor(0, 1);
    lcd.print("Irms = ");
    lcd.print(Irms, 3);
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Power = ");
    lcd.print(Power, 3);
    lcd.setCursor(0, 1);
    lcd.print("PF = ");
    lcd.print(PF, 3);
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Energy = ");
    lcd.print(Energy, 3);
    delay(2000);

    Isquared = 0;
    Vsquared = 0;
    VIproduct = 0;
    numSamples = 0;
    vMax = 0;
    iMax = 0;
    endPeriod = micros();
    Energy += Power < 0.01 ? 0 : Power * (endPeriod - startPeriod) * 2.7778e-10;
    startPeriod = micros();
    endPeriod = startPeriod;
  } else {
    endPeriod = micros();
    numSamples++;
    Isquared += iCurrentAnalogValue * iCurrentAnalogValue;
    Vsquared += vCurrentAnalogValue * vCurrentAnalogValue;
    VIproduct += vCurrentAnalogValue * iCurrentAnalogValue;
  }
}
