#include "Ui.h"
#include <math.h>

//prototypes
bool menuExit(const Menu::Action_t);
bool cycleStart(const Menu::Action_t);
bool menuDummy(const Menu::Action_t);
bool editNumericalValue(const Menu::Action_t);
bool editProfileName(const Menu::Action_t);
bool saveLoadProfile(const Menu::Action_t);
bool constantTempStart(const Menu::Action_t);
bool constantTempShortcut(const Menu::Action_t);
bool constantBeepShortcut(const Menu::Action_t);
bool manualHeating(const Menu::Action_t);
bool menuWiFi(const Menu::Action_t);
bool factoryReset(const Menu::Action_t);
bool ioDebugScreen(const Menu::Action_t);
bool keyboard(const char * name, char * buffer, uint32_t length, bool init, bool trigger);
bool numericKeyboard(const char * name, char * buffer, uint32_t length, bool init, bool trigger, bool allowDecimal);
bool saveConstTempSettings();
bool loadConstTempSettings();
bool saveControlTuningSettings();
bool loadControlTuningSettings();
void renderMenuItem(const Menu::Item_t *mi, uint8_t pos);
void updateProcessDisplay();
static bool isNumericMenuItem(const Menu::Item_t *mi);
void btn_startstop_tap(Button2& btn);
void btn_startstop_long(Button2& btn);
void btn_up_hold(Button2 &btn);
void btn_up_release(Button2 &btn);
void btn_up_tap(Button2 &btn);
void btn_down_hold(Button2 &btn);
void btn_down_release(Button2 &btn);
void btn_down_tap(Button2 &btn);
void btn_stop_tap(Button2& btn);
void btn_stop_long(Button2& btn);
void btn_left_tap(Button2& btn);
void btn_right_tap(Button2& btn);
void memoryFeedbackScreen(uint8_t profileId, bool loading);
void saveProfile(unsigned int targetProfile);
void loadProfile(unsigned int targetProfile);
void makeDefaultProfile();
void makeDefaultPID();
void getProfileKey(uint8_t profile, char * buffer);
bool saveParameters(uint8_t profile);
bool loadParameters(uint8_t profile);
void loadProfileName(uint8_t id, char * buffer);
bool savePID();
bool loadPID();
void factoryReset();
void saveLastUsedProfile();
void loadLastUsedProfile();

static bool showProcessDetails = false;
static int8_t numericCursorMove = 0;
static bool constBeepEditActive = false;
static bool constBeepEditInit = false;
static bool constBeepEditTrigger = false;
static bool constBeepEditCancel = false;
static int16_t constBeepEditLast = 0;
static char constBeepEditBuf[12];

enum AdcKey {
  ADC_NONE = 0,
  ADC_MINUS,
  ADC_PLUS,
  ADC_TIME,
  ADC_TEMP,
  ADC_UNKNOWN
};

static AdcKey decodeAdcKey(int value) {
  if (value >= 0 && value <= 60) {
    return ADC_MINUS;
  }
  if (value >= 600 && value <= 700) {
    return ADC_PLUS;
  }
  if (value >= 1000 && value <= 1300) {
    return ADC_TIME;
  }
  if (value >= 1500 && value <= 1700) {
    return ADC_TEMP;
  }
  if (value >= 4090) {
    return ADC_NONE;
  }
  return ADC_UNKNOWN;
}

static AdcKey readAdcKeyLevel() {
  AdcKey now = decodeAdcKey(analogRead(BTN_12_ADC));
  if (now == ADC_UNKNOWN) {
    return ADC_NONE;
  }
  return now;
}

static AdcKey readAdcKeyEdge() {
  static AdcKey last = ADC_NONE;
  AdcKey now = readAdcKeyLevel();
  AdcKey edge = ADC_NONE;
  if (now != ADC_NONE && last == ADC_NONE) {
    edge = now;
  }
  if (now == ADC_NONE) {
    last = ADC_NONE;
  } else {
    last = now;
  }
  return edge;
}

// ----------------------------------------------------------------------------------------------------------------------------------------
//       Name,            Label,              Next,               Previous,         Parent,           Child,            Callback
MenuItem(miExit,          "EXIT",             Menu::NullItem,     Menu::NullItem,   Menu::NullItem,   miCycleStart,     menuExit);
  MenuItem(miCycleStart,    "Start Cycle",      miConstTemp,        Menu::NullItem,   miExit,           Menu::NullItem,   cycleStart);
  MenuItem(miConstTemp,     "Constant Temp",   miEditProfile,      miCycleStart,     miExit,           miConstTempStart, menuDummy);
    MenuItem(miConstTempStart,"Start Hold",     miConstTempSet,     Menu::NullItem,   miConstTemp,      Menu::NullItem,   constantTempStart);
    MenuItem(miConstTempSet, "Set Temp",        miConstTemp40,      miConstTempStart, miConstTemp,      Menu::NullItem,   editNumericalValue);
    MenuItem(miConstTemp40,  "Temp 40C",        miConstTemp60,      miConstTempSet,   miConstTemp,      Menu::NullItem,   constantTempShortcut);
    MenuItem(miConstTemp60,  "Temp 60C",        miConstTemp80,      miConstTemp40,    miConstTemp,      Menu::NullItem,   constantTempShortcut);
    MenuItem(miConstTemp80,  "Temp 80C",        miConstBeepMin,     miConstTemp60,    miConstTemp,      Menu::NullItem,   constantTempShortcut);
    MenuItem(miConstBeepMin, "Beep After",      miConstBeepOff,     miConstTemp80,    miConstTemp,      Menu::NullItem,   editNumericalValue);
    MenuItem(miConstBeepOff, "Beep Off",        miConstBeep1h,      miConstBeepMin,   miConstTemp,      Menu::NullItem,   constantBeepShortcut);
    MenuItem(miConstBeep1h,  "Beep 1h",         miConstBeep2h,      miConstBeepOff,   miConstTemp,      Menu::NullItem,   constantBeepShortcut);
    MenuItem(miConstBeep2h,  "Beep 2h",         miConstBeep4h,      miConstBeep1h,    miConstTemp,      Menu::NullItem,   constantBeepShortcut);
    MenuItem(miConstBeep4h,  "Beep 4h",         miConstBeep8h,      miConstBeep2h,    miConstTemp,      Menu::NullItem,   constantBeepShortcut);
    MenuItem(miConstBeep8h,  "Beep 8h",         miConstBeep24h,     miConstBeep4h,    miConstTemp,      Menu::NullItem,   constantBeepShortcut);
    MenuItem(miConstBeep24h, "Beep 24h",        Menu::NullItem,     miConstBeep8h,    miConstTemp,      Menu::NullItem,   constantBeepShortcut);
  MenuItem(miEditProfile,   "Edit Profile",     miLoadProfile,      miConstTemp,      miExit,           miName,            menuDummy);
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
    MenuItem(miPidSettingD,   "Heater Kd",        miGuardSettings,    miPidSettingI,    miPidSettings,    Menu::NullItem,   editNumericalValue);
    MenuItem(miGuardSettings, "Temp Guard",       Menu::NullItem,     miPidSettingD,    miPidSettings,    miGuardLead,      menuDummy);
      MenuItem(miGuardLead,     "Lead Sec",       miGuardHyst,        Menu::NullItem,   miGuardSettings,  Menu::NullItem,   editNumericalValue);
      MenuItem(miGuardHyst,     "Hyst C",         miGuardRise,        miGuardLead,      miGuardSettings,  Menu::NullItem,   editNumericalValue);
      MenuItem(miGuardRise,     "Min Rise",       miGuardMax,         miGuardHyst,      miGuardSettings,  Menu::NullItem,   editNumericalValue);
      MenuItem(miGuardMax,      "Max Set",        miConstSlew,        miGuardRise,      miGuardSettings,  Menu::NullItem,   editNumericalValue);
      MenuItem(miConstSlew,     "Const Slew",     miIntegralBand,     miGuardMax,       miGuardSettings,  Menu::NullItem,   editNumericalValue);
      MenuItem(miIntegralBand,  "I Band C",       Menu::NullItem,     miConstSlew,      miGuardSettings,  Menu::NullItem,   editNumericalValue);
  MenuItem(miManual,        "Manual Heating",   miIODebug,          miPidSettings,    miExit,           Menu::NullItem,   manualHeating);
  MenuItem(miIODebug,       "IO Debug",         miWIFI,             miManual,         miExit,           Menu::NullItem,   ioDebugScreen);
  MenuItem(miWIFI,          "WIFI",             miFactoryReset,     miIODebug,        miExit,           miWIFIUseSaved,   menuWiFi);
    MenuItem(miWIFIUseSaved,  "Connect to Saved", Menu::NullItem,   Menu::NullItem,   miWIFI,           Menu::NullItem,   menuWiFi);
  MenuItem(miFactoryReset,  "Factory Reset",    Menu::NullItem,     miWIFI,           miExit,           Menu::NullItem,   factoryReset);

