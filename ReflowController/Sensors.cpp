#include "Sensors.h"

static void readThermocouple(struct Thermocouple* input) {
  MAX31855_t sensor;

  uint8_t lcdState = digitalRead(LCD_CS);
  digitalWrite(LCD_CS, HIGH);
  MYSPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(input->chipSelect, LOW);
  delay(1);
  
  for (int8_t i = 1; i >= 0; i--) {
    sensor.bytes[i] = MYSPI.transfer(0x00);
  }
  digitalWrite(input->chipSelect, HIGH);
  MYSPI.endTransaction();
  digitalWrite(LCD_CS, lcdState);

  input->stat = sensor.bytes[0] & 0b100;

  uint16_t value = (sensor.value >> 3) & 0x1FFF; // mask off the sign bit and shit to the correct alignment for the temp data  
  input->temperature = value * 0.25;
}

void sensorsInit() {
  //INIT Temps
  pinMode(TEMP1_CS, OUTPUT);
  Temp_1.chipSelect = TEMP1_CS;
}

void sensorsUpdate() {
  // Temp messurment and averageing
  if(!timer_temp.repeat()) {
    return;
  }

  readThermocouple(&Temp_1);

  static float average[READ_TEMP_AVERAGE_COUNT];
  static uint8_t stats[READ_TEMP_AVERAGE_COUNT];
  static uint8_t pointer =0;

  average[pointer]=Temp_1.temperature;
  stats[pointer]=Temp_1.stat;
  pointer=(pointer+1)%READ_TEMP_AVERAGE_COUNT;

  float sum =0;
  uint8_t state_count=0;
  uint8_t stat=0;
  for (int i=0;i<READ_TEMP_AVERAGE_COUNT;i++)
  {
    sum +=average[i];
    stat &=stats[i];
    if (stats[i]) 
    {
      state_count++;
    }
  }
  
  // if (state_count>READ_TEMP_AVERAGE_COUNT/2) {
  //     switch (stat) {
  //       case 0b001:
  //         reportError("Temp Sensor 1: Open Circuit");
  //         break;
  //       case 0b010:
  //         reportError("Temp Sensor 1: GND Short");
  //         break;
  //       case 0b100:
  //         reportError("Temp Sensor 1: VCC Short");
  //         break;
  //       default:
  //         reportError("Temp Sensor 1: Multiple errors!");
  //         break;
  //     }
  // }

  aktSystemTemperature = sum/READ_TEMP_AVERAGE_COUNT;

  static float averagees[READ_TEMP_RAMP_WINDOW_MS / READ_TEMP_INTERVAL_MS];
  static uint16_t p=0;
  static bool rampInit=false;

  if (!rampInit) {
    for (uint16_t i = 0; i < (READ_TEMP_RAMP_WINDOW_MS / READ_TEMP_INTERVAL_MS); i++) {
      averagees[i] = aktSystemTemperature;
    }
    aktSystemTemperatureRamp = 0.0f;
    rampInit = true;
  } else {
    const float window_s = (float)READ_TEMP_RAMP_WINDOW_MS / 1000.0f;
    aktSystemTemperatureRamp = (aktSystemTemperature - averagees[p]) / window_s;
  }

  averagees[p]=aktSystemTemperature;
  p=(p+1)%(READ_TEMP_RAMP_WINDOW_MS / READ_TEMP_INTERVAL_MS);
}
