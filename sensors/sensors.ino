// This #include statement was automatically added by the Particle IDE.
#include <OneWire.h>



OneWire ds = OneWire(D4);  // 1-wire signal on pin D4

unsigned long lastUpdate = 0;

float lastTemp;

int photoPin = A2;
int photoPower = A4;

int val = 0; //value for storing moisture value
int soilPin = A2;//Declare a variable for the soil moisture sensor
int soilPower = D0;//Variable for Soil moisture Power

void setup() {

    Serial.begin(9600);

    pinMode(photoPin, INPUT);
    pinMode(photoPower, OUTPUT);
    analogWrite(photoPower, HIGH);

    pinMode(soilPower, OUTPUT);//Set D7 as an OUTPUT
    digitalWrite(soilPower, HIGH);


}

// up to here, it is the same as the address acanner
// we need a few more variables for this example

void loop(void) {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;

  Serial.print("Soil Moisture = ");
  Serial.println(readSoil());
  delay(1000);

  //Read from the Photoresistor Pin

  int photoRaw = analogRead(photoPin);
  String photoPublish = String(photoRaw);



  if ( !ds.search(addr)) {
    ds.reset_search();
    delay(250);
    return;
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {

      return;
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


  // now that we have the readings, we can publish them to the cloud
  String temperature = String(fahrenheit); // store temp in "temperature" string
  Particle.publish("Temperature", temperature, PRIVATE); // publish to cloud
  Particle.publish("Photoresistor Reading", photoPublish, PRIVATE);
  delay(2000); // 5 second delay
}


int readSoil()
{

    digitalWrite(soilPower, HIGH);//turn D7 "On"
    delay(10);//wait 10 milliseconds
    val = analogRead(soilPin);//Read the SIG value form sensor
    digitalWrite(soilPower, LOW);//turn D7 "Off"
    return val;//send current moisture value
}
