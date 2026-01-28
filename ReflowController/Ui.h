#pragma once

#include "Globals.h"

void uiInit();
void uiInitButtons();
void uiInitBuzzer();
void uiShowSplash();

void uiHandleButtons();
void uiHandleCountupdown();
void uiUpdateBeep();
void uiUpdateBottomLine();
void uiUpdateMenu();
void uiUpdateProcessDisplay();

void reportError(char *text);
void displayMenusInfos();
void clearLastMenuItemRenderState();
void printfloat(float val);
void printfloat2(float val);

bool menuExit(const Menu::Action_t);
bool menuDummy(const Menu::Action_t);
bool editNumericalValue(const Menu::Action_t);
bool editProfileName(const Menu::Action_t);
bool saveLoadProfile(const Menu::Action_t);
bool manualHeating(const Menu::Action_t);
bool menuWiFi(const Menu::Action_t);
bool factoryReset(const Menu::Action_t);
bool cycleStart(const Menu::Action_t);

bool savePID();
bool loadPID();
bool saveConstTempSettings();
bool loadConstTempSettings();
bool saveControlTuningSettings();
bool loadControlTuningSettings();
void clampControlTuningValues();
void loadLastUsedProfile();
void saveLastUsedProfile();
bool saveParameters(uint8_t profile);
bool loadParameters(uint8_t profile);
void loadProfileName(uint8_t id, char * buffer);
