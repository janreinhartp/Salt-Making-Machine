#include <Arduino.h>
#include "control.h"
#include <Preferences.h>

#include "PCF8575.h"
PCF8575 pcf8575(0x22);

// Declaration for LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);

int rHeater = P0;
int rWaterValve = P1;
int rVacuum = P2;
int rReleaseVacuum = P3;

int LowLevelSensor = 4;
int HighLevelSensor = 25;

int runAutoStatus = 0;
int runCookingStatus = 0;

bool LowLevelStatus, HighLevelStatus = false;

Preferences Settings;

void initRelays()
{
    pcf8575.pinMode(rHeater, OUTPUT);
    pcf8575.digitalWrite(rHeater, HIGH);

    pcf8575.pinMode(rWaterValve, OUTPUT);
    pcf8575.digitalWrite(rWaterValve, HIGH);

    pcf8575.pinMode(rVacuum, OUTPUT);
    pcf8575.digitalWrite(rVacuum, HIGH);

    pcf8575.pinMode(rReleaseVacuum, OUTPUT);
    pcf8575.digitalWrite(rReleaseVacuum, HIGH);

    pcf8575.begin();
}

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
const int NUM_SETTING_ITEMS = 3;
const int NUM_TESTMACHINE_ITEMS = 5;

int currentMainScreen;
int currentSettingScreen;
int currentTestMenuScreen;
bool settingFlag, settingEditFlag, testMenuFlag, runAutoFlag, refreshScreen = false;

String menu_items[NUM_MAIN_ITEMS][2] = { // array with item names
    {"SETTING", "ENTER TO EDIT"},
    {"TEST MACHINE", "ENTER TO TEST"},
    {"RUN AUTO", "ENTER TO RUN AUTO"}};

String setting_items[NUM_SETTING_ITEMS][2] = { // array with item names
    {"COOKING TIME", "MIN"},
    {"DRYING TIME", "MIN"},
    {"SAVE"}};

int parametersTimer[NUM_SETTING_ITEMS] = {1, 1, 1};
int parametersTimerMaxValue[NUM_SETTING_ITEMS] = {1200, 1200, 1200};

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

Control TimerCooking(0);
Control TimerDrying(0);

void stopAllMotors()
{
    pcf8575.digitalWrite(rWaterValve, HIGH);
    pcf8575.digitalWrite(rReleaseVacuum, HIGH);
    pcf8575.digitalWrite(rVacuum, HIGH);
    pcf8575.digitalWrite(rHeater, HIGH);
}
char *secondsToHHMMSS(int total_seconds)
{
    int hours, minutes, seconds;

    hours = total_seconds / 3600;         // Divide by number of seconds in an hour
    total_seconds = total_seconds % 3600; // Get the remaining seconds
    minutes = total_seconds / 60;         // Divide by number of seconds in a minute
    seconds = total_seconds % 60;         // Get the remaining seconds

    // Format the output string
    static char hhmmss_str[7]; // 6 characters for HHMMSS + 1 for null terminator
    sprintf(hhmmss_str, "%02d%02d%02d", hours, minutes, seconds);
    return hhmmss_str;
}

void saveSettings()
{
    Settings.putInt("cookingtime", parametersTimer[0]);
    Settings.putInt("dryingtime", parametersTimer[1]);
    Serial.println("---- Saving Timer  Settings ----");
    Serial.println("Cooking Time : " + String(parametersTimer[0]));
    Serial.println("Drying Time : " + String(parametersTimer[1]));
    Serial.println("---- Saving Timer  Settings ----");
}
void loadSettings()
{
    Serial.println("---- Start Reading Settings ----");
    parametersTimer[0] = Settings.getInt("cookingtime");
    parametersTimer[1] = Settings.getInt("dryingtime");
    Serial.println("Pump Timer : " + String(parametersTimer[0]));
    Serial.println("Pump Bleach Timer : " + String(parametersTimer[1]));
    Serial.println("---- End Reading Settings ----");
    TimerCooking.setTimer(secondsToHHMMSS(parametersTimer[0] * 60));
    TimerDrying.setTimer(secondsToHHMMSS(parametersTimer[1] * 60));
}

static const int buttonPin = 12;
int buttonStatePrevious = HIGH;

static const int buttonPin2 = 15;
int buttonStatePrevious2 = HIGH;

static const int buttonPin3 = 13;
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

unsigned long currentMillisRunAuto;
unsigned long previousMillisRunAuto;
unsigned long intervalRunAuto = 1000;

