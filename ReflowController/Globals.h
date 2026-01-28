#pragma once

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <SPI.h>
#include <Ticker.h>
#include "max6675.h"
#include "Button2.h"
#include "src/Neotimer/neotimer.h"
#include "driver/ledc.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "src/Adafruit_GFX_Library/Adafruit_GFX.h"
#include "src/Adafruit-ST7735-Library/Adafruit_ST7735.h"
#include "src/Menu/Menu.h"
#include "src/ClickEncoder/ClickEncoder.h"

//Devdefins
//#define NOEDGEERRORREPORT

//Pin Mapping
#define LCD_CS      15
#define LCD_DC      27
#define LCD_RESET   33
#define SD_CS       5
#define ENC1        35
#define ENC2        32
#define ENC_B       33

#define BTN_2       4 //startstop
#define BTN_13      16  //warmer
#define BTN_4       17  //grill
#define BTN_3       5 //toast
#define BTN_14      18  //bake
#define BTN_11      19  //clock
#define BTN_12_ADC  36    //ADC1_0  -(0-60) +(600-700) time(1000-1300) temp(1500-1700) open 4095

#define HEATER1     22
#define ZEROX       4
#define FAN1        21

#define TEMP1_CS    32
#define NTC_ADC     19    //ADC1_3

#define BUZZER      2

// HSPI pin mapping (adjust to match your wiring)
#define HSPI_SCK    14
#define HSPI_MISO   12
#define HSPI_MOSI   13

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (5) // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (4095) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz

#define  SCR_PWM_CHANNEL  0
#define  SCR_PWM_FREQUENCY  3
#define  SCR_PWM_RESOLUTION 8

//constance
#define RECAL_ZEROX_TIME_MS 100
#define ZEROX_TIMEOUT_MS 5000
#define READ_TEMP_INTERVAL_MS 250
#define READ_TEMP_AVERAGE_COUNT 1

#define RGB_LED_BRITHNESS_1TO255  125
#define IDLE_TEMP     50
#define MAX_PROFILES  30
#define MENUE_ITEMS_VISIBLE 8
#define MENU_ITEM_HIEGT 12
#define PROFILE_NAME_LENGTH 11

// Default tuning values for inertia guard and ConstantTemp slew limiter.
#define INERTIA_GUARD_LEAD_DEFAULT_SEC 6.0f
#define INERTIA_GUARD_HYST_DEFAULT_C 0.5f
#define INERTIA_GUARD_MIN_RISE_DEFAULT_CPS 0.02f
#define INERTIA_GUARD_MAX_SETPOINT_DEFAULT_C 120.0f
#define CONST_TEMP_SLEW_DEFAULT_CPS 0.5f

//structs
// data type for the values used in the reflow profile
typedef struct profileValues_s {
  char    name[PROFILE_NAME_LENGTH];
  int16_t soakTemp;
  int16_t soakDuration;
  int16_t peakTemp;
  int16_t peakDuration;
  float  rampUpRate;
  float  rampDownRate;
} Profile_t;

typedef union {
  uint32_t value;
  uint8_t bytes[4];
} __attribute__((packed)) MAX31855_t;

typedef struct Thermocouple {
  float temperature;
  uint8_t stat;
  uint8_t chipSelect;
};

typedef enum {
  None     = 0,
  Idle     = 1,
  Settings = 2,
  Edit     = 3,

  UIMenuEnd = 9,

  RampToSoak = 10,
  Soak,
  RampUp,
  Peak,
  CoolDown,

  Complete = 20,

  ConstantTemp = 25,

  PreTune = 30,
  Tune,
} State;

typedef struct {
  const Menu::Item_t *mi;
  uint8_t pos;
  bool current;
} LastItemState_t;

typedef struct {
  float Kp;
  float Ki;
  float Kd;
} PID_t;

// Globals
extern Neotimer timer_beep;
extern Neotimer timer_temp;
extern Neotimer timer_dsp_btmln;
extern Neotimer timer_display;
extern Neotimer timer_control;
extern Neotimer timer_countupdown;

