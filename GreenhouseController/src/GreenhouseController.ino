#include <OneWire.h>

// Code to start the antenna. The ANT_AUTO argument assures that if the antenna isn't working for whatever reason,
// the photon will default to the in-built antenna. The photon essentially uses whichever antenna is receving the
// stronger WiFi signal
STARTUP(WiFi.selectAntenna(ANT_AUTO));

//Actuation pins
int valvePin = D4;

//Sensor pins
int moistureSensor1 = A0;
int moistureSensor2 = A1;
int moistureSensor3 = A2;
int moisturePower1 = D0;
int moisturePower2 = D1;
int moisturePower3 = D2;
int photoPower = D3;
int photoPin = A3;
OneWire ds = OneWire(D5);  // 1-wire signal on pin D4 for temperature sensor

//Variables for rolling avg
const int numReadings = 25;

int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
float total = 0.0;                  // the running total
float average = 0.0;

//Temperature sensor global vars
byte i;
byte present = 0;
byte type_s;
byte data[12];
byte addr[8];
float celsius, fahrenheit;

//Miscellaneous global variables
float avgMoisture;
int actuationThreshold = 2350;
int isWatering = false;
int wetThreshold = 2100;
bool isComplete;
unsigned long lastUpdate = 0;
float lastTemp;

// //Particle variables for getting each moisture sensor's reading
// int moistureVal1;
// int moistureVal2;
// int moistureVal3;



void setup() {

    Serial.begin(9600);
    Particle.function("actuate", actuate);
    Particle.function("getState", getState);
    Particle.function("setDry", setHighThreshold);
    Particle.function("setWet", setLowThreshold);
    // Particle.variable("moisture1", moistureVal1);
    // Particle.variable("moisture2", moistureVal2);
    // Particle.variable("moisture3", moistureVal3);
    setupHardware();
    timerStart();
    Particle.subscribe("moisture", actuationController);
    //Initialize rolling avg array w/ zeros
    for (int thisReading = 0; thisReading < numReadings; thisReading++) {
      readings[thisReading] = 0;
    }


}

// Event listener function. Every time there is a "moisture" event published, that data value is compared to the dry threshold.
// If it is greater than the threshold, isWatering is set to true, and the actuation system starts running
void actuationController(const char *event, const char *data){
  String reading = String(data);
  float soilValue = reading.toFloat();
  Serial.println(soilValue);
  if(soilValue >= actuationThreshold){
    Particle.publish("wateringStatus", "started");
    actuate("open");
    // Initialize the rolling average array to be all zeros, isComplete to false, and isWatering to true to start the
    // actuation controller.
    for(int i=0; i<numReadings; i++){
      readings[i] = 0;
    }
    isComplete = false;
    isWatering = true;

    total = 0;
    readIndex = 0;
  }

}

//Turn on valve
void valveOn(){
  digitalWrite(valvePin, LOW);
}

//Turn off valve
void valveOff(){
  digitalWrite(valvePin, HIGH);
}

//Close the valve and configure the pins
void setupHardware(){
  valveOff();
  pinMode(valvePin, OUTPUT);
  pinMode(moistureSensor1, INPUT);
  pinMode(moistureSensor2, INPUT);
  pinMode(moistureSensor3, INPUT);
  pinMode(moisturePower1, OUTPUT);
  pinMode(moisturePower2, OUTPUT);
  pinMode(moisturePower3, OUTPUT);
  pinMode(photoPin, INPUT);
  pinMode(photoPower, OUTPUT);
  powerSensors();

}

String getTemperature(){
  if ( !ds.search(addr)) {
    ds.reset_search();
    delay(250);
    return "";
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {

      return "";
  }

  ds.reset();               // first clear the 1-wire bus
  ds.select(addr);          // now select the device we just found
  // ds.write(0x44, 1);     // tell it to start a conversion, with parasite power on at the end
  ds.write(0x44, 0);        // or start conversion in powered mode (bus finishes low)

  // just wait a second while the conversion takes place
  // different chips have different conversion times, check the specs, 1 sec is worse case + 250ms
  // you could also communicate with other devices if you like but you would need
  // to already know their address to select them.

  delay(1000);     // maybe 750ms is enough, maybe not, wait 1 sec for conversion

  // we might do a ds.depower() (parasite) here, but the reset will take care of it.

  // first make sure current values are in the scratch pad

  present = ds.reset();
  ds.select(addr);
  ds.write(0xB8,0);         // Recall Memory 0
  ds.write(0x00,0);         // Recall Memory 0

  // now read the scratch pad

  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE,0);         // Read Scratchpad
  if (type_s == 2) {
    ds.write(0x00,0);       // The DS2438 needs a page# to read
  }


  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

  int16_t raw = (data[1] << 8) | data[0];
  if (type_s == 2) raw = (data[2] << 8) | data[1];
  byte cfg = (data[4] & 0x60);

  switch (type_s) {
    case 1:
      raw = raw << 3; // 9 bit resolution default
      if (data[7] == 0x10) {
        // "count remain" gives full 12 bit resolution
        raw = (raw & 0xFFF0) + 12 - data[6];
      }
      celsius = (float)raw * 0.0625;
      break;
    case 0:
      // at lower res, the low bits are undefined, so let's zero them
      if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
      if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
      if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
      // default is 12 bit resolution, 750 ms conversion time
      celsius = (float)raw * 0.0625;
      break;

    case 2:
      data[1] = (data[1] >> 3) & 0x1f;
      if (data[2] > 127) {
        celsius = (float)data[2] - ((float)data[1] * .03125);
      }else{
        celsius = (float)data[2] + ((float)data[1] * .03125);
      }
  }

  // remove random errors
  if((((celsius <= 0 && celsius > -1) && lastTemp > 5)) || celsius > 125) {
      celsius = lastTemp;
  }

  fahrenheit = celsius * 1.8 + 32.0;
  lastTemp = celsius;

  return String(fahrenheit);
}

