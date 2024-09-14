#include <Arduino.h>
#include "control.h"
#include <Preferences.h>

// Declaration for LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);

byte enterChar[] = {
    B10000,
    B10000,
    B10100,
    B10110,
    B11111,
    B00110,
    B00100,
    B00000};

byte fastChar[] = {
    B00110,
    B01110,
    B00110,
    B00110,
    B01111,
    B00000,
    B00100,
    B01110};
byte slowChar[] = {
    B00011,
    B00111,
    B00011,
    B11011,
    B11011,
    B00000,
    B00100,
    B01110};

// Declaration of LCD Variables
const int NUM_MAIN_ITEMS = 3;
const int NUM_SETTING_ITEMS = 5;
const int NUM_TESTMACHINE_ITEMS = 5;

int currentMainScreen;
int currentSettingScreen;
int currentTestMenuScreen;
bool menuFlag, settingFlag, settingEditFlag, testMenuFlag, refreshScreen = false;

String menu_items[NUM_MAIN_ITEMS][2] = { // array with item names
    {"SETTING", "ENTER TO EDIT"},
    {"TEST MACHINE", "ENTER TO TEST"},
    {"RUN AUTO", "ENTER TO RUN AUTO"}};

String setting_items[NUM_SETTING_ITEMS][2] = { // array with item names
    {"MOTOR RUN", "SEC"},
    {"SET TIME", "24 HR SETTING"},
    {"SET MAX PRESSURE", "PSI RELEASE"},
    {"SAVING INTERVAL", "MIN"},
    {"SAVE"}};

Preferences Settings;
int parametersTimer[NUM_SETTING_ITEMS] = {1, 1, 1, 1};
int parametersTimerMaxValue[NUM_SETTING_ITEMS] = {1200, 24, 10, 60};

String testmachine_items[NUM_TESTMACHINE_ITEMS] = { // array with item names
    "HEATER",
    "VACUUM",
    "RELEASE VACUUM",
    "WATER VALVE",
    "EXIT"};

Control Heater(0);
Control Vacuum(0);
Control WaterValve(0);
Control VacuumRelease(0);

static const int buttonPin = 2;
int buttonStatePrevious = HIGH;

static const int buttonPin2 = 3;
int buttonStatePrevious2 = HIGH;

static const int buttonPin3 = 4;
int buttonStatePrevious3 = HIGH;

unsigned long minButtonLongPressDuration = 2000;
unsigned long buttonLongPressUpMillis;
unsigned long buttonLongPressDownMillis;
unsigned long buttonLongPressEnterMillis;
bool buttonStateLongPressUp = false;
bool buttonStateLongPressDown = false;
bool buttonStateLongPressEnter = false;

const int intervalButton = 50;
unsigned long previousButtonMillis;
unsigned long buttonPressDuration;
unsigned long currentMillis;

const int intervalButton2 = 50;
unsigned long previousButtonMillis2;
unsigned long buttonPressDuration2;
unsigned long currentMillis2;

const int intervalButton3 = 50;
unsigned long previousButtonMillis3;
unsigned long buttonPressDuration3;
unsigned long currentMillis3;

void InitializeButtons()
{
    pinMode(buttonPin, INPUT_PULLUP);
    pinMode(buttonPin2, INPUT_PULLUP);
    pinMode(buttonPin3, INPUT_PULLUP);
}

