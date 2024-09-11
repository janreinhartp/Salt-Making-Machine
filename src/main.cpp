#include <Arduino.h>
#include "control.h"
#include <Preferences.h>
#include <PCF8575.h>

// Declaration for LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);

bool triggerType = LOW;
PCF8575 pcf8575(0x25);
const int controlPin[16] = {P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11, P12, P13, P14, P15};

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

void setPinsForRelay()
{
    for (int i = 0; i < 16; i++)
    {
        pcf8575.pinMode(controlPin[i], OUTPUT); // set pin as output
    }
    pcf8575.begin();

    for (int i = 0; i < 16; i++)
    {
        if (triggerType == LOW)
        {
            pcf8575.digitalWrite(controlPin[i], HIGH);
        }
        else
        {
            pcf8575.digitalWrite(controlPin[i], LOW);
        }
    }
}

void setup()
{
  // put your setup code here, to run once:
}

void loop()
{
  // put your main code here, to run repeatedly:
}