void uiInit() {
  // Ensure the TFT is fully reset after ESP32 reset (panel may stay powered).
  pinMode(LCD_CS, OUTPUT);
  digitalWrite(LCD_CS, HIGH);
  pinMode(LCD_DC, OUTPUT);
  digitalWrite(LCD_DC, HIGH);
  if (LCD_RESET >= 0) {
    pinMode(LCD_RESET, OUTPUT);
    digitalWrite(LCD_RESET, HIGH);
    delay(5);
    digitalWrite(LCD_RESET, LOW);
    delay(20);
    digitalWrite(LCD_RESET, HIGH);
    delay(120);
  }

  //LCD init
  tft.initR(INITR_BLACKTAB);
  tft.setTextWrap(false);
  tft.setTextSize(1);
  tft.setRotation(1);
  tft.fillScreen(ST7735_WHITE);
  tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
}

void uiInitButtons() {
  //init GPIOs
  pinMode(BTN_13, INPUT);
  pinMode(BTN_4, INPUT);
  pinMode(BTN_3, INPUT);
  pinMode(BTN_14, INPUT);
  pinMode(BTN_11, INPUT);

  btn_startstop.setTapHandler(btn_startstop_tap);
  btn_startstop.setLongClickHandler(btn_startstop_long);
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

  btn_right.begin(BTN_3, INPUT, false, false);
  btn_right.setTapHandler(btn_right_tap);

  btn_stop.begin(BTN_11, INPUT, false, false);
  btn_stop.setTapHandler(btn_stop_tap);
  btn_stop.setLongClickHandler(btn_stop_long);
}

void uiInitBuzzer() {
  pinMode(BUZZER,OUTPUT);
  ledcSetup(0,1000,0);
}

