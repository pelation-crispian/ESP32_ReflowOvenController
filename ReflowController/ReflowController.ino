// ----------------------------------------------------------------------------
// Reflow Oven Controller
// (c) 2019      Patrick Knöbel
// (c) 2014 Karl Pitrich <karl@pitrich.com>
// (c) 2012-2013 Ed Simmons
// ----------------------------------------------------------------------------

#include "Globals.h"
#include "Ui.h"
#include "Sensors.h"
#include "Control.h"
#include "root_html.h"

Neotimer timer_beep = Neotimer(500);
Neotimer timer_temp = Neotimer(READ_TEMP_INTERVAL_MS);
Neotimer timer_dsp_btmln = Neotimer(200);
Neotimer timer_display = Neotimer(1000);
Neotimer timer_control = Neotimer(READ_TEMP_INTERVAL_MS);
Neotimer timer_countupdown = Neotimer(200);

Button2 btn_startstop;
Button2 btn_up;
Button2 btn_down;
Button2 btn_left;
Button2 btn_right;
Button2 btn_stop;

bool countup = false;
bool countdown = false;

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
bool systemPaused = false;
int8_t fanOverride = -1;
volatile bool pidTuningsDirty = false;

static void syncUiAfterWebChange();
static void sendJson(WebServer &srv, const String &payload);
static String jsonEscape(const char *input);
static uint32_t estimateTotalTimeSec();
static void loadAutoTuneSettings();
static void saveAutoTuneSettings();
static String makeStatusJson();
static String makeSettingsJson();
static String makeProfilesJson();
static void handleProfileSelect();
static void handleProfileUpdate();
static void handlePidUpdate();
static void handleConstTempUpdate();
static void handleTuneUpdate();
static void handleControlUpdate();
static void handleAction();
static bool isManualMode();
static bool isRunState();
static bool allowSettingsChange();
void saveLastUsedProfile();
void loadLastUsedProfile();

float aktSystemTemperature;
float aktSystemTemperatureRamp; //°C/s

int16_t tuningHeaterOutput=30;
int16_t tuningNoiseBand=1;
int16_t tuningOutputStep=10;
int16_t tuningLookbackSec=60;
int16_t constantTempSetpoint=60;
int16_t constantTempBeepMinutes=20;
float inertiaGuardLeadSec = INERTIA_GUARD_LEAD_DEFAULT_SEC;
float inertiaGuardHysteresisC = INERTIA_GUARD_HYST_DEFAULT_C;
float inertiaGuardMinRiseCps = INERTIA_GUARD_MIN_RISE_DEFAULT_CPS;
float inertiaGuardMaxSetpointC = INERTIA_GUARD_MAX_SETPOINT_DEFAULT_C;
float constTempRampCps = CONST_TEMP_SLEW_DEFAULT_CPS;


int activeProfileId = 0;
Profile_t activeProfile; // the one and only instance

int16_t encAbsolute;

State currentState  = Idle;
uint64_t stateChangedTicks = 0;

// track menu item state to improve render preformance
LastItemState_t currentlyRenderedItems[MENUE_ITEMS_VISIBLE];

float heaterSetpoint;
float heaterInput;
float heaterOutput;

PID_t heaterPID;

bool menuUpdateRequest = true;
bool initialProcessDisplay = false;

uint64_t cycleStartTime=0;

uint8_t beepcount = 1;

