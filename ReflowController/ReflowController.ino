// ----------------------------------------------------------------------------
// Reflow Oven Controller
// (c) 2019      Patrick Knöbel
// (c) 2014 Karl Pitrich <karl@pitrich.com>
// (c) 2012-2013 Ed Simmons
// ----------------------------------------------------------------------------

//includes
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <SPI.h>
#include <Ticker.h>
#include "max6675.h"
#include "Button2.h"
#include <neotimer.h>
#include "driver/ledc.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "src/Adafruit_GFX_Library/Adafruit_GFX.h"
#include "src/Adafruit-ST7735-Library/Adafruit_ST7735.h"
#include "src/PID_v1/PID_v1.h"
#include "src/PID_AutoTune_v0/PID_AutoTune_v0.h"
#include "src/Menu/Menu.h"
#include "src/ClickEncoder/ClickEncoder.h"

#include "root_html.h"

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

Neotimer timer_beep = Neotimer(500);
Neotimer timer_temp = Neotimer(READ_TEMP_INTERVAL_MS);
Neotimer timer_dsp_btmln = Neotimer(200);
Neotimer timer_display = Neotimer(1000);
Neotimer timer_control = Neotimer(100);
Neotimer timer_countupdown = Neotimer(200);

Button2 btn_startstop;
Button2 btn_up;
Button2 btn_down;
Button2 btn_left;
Button2 btn_right;
Button2 btn_stop;

bool countup = false;
bool countdown = false;

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

  PreTune = 30,
  Tune,
} State;


typedef struct {
  const Menu::Item_t *mi;
  uint8_t pos;
  bool current;
} LastItemState_t;


//prototypes
bool menuExit(const Menu::Action_t);
bool cycleStart(const Menu::Action_t);
bool menuDummy(const Menu::Action_t);
bool editNumericalValue(const Menu::Action_t);
bool editProfileName(const Menu::Action_t);
bool saveLoadProfile(const Menu::Action_t);
bool manualHeating(const Menu::Action_t);
bool menuWiFi(const Menu::Action_t);
bool factoryReset(const Menu::Action_t);


//Varables
const char * ver = "4.0";

//https://docs.google.com/spreadsheets/d/1syyYxHWjEHy5YJJYK1BmVZYog_yKRtXpa9yudWSkGAE/edit?usp=sharing
//(ASIN(A3/256*2-1)+PI()/2)/PI()*256
const uint8_t asinelookupTable[] {0,10,14,17,20,22,25,27,28,30,32,34,35,37,38,39,41,42,43,44,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,67,68,69,70,71,72,72,73,74,75,76,76,77,78,79,80,80,81,82,83,83,84,85,86,86,87,88,88,89,90,91,91,92,93,93,94,95,95,96,97,98,98,99,100,100,101,102,102,103,104,104,105,106,106,107,108,108,109,110,110,111,111,112,113,113,114,115,115,116,117,117,118,119,119,120,120,121,122,122,123,124,124,125,126,126,127,128,128,129,129,130,131,131,132,133,133,134,135,135,136,136,137,138,138,139,140,140,141,142,142,143,144,144,145,145,146,147,147,148,149,149,150,151,151,152,153,153,154,155,155,156,157,157,158,159,160,160,161,162,162,163,164,164,165,166,167,167,168,169,169,170,171,172,172,173,174,175,175,176,177,178,179,179,180,181,182,183,183,184,185,186,187,188,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,211,212,213,214,216,217,218,220,221,223,225,227,228,230,233,235,238,241,245};

SPIClass MYSPI(HSPI); 
Adafruit_ST7735 tft = Adafruit_ST7735(&MYSPI,LCD_CS, LCD_DC, LCD_RESET);
Thermocouple Temp_1;
Menu::Engine myMenue;
Preferences PREF;

WebServer server(80);
WebServer serverAction(8080);

volatile boolean globalError=false;

volatile uint16_t  powerHeater=0;

float aktSystemTemperature;
float aktSystemTemperatureRamp; //°C/s

int16_t tuningHeaterOutput=30;
int16_t tuningNoiseBand=1;
int16_t tuningOutputStep=10;
int16_t tuningLookbackSec=60;


int activeProfileId = 0;
Profile_t activeProfile; // the one and only instance

int16_t encAbsolute;

State currentState  = Idle;
uint64_t stateChangedTicks = 0;

// track menu item state to improve render preformance
LastItemState_t currentlyRenderedItems[MENUE_ITEMS_VISIBLE];

// ----------------------------------------------------------------------------------------------------------------------------------------
//       Name,            Label,              Next,               Previous,         Parent,           Child,            Callback
MenuItem(miExit,          "EXIT",             Menu::NullItem,     Menu::NullItem,   Menu::NullItem,   miCycleStart,     menuExit);
  MenuItem(miCycleStart,    "Start Cycle",      miEditProfile,      Menu::NullItem,   miExit,           Menu::NullItem,   cycleStart);
  MenuItem(miEditProfile,   "Edit Profile",     miLoadProfile,      miCycleStart,     miExit,           miName,            menuDummy);
    MenuItem(miName,          "Name     ",        miRampUpRate,       Menu::NullItem,   miEditProfile,    Menu::NullItem,   editProfileName);
    MenuItem(miRampUpRate,    "Ramp up  ",        miSoakTemp,         miName,           miEditProfile,    Menu::NullItem,   editNumericalValue);
    MenuItem(miSoakTemp,      "Soak temp",        miSoakTime,         miRampUpRate,     miEditProfile,    Menu::NullItem,   editNumericalValue);
    MenuItem(miSoakTime,      "Soak time",        miPeakTemp,         miSoakTemp,       miEditProfile,    Menu::NullItem,   editNumericalValue);
    MenuItem(miPeakTemp,      "Peak temp",        miPeakTime,         miSoakTime,       miEditProfile,    Menu::NullItem,   editNumericalValue);
    MenuItem(miPeakTime,      "Peak time",        miRampDnRate,       miPeakTemp,       miEditProfile,    Menu::NullItem,   editNumericalValue);
    MenuItem(miRampDnRate,    "Ramp down",        Menu::NullItem,     miPeakTime,       miEditProfile,    Menu::NullItem,   editNumericalValue);
  MenuItem(miLoadProfile,   "Load Profile",     miSaveProfile,      miEditProfile,    miExit,           Menu::NullItem,   saveLoadProfile);
  MenuItem(miSaveProfile,   "Save Profile",     miPidSettings,      miLoadProfile,    miExit,           Menu::NullItem,   saveLoadProfile);
  MenuItem(miPidSettings,   "PID Settings",     miManual,           miSaveProfile,    miExit,           miAutoTune,       menuDummy);
    MenuItem(miAutoTune,      "Autotune",         miPidSettingP,      Menu::NullItem,   miPidSettings,    miHeaterOutput,   menuDummy);
      MenuItem(miHeaterOutput,  "Output   ",     miNoiseBand,        Menu::NullItem,   miAutoTune,       Menu::NullItem,   editNumericalValue);
      MenuItem(miNoiseBand,     "NoiseBand",        miOutputStep,       miHeaterOutput,   miAutoTune,       Menu::NullItem,   editNumericalValue);
      MenuItem(miOutputStep,    "Step     ",       miLookbackSec,      miNoiseBand,      miAutoTune,       Menu::NullItem,   editNumericalValue);
      MenuItem(miLookbackSec,   "Lookback ",      miCycleStartAT,     miOutputStep,     miAutoTune,       Menu::NullItem,   editNumericalValue);
      MenuItem(miCycleStartAT,  "Start Autotune",   Menu::NullItem,     miLookbackSec,    miAutoTune,       Menu::NullItem,   cycleStart);
    MenuItem(miPidSettingP,   "Heater Kp",        miPidSettingI,      miAutoTune,       miPidSettings,    Menu::NullItem,   editNumericalValue);
    MenuItem(miPidSettingI,   "Heater Ki",        miPidSettingD,      miPidSettingP,    miPidSettings,    Menu::NullItem,   editNumericalValue);
    MenuItem(miPidSettingD,   "Heater Kd",        Menu::NullItem,     miPidSettingI,    miPidSettings,    Menu::NullItem,   editNumericalValue);
  MenuItem(miManual,        "Manual Heating",   miWIFI,             miPidSettings,    miExit,           Menu::NullItem,   manualHeating);
  MenuItem(miWIFI,          "WIFI",             miFactoryReset,     miManual,         miExit,           miWIFIUseSaved,   menuWiFi);
    MenuItem(miWIFIUseSaved,  "Connect to Saved", Menu::NullItem,   Menu::NullItem,   miWIFI,           Menu::NullItem,   menuWiFi);
  MenuItem(miFactoryReset,  "Factory Reset",    Menu::NullItem,     miWIFI,           miExit,           Menu::NullItem,   factoryReset);