void readButtonUpState()
{
    if (currentMillis - previousButtonMillis > intervalButton)
    {
        int buttonState = digitalRead(buttonPin);
        if (buttonState == LOW && buttonStatePrevious == HIGH && !buttonStateLongPressUp)
        {
            buttonLongPressUpMillis = currentMillis;
            buttonStatePrevious = LOW;
        }
        buttonPressDuration = currentMillis - buttonLongPressUpMillis;
        if (buttonState == LOW && !buttonStateLongPressUp && buttonPressDuration >= minButtonLongPressDuration)
        {
            buttonStateLongPressUp = true;
        }
        if (buttonStateLongPressUp == true)
        {
            // Insert Fast Scroll Up
            refreshScreen = true;
            if (settingFlag == true)
            {
                if (settingEditFlag == true)
                {
                    if (parametersTimer[currentSettingScreen] >= parametersTimerMaxValue[currentSettingScreen] - 1)
                    {
                        parametersTimer[currentSettingScreen] = parametersTimerMaxValue[currentSettingScreen];
                    }
                    else
                    {
                        parametersTimer[currentSettingScreen] += 1;
                    }
                }
                else
                {
                    if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
                    {
                        currentSettingScreen = 0;
                    }
                    else
                    {
                        currentSettingScreen++;
                    }
                }
            }
            else if (testMenuFlag == true)
            {
                if (currentTestMenuScreen == NUM_TESTMACHINE_ITEMS - 1)
                {
                    currentTestMenuScreen = 0;
                }
                else
                {
                    currentTestMenuScreen++;
                }
            }
            else
            {
                if (currentMainScreen == NUM_MAIN_ITEMS - 1)
                {
                    currentMainScreen = 0;
                }
                else
                {
                    currentMainScreen++;
                }
            }
        }

        if (buttonState == HIGH && buttonStatePrevious == LOW)
        {
            buttonStatePrevious = HIGH;
            buttonStateLongPressUp = false;
            if (buttonPressDuration < minButtonLongPressDuration)
            {
                // Short Scroll Up
                refreshScreen = true;
                if (settingFlag == true)
                {
                    if (settingEditFlag == true)
                    {
                        if (parametersTimer[currentSettingScreen] >= parametersTimerMaxValue[currentSettingScreen] - 1)
                        {
                            parametersTimer[currentSettingScreen] = parametersTimerMaxValue[currentSettingScreen];
                        }
                        else
                        {
                            parametersTimer[currentSettingScreen] += 1;
                        }
                    }
                    else
                    {
                        if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
                        {
                            currentSettingScreen = 0;
                        }
                        else
                        {
                            currentSettingScreen++;
                        }
                    }
                }
                else if (testMenuFlag == true)
                {
                    if (currentTestMenuScreen == NUM_TESTMACHINE_ITEMS - 1)
                    {
                        currentTestMenuScreen = 0;
                    }
                    else
                    {
                        currentTestMenuScreen++;
                    }
                }
                else
                {
                    if (currentMainScreen == NUM_MAIN_ITEMS - 1)
                    {
                        currentMainScreen = 0;
                    }
                    else
                    {
                        currentMainScreen++;
                    }
                }
            }
        }
        previousButtonMillis = currentMillis;
    }
}

void readButtonDownState()
{
    if (currentMillis2 - previousButtonMillis2 > intervalButton2)
    {
        int buttonState2 = digitalRead(buttonPin2);
        if (buttonState2 == LOW && buttonStatePrevious2 == HIGH && !buttonStateLongPressDown)
        {
            buttonLongPressDownMillis = currentMillis2;
            buttonStatePrevious2 = LOW;
        }
        buttonPressDuration2 = currentMillis2 - buttonLongPressDownMillis;
        if (buttonState2 == LOW && !buttonStateLongPressDown && buttonPressDuration2 >= minButtonLongPressDuration)
        {
            buttonStateLongPressDown = true;
        }
        if (buttonStateLongPressDown == true)
        {
            refreshScreen = true;
            if (settingFlag == true)
            {
                if (settingEditFlag == true)
                {
                    if (parametersTimer[currentSettingScreen] <= 0)
                    {
                        parametersTimer[currentSettingScreen] = 0;
                    }
                    else
                    {
                        parametersTimer[currentSettingScreen] -= 1;
                    }
                }
                else
                {
                    if (currentSettingScreen == 0)
                    {
                        currentSettingScreen = NUM_SETTING_ITEMS - 1;
                    }
                    else
                    {
                        currentSettingScreen--;
                    }
                }
            }
            else if (testMenuFlag == true)
            {
                if (currentTestMenuScreen == 0)
                {
                    currentTestMenuScreen = NUM_TESTMACHINE_ITEMS - 1;
                }
                else
                {
                    currentTestMenuScreen--;
                }
            }
            else
            {
                if (currentMainScreen == 0)
                {
                    currentMainScreen = NUM_MAIN_ITEMS - 1;
                }
                else
                {
                    currentMainScreen--;
                }
            }
        }

        if (buttonState2 == HIGH && buttonStatePrevious2 == LOW)
        {
            buttonStatePrevious2 = HIGH;
            buttonStateLongPressDown = false;
            if (buttonPressDuration2 < minButtonLongPressDuration)
            {
                refreshScreen = true;
                if (settingFlag == true)
                {
                    if (settingEditFlag == true)
                    {
                        if (currentSettingScreen == 2)
                        {
                            if (parametersTimer[currentSettingScreen] <= 2)
                            {
                                parametersTimer[currentSettingScreen] = 2;
                            }
                            else
                            {
                                parametersTimer[currentSettingScreen] -= 1;
                            }
                        }
                        else
                        {
                            if (parametersTimer[currentSettingScreen] <= 0)
                            {
                                parametersTimer[currentSettingScreen] = 0;
                            }
                            else
                            {
                                parametersTimer[currentSettingScreen] -= 1;
                            }
                        }
                    }
                    else
                    {
                        if (currentSettingScreen == 0)
                        {
                            currentSettingScreen = NUM_SETTING_ITEMS - 1;
                        }
                        else
                        {
                            currentSettingScreen--;
                        }
                    }
                }
                else if (testMenuFlag == true)
                {
                    if (currentTestMenuScreen == 0)
                    {
                        currentTestMenuScreen = NUM_TESTMACHINE_ITEMS - 1;
                    }
                    else
                    {
                        currentTestMenuScreen--;
                    }
                }
                else
                {
                    if (currentMainScreen == 0)
                    {
                        currentMainScreen = NUM_MAIN_ITEMS - 1;
                    }
                    else
                    {
                        currentMainScreen--;
                    }
                }
            }
        }
        previousButtonMillis2 = currentMillis2;
    }
}