void InitializeButtons()
{
    pinMode(buttonPin, INPUT_PULLUP);
    pinMode(buttonPin2, INPUT_PULLUP);
    pinMode(buttonPin3, INPUT_PULLUP);

    pinMode(LowLevelSensor, INPUT_PULLUP);
    pinMode(HighLevelSensor, INPUT_PULLUP);
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
                        saveSettings();
                        loadSettings();
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
                        stopAllMotors();
                    }
                    else if (currentTestMenuScreen == 0)
                    {
                        if (Heater.getMotorState() == false)
                        {
                            Heater.relayOn();
                            pcf8575.digitalWrite(rHeater, false);
                        }
                        else
                        {
                            Heater.relayOff();
                            pcf8575.digitalWrite(rHeater, true);
                        }
                    }
                    else if (currentTestMenuScreen == 1)
                    {
                        if (Vacuum.getMotorState() == false)
                        {
                            Vacuum.relayOn();
                            pcf8575.digitalWrite(rVacuum, false);
                        }
                        else
                        {
                            Vacuum.relayOff();
                            pcf8575.digitalWrite(rVacuum, true);
                        }
                    }
                    else if (currentTestMenuScreen == 2)
                    {
                        if (VacuumRelease.getMotorState() == false)
                        {
                            VacuumRelease.relayOn();
                            pcf8575.digitalWrite(rReleaseVacuum, false);
                        }
                        else
                        {
                            VacuumRelease.relayOff();
                            pcf8575.digitalWrite(rReleaseVacuum, true);
                        }
                    }
                    else if (currentTestMenuScreen == 3)
                    {
                        if (WaterValve.getMotorState() == false)
                        {
                            WaterValve.relayOn();
                            pcf8575.digitalWrite(rWaterValve, false);
                        }
                        else
                        {
                            WaterValve.relayOff();
                            pcf8575.digitalWrite(rWaterValve, true);
                        }
                    }
                }
                else if (currentMainScreen == 2 && runAutoFlag == true)
                {
                    stopAllMotors();
                    runAutoFlag = false;
                    runAutoStatus = 0;
                    runCookingStatus = 0;
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
                        runAutoFlag = true;
                        runAutoStatus = 1;
                        runCookingStatus = 1;
                        TimerCooking.start();
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

void initializeLCD()
{
    lcd.init();
    lcd.clear();
    lcd.createChar(0, enterChar);
    lcd.createChar(1, fastChar);
    lcd.createChar(2, slowChar);
    lcd.backlight();
    refreshScreen = true;
}

void printTestScreen(String TestMenuTitle, String Job, bool Status, bool ExitFlag)
{
    lcd.clear();
    lcd.print(TestMenuTitle);
    if (ExitFlag == false)
    {
        lcd.setCursor(0, 2);
        lcd.print(Job);
        lcd.print(" : ");
        if (Status == true)
        {
            lcd.print("ON");
        }
        else
        {
            lcd.print("OFF");
        }
    }

    if (ExitFlag == true)
    {
        lcd.setCursor(0, 3);
        lcd.print("Click to Exit Test");
    }
    else
    {
        lcd.setCursor(0, 3);
        lcd.print("Click to Run Test");
    }
    refreshScreen = false;
}

void printMainMenu(String MenuItem, String Action)
{
    lcd.clear();
    lcd.print(MenuItem);
    lcd.setCursor(0, 3);
    lcd.write(0);
    lcd.setCursor(2, 3);
    lcd.print(Action);
    refreshScreen = false;
}

void printSettingScreen(String SettingTitle, String Unit, int Value, bool EditFlag, bool SaveFlag)
{
    lcd.clear();
    lcd.print(SettingTitle);
    lcd.setCursor(0, 1);

    if (SaveFlag == true)
    {
        lcd.setCursor(0, 3);
        lcd.write(0);
        lcd.setCursor(2, 3);
        lcd.print("ENTER TO SAVE ALL");
    }
    else
    {
        lcd.print(Value);
        lcd.print(" ");
        lcd.print(Unit);
        lcd.setCursor(0, 3);
        lcd.write(0);
        lcd.setCursor(2, 3);
        if (EditFlag == false)
        {
            lcd.print("ENTER TO EDIT");
        }
        else
        {
            lcd.print("ENTER TO SAVE");
        }
    }
    refreshScreen = false;
}

void printRunAuto(String SettingTitle, String Process, String TimeRemaining)
{
    lcd.clear();
    lcd.print(SettingTitle);
    lcd.setCursor(0, 1);
    lcd.print(Process);
    lcd.setCursor(0, 2);
    lcd.print(TimeRemaining);
    refreshScreen = false;
}

void printScreen()
{
    if (settingFlag == true)
    {
        if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
        {
            printSettingScreen(setting_items[currentSettingScreen][0], setting_items[currentSettingScreen][1], parametersTimer[currentSettingScreen], settingEditFlag, true);
        }
        else
        {
            printSettingScreen(setting_items[currentSettingScreen][0], setting_items[currentSettingScreen][1], parametersTimer[currentSettingScreen], settingEditFlag, false);
        }
    }
    else if (testMenuFlag == true)
    {
        switch (currentTestMenuScreen)
        {
        case 0:
            // printTestScreen(testmachine_items[currentTestMenuScreen], "Status", ContactorVFD.getMotorState(), false);
            printTestScreen(testmachine_items[currentTestMenuScreen], "Status", Heater.getMotorState(), false);
            break;
        case 1:
            printTestScreen(testmachine_items[currentTestMenuScreen], "Status", Vacuum.getMotorState(), false);
            break;
        case 2:
            printTestScreen(testmachine_items[currentTestMenuScreen], "Status", VacuumRelease.getMotorState(), false);
            break;
        case 3:
            printTestScreen(testmachine_items[currentTestMenuScreen], "Status", WaterValve.getMotorState(), false);
            break;
        case 4:
            printTestScreen(testmachine_items[currentTestMenuScreen], "", true, true);
            break;
        default:
            break;
        }
    }
    else if (runAutoFlag == true)
    {
        switch (runAutoStatus)
        {
        case 1:
            switch (runCookingStatus)
            {
            case 1:
                printRunAuto("Cooking", "Pouring Water", TimerCooking.getTimeRemaining());
                break;
            case 2:
                printRunAuto("Cooking", "Boiling", TimerCooking.getTimeRemaining());
                break;
            default:
                break;
            }
            break;

        case 2:
            printRunAuto("Drying", "N/A", TimerDrying.getTimeRemaining());
            break;
        default:
            break;
        }
    }
    else
    {
        printMainMenu(menu_items[currentMainScreen][0], menu_items[currentMainScreen][1]);
    }
}

void runAuto()
{
    /*Flow
    Case 1 - Cook until Timer is done
        Case 1.1 - Check for Water Level
            If Water Level is not HIGH
                Open Water Valve/Release Pressure and wait for the Sensor to tell High Level
            Then
                Move to Case 1.2
        Case 1.2 - Boil the water until Low Level
            If Water Level is not LOW
                Heater and Vacuum On
            then
                Move to Case 1.1

    Case 2 -
        Run the Vacuum and Heater until Desired Time

    */
    switch (runAutoStatus)
    {
    case 1:
        if (TimerCooking.isStopped() == false)
        {
            TimerCooking.run();
            if (TimerCooking.isTimerCompleted() == true)
            {
                runAutoStatus = 2;
                TimerDrying.start();
                stopAllMotors();
            }
            else
            {
                switch (runCookingStatus)
                {
                case 1:
                    if (HighLevelStatus = true && LowLevelStatus == true)
                    {
                        runCookingStatus = 2;
                        pcf8575.digitalWrite(rWaterValve, HIGH);
                        pcf8575.digitalWrite(rReleaseVacuum, HIGH);
                    }
                    else
                    {
                        pcf8575.digitalWrite(rWaterValve, LOW);
                        pcf8575.digitalWrite(rReleaseVacuum, LOW);
                        pcf8575.digitalWrite(rVacuum, HIGH);
                        pcf8575.digitalWrite(rHeater, HIGH);
                    }

                    break;
                case 2:
                    if (HighLevelStatus = false && LowLevelStatus == false)
                    {
                        runCookingStatus = 1;
                        pcf8575.digitalWrite(rVacuum, HIGH);
                        pcf8575.digitalWrite(rHeater, HIGH);
                    }
                    else
                    {
                        pcf8575.digitalWrite(rVacuum, LOW);
                        pcf8575.digitalWrite(rHeater, LOW);
                    }
                    break;
                default:
                    break;
                }
            }
        }
        break;

    case 2:
        if (TimerDrying.isStopped() == false)
        {
            TimerDrying.run();
            if (TimerDrying.isTimerCompleted() == true)
            {
                runAutoStatus = 0;
            }
            else
            {
                pcf8575.digitalWrite(rVacuum, LOW);
                pcf8575.digitalWrite(rHeater, LOW);
            }
        }
        break;
    default:
        runAutoStatus = 0;
        runCookingStatus = 0;
        stopAllMotors();
        break;
    }
}

void setup()
{
    Serial.begin(9600);
    initializeLCD();
    pinMode(buttonPin, INPUT_PULLUP);
    pinMode(buttonPin2, INPUT_PULLUP);
    pinMode(buttonPin3, INPUT_PULLUP);
    Settings.begin("timerSetting", false);
    initRelays();

    // saveSettings();
    loadSettings();
}

void loop()
{
    ReadButtons();
    if (refreshScreen == true)
    {
        printScreen();
        refreshScreen = false;
    }

    if (runAutoFlag == true)
    {
        runAuto();

        unsigned long currentMillisRunAuto = millis();
        if (currentMillisRunAuto - previousMillisRunAuto >= intervalRunAuto)
        {
            previousMillisRunAuto = currentMillisRunAuto;
            refreshScreen = true;
        }
    }
}