float heaterSetpoint;
float heaterInput;
float heaterOutput;

typedef struct {
  float Kp;
  float Ki;
  float Kd;
} PID_t;

PID_t heaterPID;

PID PID(&heaterInput, &heaterOutput, &heaterSetpoint, heaterPID.Kp, heaterPID.Ki, heaterPID.Kd, DIRECT);

PID_ATune PIDTune(&heaterInput, &heaterOutput);

bool menuUpdateRequest = true;
bool initialProcessDisplay = false;

uint64_t cycleStartTime=0;

void reportError(char *text)
{
  globalError=true;

  //Turn off heaters
  digitalWrite(HEATER1,LOW);
  ledcWrite(SCR_PWM_CHANNEL,0);
  digitalWrite(FAN1,0);
  
  Serial.print("Report Error: ");
  Serial.println(text);

  tft.setTextColor(ST7735_WHITE, ST7735_RED);
  tft.fillScreen(ST7735_RED);

  tft.setCursor(5, 10);
  
  tft.setTextSize(2);
  tft.println("!!!!ERROR!!!!");
  tft.println();
  tft.setTextSize(1);
  tft.setTextWrap(true);
  tft.println(text);
  tft.setTextWrap(false);


  tft.setCursor(10, 115);
  tft.println("Power off!");

  while(1){

    delay(1000);
    delay(1000);
  }
}