void uiShowSplash() {
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

void uiHandleButtons() {
  // Button counting
  btn_startstop.loop();
  btn_up.loop();
  btn_down.loop();
  btn_left.loop();
  btn_right.loop();
  btn_stop.loop();

  if (currentState != Edit && currentState <= UIMenuEnd) {
    AdcKey key = readAdcKeyEdge();
    if (key == ADC_TEMP) {
      myMenue.navigate(&miConstTempSet);
      myMenue.invoke();
      menuUpdateRequest = true;
    } else if (key == ADC_TIME) {
      myMenue.navigate(&miConstBeepMin);
      myMenue.invoke();
      menuUpdateRequest = true;
    }
  } else if (currentState == ConstantTemp && !constBeepEditActive) {
    AdcKey key = readAdcKeyEdge();
    if (key == ADC_PLUS) {
      constantTempSetpoint += 1;
      if (constantTempSetpoint < 0) constantTempSetpoint = 0;
      saveConstTempSettings();
      menuUpdateRequest = true;
    } else if (key == ADC_MINUS) {
      constantTempSetpoint -= 1;
      if (constantTempSetpoint < 0) constantTempSetpoint = 0;
      saveConstTempSettings();
      menuUpdateRequest = true;
    }
  }

  uiHandleCountupdown();
}

void uiHandleCountupdown() {
  static uint16_t holdTicks = 0;
  if (countdown)
  {
    if (timer_countupdown.done())
    {
      holdTicks++;
      int step = 1;
      if (holdTicks > 20) step = 10;
      else if (holdTicks > 12) step = 5;
      else if (holdTicks > 6) step = 2;
      if ((currentState == Edit &&
           myMenue.currentItem != &Menu::NullItem &&
           isNumericMenuItem(myMenue.currentItem)) ||
          constBeepEditActive) {
        numericCursorMove += step;
      } else {
        encAbsolute += 2;
      }
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
      holdTicks++;
      int step = 1;
      if (holdTicks > 20) step = 10;
      else if (holdTicks > 12) step = 5;
      else if (holdTicks > 6) step = 2;
      if ((currentState == Edit &&
           myMenue.currentItem != &Menu::NullItem &&
           isNumericMenuItem(myMenue.currentItem)) ||
          constBeepEditActive) {
        numericCursorMove -= step;
      } else {
        encAbsolute -= 2;
      }
      if ((myMenue.currentItem != &Menu::NullItem) && (currentState == Edit))
      {
        myMenue.executeCallbackAction(Menu::actionDisplay);
      }
      menuUpdateRequest = true;
      timer_countupdown.reset();
      timer_countupdown.start();
    }
  }

  if (!countdown && !countup) {
    holdTicks = 0;
  }
}

void uiUpdateBeep() {
  if (!timer_beep.repeat()) {
    return;
  }
  static boolean isbeeping=false;
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

void uiUpdateBottomLine() {
  if (!timer_dsp_btmln.repeat()) {
    return;
  }
  if (currentState == Settings) {
    displayMenusInfos();
  } else if (currentState == Edit && myMenue.currentItem != &Menu::NullItem) {
    if (isNumericMenuItem(myMenue.currentItem)) {
      myMenue.executeCallbackAction(Menu::actionDisplay);
    }
  }
}

void uiUpdateMenu() {
  if (!menuUpdateRequest) {
    return;
  }
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

void uiUpdateProcessDisplay() {
  if (!timer_display.repeat()) {
    return;
  }
  if (currentState > UIMenuEnd) {
    uint64_t dtime=esp_timer_get_time();
    updateProcessDisplay();
    Serial.print("Display update took: ");
    Serial.print((uint32_t)(esp_timer_get_time() - dtime));
    Serial.println("us!");
  } else if (currentState == Edit && myMenue.currentItem == &miIODebug) {
    ioDebugScreen(Menu::actionDisplay);
  }
}

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

bool numericKeyboard(const char * name, char * buffer, uint32_t length, bool init, bool trigger, bool allowDecimal)
{
  bool finisched=false;
  static uint32_t p;
  static uint32_t cursor;
  if(init)
  {
    p=0;
    encAbsolute=0;
    trigger=false;
    tft.fillScreen(ST7735_WHITE);    
    tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
    tft.setTextSize(1);
    tft.setCursor(2, 10);
    tft.print(name);
    tft.print(":");
    p = strlen(buffer);
    if (p >= length) {
      p = length > 0 ? length - 1 : 0;
      buffer[p] = 0;
    }
    cursor = p;
  }

  static int numElements=0;
  static int lastencAbsolute=0;
  char start[] =  {'0', 1, 3, 4, '.', 2 };
  char end[]   =  {'9', 1, 3, 4, '.', 2 };
  int y[]      =  {50,  70, 70, 70, 90, 90};
  int x[]      =  {2,   2,  70, 110, 70, 110};
  int spacing[]=  {14,  0,  0,  0,  0,  0};
  int akt=0;  
  
  if(encAbsolute<0) encAbsolute=0;
  if(encAbsolute>numElements) encAbsolute=numElements;
  
  for(int i=0;i<sizeof(start);i++){
    for(char c=start[i];c<=end[i];c++)
    {
      if(c=='.' && !allowDecimal) continue;
      tft.setCursor(x[i]+(c-start[i])*spacing[i], y[i]);
      if(init || encAbsolute== akt || lastencAbsolute== akt){
        if(encAbsolute== akt){
          tft.setTextColor(ST7735_WHITE, ST7735_BLUE);          
        }
        else{
          tft.setTextColor(ST7735_BLACK, ST7735_WHITE);          
        }
        switch(c){
          case 1:
            tft.print("DEL");
            if(trigger) 
            {
              if (cursor > 0) {
                for (uint32_t i = cursor - 1; i < p; ++i) {
                  buffer[i] = buffer[i + 1];
                }
                cursor--;
                p--;
              }
            }
            break;
          case 2:
            tft.print("OK");
            if(trigger) 
            {
              finisched=true;
            }
            break;
          case 3:
            tft.print("<<");
            if (trigger && cursor > 0) {
              cursor--;
            }
            break;
          case 4:
            tft.print(">>");
            if (trigger && cursor < p) {
              cursor++;
            }
            break;
          case '.':
            tft.print('.');
            if(trigger) 
            {
              if (allowDecimal && p<(length-1)) {
                bool hasDot=false;
                for(uint32_t i=0;i<p;i++){
                  if(buffer[i]=='.'){ hasDot=true; break; }
                }
                if(!hasDot){
                  for (uint32_t i = p; i > cursor; --i) {
                    buffer[i] = buffer[i - 1];
                  }
                  buffer[cursor++]='.';
                  buffer[++p]=0;
                }
              }
            }
            break;
          default:
            tft.print(c);
            if(trigger) 
            {
              if (p<(length-1)){
                for (uint32_t i = p; i > cursor; --i) {
                  buffer[i] = buffer[i - 1];
                }
                buffer[cursor++]=c;
                buffer[++p]=0;                
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
  char display[32];
  uint32_t maxLen = length < (sizeof(display) - 1) ? length : (sizeof(display) - 1);
  uint32_t out = 0;
  for (uint32_t i = 0; i < p && out < maxLen - 1; ++i) {
    if (i == cursor && out < maxLen - 1) {
      display[out++] = '|';
    }
    display[out++] = buffer[i];
  }
  if (cursor >= p && out < maxLen) {
    display[out++] = '|';
  }
  display[out] = 0;
  tft.print(display);
  for(uint32_t i=out;i<maxLen;i++)
  {
    tft.print(" ");
  }
  tft.setTextWrap(false);  
  tft.setTextColor(ST7735_BLACK, ST7735_WHITE);  
  
  return finisched;
}

static float pow10f_int(int exp) {
  float v = 1.0f;
  if (exp >= 0) {
    for (int i = 0; i < exp; ++i) v *= 10.0f;
  } else {
    for (int i = 0; i < -exp; ++i) v *= 0.1f;
  }
  return v;
}

static uint32_t clampCursorToDigit(const char *buffer, uint32_t len, int32_t cursor) {
  if (len == 0) return 0;
  if (cursor < 0) cursor = 0;
  if ((uint32_t)cursor >= len) cursor = (int32_t)len - 1;
  if (buffer[cursor] >= '0' && buffer[cursor] <= '9') {
    return (uint32_t)cursor;
  }
  int32_t left = cursor - 1;
  int32_t right = cursor + 1;
  while (left >= 0 || (uint32_t)right < len) {
    if (left >= 0 && buffer[left] >= '0' && buffer[left] <= '9') {
      return (uint32_t)left;
    }
    if ((uint32_t)right < len && buffer[right] >= '0' && buffer[right] <= '9') {
      return (uint32_t)right;
    }
    left--;
    right++;
  }
  return 0;
}

static void applyNumericStep(char *buffer, uint32_t length, uint32_t &cursor, uint8_t decimals, int dir) {
  float value = atof(buffer);
  uint32_t len = strlen(buffer);
  int32_t dot = -1;
  for (uint32_t i = 0; i < len; ++i) {
    if (buffer[i] == '.') {
      dot = (int32_t)i;
      break;
    }
  }
  float step = 1.0f;
  if (dot < 0) {
    int32_t digitsRight = (int32_t)len - 1 - (int32_t)cursor;
    if (digitsRight < 0) digitsRight = 0;
    step = pow10f_int(digitsRight);
  } else {
    if ((int32_t)cursor < dot) {
      int32_t digitsRight = (dot - 1 - (int32_t)cursor);
      if (digitsRight < 0) digitsRight = 0;
      step = pow10f_int(digitsRight);
    } else {
      int32_t decPos = (int32_t)cursor - dot;
      if (decPos < 1) decPos = 1;
      step = pow10f_int(-decPos);
    }
  }

  float newValue = value + (dir > 0 ? step : -step);
  if (newValue < 0.0f) newValue = 0.0f;
  if (decimals > 0) {
    float scale = pow10f_int(decimals);
    newValue = roundf(newValue * scale) / scale;
    char fmt[10];
    snprintf(fmt, sizeof(fmt), "%%.%uf", (unsigned int)decimals);
    snprintf(buffer, length, fmt, newValue);
  } else {
    int32_t iv = (int32_t)roundf(newValue);
    if (iv < 0) iv = 0;
    snprintf(buffer, length, "%ld", (long)iv);
  }

  uint32_t newLen = strlen(buffer);
  if (dot >= 0) {
    int32_t newDot = -1;
    for (uint32_t i = 0; i < newLen; ++i) {
      if (buffer[i] == '.') {
        newDot = (int32_t)i;
        break;
      }
    }
    if (newDot >= 0) {
      if ((int32_t)cursor < dot) {
        cursor = clampCursorToDigit(buffer, newLen, newDot - (dot - (int32_t)cursor));
      } else {
        cursor = clampCursorToDigit(buffer, newLen, newDot + ((int32_t)cursor - dot));
      }
    } else {
      cursor = clampCursorToDigit(buffer, newLen, (int32_t)cursor);
    }
  } else {
    int32_t posFromRight = (int32_t)len - 1 - (int32_t)cursor;
    cursor = clampCursorToDigit(buffer, newLen, (int32_t)newLen - 1 - posFromRight);
  }
}

static bool numericDigitEditor(const char *name, char *buffer, uint32_t length, bool init, bool trigger, bool allowDecimal, uint8_t decimals) {
  bool finished = false;
  static uint32_t cursor = 0;
  static AdcKey heldKey = ADC_NONE;
  static uint32_t heldSince = 0;
  static uint32_t lastRepeat = 0;
  if (init) {
    numericCursorMove = 0;
    heldKey = ADC_NONE;
    heldSince = 0;
    lastRepeat = 0;
    tft.setTextSize(1);
    tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
    tft.fillScreen(ST7735_WHITE);
    tft.setCursor(2, 10);
    tft.print(name);
    tft.print(":");
    if (buffer[0] == 0) {
      snprintf(buffer, length, "0");
    }
    uint32_t len = strlen(buffer);
    cursor = clampCursorToDigit(buffer, len, (int32_t)(len ? len - 1 : 0));
  }

  uint32_t now = millis();
  AdcKey edge = readAdcKeyEdge();
  if (edge == ADC_PLUS) {
    applyNumericStep(buffer, length, cursor, decimals, 1);
    heldKey = ADC_PLUS;
    heldSince = now;
    lastRepeat = now;
  } else if (edge == ADC_MINUS) {
    applyNumericStep(buffer, length, cursor, decimals, -1);
    heldKey = ADC_MINUS;
    heldSince = now;
    lastRepeat = now;
  }

  AdcKey level = readAdcKeyLevel();
  if (level != ADC_PLUS && level != ADC_MINUS) {
    heldKey = ADC_NONE;
  } else if (level == heldKey) {
    uint32_t heldMs = now - heldSince;
    uint32_t interval = 0;
    if (heldMs > 1200) interval = 80;
    else if (heldMs > 500) interval = 150;
    else interval = 400;
    if (now - lastRepeat >= interval) {
      applyNumericStep(buffer, length, cursor, decimals, heldKey == ADC_PLUS ? 1 : -1);
      lastRepeat = now;
    }
  }

  if (numericCursorMove != 0) {
    int32_t dir = numericCursorMove > 0 ? 1 : -1;
    numericCursorMove = 0;
    uint32_t len = strlen(buffer);
    int32_t next = (int32_t)cursor + dir;
    while (next >= 0 && (uint32_t)next < len) {
      if (buffer[next] >= '0' && buffer[next] <= '9') {
        cursor = (uint32_t)next;
        break;
      }
      next += dir;
    }
  }

  if (trigger) {
    finished = true;
  }

  tft.setCursor(2, 30);
  tft.setTextColor(ST7735_WHITE, ST7735_RED);
  tft.setTextWrap(false);
  char display[32];
  uint32_t len = strlen(buffer);
  uint32_t maxLen = (length < (sizeof(display) - 1)) ? length : (sizeof(display) - 1);
  uint32_t out = 0;
  uint32_t caretIndex = 0;
  for (uint32_t i = 0; i < len && out < maxLen - 1; ++i) {
    if (i == cursor && out < maxLen - 1) {
      caretIndex = out;
      display[out++] = '^';
    }
    display[out++] = buffer[i];
  }
  if (cursor >= len && out < maxLen) {
    caretIndex = out;
    display[out++] = '^';
  }
  display[out] = 0;
  uint8_t maxChars = (tft.width() > 2) ? (tft.width() - 2) / 6 : 1;
  if (maxChars < 1) maxChars = 1;
  uint32_t fullLen = out;
  uint32_t start = 0;
  if (fullLen > maxChars) {
    if (caretIndex >= maxChars) {
      start = caretIndex - maxChars + 1;
    }
    if (start > fullLen - maxChars) {
      start = fullLen - maxChars;
    }
  }
  uint32_t visible = fullLen - start;
  if (visible > maxChars) visible = maxChars;
  for (uint32_t i = 0; i < visible; ++i) {
    tft.print(display[start + i]);
  }
  for (uint32_t i = visible; i < maxChars; ++i) {
    tft.print(" ");
  }
  tft.setTextColor(ST7735_BLACK, ST7735_WHITE);

  tft.fillRect(0, 50, tft.width(), 16, ST7735_WHITE);
  tft.setCursor(2, 50);
  tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
  tft.print("Up/Down: Move  +/-: Adj");
  tft.setCursor(2, 60);
  tft.print("Start:OK  Back:Cancel");

  return finished;
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

static void updateTextField(uint16_t fillX, uint16_t fillY, uint16_t fillW, uint16_t fillH,
                            uint16_t cursorX, uint16_t cursorY, const char *text,
                            char *last, size_t lastLen) {
  if (strncmp(text, last, lastLen) == 0) {
    return;
  }
  tft.fillRect(fillX, fillY, fillW, fillH, ST7735_WHITE);
  tft.setCursor(cursorX, cursorY);
  tft.print(text);
  strncpy(last, text, lastLen - 1);
  last[lastLen - 1] = '\0';
}

static void formatTimeShort(char *buf, size_t len, uint32_t seconds) {
  uint32_t hh = seconds / 3600;
  uint32_t mm = (seconds % 3600) / 60;
  uint32_t ss = seconds % 60;
  if (hh > 0) {
    snprintf(buf, len, "%02lu:%02lu", (unsigned long)hh, (unsigned long)mm);
  } else {
    snprintf(buf, len, "%02lu:%02lu", (unsigned long)mm, (unsigned long)ss);
  }
}

static void formatTimeLong(char *buf, size_t len, uint32_t seconds) {
  uint32_t hh = seconds / 3600;
  uint32_t mm = (seconds % 3600) / 60;
  uint32_t ss = seconds % 60;
  snprintf(buf, len, "%02lu:%02lu:%02lu", (unsigned long)hh, (unsigned long)mm, (unsigned long)ss);
}

static uint16_t stateBarColor(State state) {
  switch (state) {
    case RampToSoak:
    case RampUp:
    case Peak:
    case ConstantTemp:
      return ST7735_ORANGE;
    case Soak:
      return ST7735_YELLOW;
    case CoolDown:
      return ST7735_CYAN;
    case Complete:
      return ST7735_GREEN;
    case PreTune:
    case Tune:
      return ST7735_MAGENTA;
    default:
      return ST7735_BLUE;
  }
}

static uint16_t stateBarTextColor(State state) {
  switch (state) {
    case Soak:
      return ST7735_BLACK;
    default:
      return ST7735_WHITE;
  }
}

static const char *shortStateLabel(State state) {
  switch (state) {
    case RampToSoak: return "RAMP TO SOAK";
    case Soak: return "SOAK";
    case RampUp: return "RAMP UP";
    case Peak: return "PEAK";
    case CoolDown: return "COOL";
    case Complete: return "COMPLETE";
    case ConstantTemp: return "CONST TEMP";
    case PreTune: return "PRE-TUNE";
    case Tune: return "TUNE";
    default: return "RUN";
  }
}

void getItemValuePointer(const Menu::Item_t *mi, float **d, int16_t **i) {
  if (mi == &miRampUpRate)  *d = &activeProfile.rampUpRate;
  if (mi == &miRampDnRate)  *d = &activeProfile.rampDownRate;
  if (mi == &miSoakTime)    *i = &activeProfile.soakDuration;
  if (mi == &miSoakTemp)    *i = &activeProfile.soakTemp;
  if (mi == &miPeakTime)    *i = &activeProfile.peakDuration;
  if (mi == &miPeakTemp)    *i = &activeProfile.peakTemp;
  if (mi == &miConstTempSet) *i = &constantTempSetpoint;
  if (mi == &miConstBeepMin) *i = &constantTempBeepMinutes;
  if (mi == &miPidSettingP) *d = &heaterPID.Kp;
  if (mi == &miPidSettingI) *d = &heaterPID.Ki;
  if (mi == &miPidSettingD) *d = &heaterPID.Kd; 
  if (mi == &miGuardLead)   *d = &inertiaGuardLeadSec;
  if (mi == &miGuardHyst)   *d = &inertiaGuardHysteresisC;
  if (mi == &miGuardRise)   *d = &inertiaGuardMinRiseCps;
  if (mi == &miGuardMax)    *d = &inertiaGuardMaxSetpointC;
  if (mi == &miConstSlew)   *d = &constTempRampCps;
  if (mi == &miIntegralBand)*d = &integralEnableBandC;
  if (mi == &miHeaterOutput)*i = &tuningHeaterOutput;
  if (mi == &miNoiseBand)   *i = &tuningNoiseBand;
  if (mi == &miOutputStep)  *i = &tuningOutputStep;
  if (mi == &miLookbackSec) *i = &tuningLookbackSec;
}

bool isPidSetting(const Menu::Item_t *mi) {
  return mi == &miPidSettingP || mi == &miPidSettingI || mi == &miPidSettingD;
}

static bool isNumericMenuItem(const Menu::Item_t *mi) {
  int16_t *iValue = NULL;
  float *dValue = NULL;
  getItemValuePointer(mi, &dValue, &iValue);
  return iValue != NULL || dValue != NULL;
}

bool isControlTuningSetting(const Menu::Item_t *mi) {
  return mi == &miGuardLead ||
         mi == &miGuardHyst ||
         mi == &miGuardRise ||
         mi == &miGuardMax ||
         mi == &miConstSlew ||
         mi == &miIntegralBand;
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
  else if (mi == &miGuardLead) {
    sprintf(label,"%.2fs",*dValue);
  }
  else if (mi == &miGuardHyst) {
    sprintf(label,"%.2f\367C",*dValue);
  }
  else if (mi == &miGuardRise || mi == &miConstSlew) {
    sprintf(label,"%.2f\367C/s",*dValue);
  }
  else if (mi == &miGuardMax) {
    sprintf(label,"%.2f\367C",*dValue);
  }
  else if (mi == &miIntegralBand) {
    sprintf(label,"%.2f\367C",*dValue);
  }
  else if (mi == &miPeakTemp || mi == &miSoakTemp) {
    sprintf(label,"%d\367C",*iValue);        
  }
  else if (mi == &miConstTempSet) {
    sprintf(label,"%d\367C",*iValue);
  }
  else if (mi == &miPeakTime || mi == &miSoakTime || mi == &miLookbackSec) {
    sprintf(label,"%ds",*iValue);        
  }
  else if (mi == &miConstBeepMin) {
    if (*iValue <= 0) {
      sprintf(label,"off");
    } else {
      sprintf(label,"%dm",*iValue);
    }
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
  static char    buffer[20];
  int16_t *iValue = NULL;
  float  *dValue = NULL;
  bool init=false;
  bool trigger=false;
  getItemValuePointer(myMenue.currentItem, &dValue, &iValue);
  if ((init=(action == Menu::actionTrigger && currentState != Edit)) || action == Menu::actionDisplay || (trigger=(action == Menu::actionTrigger && currentState == Edit)))
  {
    if (init) 
    {
      currentState = Edit;
      //save last Value
      if(iValue!=NULL) iValueLast=*iValue;
      if(dValue!=NULL) dValueLast=*dValue;
      if (dValue!=NULL) {
        if (isPidSetting(myMenue.currentItem) || isControlTuningSetting(myMenue.currentItem)) {
          snprintf(buffer, sizeof(buffer), "%.3f", *dValue);
        } else {
          snprintf(buffer, sizeof(buffer), "%.1f", *dValue);
        }
      } else if (iValue!=NULL) {
        snprintf(buffer, sizeof(buffer), "%d", *iValue);
      } else {
        buffer[0]=0;
      }
    }
    bool allowDecimal = (isRampSetting(myMenue.currentItem) ||
                         isPidSetting(myMenue.currentItem) ||
                         isControlTuningSetting(myMenue.currentItem));
    uint8_t decimals = 0;
    if (allowDecimal) {
      decimals = (isPidSetting(myMenue.currentItem) || isControlTuningSetting(myMenue.currentItem)) ? 3 : 1;
    }
    if (numericDigitEditor(myMenue.getLabel(myMenue.currentItem), buffer, sizeof(buffer), init, trigger, allowDecimal, decimals))
    {
      if(buffer[0]==0){
        if(iValue!=NULL) *iValue=iValueLast;
        if(dValue!=NULL) *dValue=dValueLast;
      }
      else if (dValue!=NULL) {
        *dValue = atof(buffer);
      } else if (iValue!=NULL) {
        *iValue = atoi(buffer);
      }
      if (isPidSetting(myMenue.currentItem)) {
        savePID();
      }
      if (isControlTuningSetting(myMenue.currentItem)) {
        clampControlTuningValues();
        saveControlTuningSettings();
      }
      if (myMenue.currentItem == &miConstTempSet || myMenue.currentItem == &miConstBeepMin) {
        saveConstTempSettings();
      }
      currentState = Settings;
      clearLastMenuItemRenderState();
    }
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

bool ioDebugScreen(const Menu::Action_t action) {
  bool init = false;
  if ((init = (action == Menu::actionTrigger && currentState != Edit)) || action == Menu::actionDisplay) {
    if (init) {
      currentState = Edit;
      tft.setTextSize(1);
      tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
      tft.fillScreen(ST7735_WHITE);
      tft.setCursor(2, 0);
      tft.print("IO DEBUG");
      tft.setCursor(90, 0);
      tft.print("Left=Back");
    }

    const int adcButtons = analogRead(BTN_12_ADC);
    const int adcNtc = analogRead(NTC_ADC);

    tft.fillRect(0, 12, 160, 116, ST7735_WHITE);
    uint8_t y = 14;

    const char *adcLabel = "Open";
    if (adcButtons >= 0 && adcButtons <= 60) {
      adcLabel = "-";
    } else if (adcButtons >= 600 && adcButtons <= 700) {
      adcLabel = "+";
    } else if (adcButtons >= 1000 && adcButtons <= 1300) {
      adcLabel = "Time";
    } else if (adcButtons >= 1500 && adcButtons <= 1700) {
      adcLabel = "Temp";
    } else if (adcButtons < 4095) {
      adcLabel = "Unknown";
    }

    tft.setCursor(2, y);
    tft.print("ADC1_0:");
    tft.print(adcButtons);
    tft.print(" ");
    tft.print(adcLabel);
    y += 10;

    tft.setCursor(2, y);
    tft.print("NTC ADC:");
    tft.print(adcNtc);
    y += 10;

    tft.setCursor(2, y);
    tft.print("HEAT:");
    tft.print(powerHeater);
    tft.print(" FAN:");
    tft.print(digitalRead(FAN1));
    y += 10;

    tft.setCursor(2, y);
    tft.print("ZC:");
    tft.print(digitalRead(ZEROX));
    tft.print(" SS:");
    tft.print(digitalRead(BTN_2));
    y += 10;

    tft.setCursor(2, y);
    tft.print("W:");
    tft.print(digitalRead(BTN_13));
    tft.print(" G:");
    tft.print(digitalRead(BTN_4));
    tft.print(" T:");
    tft.print(digitalRead(BTN_3));
    y += 10;

    tft.setCursor(2, y);
    tft.print("B:");
    tft.print(digitalRead(BTN_14));
    tft.print(" C:");
    tft.print(digitalRead(BTN_11));
    tft.print(" ENC:");
    tft.print(digitalRead(ENC_B));
    y += 10;

    tft.setCursor(2, y);
    tft.print("ENC A:");
    tft.print(digitalRead(ENC1));
    tft.print(" B:");
    tft.print(digitalRead(ENC2));
    y += 10;

    tft.setCursor(2, y);
    tft.print("ENC ABS:");
    tft.print(encAbsolute);
  } else if ((action == Menu::actionParent || action == Menu::actionTrigger) && currentState == Edit) {
    currentState = Settings;
    clearLastMenuItemRenderState();
  }
  return true;
}

bool menuWiFi(const Menu::Action_t action){
  static int wifiItemsCount=0;
  static Menu::Item_t wifiItems[20];
  static char wifiLabels[20][24];
  if(action==Menu::actionTrigger && myMenue.currentItem == &miWIFI)
  {
    //enter Wifi Menu:
    tft.fillScreen(ST7735_RED);
    tft.setTextColor(ST7735_WHITE);
    tft.setCursor(10, 50);
    tft.print("Scanning...");

    //Search for networks
    wifiItemsCount = WiFi.scanNetworks();
    Serial.print("Wifis found: ");Serial.println(wifiItemsCount);
    
    if(wifiItemsCount==WIFI_SCAN_FAILED){
      wifiItemsCount=1;
      wifiItems[0].Label="WIFI_SCAN_FAILED!";
      wifiItems[0].Callback=NULL;              
    }
    else if (wifiItemsCount == 0) 
    {      
      wifiItemsCount=1;
      wifiItems[0].Label="No WiFis found!";
      wifiItems[0].Callback=NULL;        
    } 
    else 
    {
      if (wifiItemsCount > 20) wifiItemsCount = 20;
      for (int i = 0; i < wifiItemsCount; ++i) 
      {
        String name=WiFi.SSID(i);
        name.toCharArray(wifiLabels[i],24);
        if(name.length()>23){
          wifiLabels[i][20]='.';
          wifiLabels[i][21]='.';
          wifiLabels[i][22]='.';
          wifiLabels[i][23]=0;
        }
        wifiItems[i].Label=wifiLabels[i];
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
    systemPaused = false;
    
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

bool constantTempStart(const Menu::Action_t action) {
  if (action == Menu::actionTrigger) {
    menuExit(action);
    systemPaused = false;
    cycleStartTime= esp_timer_get_time();
    currentState = ConstantTemp;
    initialProcessDisplay = true;
    menuUpdateRequest = false;
  }
  return true;
}

bool constantTempShortcut(const Menu::Action_t action) {
  if (action == Menu::actionTrigger) {
    if (myMenue.currentItem == &miConstTemp40) constantTempSetpoint = 40;
    if (myMenue.currentItem == &miConstTemp60) constantTempSetpoint = 60;
    if (myMenue.currentItem == &miConstTemp80) constantTempSetpoint = 80;
    saveConstTempSettings();
    clearLastMenuItemRenderState();
  }
  return true;
}

bool constantBeepShortcut(const Menu::Action_t action) {
  if (action == Menu::actionTrigger) {
    if (myMenue.currentItem == &miConstBeepOff) constantTempBeepMinutes = 0;
    if (myMenue.currentItem == &miConstBeep1h) constantTempBeepMinutes = 60;
    if (myMenue.currentItem == &miConstBeep2h) constantTempBeepMinutes = 120;
    if (myMenue.currentItem == &miConstBeep4h) constantTempBeepMinutes = 240;
    if (myMenue.currentItem == &miConstBeep8h) constantTempBeepMinutes = 480;
    if (myMenue.currentItem == &miConstBeep24h) constantTempBeepMinutes = 1440;
    saveConstTempSettings();
    clearLastMenuItemRenderState();
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

  bool suppressValue = false;
  if (mi == &miConstBeepMin && constantTempBeepMinutes <= 0) {
    tft.print("Beep Off");
    suppressValue = true;
  } else {
    tft.print(myMenue.getLabel(mi));
  }

  // show values if in-place editable items
  char buf[20];
  if (!suppressValue && getItemValueLabel(mi, buf)) {
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
  static float pxPerS;
  static float pxPerC;
  static float estimatedTotalTime;
  static uint16_t maxTemp;
  static State lastBarState = None;
  static bool lastPaused = false;
  static bool lastShowDetails = false;
  static char lastTempBuf[16];
  static char lastSpBuf[20];
  static char lastRemBuf[16];
  static char lastPowerBuf[12];
  static char lastPidOutBuf[32];
  static uint16_t lastPowerPercent = 0xFFFF;
  static uint8_t lastHeaterOn = 2;
  static uint8_t lastFanStatus = 2;
  static char lastDetailLines[8][32];
  static bool lastDetailWasConst = false;
  static uint32_t lastGraphElapsed = 0;

  const uint8_t screenW = tft.width();
  const uint8_t screenH = tft.height();
  const uint8_t barH = MENU_ITEM_HIEGT;
  const uint8_t bottomH = 10;
  const uint8_t bigTextY = barH + 2;
  const uint8_t bigTextH = 16;
  const uint8_t infoY = bigTextY + bigTextH + 2;
  const uint8_t infoH = 8;
  const uint8_t graphTop = infoY + infoH + 2;
  const uint8_t graphBottom = screenH - bottomH - 2;
  const uint16_t graphH = graphBottom - graphTop;
  const uint16_t graphW = screenW;
  const bool paused = systemPaused;

  if (constBeepEditActive) {
    if (currentState != ConstantTemp) {
      constBeepEditActive = false;
      constBeepEditCancel = false;
      constBeepEditTrigger = false;
      initialProcessDisplay = true;
      return;
    }
    if (constBeepEditInit) {
      constBeepEditLast = constantTempBeepMinutes;
      if (constBeepEditLast < 0) constBeepEditLast = 0;
      snprintf(constBeepEditBuf, sizeof(constBeepEditBuf), "%d", constBeepEditLast);
    }
    bool done = numericDigitEditor("Beep After (min)", constBeepEditBuf, sizeof(constBeepEditBuf),
                                   constBeepEditInit, constBeepEditTrigger, false, 0);
    constBeepEditInit = false;
    constBeepEditTrigger = false;
    if (constBeepEditCancel) {
      constantTempBeepMinutes = constBeepEditLast;
      constBeepEditCancel = false;
      constBeepEditActive = false;
      initialProcessDisplay = true;
      return;
    }
    if (done) {
      if (constBeepEditBuf[0] == 0) {
        constantTempBeepMinutes = constBeepEditLast;
      } else {
        constantTempBeepMinutes = atoi(constBeepEditBuf);
        if (constantTempBeepMinutes < 0) constantTempBeepMinutes = 0;
      }
      saveConstTempSettings();
      constBeepEditActive = false;
      initialProcessDisplay = true;
      return;
    }
    return;
  }

  if (initialProcessDisplay) {
    initialProcessDisplay = false;

    tft.fillScreen(ST7735_WHITE);
    lastBarState = None;
    lastShowDetails = showProcessDetails;
    lastTempBuf[0] = '\0';
    lastSpBuf[0] = '\0';
    lastRemBuf[0] = '\0';
    lastPowerBuf[0] = '\0';
    lastPidOutBuf[0] = '\0';
    lastPowerPercent = 0xFFFF;
    lastHeaterOn = 2;
    lastFanStatus = 2;
    for (uint8_t i = 0; i < 8; i++) {
      lastDetailLines[i][0] = '\0';
    }
    lastDetailWasConst = false;
    lastGraphElapsed = 0;
    
    if(currentState == PreTune)
    {
      estimatedTotalTime=tuningLookbackSec*20;
      maxTemp = 300 ;
    }
    else if (currentState == ConstantTemp)
    {
      estimatedTotalTime = (constantTempBeepMinutes > 0) ? (constantTempBeepMinutes * 60) : 3600;
      maxTemp =  max(100, (int)(constantTempSetpoint * 1.2));
    }
    else{
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
    pxPerC =  (float) graphH / maxTemp;
    pxPerS = (graphW - 1) / max(1.0f, estimatedTotalTime);
    Serial.print("pxPerC=");Serial.println(pxPerC);
    Serial.print("pxPerS=");Serial.println(pxPerS);

    // state bar
    uint16_t barColor = paused ? ST7735_MAGENTA : stateBarColor(currentState);
    tft.fillRect(0, 0, screenW, barH, barColor);
    tft.setTextColor(paused ? ST7735_WHITE : stateBarTextColor(currentState), barColor);
    tft.setTextSize(1);
    tft.setCursor(2, 2);
    tft.print(paused ? "PAUSED" : shortStateLabel(currentState));
    lastBarState = currentState;
    lastPaused = paused;

    tft.drawRect(0, graphTop, graphW, graphH + 1, 0xC618);
    if (!showProcessDetails) {
      for (uint16_t tg = 0; tg < maxTemp; tg += 50) {
        int16_t gy = graphBottom - (tg * pxPerC);
        if (gy < graphTop) {
          break;
        }
        tft.drawFastHLine(0, gy, graphW, 0xC618);
      }
    }
  }

  if (currentState != lastBarState || paused != lastPaused) {
    uint16_t barColor = paused ? ST7735_MAGENTA : stateBarColor(currentState);
    tft.fillRect(0, 0, screenW, barH, barColor);
    tft.setTextColor(paused ? ST7735_WHITE : stateBarTextColor(currentState), barColor);
    tft.setTextSize(1);
    tft.setCursor(2, 2);
    tft.print(paused ? "PAUSED" : shortStateLabel(currentState));
    lastBarState = currentState;
    lastPaused = paused;
  }

  if (showProcessDetails != lastShowDetails) {
    if (showProcessDetails) {
      tft.fillRect(1, graphTop + 1, graphW - 2, graphH - 1, ST7735_WHITE);
      for (uint8_t i = 0; i < 8; i++) {
        lastDetailLines[i][0] = '\0';
      }
      lastDetailWasConst = false;
      lastGraphElapsed = 0;
    }
    lastShowDetails = showProcessDetails;
  }

  uint32_t elapsed = (uint32_t)((esp_timer_get_time() - cycleStartTime) / 1000000ULL);
  uint32_t remaining = (elapsed < (uint32_t)estimatedTotalTime) ? (uint32_t)estimatedTotalTime - elapsed : 0;

  // temperature
  tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
  tft.setTextSize(2);
  char tempBuf[16];
  snprintf(tempBuf, sizeof(tempBuf), "%5.1f\367C", aktSystemTemperature);
  updateTextField(0, bigTextY, screenW, bigTextH + 2, 2, bigTextY, tempBuf, lastTempBuf, sizeof(lastTempBuf));

  // setpoint + remaining
  tft.setTextSize(1);
  char spBuf[20];
  float displaySetpoint = (currentState == ConstantTemp) ? (float)constantTempSetpoint : heaterSetpoint;
  snprintf(spBuf, sizeof(spBuf), "SP %.1f\367C", displaySetpoint);
  updateTextField(0, infoY, 88, infoH + 2, 2, infoY, spBuf, lastSpBuf, sizeof(lastSpBuf));
  char remBuf[8];
  formatTimeShort(remBuf, sizeof(remBuf), remaining);
  char remLabel[16];
  snprintf(remLabel, sizeof(remLabel), "Rem %s", remBuf);
  updateTextField(88, infoY, screenW - 88, infoH + 2, 90, infoY, remLabel, lastRemBuf, sizeof(lastRemBuf));

  if (showProcessDetails) {
    bool isConstMode = (currentState == ConstantTemp);
    if (isConstMode != lastDetailWasConst) {
      tft.fillRect(1, graphTop + 1, graphW - 2, graphH - 1, ST7735_WHITE);
      for (uint8_t i = 0; i < 8; i++) {
        lastDetailLines[i][0] = '\0';
      }
      lastDetailWasConst = isConstMode;
    }
    uint8_t y = graphTop + 2;
    const uint8_t lineStep = 9;
    tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
    tft.setTextSize(1);
    char lineBuf[32];
    uint8_t idx = 0;
    if (currentState == ConstantTemp) {
      snprintf(lineBuf, sizeof(lineBuf), "Mode: Const %d\367C", constantTempSetpoint);
    } else if (currentState == PreTune || currentState == Tune) {
      snprintf(lineBuf, sizeof(lineBuf), "Mode: Tune");
    } else {
      snprintf(lineBuf, sizeof(lineBuf), "Profile: %s", activeProfile.name);
    }
    updateTextField(1, y - 1, graphW - 2, lineStep, 2, y, lineBuf, lastDetailLines[idx], sizeof(lastDetailLines[idx]));
    idx++;
    y += lineStep;

    char elapsedBuf[12];
    formatTimeLong(elapsedBuf, sizeof(elapsedBuf), elapsed);
    snprintf(lineBuf, sizeof(lineBuf), "Elapsed %s", elapsedBuf);
    updateTextField(1, y - 1, graphW - 2, lineStep, 2, y, lineBuf, lastDetailLines[idx], sizeof(lastDetailLines[idx]));
    idx++;
    y += lineStep;

    char remainBuf[8];
    formatTimeShort(remainBuf, sizeof(remainBuf), remaining);
    snprintf(lineBuf, sizeof(lineBuf), "Remain  %s", remainBuf);
    updateTextField(1, y - 1, graphW - 2, lineStep, 2, y, lineBuf, lastDetailLines[idx], sizeof(lastDetailLines[idx]));
    idx++;
    y += lineStep;

    if (currentState == ConstantTemp) {
      if (constantTempBeepMinutes <= 0) {
        snprintf(lineBuf, sizeof(lineBuf), "Beep Off");
      } else {
        int32_t beepRemaining = (int32_t)constantTempBeepMinutes * 60 - (int32_t)elapsed;
        if (beepRemaining <= 0) {
          snprintf(lineBuf, sizeof(lineBuf), "Beep Done");
        } else {
          char beepBuf[8];
          formatTimeShort(beepBuf, sizeof(beepBuf), (uint32_t)beepRemaining);
          snprintf(lineBuf, sizeof(lineBuf), "Beep %s", beepBuf);
        }
      }
      updateTextField(1, y - 1, graphW - 2, lineStep, 2, y, lineBuf, lastDetailLines[idx], sizeof(lastDetailLines[idx]));
      idx++;
      y += lineStep;
    }

    snprintf(lineBuf, sizeof(lineBuf), "Ramp %.2f\367C/s", aktSystemTemperatureRamp);
    updateTextField(1, y - 1, graphW - 2, lineStep, 2, y, lineBuf, lastDetailLines[idx], sizeof(lastDetailLines[idx]));
    idx++;
    y += lineStep;

    uint16_t powerPercent = (uint16_t)heaterOutput*100/256;
    snprintf(lineBuf, sizeof(lineBuf), "Power %u%%", powerPercent);
    updateTextField(1, y - 1, graphW - 2, lineStep, 2, y, lineBuf, lastDetailLines[idx], sizeof(lastDetailLines[idx]));
    idx++;
    y += lineStep;

    snprintf(lineBuf, sizeof(lineBuf), "PID P:%.2f I:%.2f D:%.2f", heaterPID.Kp, heaterPID.Ki, heaterPID.Kd);
    updateTextField(1, y - 1, graphW - 2, lineStep, 2, y, lineBuf, lastDetailLines[idx], sizeof(lastDetailLines[idx]));
    idx++;
    y += lineStep;

    snprintf(lineBuf, sizeof(lineBuf), "Out P:%.1f I:%.1f D:%.1f", pidOutP, pidOutI, pidOutD);
    updateTextField(1, y - 1, graphW - 2, lineStep, 2, y, lineBuf, lastDetailLines[idx], sizeof(lastDetailLines[idx]));
  } else {
    if(currentState != Complete && !paused)
    {
      uint32_t graphElapsed = elapsed;
      if (currentState == ConstantTemp && estimatedTotalTime > 0) {
        graphElapsed = elapsed % (uint32_t)estimatedTotalTime;
        if (graphElapsed < lastGraphElapsed) {
          tft.fillRect(1, graphTop + 1, graphW - 2, graphH - 1, ST7735_WHITE);
          for (uint16_t tg = 0; tg < maxTemp; tg += 50) {
            int16_t gy = graphBottom - (tg * pxPerC);
            if (gy < graphTop) {
              break;
            }
            tft.drawFastHLine(0, gy, graphW, 0xC618);
          }
        }
      }
      lastGraphElapsed = graphElapsed;
      uint16_t dx = (uint16_t)(graphElapsed * pxPerS);
      if (dx >= graphW) {
        dx = graphW - 1;
      }

      int16_t dy = graphBottom - (heaterSetpoint * pxPerC);
      if (dy < graphTop) dy = graphTop;
      if (dy > graphBottom) dy = graphBottom;
      tft.drawPixel(dx, dy, ST7735_BLUE);
    
      dy = graphBottom - (aktSystemTemperature * pxPerC);
      if (dy < graphTop) dy = graphTop;
      if (dy > graphBottom) dy = graphBottom;
      tft.drawPixel(dx, dy, ST7735_RED);

      if (currentState == ConstantTemp) {
        float pScaled = (float)powerHeater;
        if (pScaled < 0) pScaled = 0;
        if (pScaled > 255) pScaled = 255;
        int16_t py = graphBottom - (pScaled / 255.0f) * graphH;
        if (py < graphTop) py = graphTop;
        if (py > graphBottom) py = graphBottom;
        tft.drawPixel(dx, py, ST7735_GREEN);
      }
    }
  }
  
  const bool fanOn =
    currentState == RampToSoak ||
    currentState == Soak ||
    currentState == RampUp ||
    currentState == Peak ||
    currentState == CoolDown ||
    currentState == ConstantTemp ||
    currentState == PreTune ||
    currentState == Tune;
  bool fanStatus = fanOn && !paused;
  if (fanOverride == 1) fanStatus = true;
  if (fanOverride == 0) fanStatus = false;

  tft.setTextSize(1);
  uint16_t percent = (uint16_t)heaterOutput*100/256;
  const bool heaterOn = percent > 0;
  if (lastHeaterOn != (uint8_t)heaterOn) {
    tft.setTextColor(heaterOn ? ST7735_RED : ST7735_BLACK, ST7735_WHITE);
    tft.fillRect(0, screenH - bottomH, 45, bottomH, ST7735_WHITE);
    tft.setCursor(2, screenH - bottomH + 1);
    tft.print(heaterOn ? "H:On " : "H:Off");
    lastHeaterOn = heaterOn ? 1 : 0;
  }

  uint8_t fanDisplay = (fanOverride == 1) ? 2 : (fanOverride == 0 ? 3 : (fanStatus ? 1 : 0));
  if (lastFanStatus != fanDisplay) {
    tft.setTextColor(fanStatus ? ST7735_BLUE : ST7735_BLACK, ST7735_WHITE);
    tft.fillRect(48, screenH - bottomH, 45, bottomH, ST7735_WHITE);
    tft.setCursor(50, screenH - bottomH + 1);
    if (fanOverride == 1) {
      tft.print("F:On*");
    } else if (fanOverride == 0) {
      tft.print("F:Off");
    } else {
      tft.print(fanStatus ? "F:On " : "F:Off");
    }
    lastFanStatus = fanDisplay;
  }

  if (lastPowerPercent != percent) {
    tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
    char powerBuf[12];
    snprintf(powerBuf, sizeof(powerBuf), "PWM%3u%%", percent);
    updateTextField(96, screenH - bottomH, screenW - 96, bottomH, 100, screenH - bottomH + 1,
                    powerBuf, lastPowerBuf, sizeof(lastPowerBuf));
    lastPowerPercent = percent;
  }

  // PID contribution debug row above the bottom bar (ConstantTemp only).
  const uint8_t pidRowY = screenH - bottomH - 9;
  if (currentState == ConstantTemp) {
    tft.setTextColor(ST7735_BLACK, ST7735_WHITE);
    char pidBuf[32];
    snprintf(pidBuf, sizeof(pidBuf), "PID P%.1f I%.1f D%.1f", pidOutP, pidOutI, pidOutD);
    updateTextField(0, pidRowY, screenW, 9, 2, pidRowY + 1, pidBuf, lastPidOutBuf, sizeof(lastPidOutBuf));
  } else if (lastPidOutBuf[0] != '\0') {
    tft.fillRect(0, pidRowY, screenW, 9, ST7735_WHITE);
    lastPidOutBuf[0] = '\0';
  }
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

// Default tuning values for the low-temp inertia guard and ConstantTemp slew limiter.
void makeDefaultControlTuning() {
  inertiaGuardLeadSec = INERTIA_GUARD_LEAD_DEFAULT_SEC;
  inertiaGuardHysteresisC = INERTIA_GUARD_HYST_DEFAULT_C;
  inertiaGuardMinRiseCps = INERTIA_GUARD_MIN_RISE_DEFAULT_CPS;
  inertiaGuardMaxSetpointC = INERTIA_GUARD_MAX_SETPOINT_DEFAULT_C;
  constTempRampCps = CONST_TEMP_SLEW_DEFAULT_CPS;
  integralEnableBandC = INTEGRAL_ENABLE_BAND_DEFAULT_C;
}

void clampControlTuningValues() {
  if (isnan(inertiaGuardLeadSec) || inertiaGuardLeadSec < 0.0f) inertiaGuardLeadSec = 0.0f;
  if (isnan(inertiaGuardHysteresisC) || inertiaGuardHysteresisC < 0.0f) inertiaGuardHysteresisC = 0.0f;
  if (isnan(inertiaGuardMinRiseCps) || inertiaGuardMinRiseCps < 0.0f) inertiaGuardMinRiseCps = 0.0f;
  if (isnan(inertiaGuardMaxSetpointC) || inertiaGuardMaxSetpointC < 0.0f) inertiaGuardMaxSetpointC = 0.0f;
  if (isnan(constTempRampCps) || constTempRampCps < 0.0f) constTempRampCps = 0.0f;
  if (isnan(integralEnableBandC) || integralEnableBandC < 0.0f) integralEnableBandC = 0.0f;

  if (inertiaGuardLeadSec > 120.0f) inertiaGuardLeadSec = 120.0f;
  if (inertiaGuardHysteresisC > 20.0f) inertiaGuardHysteresisC = 20.0f;
  if (inertiaGuardMinRiseCps > 5.0f) inertiaGuardMinRiseCps = 5.0f;
  if (inertiaGuardMaxSetpointC > 350.0f) inertiaGuardMaxSetpointC = 350.0f;
  if (constTempRampCps > 20.0f) constTempRampCps = 20.0f;
  if (integralEnableBandC > 200.0f) integralEnableBandC = 200.0f;
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

bool saveControlTuningSettings() {
  PREF.putFloat("IG_LEAD", inertiaGuardLeadSec);
  PREF.putFloat("IG_HYST", inertiaGuardHysteresisC);
  PREF.putFloat("IG_RISE", inertiaGuardMinRiseCps);
  PREF.putFloat("IG_MAX", inertiaGuardMaxSetpointC);
  PREF.putFloat("CT_SLEW", constTempRampCps);
  PREF.putFloat("I_BAND", integralEnableBandC);
  return true;
}

bool loadControlTuningSettings() {
  makeDefaultControlTuning();
  inertiaGuardLeadSec = PREF.getFloat("IG_LEAD", inertiaGuardLeadSec);
  inertiaGuardHysteresisC = PREF.getFloat("IG_HYST", inertiaGuardHysteresisC);
  inertiaGuardMinRiseCps = PREF.getFloat("IG_RISE", inertiaGuardMinRiseCps);
  inertiaGuardMaxSetpointC = PREF.getFloat("IG_MAX", inertiaGuardMaxSetpointC);
  constTempRampCps = PREF.getFloat("CT_SLEW", constTempRampCps);
  integralEnableBandC = PREF.getFloat("I_BAND", integralEnableBandC);
  clampControlTuningValues();
  return true;
}

bool saveConstTempSettings() {
  PREF.putShort("CT_SET", constantTempSetpoint);
  PREF.putShort("CT_BEEP", constantTempBeepMinutes);
  return true;
}

bool loadConstTempSettings() {
  constantTempSetpoint = PREF.getShort("CT_SET", constantTempSetpoint);
  constantTempBeepMinutes = PREF.getShort("CT_BEEP", constantTempBeepMinutes);
  if (constantTempSetpoint < 0) constantTempSetpoint = 0;
  if (constantTempBeepMinutes < 0) constantTempBeepMinutes = 0;
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
  if (constBeepEditActive) {
    return;
  }
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
  else if (currentState == ConstantTemp)
  {
    currentState = Complete;
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
  if (constBeepEditActive) {
    numericCursorMove -= 1;
    return;
  }
  if (currentState == Settings)
  {
    encAbsolute -= 1;
    myMenue.navigate(myMenue.getPrev());
    menuUpdateRequest = true;
  }
  else if (currentState == Edit)
  {
    if (myMenue.currentItem != &Menu::NullItem)
    {
      if (isNumericMenuItem(myMenue.currentItem)) {
        numericCursorMove -= 1;
      } else {
        encAbsolute -= 1;
      }
      myMenue.executeCallbackAction(Menu::actionDisplay);
    }
  }
}

void btn_startstop_long(Button2& btn)
{
  (void)btn;
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
  if (constBeepEditActive) {
    numericCursorMove += 1;
    return;
  }
  if (currentState == Settings)
  {
    encAbsolute += 1;
    myMenue.navigate(myMenue.getNext());
    menuUpdateRequest = true;
  }
  else if (currentState == Edit)
  {
    if (myMenue.currentItem != &Menu::NullItem)
    {
      if (isNumericMenuItem(myMenue.currentItem)) {
        numericCursorMove += 1;
      } else {
        encAbsolute += 1;
      }
      myMenue.executeCallbackAction(Menu::actionDisplay);
    }
  }
}

void btn_stop_tap(Button2& btn)
{
  if (currentState == ConstantTemp) {
    if (!constBeepEditActive) {
      constBeepEditActive = true;
      constBeepEditInit = true;
      constBeepEditTrigger = false;
      constBeepEditCancel = false;
      return;
    }
    constBeepEditTrigger = true;
    return;
  }
  reportError("E-Stop");
}

void btn_stop_long(Button2& btn)
{
  (void)btn;
  reportError("E-Stop");
}

void btn_left_tap(Button2& btn)
{
  if (constBeepEditActive) {
    constBeepEditCancel = true;
    return;
  }
  if (currentState > UIMenuEnd) {
    showProcessDetails = !showProcessDetails;
    initialProcessDisplay = true;
    return;
  }
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

void btn_right_tap(Button2& btn)
{
  (void)btn;
  if (constBeepEditActive) {
    return;
  }
  if (currentState > UIMenuEnd) {
    fanOverride = (fanOverride == 0) ? -1 : 0;
  }
}