extern Button2 btn_startstop;
extern Button2 btn_up;
extern Button2 btn_down;
extern Button2 btn_left;
extern Button2 btn_right;
extern Button2 btn_stop;

extern bool countup;
extern bool countdown;

extern const char * ver;
extern const uint8_t asinelookupTable[256];

extern SPIClass MYSPI;
extern Adafruit_ST7735 tft;
extern Thermocouple Temp_1;
extern Menu::Engine myMenue;
extern Preferences PREF;

extern WebServer server;
extern WebServer serverAction;

extern volatile boolean globalError;

extern volatile uint16_t  powerHeater;
extern bool systemPaused;
extern int8_t fanOverride;
extern volatile bool pidTuningsDirty;

extern float aktSystemTemperature;
extern float aktSystemTemperatureRamp; //°C/s

extern int16_t tuningHeaterOutput;
extern int16_t tuningNoiseBand;
extern int16_t tuningOutputStep;
extern int16_t tuningLookbackSec;
extern int16_t constantTempSetpoint;
extern int16_t constantTempBeepMinutes;
extern float inertiaGuardLeadSec;
extern float inertiaGuardHysteresisC;
extern float inertiaGuardMinRiseCps;
extern float inertiaGuardMaxSetpointC;
extern float constTempRampCps;

extern int activeProfileId;
extern Profile_t activeProfile; // the one and only instance

extern int16_t encAbsolute;

extern State currentState;
extern uint64_t stateChangedTicks;

// track menu item state to improve render preformance
extern LastItemState_t currentlyRenderedItems[MENUE_ITEMS_VISIBLE];

extern Menu::Item_t miExit;
extern Menu::Item_t miCycleStart;
extern Menu::Item_t miConstTemp;
extern Menu::Item_t miConstTempStart;
extern Menu::Item_t miConstTempSet;
extern Menu::Item_t miConstTemp40;
extern Menu::Item_t miConstTemp60;
extern Menu::Item_t miConstTemp80;
extern Menu::Item_t miConstBeepMin;
extern Menu::Item_t miConstBeepOff;
extern Menu::Item_t miConstBeep20;
extern Menu::Item_t miConstBeep30;
extern Menu::Item_t miConstBeep60;
extern Menu::Item_t miEditProfile;
extern Menu::Item_t miName;
extern Menu::Item_t miRampUpRate;
extern Menu::Item_t miSoakTemp;
extern Menu::Item_t miSoakTime;
extern Menu::Item_t miPeakTemp;
extern Menu::Item_t miPeakTime;
extern Menu::Item_t miRampDnRate;
extern Menu::Item_t miLoadProfile;
extern Menu::Item_t miSaveProfile;
extern Menu::Item_t miPidSettings;
extern Menu::Item_t miAutoTune;
extern Menu::Item_t miHeaterOutput;
extern Menu::Item_t miNoiseBand;
extern Menu::Item_t miOutputStep;
extern Menu::Item_t miLookbackSec;
extern Menu::Item_t miCycleStartAT;
extern Menu::Item_t miPidSettingP;
extern Menu::Item_t miPidSettingI;
extern Menu::Item_t miPidSettingD;
extern Menu::Item_t miGuardSettings;
extern Menu::Item_t miGuardLead;
extern Menu::Item_t miGuardHyst;
extern Menu::Item_t miGuardRise;
extern Menu::Item_t miGuardMax;
extern Menu::Item_t miConstSlew;
extern Menu::Item_t miManual;
extern Menu::Item_t miIODebug;
extern Menu::Item_t miWIFI;
extern Menu::Item_t miWIFIUseSaved;
extern Menu::Item_t miFactoryReset;

extern float heaterSetpoint;
extern float heaterInput;
extern float heaterOutput;

extern PID_t heaterPID;

extern bool menuUpdateRequest;
extern bool initialProcessDisplay;

extern uint64_t cycleStartTime;

extern uint8_t beepcount;

const char * currentStateToString();