void readThermocouple(struct Thermocouple* input) {
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

void spashscreen()
{
  // splash screen
  tft.setCursor(10, 30);
  tft.setTextSize(2);
  tft.print("Reflow");
  tft.setCursor(24, 48);
  tft.print("Controller");
  tft.setTextSize(1);
  tft.setCursor(52, 67);
  tft.print("v"); tft.print(ver);
  tft.setCursor(7, 109);
  tft.print("(c)2014 karl@pitrich.com");
  tft.setCursor(7, 119);
  tft.print("(c)2019 reflow@im-pro.at");
  
#ifdef NOEDGEERRORREPORT
  tft.setCursor(10, 30);
  tft.setTextSize(4);
  tft.setTextWrap(true);
  tft.print("DEVMODE");
  tft.setTextWrap(false);
  tft.setTextSize(1);
#endif
  
}  



bool keyboard(const char * name, char * buffer, uint32_t length, bool init, bool trigger)
{
  bool finisched=false;
  static uint32_t p;
  if(init)
  {
    p=0;
    buffer[0]=0;
    encAbsolute=0;
    trigger=false;
    tft.fillScreen(ST7735_WHITE);    
    tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
    tft.setTextSize(1);
    tft.setCursor(2, 10);
    tft.print(name);
    tft.print(":");
  }

  static int numElements=0;
  static int lastencAbsolute=0;
  char start[] =  {'a', 'A', '0', '!', ':', '[', '{', 0 }; 
  char end[]   =  {'z', 'Z', '9', '/', '@', '`', '~', 2 };
  int y[]      =  {0,   10,  20,  20,  30,  30,  30,  40};
  int x[]      =  {2,   2,   2,   62,  2,   44,  80,  2 };
  int spacing[]=  {6,   6,   6,   6,   6,   6,   6,   60};
  int akt=0;  
  
  if(encAbsolute<0) encAbsolute=0;
  if(encAbsolute>numElements) encAbsolute=numElements;
  
  for(int i=0;i<sizeof(start);i++){
    for(char c=start[i];c<=end[i];c++)
    {
      tft.setCursor(x[i]+(c-start[i])*spacing[i], y[i]+50);
      if(init || encAbsolute== akt || lastencAbsolute== akt){
        if(encAbsolute== akt){
          tft.setTextColor(ST7735_WHITE, ST7735_BLUE);          
        }
        else{
          tft.setTextColor(ST7735_BLACK, ST7735_WHITE);          
        }
        switch(c){
          case 1:
            tft.print("DELETE");
            if(trigger) 
            {
              if(p>0)
                buffer[--p]=0;
            }
            break;
          case 2:
            tft.print("OK");
            if(trigger) 
            {
              finisched=true;
            }
            break;
          case 0:
          default:
            if(c==0){
              tft.print("SPACE");
            }
            else{
              tft.print(c);
            }
            if(trigger) 
            {
              if (p<(length-1)){
                buffer[p++]=(c==0?' ':c);
                buffer[p]=0;                
              }
            }
            break;
        } 
      } 
      akt++;      
    }
  }  
  
  numElements=akt-1;
  lastencAbsolute=encAbsolute;

  //update diaply
  tft.setCursor(2, 20);
  tft.setTextColor(ST7735_WHITE, ST7735_RED);
  tft.setTextWrap(true);
  tft.print(buffer);
  for(int i=0;i<length-1-p;i++)
  {
    tft.print(" "); 
  }
  tft.setTextWrap(false);  
  tft.setTextColor(ST7735_BLACK, ST7735_WHITE);  
  
  return finisched;
}

void clearLastMenuItemRenderState() {
  tft.fillScreen(ST7735_WHITE);
  displayMenusInfos();
  for (uint8_t i = 0; i < MENUE_ITEMS_VISIBLE; i++) {
    currentlyRenderedItems[i].mi = NULL;
    currentlyRenderedItems[i].pos = 0xff;
    currentlyRenderedItems[i].current = false;
  }
  menuUpdateRequest = true;
}

bool menuExit(const Menu::Action_t a) {
  clearLastMenuItemRenderState();
  return false;
}

bool menuDummy(const Menu::Action_t a) {
  if(a!=Menu::actionLabel)
  {    
    clearLastMenuItemRenderState();
  }
  return true;
}

void printfloat(float val) {
  char buf[20];
  snprintf(buf,20,"%.1f",val);
  tft.print(buf);
}
void printfloat2(float val) {
  char buf[20];
  snprintf(buf,20,"%.2f",val);
  tft.print(buf);
}


const char * currentStateToString()
{
  #define casePrintState(state) case state: return #state;
  switch (currentState) {
    casePrintState(RampToSoak);
    casePrintState(Soak);
    casePrintState(RampUp);
    casePrintState(Peak);
    casePrintState(CoolDown);
    casePrintState(Complete);
    casePrintState(PreTune);
    casePrintState(Tune);
    default: return "Idle";
  }
}


void getItemValuePointer(const Menu::Item_t *mi, float **d, int16_t **i) {
  if (mi == &miRampUpRate)  *d = &activeProfile.rampUpRate;
  if (mi == &miRampDnRate)  *d = &activeProfile.rampDownRate;
  if (mi == &miSoakTime)    *i = &activeProfile.soakDuration;
  if (mi == &miSoakTemp)    *i = &activeProfile.soakTemp;
  if (mi == &miPeakTime)    *i = &activeProfile.peakDuration;
  if (mi == &miPeakTemp)    *i = &activeProfile.peakTemp;
  if (mi == &miPidSettingP) *d = &heaterPID.Kp;
  if (mi == &miPidSettingI) *d = &heaterPID.Ki;
  if (mi == &miPidSettingD) *d = &heaterPID.Kd; 
  if (mi == &miHeaterOutput)*i = &tuningHeaterOutput;
  if (mi == &miNoiseBand)   *i = &tuningNoiseBand;
  if (mi == &miOutputStep)  *i = &tuningOutputStep;
  if (mi == &miLookbackSec) *i = &tuningLookbackSec;
}

bool isPidSetting(const Menu::Item_t *mi) {
  return mi == &miPidSettingP || mi == &miPidSettingI || mi == &miPidSettingD;
}

bool isRampSetting(const Menu::Item_t *mi) {
  return mi == &miRampUpRate || mi == &miRampDnRate;
}

bool getItemValueLabel(const Menu::Item_t *mi, char *label) {
  int16_t *iValue = NULL;
  float  *dValue = NULL;
  char *p;
  
  getItemValuePointer(mi, &dValue, &iValue);

  if(isRampSetting(mi)){
    sprintf(label,"%.1f\367C/s",*dValue);    
  }
  else if(isPidSetting(mi)){
    sprintf(label,"%.2f",*dValue);    
  }
  else if (mi == &miPeakTemp || mi == &miSoakTemp) {
    sprintf(label,"%d\367C",*iValue);        
  }
  else if (mi == &miPeakTime || mi == &miSoakTime || mi == &miLookbackSec) {
    sprintf(label,"%ds",*iValue);        
  }
  else if (mi == &miHeaterOutput || mi == &miOutputStep) {
    sprintf(label,"%d%%",*iValue);        
  }
  else if(mi == &miNoiseBand) {
    sprintf(label,"%d",*iValue);        
  }
  else if(mi == &miName) {
    sprintf(label,"%s",activeProfile.name);
  }
  else{
    return false;    
  }
  return true;

}

bool editNumericalValue(const Menu::Action_t action) 
{ 
  static int16_t iValueLast;
  static float   dValueLast;
  int16_t *iValue = NULL;
  float  *dValue = NULL;
  bool init=false;
  getItemValuePointer(myMenue.currentItem, &dValue, &iValue);
  if ((init=(action == Menu::actionTrigger && currentState != Edit)) || action == Menu::actionDisplay) 
  {
    if (init) 
    {
      currentState = Edit;
      tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
      tft.setCursor(10, 80);
      tft.print("Edit & click to save.");
      //save last Value
      if(iValue!=NULL) iValueLast=*iValue;
      if(dValue!=NULL) dValueLast=*dValue;
    }

    for (uint8_t i = 0; i < MENUE_ITEMS_VISIBLE; i++) 
    {
      if (currentlyRenderedItems[i].mi == myMenue.currentItem) 
      {
        uint8_t y = currentlyRenderedItems[i].pos * MENU_ITEM_HIEGT + 2;

        if (init) 
        {
          tft.fillRect(69, y - 1, 60, MENU_ITEM_HIEGT - 2, ST7735_RED);
        }

        tft.setCursor(70, y);
        break;
      }
    }

    tft.setTextColor(ST7735_WHITE, ST7735_RED);

    //no negative numbers nedded!
    if(encAbsolute<0) encAbsolute=0;       

    if (isRampSetting(myMenue.currentItem) || isPidSetting(myMenue.currentItem)) 
    {
      float tmp;
      float factor = (isPidSetting(myMenue.currentItem)) ? 100 : 10;

      if (init) {
        tmp = *dValue;
        tmp *= factor;
        encAbsolute = (int16_t)tmp;
      }
      else {
        tmp = encAbsolute;
        tmp /= factor;
        *dValue = tmp;
      }      
    }
    else {
      if (init) encAbsolute = *iValue;
      else *iValue = encAbsolute;
    }
    char buf[20];
    getItemValueLabel(myMenue.currentItem, buf);
    tft.print(buf);
    tft.print(" ");
    tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
  }
  else if ((action == Menu::actionParent || action == Menu::actionTrigger ) && currentState == Edit) 
  {
    currentState = Settings;
    clearLastMenuItemRenderState();

    if(action == Menu::actionParent){
      //restor Value
      if(iValue!=NULL) *iValue=iValueLast;
      if(dValue!=NULL) *dValue=dValueLast;
    }
    else if (isPidSetting(myMenue.currentItem)) {
      savePID();
    }
  }
  return true;
}

bool editProfileName(const Menu::Action_t action) {
  static char name[PROFILE_NAME_LENGTH];
  if (action == Menu::actionTrigger || action == Menu::actionDisplay) 
  {
    bool trigger =false;
    bool init=false;  
    if (action == Menu::actionTrigger && currentState != Edit) 
    {
      Serial.println("Init");
      init =true;
      currentState = Edit;
      name[0]=0;
    }
    else if(action==Menu::actionTrigger)
    {
      Serial.println("Trigger");
      trigger=true;
    }
    if(keyboard("Profile Name",name,PROFILE_NAME_LENGTH,init,trigger))
    {
      //Done!
      snprintf(activeProfile.name,PROFILE_NAME_LENGTH,"%s",name);      
      currentState = Settings;
      clearLastMenuItemRenderState();      
    }    
  }
  else if (action == Menu::actionParent && currentState == Edit) 
  {
    currentState = Settings;
    clearLastMenuItemRenderState();
  }
  return true;  
}

bool manualHeating(const Menu::Action_t action) {
  bool init=false;
  if ((init=(action == Menu::actionTrigger && currentState != Edit)) || action == Menu::actionDisplay) 
  {
    if (init) 
    {
      currentState = Edit;
      tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
      tft.fillScreen(ST7735_WHITE);
      encAbsolute = 0;      
      tft.setCursor(10, 100);
      tft.print("[Double]click to stop!");
    }
    if (encAbsolute > 100) encAbsolute = 100;
    if (encAbsolute <  0) encAbsolute =  0;

    tft.setCursor(10, 80);
    tft.print("Power:  ");
    tft.setTextColor(ST7735_WHITE, ST7735_RED);
    tft.print(encAbsolute);
    tft.setTextColor(ST7735_BLACK, ST7735_WHITE);    
    tft.print("%   ");
  }
  else if ((action == Menu::actionParent || action == Menu::actionTrigger ) && currentState == Edit) 
  {
    currentState = Settings;
    clearLastMenuItemRenderState();
  }
  return true;
  
}

bool menuWiFi(const Menu::Action_t action){
  static int wifiItemsCount=0;
  static Menu::Item_t *wifiItems=NULL;
  if(action==Menu::actionTrigger && myMenue.currentItem == &miWIFI)
  {
    //enter Wifi Menu:
    tft.fillScreen(ST7735_RED);
    tft.setTextColor(ST7735_WHITE);
    tft.setCursor(10, 50);
    tft.print("Scanning...");
    
    //free last Menu List
    for(int i=0;i<wifiItemsCount;i++) free((void*)wifiItems[i].Label);
    if (wifiItems!=NULL) free(wifiItems);
    
    //Search for networks
    wifiItemsCount = WiFi.scanNetworks();
    Serial.print("Wifis found: ");Serial.println(wifiItemsCount);
    
    //generate Menu List
    wifiItems=(Menu::Item_t *)malloc(max(1,wifiItemsCount)*sizeof(Menu::Item_t));
    if(wifiItems==NULL){
      reportError("Melloc Error!");
    }
    
    if(wifiItemsCount==WIFI_SCAN_FAILED){
      wifiItemsCount=0;
      wifiItems[0].Label="WIFI_SCAN_FAILED!";
      wifiItems[0].Callback=NULL;              
    }
    else if (wifiItemsCount == 0) 
    {      
      wifiItems[0].Label="No WiFis found!";
      wifiItems[0].Callback=NULL;        
    } 
    else 
    {
      for (int i = 0; i < wifiItemsCount; ++i) 
      {
        String name=WiFi.SSID(i);
        char * buffer= (char*)malloc(24);
        if(name==NULL){
          reportError("Melloc Error!");
        }
        name.toCharArray(buffer,24);
        if(name.length()>23){
          buffer[20]='.';
          buffer[21]='.';
          buffer[22]='.';
          buffer[23]=0;
        }
        wifiItems[i].Label=buffer;
        wifiItems[i].Callback=menuWiFi;        
      }
    }    
    //Link Meun;
    miWIFIUseSaved.Next=&wifiItems[0];
    for (int i = 0; i < max(1,wifiItemsCount); ++i) 
    {
      wifiItems[i].Next=i<(wifiItemsCount-1)?&wifiItems[i+1]:&Menu::NullItem;
      wifiItems[i].Previous=i>0?&wifiItems[i-1]:&miWIFIUseSaved;        
      wifiItems[i].Parent=&miWIFI;        
      wifiItems[i].Child=&Menu::NullItem;        
    }
    
  }
  else if(action==Menu::actionTrigger && myMenue.currentItem == &miWIFIUseSaved)    
  {
    //Conneced to Saved WiFi:
    char ssid[32],password[32];
    ssid[0]=0;
    password[0]=0;
    PREF.getString("WIFI_SSID", ssid, 32);
    PREF.getString("WIFI_PASSWORD", password, 32);

    tft.fillScreen(ST7735_RED);
    tft.setTextColor(ST7735_WHITE);
    tft.setCursor(10, 50);

    if(ssid[0]==0){
      tft.print("No WiFi Saved! ");       
    }
    else
    {
      tft.print("Connecting to: ");
      tft.setCursor(10, 60);
      tft.print(ssid);
      
      WiFi.disconnect();
      
      if(password[0]==0)
      {
        WiFi.begin(ssid);            
      }
      else
      {
        WiFi.begin(ssid, password);      
      }

      tft.setCursor(10, 70);
      for(int i=0;i<20;i++){
        if (WiFi.status() == WL_CONNECTED) break;
        delay(500);
        tft.print(".");
        
      }
      tft.setCursor(10, 80);
      if (WiFi.status() == WL_CONNECTED){
        tft.print("Connected!");
      }
      else{
        tft.print("ERROR! (");      
        tft.print(WiFi.status());      
        tft.print(")");      
        tft.setCursor(10, 90);
        tft.print("Retying in the BG ...");      
      }
    }
    delay(1000);
  }
  else if(action==Menu::actionTrigger || action == Menu::actionDisplay){
    //Choose new network
    int index=-1;
    for (int i = 0; i < wifiItemsCount; i++) 
    {
      if( myMenue.currentItem == &wifiItems[i]) index=i;
    }
    if(index!=-1){
      if(WiFi.encryptionType(index) == WIFI_AUTH_OPEN){
        PREF.putString("WIFI_SSID", WiFi.SSID(index).c_str());
        PREF.putString("WIFI_PASSWORD", "");
        myMenue.navigate(&miWIFIUseSaved);
        myMenue.executeCallbackAction(Menu::actionTrigger);
      }
      else{
        //enter password
        bool init =false;
        bool trigger =false;
        static char buffer[32];
        if(action==Menu::actionTrigger && currentState != Edit)
        {
          init =true; 
          currentState = Edit;
        }
        else if(action==Menu::actionTrigger)
        {
          trigger=true;
        }
        if(keyboard("Password",buffer,32,init,trigger))
        {
          //Done!
          PREF.putString("WIFI_SSID", WiFi.SSID(index).c_str());
          PREF.putString("WIFI_PASSWORD", buffer);
          currentState = Settings;
          myMenue.navigate(&miWIFIUseSaved);
          myMenue.executeCallbackAction(Menu::actionTrigger);          
        }
        
        
      }
    }
  }
  if(action == Menu::actionParent && currentState== Edit){
    currentState = Settings;
  }
  //clear Display if needed:
  if(action!=Menu::actionLabel && currentState!= Edit)
  { 
    clearLastMenuItemRenderState();
  }
  return true;
}

bool factoryReset(const Menu::Action_t action) {
  if (action == Menu::actionTrigger && currentState != Edit) 
  {
      currentState = Edit;
      
      tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
      tft.setCursor(10, 80);
      tft.print("Click to confirm");
      tft.setCursor(10, 90);
      tft.print("Doubleclick to exit");
  }
  else if ((action == Menu::actionParent || action == Menu::actionTrigger ) && currentState == Edit) 
  {
    currentState = Settings;
    if(action == Menu::actionTrigger) factoryReset();
    clearLastMenuItemRenderState();
  }
  return true;
}

bool saveLoadProfile(const Menu::Action_t action) {
  bool isLoad = myMenue.currentItem == &miLoadProfile;
  bool init=false;
  if ((init=(action == Menu::actionTrigger && currentState != Edit)) || action == Menu::actionDisplay) 
  {
    if (init) 
    {
      currentState = Edit;
      tft.setTextColor(ST7735_BLACK, ST7735_WHITE);

      encAbsolute = activeProfileId;      
      tft.setCursor(10, 100);
      tft.print("Doubleclick to exit");
    }
    if (encAbsolute > MAX_PROFILES) encAbsolute = MAX_PROFILES;
    if (encAbsolute <  0) encAbsolute =  0;

    tft.setCursor(10, 80);
    tft.print("Click to ");
    tft.print((isLoad) ? "load " : "override ");
    tft.setTextColor(ST7735_WHITE, ST7735_RED);
    tft.print(encAbsolute);
    tft.setTextColor(ST7735_BLACK, ST7735_WHITE);    
    tft.print("   ");
    tft.setCursor(10, 90);
    tft.print("Profile name: ");
    tft.setTextColor(ST7735_WHITE, ST7735_RED);
    char buffer[PROFILE_NAME_LENGTH];
    loadProfileName(encAbsolute,buffer);
    tft.print(buffer);
    tft.setTextColor(ST7735_BLACK, ST7735_WHITE);    
    tft.print("                           ");
  }
  else if ((action == Menu::actionParent || action == Menu::actionTrigger ) && currentState == Edit) 
  {
    currentState = Settings;
    if(action == Menu::actionTrigger) (isLoad) ? loadProfile(encAbsolute) : saveProfile(encAbsolute);
    clearLastMenuItemRenderState();
  }
  return true;
}


bool cycleStart(const Menu::Action_t action) {
  if (action == Menu::actionTrigger) {

    menuExit(action);
    
    cycleStartTime= esp_timer_get_time();

    if(myMenue.currentItem == &miCycleStart)
    {
      currentState = RampToSoak;      
    }
    else
    {
    	//we want to preheat till we have a stable temprratur
      currentState = PreTune;      
    }

    initialProcessDisplay = true;
    menuUpdateRequest = false;
  }

  return true;
}

void renderMenuItem(const Menu::Item_t *mi, uint8_t pos) {
  bool isCurrent = myMenue.currentItem == mi;
  uint8_t y = pos * MENU_ITEM_HIEGT + 2;

  if (currentlyRenderedItems[pos].mi == mi 
      && currentlyRenderedItems[pos].pos == pos 
      && currentlyRenderedItems[pos].current == isCurrent) 
  {
    return; // don't render the same item in the same state twice
  }

  tft.setCursor(10, y);

  // menu cursor bar
  tft.fillRect(8, y - 2, tft.width() - 16, MENU_ITEM_HIEGT, isCurrent ? ST7735_BLUE : ST7735_WHITE);
  if (isCurrent) tft.setTextColor(ST7735_WHITE, ST7735_BLUE);
  else tft.setTextColor(ST7735_BLACK, ST7735_WHITE);

  tft.print(myMenue.getLabel(mi));

  // show values if in-place editable items
  char buf[20];
  if (getItemValueLabel(mi, buf)) {
    tft.print(' '); tft.print(buf); tft.print("   ");
  }

  // mark items that have children
  if (myMenue.getChild(mi) != &Menu::NullItem) {
    tft.print(" \x10   "); // 0x10 -> filled right arrow
  }

  currentlyRenderedItems[pos].mi = mi;
  currentlyRenderedItems[pos].pos = pos;
  currentlyRenderedItems[pos].current = isCurrent;
}


void alignRightPrefix(uint16_t v) {
  if (v < 1e2) tft.print(' '); 
  if (v < 1e1) tft.print(' ');
}

void updateProcessDisplay() {
  static uint16_t starttime_s=0;
  static float pxPerS;
  static float pxPerC;
  static float estimatedTotalTime;

  const uint8_t h =  86;
  const uint8_t w = 160;
  const uint8_t yOffset =  30; // space not available for graph  
  

  uint16_t dx, dy;
  uint8_t y = 2;

  // header & initial view
  tft.setTextColor(ST7735_WHITE, ST7735_BLUE);

  if (initialProcessDisplay) {
    initialProcessDisplay = false;

    starttime_s=esp_timer_get_time()/1000000;
    
    tft.fillScreen(ST7735_WHITE);
    tft.fillRect(0, 0, tft.width(), MENU_ITEM_HIEGT, ST7735_BLUE);
    tft.setCursor(2, y);
    
    uint16_t maxTemp;
    if(currentState == PreTune)
    {
      tft.print("Tuning ");
      
      estimatedTotalTime=tuningLookbackSec*20;
      maxTemp = 300 ;
    }
    else{
      tft.print("Profile ");
      tft.print(activeProfile.name);

      // estimate total run time for current profile
      estimatedTotalTime = activeProfile.soakDuration + activeProfile.peakDuration;
      estimatedTotalTime += (activeProfile.soakTemp - aktSystemTemperature) / (activeProfile.rampUpRate );
      estimatedTotalTime += (activeProfile.peakTemp - activeProfile.soakTemp) / (activeProfile.rampUpRate );
      estimatedTotalTime += (activeProfile.peakTemp - IDLE_TEMP) / (activeProfile.rampDownRate );
      estimatedTotalTime *=1.2;
      
      maxTemp =  activeProfile.peakTemp * 1.2;
    }
    Serial.print("maxTemp=");Serial.println(maxTemp);
    Serial.print("estimatedTotalTime=");Serial.println(estimatedTotalTime);
    pxPerC =  (float) h / maxTemp;
    pxPerS = 160 / estimatedTotalTime;
    Serial.print("pxPerC=");Serial.println(pxPerC);
    Serial.print("pxPerS=");Serial.println(pxPerS);

    // 50°C grid
    for (uint16_t tg = 0; tg < activeProfile.peakTemp * 1.20; tg += 50) {
      tft.drawFastHLine(0, h - (tg * pxPerC) + yOffset, 160, 0xC618);
    }
    
  }

  // elapsed time
  uint16_t elapsed = (esp_timer_get_time()/1000000 -starttime_s);
  tft.setCursor(125, y);
  if(currentState != Complete)
  {
    alignRightPrefix(elapsed); 
    tft.print(elapsed);
    tft.print("s");    
  }
  

  y += MENU_ITEM_HIEGT + 2;

  tft.setCursor(2, y);
  tft.setTextColor(ST7735_BLACK, ST7735_WHITE);

  // temperature
  tft.setTextSize(2);
  alignRightPrefix((int)aktSystemTemperature);
  printfloat(aktSystemTemperature);
  tft.print("\367C ");  
  tft.setTextSize(1);

  // current state
  y -= 2;
  tft.setCursor(95, y);
  tft.setTextColor(ST7735_BLACK, ST7735_GREEN);
  
  tft.print(currentStateToString());

  tft.print("        "); // lazy: fill up space

  tft.setTextColor(ST7735_BLACK, ST7735_WHITE);

  // set point
  y += 10;
  tft.setCursor(95, y);
  tft.print("Sp:"); 
  alignRightPrefix((int)heaterSetpoint); 
  printfloat(heaterSetpoint);
  tft.print("\367C  ");

  // draw temperature curves
  //
  if(currentState != Complete)
  {
    dx = ((uint16_t)(elapsed * pxPerS))%w;

    // temperature setpoint
    dy = h - (heaterSetpoint * pxPerC) + yOffset;
    tft.drawPixel(dx, dy, ST7735_BLUE);
  
    // actual temperature
    dy = h - (aktSystemTemperature * pxPerC) + yOffset;
    tft.drawPixel(dx, dy, ST7735_RED);
  }
  
  // set values
  tft.setCursor(2, 119);
  uint16_t percent=(uint16_t)heaterOutput*100/256;
  alignRightPrefix(percent); 
  tft.print(percent);
  tft.print('%');

  tft.print("      \x12 "); // alternative: \x7f
  printfloat2(aktSystemTemperatureRamp);
  tft.print("\367C/s    ");
}

void memoryFeedbackScreen(uint8_t profileId, bool loading) {
  tft.fillScreen(ST7735_GREEN);
  tft.setTextColor(ST7735_BLACK);
  tft.setCursor(10, 50);
  tft.print(loading ? "Loading" : "Saving");
  tft.print(" profile ");
  tft.print(profileId);  
}

void saveProfile(unsigned int targetProfile) {
  activeProfileId = targetProfile;

  memoryFeedbackScreen(activeProfileId, false);

  saveParameters(activeProfileId); // activeProfileId is modified by the menu code directly, this method is called by a menu action
  saveLastUsedProfile();
  
  delay(500);
}

void loadProfile(unsigned int targetProfile) {
  memoryFeedbackScreen(targetProfile, true);
  loadParameters(targetProfile);

  // save in any way, as we have no undo
  activeProfileId = targetProfile;
  saveLastUsedProfile();

  delay(500);
}

void makeDefaultProfile() {
  snprintf(activeProfile.name,PROFILE_NAME_LENGTH,"%s","Default");
  activeProfile.soakTemp     = 130;
  activeProfile.soakDuration =  80;
  activeProfile.peakTemp     = 220;
  activeProfile.peakDuration =  40;
  activeProfile.rampUpRate   =   0.80;
  activeProfile.rampDownRate =   2.0;
}
void makeDefaultPID() {
  heaterPID.Kp =  0.60; 
  heaterPID.Ki =  0.01;
  heaterPID.Kd = 19.70;
}

void getProfileKey(uint8_t profile, char * buffer){
  sprintf(buffer,"P%03d",profile);
}

bool saveParameters(uint8_t profile) {
  char buffer[10];
  getProfileKey(profile, buffer);
  Serial.print("Save Profile: ");
  Serial.println(buffer);
  PREF.putBytes(buffer, (uint8_t*)&activeProfile, sizeof(Profile_t));  

  return true;
}

bool loadParameters(uint8_t profile) {
  char buffer[10];
  getProfileKey(profile, buffer);
  Serial.print("Load Profile: ");
  Serial.println(buffer);

  size_t length = PREF.getBytesLength(buffer);
  
  if(length!=sizeof(Profile_t)){
    Serial.println("load default PROFILE!");
    makeDefaultProfile();  
  }
  else
  {
    PREF.getBytes(buffer, (uint8_t*)&activeProfile, length);
  }  

  return true;
}

void loadProfileName(uint8_t id, char * buffer){
  Profile_t temp;
  char profilestring[10];
  getProfileKey(id, profilestring);
  Serial.println(profilestring);

  size_t length = PREF.getBytesLength(profilestring);
  
  if(length!=sizeof(Profile_t)){
    snprintf(buffer,PROFILE_NAME_LENGTH,"%s","--FREE--");
  }
  else
  {
    PREF.getBytes(profilestring, (uint8_t*)&temp, length);
    snprintf(buffer,PROFILE_NAME_LENGTH,"%s",temp.name);    
  }  
}

bool savePID() {
  PREF.putBytes("PID", (uint8_t*)&heaterPID, sizeof(heaterPID));  
  return true;
}

bool loadPID() {
  
  size_t length = PREF.getBytesLength("PID");
  
  if(length!=sizeof(heaterPID)){
    makeDefaultPID();
    Serial.println("load default PID");
  }
  else
  {
    PREF.getBytes("PID", (uint8_t*)&heaterPID, length);
  }  
  return true;  
}


void factoryReset() {
  makeDefaultProfile();

  tft.fillScreen(ST7735_RED);
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(10, 50);
  tft.print("Resetting...");

  
  PREF.clear(); 

  activeProfileId = 0;
  makeDefaultProfile();

  delay(500);
}

void saveLastUsedProfile() {
  PREF.putUChar("ProfileID",activeProfileId);
}

void loadLastUsedProfile() {
  activeProfileId = PREF.getUChar("ProfileID", 0);
  loadParameters(activeProfileId);
}

void setup() {
  //Debug
  Serial.begin(115200);

  //LCD init
  tft.initR(INITR_BLACKTAB);
  tft.setTextWrap(false);
  tft.setTextSize(1);
  tft.setRotation(1);
  tft.fillScreen(ST7735_WHITE);
  tft.setTextColor(ST7735_BLACK, ST7735_WHITE);

  //init timers
  timer_beep.start();
  timer_temp.start();
  timer_dsp_btmln.start();
  timer_display.start();
  timer_control.start();

  //init GPIOs
  pinMode(BTN_13, INPUT);
  pinMode(BTN_4, INPUT);
  pinMode(BTN_3, INPUT);
  pinMode(BTN_14, INPUT);
  pinMode(BTN_11, INPUT);
  pinMode(FAN1, OUTPUT);

  digitalWrite(FAN1, 0);

  btn_startstop.setTapHandler(btn_startstop_tap);
  btn_startstop.begin(BTN_2, INPUT, false, false);

  btn_up.begin(BTN_14, INPUT, false, false);
  btn_up.setTapHandler(btn_up_tap);
  btn_up.setReleasedHandler(btn_up_hold);
  btn_up.setPressedHandler(btn_up_release);

  btn_down.begin(BTN_4, INPUT, false, false);
  btn_down.setTapHandler(btn_down_tap);
  btn_down.setReleasedHandler(btn_down_hold);
  btn_down.setPressedHandler(btn_down_release);

  btn_left.begin(BTN_13, INPUT, false, false);
  btn_left.setTapHandler(btn_left_tap);

  btn_stop.begin(BTN_11, INPUT, false, false);
  btn_stop.setTapHandler(btn_stop_tap);

   //init Wifi:
  WiFi.begin();  

  //init Webserver
  if (MDNS.begin("ReflowController")) {
      Serial.println("MDNS responder started");
  }
  server.on("/", []() {
    server.sendHeader("Cache-Control","no-cache");
    server.send(200, "text/html", ROOT_HTML);
  });
  server.on("/status", []() {
    server.sendHeader("Cache-Control","no-cache");
    char buffer[200];
    unsigned long time = (esp_timer_get_time()-cycleStartTime)/1000;
    snprintf(buffer,200,"{\"time\": %lu, \"temp\": %.2f, \"dt\": %.2f, \"setpoint\":  %.2f, \"power\": %.2f, \"state\": \"%s\"}",time,aktSystemTemperature,aktSystemTemperatureRamp,heaterSetpoint,heaterOutput*100/256,currentStateToString());
    server.send(200, "application/json", buffer);
  });
  serverAction.on("/start", []() {
    serverAction.sendHeader("Cache-Control","no-cache");
    serverAction.sendHeader("Access-Control-Allow-Origin","*");
    if(currentState == Settings)
    {
      //Start Revlow!
      myMenue.navigate(&miCycleStart);
      myMenue.invoke();
      serverAction.send(200, "text/plain", "OK");
    }
    else
    {
      serverAction.send(200, "text/plain", "ERROR");
    }
  });
  serverAction.on("/stop", []() {
    serverAction.sendHeader("Cache-Control","no-cache");
    serverAction.sendHeader("Access-Control-Allow-Origin","*");
    bool ok=false;
    if (currentState == Complete) 
    { 
      menuExit(Menu::actionDisplay); // reset to initial state
      myMenue.navigate(&miCycleStart);
      currentState = Settings;
      menuUpdateRequest = true;
      ok=true;
    }
    else if (currentState == CoolDown) 
    {
      currentState = Complete;
      ok=true;
    }
    else if (currentState > UIMenuEnd) 
    {
      currentState = CoolDown;
      ok=true;
    }
    if(ok){
      serverAction.send(200, "text/plain", "OK");      
    }
    else
    {
      serverAction.send(200, "text/plain", "ERROR");
    }
  });
  server.onNotFound([](){
    server.send(404, "text/plain", "404 :-(");
  });
  serverAction.onNotFound([](){
    serverAction.send(404, "text/plain", "404 :-(");
  });
  
  server.begin();  
  serverAction.begin();  

  
  //INIT Temps
  pinMode(TEMP1_CS, OUTPUT);
  Temp_1.chipSelect = TEMP1_CS;
  
  //init switchPower
  pinMode(HEATER1, OUTPUT);

  ledc_timer_config_t ledc_timer;
  ledc_channel_config_t ledc_channel;
 
  ledc_timer.speed_mode   = LEDC_LOW_SPEED_MODE;
  ledc_timer.timer_num    = LEDC_TIMER_0;
  ledc_timer.duty_resolution       = LEDC_TIMER_8_BIT;
  ledc_timer.freq_hz      = 5;
 
  ledc_channel.channel    = LEDC_CHANNEL_0;
  ledc_channel.gpio_num   = HEATER1;
  ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel.timer_sel  = LEDC_TIMER_0;
  ledc_channel.duty       = 0;
 
  ledc_timer_config(&ledc_timer);
  ledc_channel_config(&ledc_channel);

  //beep
  pinMode(BUZZER,OUTPUT);
  ledcSetup(0,1000,0);
 
  //Preferences init
  PREF.begin("REFLOW");
  loadLastUsedProfile();
  loadPID();          
  
  spashscreen();

  delay(1000);

  menuExit(Menu::actionDisplay); // reset to initial state
  myMenue.navigate(&miCycleStart);
  currentState = Settings;
  menuUpdateRequest = true;
 
  
}

void displayMenusInfos()
{
  //some basic information in the menue
  // set values
  tft.setTextSize(1);
  tft.setTextColor(ST7735_BLACK, ST7735_WHITE);    
  tft.setCursor(2, 119);
  if(WiFi.status() != WL_CONNECTED)
  {                
    tft.print("IP:Not Connnected!");      
  }
  else
  {
    tft.print("IP:");          
    tft.print(WiFi.localIP());          
  }
  tft.print("         ");      
  
  tft.setCursor(115, 119);
  printfloat(aktSystemTemperature);
  tft.print("\367C   ");
}

void btn_startstop_tap(Button2& btn)
{
  if (currentState < UIMenuEnd) 
  {
    myMenue.invoke();
    menuUpdateRequest = true;
  }
  else if (currentState == Complete) 
  { 
    menuExit(Menu::actionDisplay); // reset to initial state
    myMenue.navigate(&miCycleStart);
    currentState = Settings;
    menuUpdateRequest = true;
  }
  else if (currentState == CoolDown) 
  {
    currentState = Complete;
  }
  else if (currentState > UIMenuEnd) 
  {
    currentState = CoolDown;
  }
}

void btn_up_hold(Button2 &btn)
{
  countup = true;
  timer_countupdown.start();
}

void btn_up_release(Button2 &btn)
{
  countup = false;
  timer_countupdown.stop();
  timer_countupdown.reset();
}

void btn_up_tap(Button2 &btn)
{
  encAbsolute -= 1;
  if (currentState == Settings)
  {
    myMenue.navigate(myMenue.getPrev());
    menuUpdateRequest = true;
  }
  else if (currentState == Edit)
  {
    if (myMenue.currentItem != &Menu::NullItem)
    {
      myMenue.executeCallbackAction(Menu::actionDisplay);
    }
  }
}

void btn_down_hold(Button2 &btn)
{
  countdown = true;
  timer_countupdown.start();
}

void btn_down_release(Button2 &btn)
{
  countdown = false;
  timer_countupdown.stop();
  timer_countupdown.reset();
}


void btn_down_tap(Button2 &btn)
{
  encAbsolute += 1;
  if (currentState == Settings)
  {
    myMenue.navigate(myMenue.getNext());
    menuUpdateRequest = true;
  }
  else if (currentState == Edit)
  {
    if (myMenue.currentItem != &Menu::NullItem)
    {
      myMenue.executeCallbackAction(Menu::actionDisplay);
    }
  }
}

void btn_stop_tap(Button2& btn)
{
  reportError("E-Stop");
}

void btn_left_tap(Button2& btn)
{
  Serial.println("DClick");
  Serial.print("currentState: ");
  Serial.println(currentState);
  Serial.print("myMenue.getParent(): ");
  Serial.println((uint32_t)myMenue.getParent());
  Serial.print("&miExit: ");
  Serial.println((uint32_t)&miExit);
  if (currentState == Edit)
  {
    myMenue.executeCallbackAction(Menu::actionParent);
  }
  else if (currentState < UIMenuEnd && myMenue.getParent() != &miExit)
  {
    tft.fillScreen(ST7735_WHITE);
    displayMenusInfos();
    myMenue.navigate(myMenue.getParent());
    menuUpdateRequest = true;
  }
}
void loop()
{
  uint64_t time_ms = esp_timer_get_time()/1000;
  static uint64_t lastbeep=time_ms;
  static uint64_t lastreadTemp=time_ms;
  static uint64_t lastRGBupdate=time_ms;
  static uint64_t lastreadencoder=time_ms;
  static uint64_t lastDisplayUpdate=time_ms;
  static uint64_t lastControlloopupdate=time_ms;

  static uint8_t  beepcount =1; //start beep

  // --------------------------------------------------------------------------
  // Handle internet requests
  //
  server.handleClient();  
  serverAction.handleClient();  

  // --------------------------------------------------------------------------
  // Button counting
  //
  btn_startstop.loop();
  btn_up.loop();
  btn_down.loop();
  btn_left.loop();
  btn_stop.loop();

  if (countdown)
  {
    if (timer_countupdown.done())
    {
      encAbsolute += 2;
      menuUpdateRequest = true;
      if ((myMenue.currentItem != &Menu::NullItem) && (currentState == Edit))
      {
        myMenue.executeCallbackAction(Menu::actionDisplay);
      }
      timer_countupdown.reset();
      timer_countupdown.start();
    }
  }

  if (countup)
  {
    if (timer_countupdown.done())
    {
      encAbsolute -= 2;
      if ((myMenue.currentItem != &Menu::NullItem) && (currentState == Edit))
      {
        myMenue.executeCallbackAction(Menu::actionDisplay);
      }
      menuUpdateRequest = true;
      timer_countupdown.reset();
      timer_countupdown.start();
    }
  }
  // --------------------------------------------------------------------------
  // Do the beep if needed
  //
  if(timer_beep.repeat())
  {
    static boolean isbeeping=false;
    lastbeep=time_ms;
    if(isbeeping==false)
    {
      if(beepcount > 0)
      {
        beepcount--;
        //ledcAttachPin(BUZZER, 0);
        ledcWriteTone(0, 6000);
        isbeeping=true;
      }
    }
    else
    {
      ledcDetachPin(BUZZER);
      isbeeping=false;
    }
  }

  // --------------------------------------------------------------------------
  // Temp messurment and averageing
  //
  if(timer_temp.repeat())
  {
    lastreadTemp+=READ_TEMP_INTERVAL_MS; //interval should be regularly
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


    static float averagees[1000/READ_TEMP_INTERVAL_MS];
    static uint16_t p=0;

    aktSystemTemperatureRamp = aktSystemTemperature - averagees[p];

    averagees[p]=aktSystemTemperature;
    p=(p+1)%(1000/READ_TEMP_INTERVAL_MS);
    
  }

  // --------------------------------------------------------------------------
  // Show temp as RGB color and print display buttom line
  //
  if(timer_dsp_btmln.repeat())
  {
    lastRGBupdate=time_ms;

    float t=(300-aktSystemTemperature)/300.0/2;
    // setLEDRGBBColor(RGB_LED_BRITHNESS_1TO255 * Hue_2_RGB( 0, 1, t+0.33 ),RGB_LED_BRITHNESS_1TO255 * Hue_2_RGB( 0, 1, t ),RGB_LED_BRITHNESS_1TO255 * Hue_2_RGB( 0, 1, t-0.33 ));
    
    if (currentState < UIMenuEnd) 
    {
      displayMenusInfos();
    }
  }

  // --------------------------------------------------------------------------
  // handle menu update
  //
  if (menuUpdateRequest) 
  {
    menuUpdateRequest = false;
    uint64_t dtime=esp_timer_get_time();
    myMenue.render(renderMenuItem, MENUE_ITEMS_VISIBLE);
    
    //Scrollbar
    Menu::Info_t mi = myMenue.getItemInfo(myMenue.currentItem);
    uint8_t sbTop = 0, sbWidth = 4, sbLeft = 160-sbWidth;
    float sbHeight = MENUE_ITEMS_VISIBLE * MENU_ITEM_HIEGT;
    float sbMarkHeight = sbHeight / mi.siblings;
    float sbMarkTop = sbMarkHeight * mi.position;
    tft.fillRect(sbLeft, sbTop,     sbWidth, sbHeight,     ST7735_BLUE);
    tft.fillRect(sbLeft, sbMarkTop, sbWidth, sbMarkHeight, ST7735_RED);
    
    Serial.print("Menue render took: ");
    Serial.print((uint32_t)(esp_timer_get_time() - dtime));
    Serial.println("us!");
  }

  // --------------------------------------------------------------------------
  // update Display
  //
  if(timer_display.repeat())
  {
    if (currentState > UIMenuEnd) {
      uint64_t dtime=esp_timer_get_time();
      updateProcessDisplay();
      Serial.print("Display update took: ");
      Serial.print((uint32_t)(esp_timer_get_time() - dtime));
      Serial.println("us!");
    }
  }



  // --------------------------------------------------------------------------
  // control loop
  //
  if(timer_control.repeat())
  {

    static State previousState= Idle;
    static uint64_t stateChangedTime_ms=time_ms;
    boolean stateChanged=false;
    if (currentState != previousState) 
    {
      stateChangedTime_ms=time_ms;
      stateChanged = true;
      previousState = currentState;
    }
    static float rampToSoakStartTemp;
    static float coolDownStartTemp;
    
    heaterInput = aktSystemTemperature; 

    switch (currentState) 
    {
      case RampToSoak:
        if (stateChanged) 
        {

          rampToSoakStartTemp=aktSystemTemperature;
          heaterSetpoint = rampToSoakStartTemp;

          PID.SetMode(AUTOMATIC);
          PID.SetControllerDirection(DIRECT);
          PID.SetTunings(heaterPID.Kp, heaterPID.Ki, heaterPID.Kd);
        }

        heaterSetpoint = rampToSoakStartTemp + (activeProfile.rampUpRate * (time_ms-stateChangedTime_ms)/1000.0);

        if (heaterSetpoint >= activeProfile.soakTemp) 
        {
          currentState = Soak;
        }
        break;

      case Soak:

        heaterSetpoint = activeProfile.soakTemp;

        if (time_ms - stateChangedTime_ms >= (uint32_t)activeProfile.soakDuration * 1000) 
        {
          currentState = RampUp;
        }
        break;

      case RampUp:

        heaterSetpoint = activeProfile.soakTemp + (activeProfile.rampUpRate * (time_ms-stateChangedTime_ms)/1000.0);

        if (heaterSetpoint >= activeProfile.peakTemp) 
        {
          currentState = Peak;
        }
        break;

      case Peak:

        heaterSetpoint = activeProfile.peakTemp;

        if (time_ms - stateChangedTime_ms >= (uint32_t)activeProfile.peakDuration * 1000) {
          currentState = CoolDown;
        }
        break;

      case CoolDown:
        if (stateChanged) {
          PID.SetMode(MANUAL);

          beepcount=3;  //Beep! We need the door open!!!

          //rampDown from the last setpoint
          coolDownStartTemp=heaterSetpoint;
        }

        heaterSetpoint = coolDownStartTemp - (activeProfile.rampDownRate * (time_ms - stateChangedTime_ms) / 1000.0);
        heaterOutput = 0;

        if (heaterSetpoint < IDLE_TEMP) {
            heaterSetpoint = IDLE_TEMP;
        }
        
        if (aktSystemTemperature < IDLE_TEMP && heaterSetpoint == IDLE_TEMP) {
          currentState = Complete;
          PID.SetMode(MANUAL);

          beepcount=1;  //Beep! We are done!!!

        }
        break;
      case PreTune:
        if(stateChanged)
        {
        	PID.SetMode(MANUAL);
        	heaterSetpoint = aktSystemTemperature;
            heaterOutput = 255*tuningHeaterOutput/100;
        }
        if(heaterSetpoint+tuningNoiseBand <aktSystemTemperature || heaterSetpoint-tuningNoiseBand >aktSystemTemperature ) {
          stateChangedTime_ms=time_ms;
          heaterSetpoint = aktSystemTemperature;
        }
        if (time_ms - stateChangedTime_ms >= tuningLookbackSec*2 * 1000) {
          currentState = Tune;
        }
        break;
      case Tune:
        if (stateChanged) 
        {
          heaterSetpoint = aktSystemTemperature;
          
          PIDTune.Cancel();
          heaterOutput = 255*tuningHeaterOutput/100;
          PIDTune.SetNoiseBand(tuningNoiseBand);
          PIDTune.SetOutputStep(255*tuningOutputStep/100);
          PIDTune.SetLookbackSec(tuningLookbackSec);
          PIDTune.SetControlType(CT_PID_NO_OVERSHOOT); //We want NO Overshoot :-)
        }

        int8_t val = PIDTune.Runtime();

        if (val != 0) 
        {
          currentState = CoolDown;
          heaterPID.Kp = PIDTune.GetKp();
          heaterPID.Ki = PIDTune.GetKi();
          heaterPID.Kd = PIDTune.GetKd();

          savePID();

          tft.setCursor(40, 40);
          tft.print("Kp: "); 
          printfloat2(heaterPID.Kp);
          tft.setCursor(40, 50);
          tft.print("Ki: "); 
          printfloat2(heaterPID.Ki);
          tft.setCursor(40, 60);
          tft.print("Kd: "); 
          printfloat2(heaterPID.Kd);
        }

        break;
    }

    PID.Compute();

    if (
         currentState == RampToSoak ||
         currentState == Soak ||
         currentState == RampUp ||
         currentState == Peak ||
         currentState == PreTune ||
         currentState == Tune          
       )
    {
  
      if (heaterSetpoint+100 < aktSystemTemperature) // if we're 100 degree cooler than setpoint, abort
      {
        reportError("Temperature is Way to HOT!!!!!"); 
      }
      //make it more linear!
      powerHeater = asinelookupTable[(uint8_t)heaterOutput]; 
    } 
    else if(currentState == Edit && myMenue.currentItem==&miManual)
    {
      powerHeater=(encAbsolute*255)/100;
      Serial.print("Manual Heating:");Serial.println(powerHeater);
    }
    else
    {
      powerHeater =0;
    }

    // Set SCR PWM to powerHeater
    //ledcWrite(SCR_PWM_CHANNEL, powerHeater);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, powerHeater);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    if (powerHeater > 0)
    {
      digitalWrite(FAN1, 1);
    }
    else
    {
      digitalWrite(FAN1, 0);
    }
  }

}