void readButtonEnterState()
{
    if (currentMillis3 - previousButtonMillis3 > intervalButton3)
    {
        int buttonState3 = digitalRead(buttonPin3);
        if (buttonState3 == LOW && buttonStatePrevious3 == HIGH && !buttonStateLongPressEnter)
        {
            buttonLongPressEnterMillis = currentMillis3;
            buttonStatePrevious3 = LOW;
        }
        buttonPressDuration3 = currentMillis3 - buttonLongPressEnterMillis;
        if (buttonState3 == LOW && !buttonStateLongPressEnter && buttonPressDuration3 >= minButtonLongPressDuration)
        {
            buttonStateLongPressEnter = true;
        }
        if (buttonStateLongPressEnter == true)
        {
            // Insert Fast Scroll Enter
            Serial.println("Long Press Enter");
        }

        if (buttonState3 == HIGH && buttonStatePrevious3 == LOW)
        {
            buttonStatePrevious3 = HIGH;
            buttonStateLongPressEnter = false;
            if (buttonPressDuration3 < minButtonLongPressDuration)
            {
                refreshScreen = true;
                if (currentMainScreen == 0 && settingFlag == true)
                {
                    if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
                    {
                        settingFlag = false;
                        // saveSettings();
                        // loadSettings();
                        currentSettingScreen = 0;
                        // setTimers();
                    }
                    else
                    {
                        if (settingEditFlag == true)
                        {
                            settingEditFlag = false;
                        }
                        else
                        {
                            settingEditFlag = true;
                        }
                    }
                }
                else if (currentMainScreen == 1 && testMenuFlag == true)
                {
                    if (currentTestMenuScreen == NUM_TESTMACHINE_ITEMS - 1)
                    {
                        currentMainScreen = 0;
                        currentTestMenuScreen = 0;
                        testMenuFlag = false;
                        // stopAll();
                    }
                    else if (currentTestMenuScreen == 0)
                    {
                        if (Heater.getMotorState() == false)
                        {
                            Heater.relayOn();
                        }
                        else
                        {
                            Heater.relayOff();
                        }
                    }
                    else if (currentTestMenuScreen == 1)
                    {
                        if (Vacuum.getMotorState() == false)
                        {
                            Vacuum.relayOn();
                        }
                        else
                        {
                            Vacuum.relayOff();
                        }
                    }
                    else if (currentTestMenuScreen == 2)
                    {
                        if (VacuumRelease.getMotorState() == false)
                        {
                            VacuumRelease.relayOn();
                        }
                        else
                        {
                            VacuumRelease.relayOff();
                        }
                    }
                    else if (currentTestMenuScreen == 3)
                    {
                        if (WaterValve.getMotorState() == false)
                        {
                            WaterValve.relayOn();
                        }
                        else
                        {
                            WaterValve.relayOff();
                        }
                    }
                }
                else
                {
                    if (currentMainScreen == 0)
                    {
                        settingFlag = true;
                    }
                    else if (currentMainScreen == 1)
                    {
                        testMenuFlag = true;
                    }
                    else if (currentMainScreen == 2)
                    {
                        // sensor.resetCount();
                        // saveCount(0);
                    }
                }
            }
        }
        previousButtonMillis3 = currentMillis3;
    }
}

void ReadButtons()
{
    currentMillis = millis();
    currentMillis2 = millis();
    currentMillis3 = millis();
    readButtonEnterState();
    readButtonUpState();
    readButtonDownState();
}

void setup()
{
    // put your setup code here, to run once:
}

void loop()
{
    // put your main code here, to run repeatedly:
}