int getLightIntensity(){
  int photoReading = analogRead(photoPin);
  return photoReading;
}

//Send the sensor readings to the cloud
void publishState(){
  powerSensors();
  int sensorVal1 = analogRead(moistureSensor1);
  int sensorVal2 = analogRead(moistureSensor2);
  int sensorVal3 = analogRead(moistureSensor3);
  avgMoisture = takeAverage(sensorVal1, sensorVal2, sensorVal3);
  // String publishVal = String(avgMoisture);
  String publishVal = String(avgMoisture);
  // String temperatureVal = getTemperature();
  // int lightIntensity = getLightIntensity();
  Particle.publish("moisture", publishVal);
  // Particle.publish("temperature", temperatureVal);
  // Particle.publish("light", String(lightIntensity));

}


Timer timer(60000, publishState);

//Start the timer
void timerStart(){
  timer.start();
}

//Take an average of 3 numbers
float takeAverage(int val1, int val2, int val3){
  int sum = val1 + val2 + val3;
  float average = sum/3.0;
  return average;
}

//Power the sensors by pulling the digital pins HIGH
void powerSensors(){
  digitalWrite(moisturePower1, HIGH);
  digitalWrite(moisturePower2, HIGH);
  digitalWrite(moisturePower3, HIGH);
}

//Un-power the sensors by pulling the digital pins LOW
void unPowerSensors(){
  digitalWrite(moisturePower1, LOW);
  digitalWrite(moisturePower2, LOW);
  digitalWrite(moisturePower3, LOW);

}

//Particle function to get the state of the valve / relay pin
int getState(String command){
    int state = digitalRead(valvePin);
    Particle.publish("state", String(valvePin));
    return state;
}

//Particle function to turn on / off the valve from the UI
int actuate(String command){
    //digitalWrite to Solenoid Valve based on which button was pushed on the UI
    if(command=="open"){
        valveOn(); //open the valve
        Particle.publish("state", "Open"); //send the state back to the UI
        return 1;
    }
    else if(command=="close"){
        valveOff(); // close the valve
        Particle.publish("state", "Closed"); //send the state back to the UI
        return 0;
    }
}

int setHighThreshold(String command){
  int val = command.toInt();
  actuationThreshold = val;
  Particle.publish("Actuation thresh change", String(actuationThreshold));
}

int setLowThreshold(String command){
  int val = command.toInt();
  wetThreshold = val;
  Particle.publish("Wet thresh change", String(wetThreshold));
}


// Though the loop is running constantly, the only time any code will run is when the plant is being watered.
// This is because the only time we need to take readings at the maximum possible rate is when the plant is being watered,
// or in the actuation system. Thus, the loop is essentially the actuation system. The code runs until the plant is
// sufficiently watered. Once the plant is healthy, no code is being run by the loop, and it is effectively blank.
void loop() {

  // int moistureVal1 = analogRead(moistureSensor1);
  // int moistureVal2 = analogRead(moistureSensor2);
  // int moistureVal3 = analogRead(moistureSensor3);

  if(isWatering){

    // Serial.println("currently watering");


    powerSensors();
    valveOn();

    //Take the average of the 3 readings, and take the average of the 3
    int soilReading1 = analogRead(moistureSensor1);
    int soilReading2 = analogRead(moistureSensor2);
    int soilReading3 = analogRead(moistureSensor3);
    float avgValue = takeAverage(soilReading1, soilReading2, soilReading3);

    //Until the rolling average array has been filled (it was initialized with zeros), add data to it sequentially
    if(!isComplete){
      readings[readIndex] = avgValue;
      readIndex++;
    }

    //If readIndex reaches the length of the array, wrap it back to 0

    if(readIndex == numReadings){
      readIndex = 0;
    }

    // Once the array is filled with 20 data points, begin taking the rolling average. All the values are shifted to the left
    // one index, and the last element is replaced with the new sensor value. The average of the elements of this array is
    // computed every run through the loop
    if(isComplete){
      for(int i=1; i<numReadings; i++){
        readings[i-1] = readings[i];
      }
      readings[numReadings-1] = avgValue;
      for(int j=0; j<numReadings; j++){
        total = total+readings[j];
      }
      average = total / numReadings;
      total = 0;

      // If the rolling average is less than the wet threshold, turn the valve off, unPower the sensors, and go back to the
      // sensing controller. Setting isWatering to false effectively exits the loop, and starts listening for events
      // every 60 seconds
      if(average <= wetThreshold){
        valveOff();
        unPowerSensors();
        isWatering = false;
        Particle.publish("wateringStatus", "stopped");
      }
    }

    if(readings[numReadings-1] != 0){
      isComplete = true;
    }


    //Delay to compensate for randomness
    delay(1);

  }

  else {
    valveOff();
  }
  // actuate("open");

}