void setup() {
  //Debug
  Serial.begin(115200);

  // Initialize HSPI explicitly for the TFT + thermocouple bus.
  MYSPI.begin(HSPI_SCK, HSPI_MISO, HSPI_MOSI, LCD_CS);
  delay(10);

  uiInit();

  //init timers
  timer_beep.start();
  timer_temp.start();
  timer_dsp_btmln.start();
  timer_display.start();
  timer_control.start();

  uiInitButtons();
  controlInit();
  sensorsInit();
  uiInitBuzzer();

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
  server.on("/api/status", []() {
    sendJson(server, makeStatusJson());
  });
  server.on("/api/settings", []() {
    sendJson(server, makeSettingsJson());
  });
  server.on("/api/profiles", []() {
    sendJson(server, makeProfilesJson());
  });
  server.on("/api/profile/select", []() {
    handleProfileSelect();
  });
  server.on("/api/profile/update", []() {
    handleProfileUpdate();
  });
  server.on("/api/pid", []() {
    handlePidUpdate();
  });
  server.on("/api/consttemp", []() {
    handleConstTempUpdate();
  });
  server.on("/api/tune", []() {
    handleTuneUpdate();
  });
  server.on("/api/control", []() {
    handleControlUpdate();
  });
  server.on("/api/action", []() {
    handleAction();
  });
  serverAction.on("/start", []() {
    serverAction.sendHeader("Cache-Control","no-cache");
    serverAction.sendHeader("Access-Control-Allow-Origin","*");
    if(currentState == Settings)
    {
      //Start Revlow!
      systemPaused = false;
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
    systemPaused = false;
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
    else if (currentState == ConstantTemp)
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

  //Preferences init
  PREF.begin("REFLOW");
  loadLastUsedProfile();
  loadPID();          
  loadConstTempSettings();
  loadAutoTuneSettings();
  loadControlTuningSettings();
  
  uiShowSplash();

  delay(1000);

  menuExit(Menu::actionDisplay); // reset to initial state
  myMenue.navigate(&miCycleStart);
  currentState = Settings;
  menuUpdateRequest = true;
}

void loop()
{
  uint64_t time_ms = esp_timer_get_time()/1000;

  // Handle internet requests
  server.handleClient();  
  serverAction.handleClient();  

  uiHandleButtons();
  uiUpdateBeep();
  sensorsUpdate();
  uiUpdateBottomLine();
  uiUpdateMenu();
  uiUpdateProcessDisplay();
  controlUpdate(time_ms);
}

static void syncUiAfterWebChange() {
  if (currentState <= UIMenuEnd) {
    clearLastMenuItemRenderState();
  } else {
    initialProcessDisplay = true;
  }
}

static void sendJson(WebServer &srv, const String &payload) {
  srv.sendHeader("Cache-Control","no-cache");
  srv.send(200, "application/json", payload);
}

static String jsonEscape(const char *input) {
  String out;
  if (!input) {
    return "";
  }
  out.reserve(strlen(input) + 4);
  for (const char *p = input; *p; ++p) {
    char c = *p;
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out += c; break;
    }
  }
  return out;
}

static uint32_t estimateTotalTimeSec() {
  if (currentState == PreTune || currentState == Tune) {
    return (uint32_t)max(0, tuningLookbackSec * 20);
  }
  if (currentState == ConstantTemp) {
    if (constantTempBeepMinutes > 0) {
      return (uint32_t)constantTempBeepMinutes * 60;
    }
    return 3600;
  }
  float rampUpRate = activeProfile.rampUpRate;
  float rampDownRate = activeProfile.rampDownRate;
  if (rampUpRate < 0.1f) rampUpRate = 0.1f;
  if (rampDownRate < 0.1f) rampDownRate = 0.1f;
  float est = activeProfile.soakDuration + activeProfile.peakDuration;
  est += (activeProfile.soakTemp - aktSystemTemperature) / rampUpRate;
  est += (activeProfile.peakTemp - activeProfile.soakTemp) / rampUpRate;
  est += (activeProfile.peakTemp - IDLE_TEMP) / rampDownRate;
  est *= 1.2f;
  if (est < 0) est = 0;
  return (uint32_t)est;
}

static void loadAutoTuneSettings() {
  tuningHeaterOutput = PREF.getShort("AT_OUT", tuningHeaterOutput);
  tuningNoiseBand = PREF.getShort("AT_NOISE", tuningNoiseBand);
  tuningOutputStep = PREF.getShort("AT_STEP", tuningOutputStep);
  tuningLookbackSec = PREF.getShort("AT_LOOK", tuningLookbackSec);

  if (tuningHeaterOutput < 0) tuningHeaterOutput = 0;
  if (tuningHeaterOutput > 100) tuningHeaterOutput = 100;
  if (tuningNoiseBand < 0) tuningNoiseBand = 0;
  if (tuningOutputStep < 0) tuningOutputStep = 0;
  if (tuningOutputStep > 100) tuningOutputStep = 100;
  if (tuningLookbackSec < 1) tuningLookbackSec = 1;
}

static void saveAutoTuneSettings() {
  PREF.putShort("AT_OUT", tuningHeaterOutput);
  PREF.putShort("AT_NOISE", tuningNoiseBand);
  PREF.putShort("AT_STEP", tuningOutputStep);
  PREF.putShort("AT_LOOK", tuningLookbackSec);
}

static String makeStatusJson() {
  unsigned long time = (esp_timer_get_time()-cycleStartTime)/1000;
  uint32_t elapsedSec = time / 1000;
  uint32_t remaining = 0;
  if (currentState > UIMenuEnd) {
    uint32_t total = estimateTotalTimeSec();
    remaining = (elapsedSec < total) ? (total - elapsedSec) : 0;
  }
  bool manualMode = isManualMode();
  bool runState = isRunState();
  bool fanOn = runState && !systemPaused;
  if (fanOverride == 1) fanOn = true;
  if (fanOverride == 0) fanOn = false;
  uint16_t powerPercent = (uint16_t)powerHeater * 100 / 255;

  String json = "{";
  json += "\"time\":" + String(time);
  json += ",\"temp\":" + String(aktSystemTemperature, 2);
  json += ",\"ramp\":" + String(aktSystemTemperatureRamp, 2);
  json += ",\"setpoint\":" + String(heaterSetpoint, 2);
  json += ",\"power\":" + String(powerPercent);
  json += ",\"heaterOn\":" + String(powerHeater > 0 ? 1 : 0);
  json += ",\"fanOn\":" + String(fanOn ? 1 : 0);
  json += ",\"fanOverride\":" + String(fanOverride);
  const char *stateLabel = currentStateToString();
  if (currentState == Settings) stateLabel = "Settings";
  if (currentState == Edit) stateLabel = "Edit";
  if (manualMode) stateLabel = "Manual";
  json += ",\"state\":\"" + String(stateLabel) + "\"";
  json += ",\"paused\":" + String(systemPaused ? 1 : 0);
  json += ",\"manual\":" + String(manualMode ? 1 : 0);
  json += ",\"manualPower\":" + String(encAbsolute);
  json += ",\"profileId\":" + String(activeProfileId);
  json += ",\"profileName\":\"" + jsonEscape(activeProfile.name) + "\"";
  json += ",\"remaining\":" + String(remaining);
  json += "}";
  return json;
}

static String makeSettingsJson() {
  String json = "{";
  json += "\"profileId\":" + String(activeProfileId);
  json += ",\"profile\":{";
  json += "\"name\":\"" + jsonEscape(activeProfile.name) + "\"";
  json += ",\"soakTemp\":" + String(activeProfile.soakTemp);
  json += ",\"soakDuration\":" + String(activeProfile.soakDuration);
  json += ",\"peakTemp\":" + String(activeProfile.peakTemp);
  json += ",\"peakDuration\":" + String(activeProfile.peakDuration);
  json += ",\"rampUpRate\":" + String(activeProfile.rampUpRate, 2);
  json += ",\"rampDownRate\":" + String(activeProfile.rampDownRate, 2);
  json += "}";
  json += ",\"pid\":{";
  json += "\"kp\":" + String(heaterPID.Kp, 3);
  json += ",\"ki\":" + String(heaterPID.Ki, 3);
  json += ",\"kd\":" + String(heaterPID.Kd, 3);
  json += "}";
  json += ",\"constTemp\":{";
  json += "\"setpoint\":" + String(constantTempSetpoint);
  json += ",\"beepMinutes\":" + String(constantTempBeepMinutes);
  json += "}";
  json += ",\"tuning\":{";
  json += "\"output\":" + String(tuningHeaterOutput);
  json += ",\"noiseBand\":" + String(tuningNoiseBand);
  json += ",\"outputStep\":" + String(tuningOutputStep);
  json += ",\"lookbackSec\":" + String(tuningLookbackSec);
  json += "}";
  json += ",\"control\":{";
  json += "\"guardLeadSec\":" + String(inertiaGuardLeadSec, 2);
  json += ",\"guardHystC\":" + String(inertiaGuardHysteresisC, 2);
  json += ",\"guardMinRiseCps\":" + String(inertiaGuardMinRiseCps, 2);
  json += ",\"guardMaxSetpointC\":" + String(inertiaGuardMaxSetpointC, 2);
  json += ",\"constSlewCps\":" + String(constTempRampCps, 2);
  json += "}";
  json += ",\"fanOverride\":" + String(fanOverride);
  json += "}";
  return json;
}

static String makeProfilesJson() {
  String json = "[";
  char nameBuf[PROFILE_NAME_LENGTH];
  for (uint8_t i = 0; i < MAX_PROFILES; i++) {
    loadProfileName(i, nameBuf);
    if (i > 0) json += ",";
    json += "{\"id\":" + String(i) + ",\"name\":\"" + jsonEscape(nameBuf) + "\"}";
  }
  json += "]";
  return json;
}

static void handleProfileSelect() {
  if (!server.hasArg("id")) {
    sendJson(server, "{\"ok\":false,\"error\":\"missing id\"}");
    return;
  }
  if (!allowSettingsChange()) {
    sendJson(server, "{\"ok\":false,\"error\":\"busy\"}");
    return;
  }
  int id = server.arg("id").toInt();
  if (id < 0 || id >= MAX_PROFILES) {
    sendJson(server, "{\"ok\":false,\"error\":\"invalid id\"}");
    return;
  }
  activeProfileId = id;
  loadParameters(activeProfileId);
  saveLastUsedProfile();
  syncUiAfterWebChange();
  sendJson(server, "{\"ok\":true}");
}

static void handleProfileUpdate() {
  if (!allowSettingsChange()) {
    sendJson(server, "{\"ok\":false,\"error\":\"busy\"}");
    return;
  }
  bool changed = false;
  if (server.hasArg("name")) {
    String name = server.arg("name");
    name.trim();
    name.toCharArray(activeProfile.name, PROFILE_NAME_LENGTH);
    changed = true;
  }
  if (server.hasArg("soakTemp")) {
    int v = server.arg("soakTemp").toInt();
    v = constrain(v, 0, 350);
    activeProfile.soakTemp = v;
    changed = true;
  }
  if (server.hasArg("soakDuration")) {
    int v = server.arg("soakDuration").toInt();
    v = constrain(v, 0, 20000);
    activeProfile.soakDuration = v;
    changed = true;
  }
  if (server.hasArg("peakTemp")) {
    int v = server.arg("peakTemp").toInt();
    v = constrain(v, 0, 350);
    activeProfile.peakTemp = v;
    changed = true;
  }
  if (server.hasArg("peakDuration")) {
    int v = server.arg("peakDuration").toInt();
    v = constrain(v, 0, 20000);
    activeProfile.peakDuration = v;
    changed = true;
  }
  if (server.hasArg("rampUpRate")) {
    float v = server.arg("rampUpRate").toFloat();
    if (v < 0.1f) v = 0.1f;
    if (v > 20.0f) v = 20.0f;
    activeProfile.rampUpRate = v;
    changed = true;
  }
  if (server.hasArg("rampDownRate")) {
    float v = server.arg("rampDownRate").toFloat();
    if (v < 0.1f) v = 0.1f;
    if (v > 20.0f) v = 20.0f;
    activeProfile.rampDownRate = v;
    changed = true;
  }
  if (changed) {
    saveParameters(activeProfileId);
    saveLastUsedProfile();
    syncUiAfterWebChange();
  }
  sendJson(server, "{\"ok\":true}");
}

static void handlePidUpdate() {
  if (!allowSettingsChange()) {
    sendJson(server, "{\"ok\":false,\"error\":\"busy\"}");
    return;
  }
  bool changed = false;
  if (server.hasArg("kp")) {
    heaterPID.Kp = server.arg("kp").toFloat();
    if (heaterPID.Kp < 0) heaterPID.Kp = 0;
    changed = true;
  }
  if (server.hasArg("ki")) {
    heaterPID.Ki = server.arg("ki").toFloat();
    if (heaterPID.Ki < 0) heaterPID.Ki = 0;
    changed = true;
  }
  if (server.hasArg("kd")) {
    heaterPID.Kd = server.arg("kd").toFloat();
    if (heaterPID.Kd < 0) heaterPID.Kd = 0;
    changed = true;
  }
  if (changed) {
    savePID();
    pidTuningsDirty = true;
    syncUiAfterWebChange();
  }
  sendJson(server, "{\"ok\":true}");
}

static void handleConstTempUpdate() {
  if (!allowSettingsChange()) {
    sendJson(server, "{\"ok\":false,\"error\":\"busy\"}");
    return;
  }
  bool changed = false;
  if (server.hasArg("setpoint")) {
    int v = server.arg("setpoint").toInt();
    if (v < 0) v = 0;
    if (v > 350) v = 350;
    constantTempSetpoint = v;
    changed = true;
  }
  if (server.hasArg("beepMinutes")) {
    int v = server.arg("beepMinutes").toInt();
    if (v < 0) v = 0;
    if (v > 720) v = 720;
    constantTempBeepMinutes = v;
    changed = true;
  }
  if (changed) {
    saveConstTempSettings();
    syncUiAfterWebChange();
  }
  sendJson(server, "{\"ok\":true}");
}

static void handleTuneUpdate() {
  if (!allowSettingsChange()) {
    sendJson(server, "{\"ok\":false,\"error\":\"busy\"}");
    return;
  }
  bool changed = false;
  if (server.hasArg("output")) {
    int v = server.arg("output").toInt();
    v = constrain(v, 0, 100);
    tuningHeaterOutput = v;
    changed = true;
  }
  if (server.hasArg("noiseBand")) {
    int v = server.arg("noiseBand").toInt();
    if (v < 0) v = 0;
    tuningNoiseBand = v;
    changed = true;
  }
  if (server.hasArg("outputStep")) {
    int v = server.arg("outputStep").toInt();
    v = constrain(v, 0, 100);
    tuningOutputStep = v;
    changed = true;
  }
  if (server.hasArg("lookbackSec")) {
    int v = server.arg("lookbackSec").toInt();
    if (v < 1) v = 1;
    tuningLookbackSec = v;
    changed = true;
  }
  if (changed) {
    saveAutoTuneSettings();
    syncUiAfterWebChange();
  }
  sendJson(server, "{\"ok\":true}");
}

static void handleControlUpdate() {
  if (!allowSettingsChange()) {
    sendJson(server, "{\"ok\":false,\"error\":\"busy\"}");
    return;
  }
  bool changed = false;
  if (server.hasArg("guardLeadSec")) {
    inertiaGuardLeadSec = server.arg("guardLeadSec").toFloat();
    changed = true;
  }
  if (server.hasArg("guardHystC")) {
    inertiaGuardHysteresisC = server.arg("guardHystC").toFloat();
    changed = true;
  }
  if (server.hasArg("guardMinRiseCps")) {
    inertiaGuardMinRiseCps = server.arg("guardMinRiseCps").toFloat();
    changed = true;
  }
  if (server.hasArg("guardMaxSetpointC")) {
    inertiaGuardMaxSetpointC = server.arg("guardMaxSetpointC").toFloat();
    changed = true;
  }
  if (server.hasArg("constSlewCps")) {
    constTempRampCps = server.arg("constSlewCps").toFloat();
    changed = true;
  }
  if (changed) {
    clampControlTuningValues();
    saveControlTuningSettings();
    syncUiAfterWebChange();
  }
  sendJson(server, "{\"ok\":true}");
}

static void handleAction() {
  if (!server.hasArg("cmd")) {
    sendJson(server, "{\"ok\":false,\"error\":\"missing cmd\"}");
    return;
  }
  String cmd = server.arg("cmd");
  if (cmd == "start_cycle") {
    if (currentState == Settings || currentState == Idle) {
      systemPaused = false;
      myMenue.navigate(&miCycleStart);
      myMenue.invoke();
      sendJson(server, "{\"ok\":true}");
    } else {
      sendJson(server, "{\"ok\":false,\"error\":\"busy\"}");
    }
    return;
  }
  if (cmd == "start_const") {
    if (currentState == Settings || currentState == Idle) {
      systemPaused = false;
      myMenue.navigate(&miConstTempStart);
      myMenue.invoke();
      sendJson(server, "{\"ok\":true}");
    } else {
      sendJson(server, "{\"ok\":false,\"error\":\"busy\"}");
    }
    return;
  }
  if (cmd == "start_tune" || cmd == "start_pretune") {
    if (currentState == Settings || currentState == Idle) {
      systemPaused = false;
      myMenue.navigate(&miCycleStartAT);
      myMenue.invoke();
      sendJson(server, "{\"ok\":true}");
    } else {
      sendJson(server, "{\"ok\":false,\"error\":\"busy\"}");
    }
    return;
  }
  if (cmd == "stop") {
    systemPaused = false;
    if (currentState == Complete) 
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
    else if (currentState == ConstantTemp)
    {
      currentState = Complete;
    }
    else if (currentState > UIMenuEnd) 
    {
      currentState = CoolDown;
    }
    initialProcessDisplay = true;
    sendJson(server, "{\"ok\":true}");
    return;
  }
  if (cmd == "pause") {
    if (currentState > UIMenuEnd) {
      systemPaused = true;
      initialProcessDisplay = true;
    }
    sendJson(server, "{\"ok\":true}");
    return;
  }
  if (cmd == "resume") {
    systemPaused = false;
    initialProcessDisplay = true;
    sendJson(server, "{\"ok\":true}");
    return;
  }
  if (cmd == "manual_start" || cmd == "manual_power") {
    if (currentState > UIMenuEnd) {
      sendJson(server, "{\"ok\":false,\"error\":\"busy\"}");
      return;
    }
    int power = 0;
    if (server.hasArg("power")) {
      power = server.arg("power").toInt();
    }
    power = constrain(power, 0, 100);
    systemPaused = false;
    bool manualMode = isManualMode();
    if (!manualMode) {
      myMenue.navigate(&miManual);
      manualHeating(Menu::actionTrigger);
    }
    encAbsolute = power;
    manualHeating(Menu::actionDisplay);
    sendJson(server, "{\"ok\":true}");
    return;
  }
  if (cmd == "manual_stop") {
    if (currentState == Edit && myMenue.currentItem == &miManual) {
      currentState = Settings;
      clearLastMenuItemRenderState();
    }
    sendJson(server, "{\"ok\":true}");
    return;
  }
  if (cmd == "fan") {
    if (!server.hasArg("value")) {
      sendJson(server, "{\"ok\":false,\"error\":\"missing value\"}");
      return;
    }
    String value = server.arg("value");
    if (value == "on" || value == "1") fanOverride = 1;
    else if (value == "off" || value == "0") fanOverride = 0;
    else fanOverride = -1;
    syncUiAfterWebChange();
    sendJson(server, "{\"ok\":true}");
    return;
  }
  sendJson(server, "{\"ok\":false,\"error\":\"unknown cmd\"}");
}

static bool isManualMode() {
  return currentState == Edit && myMenue.currentItem == &miManual;
}

static bool isRunState() {
  return
    currentState == RampToSoak ||
    currentState == Soak ||
    currentState == RampUp ||
    currentState == Peak ||
    currentState == CoolDown ||
    currentState == ConstantTemp ||
    currentState == PreTune ||
    currentState == Tune;
}

static bool allowSettingsChange() {
  if (systemPaused) return false;
  if (isRunState()) return false;
  if (isManualMode()) return false;
  return true;
}
