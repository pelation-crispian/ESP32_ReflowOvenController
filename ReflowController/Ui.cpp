#include "Ui.h"

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
bool keyboard(const char * name, char * buffer, uint32_t length, bool init, bool trigger);
void renderMenuItem(const Menu::Item_t *mi, uint8_t pos);
void updateProcessDisplay();
void btn_startstop_tap(Button2& btn);
void btn_up_hold(Button2 &btn);
void btn_up_release(Button2 &btn);
void btn_up_tap(Button2 &btn);
void btn_down_hold(Button2 &btn);
void btn_down_release(Button2 &btn);
void btn_down_tap(Button2 &btn);
void btn_stop_tap(Button2& btn);
void btn_left_tap(Button2& btn);
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

void uiInit() {
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
  btn_stop.loop();

  uiHandleCountupdown();
}

void uiHandleCountupdown() {
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
  if (currentState < UIMenuEnd) 
  {
    displayMenusInfos();
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
