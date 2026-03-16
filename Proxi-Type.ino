#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h> 
#include <Keyboard.h>
#include <EEPROM.h>

// --- I2C LCD DEFINITIONS ---
#define LCD_ADDR 0x27
#define LCD_COLS 20
#define LCD_ROWS 4
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

// --- VERSION INFO ---
#define SYSTEM_NAME "Proxi-Type"
#define VERSION_DATE "v1.6.1 - 16/03/2026"

// --- PIN DEFINITIONS ---
#define BUTTON_UP    7 
#define BUTTON_DOWN  8 
#define BUTTON_SELECT 6
#define SS_PIN 10
#define RST_PIN 9

MFRC522 mfrc522(SS_PIN, RST_PIN);

// --- STORAGE ---
const int MAX_DATA_LENGTH = 64;
const int APP_COUNT = 5;
const int CONFIG_INDEX = APP_COUNT;
const int TOTAL_MENU_COUNT = APP_COUNT + 1;
const int AUTH_SLOTS = 3;

byte authorisedUIDs[AUTH_SLOTS][4]; 
char apps[APP_COUNT + 1][MAX_DATA_LENGTH];
char usernames[APP_COUNT + 1][MAX_DATA_LENGTH]; 
char passwords[APP_COUNT + 1][MAX_DATA_LENGTH];
bool loginStepDelay[APP_COUNT]; // Stores if app needs Return+Delay [cite: 5]

int currentAppIndex = 0; 
int menuLevel = 0; 
unsigned long lastMove = 0; 

// --- UTILITIES ---

void typeUK(char* text) {
  for (int i = 0; i < (int)strlen(text); i++) {
    char c = text[i];
    if (c == '@') Keyboard.write('"');      
    else if (c == '"') Keyboard.write('@'); 
    else if (c == '#') Keyboard.write((char)0x32); 
    else Keyboard.write(c);
  }
}

void saveDataToEEPROM() {
    int addr = 0;
    for (int i = 0; i < APP_COUNT; i++) {
        EEPROM.put(addr, apps[i]);      addr += MAX_DATA_LENGTH;
        EEPROM.put(addr, usernames[i]); addr += MAX_DATA_LENGTH;
        EEPROM.put(addr, passwords[i]); addr += MAX_DATA_LENGTH;
        EEPROM.put(addr, loginStepDelay[i]); addr += sizeof(bool); // [cite: 9, 10]
    }
    addr += 10; 
    for(int s = 0; s < AUTH_SLOTS; s++) {
        for(int b = 0; b < 4; b++) { EEPROM.write(addr++, authorisedUIDs[s][b]); }
    }
}

void loadDataFromEEPROM() {
    int addr = 0;
    for (int i = 0; i < APP_COUNT; i++) {
        EEPROM.get(addr, apps[i]);      addr += MAX_DATA_LENGTH;
        EEPROM.get(addr, usernames[i]); addr += MAX_DATA_LENGTH;
        EEPROM.get(addr, passwords[i]); addr += MAX_DATA_LENGTH;
        EEPROM.get(addr, loginStepDelay[i]); addr += sizeof(bool); // [cite: 15, 16]
    }
    addr += 10;
    for(int s = 0; s < AUTH_SLOTS; s++) {
        for(int b = 0; b < 4; b++) { authorisedUIDs[s][b] = EEPROM.read(addr++); }
    }
    if (apps[0][0] == 0 || (byte)apps[0][0] == 255) {
        strcpy(apps[0], "Windows Logon"); strcpy(usernames[0], "user"); strcpy(passwords[0], "pass");
        loginStepDelay[0] = false;
    }
    strcpy_P(apps[CONFIG_INDEX], PSTR("*** Configure ***"));
}

void wipeSystem() {
    for(int s=0; s<AUTH_SLOTS; s++) for(int b=0; b<4; b++) authorisedUIDs[s][b] = 0;
    for(int i=0; i<APP_COUNT; i++) {
        strcpy(apps[i], "Empty Slot"); strcpy(usernames[i], "user"); 
        strcpy(passwords[i], "pass"); loginStepDelay[i] = false;
    }
    saveDataToEEPROM();
}

// --- AUTH & UI ---

void updateMenu() {
    lcd.clear();
    int startItem = (currentAppIndex >= LCD_ROWS) ? currentAppIndex - (LCD_ROWS - 1) : 0;
    for (int i = 0; i < LCD_ROWS; i++) {
        int itemIdx = startItem + i;
        if (itemIdx < TOTAL_MENU_COUNT) {
            lcd.setCursor(0, i);
            lcd.print(itemIdx == currentAppIndex ? F(">") : F(" ")); 
            lcd.print(apps[itemIdx]);
        }
    }
}

bool isAuthorised() {
    lcd.clear();
    lcd.setCursor(4, 0); lcd.print(F("PROXI-TYPE"));
    lcd.setCursor(2, 1); lcd.print(F("TAP MASTER/USER"));
    unsigned long startWait = millis();
    while (millis() - startWait < 10000) {
        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
            int matchedSlot = -1;
            for (int s = 0; s < AUTH_SLOTS; s++) {
                bool match = true;
                for (byte i = 0; i < 4; i++) { if (mfrc522.uid.uidByte[i] != authorisedUIDs[s][i]) match = false; }
                if (match) { matchedSlot = s; break; }
            }
            mfrc522.PICC_HaltA();
            if (matchedSlot != -1) {
                lcd.setCursor(2, 2); lcd.print(F("ACCESS GRANTED"));
                delay(1000); return true;
            } else {
                lcd.setCursor(4, 2); lcd.print(F("INVALID KEY!")); delay(1000); return false;
            }
        }
        if (digitalRead(BUTTON_SELECT) == LOW) return false;
        delay(50);
    }
    return false;
}

// --- CONFIG MODE ---

void serialEdit() {
    Serial.println(F("\nSlot (0-4):"));
    while (!Serial.available());
    int slot = Serial.parseInt();
    while (Serial.available()) Serial.read(); 

    if (slot >= 0 && slot < APP_COUNT) {
        Serial.println(F("New Service Name:"));
        while (!Serial.available()); String sName = Serial.readStringUntil('\n'); sName.trim();
        sName.toCharArray(apps[slot], MAX_DATA_LENGTH);

        Serial.println(F("New Username:"));
        while (!Serial.available()); String uName = Serial.readStringUntil('\n'); uName.trim();
        uName.toCharArray(usernames[slot], MAX_DATA_LENGTH);

        Serial.println(F("New Password:"));
        while (!Serial.available()); String pWord = Serial.readStringUntil('\n'); pWord.trim();
        pWord.toCharArray(passwords[slot], MAX_DATA_LENGTH);

        Serial.println(F("Login Step: (0=Tab, 1=Return+Delay)")); // 
        while (!Serial.available());
        loginStepDelay[slot] = (Serial.parseInt() == 1);
        while (Serial.available()) Serial.read();

        Serial.println(F("Updated. Type WRITE to save."));
    }
}

void configureMode() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n'); command.trim(); command.toUpperCase();
        if (command == "DUMP") {
            Serial.println(F("\n--- PROXI-TYPE DATA ---"));
            for (int i = 0; i < APP_COUNT; i++) {
                Serial.print(i); Serial.print(F(": ")); Serial.print(apps[i]);
                Serial.print(F(" | Delay: ")); Serial.print(loginStepDelay[i] ? "YES" : "NO");
                Serial.print(F(" | U: ")); Serial.print(usernames[i]);
                Serial.print(F(" | P: ")); Serial.println(passwords[i]);
            }
        } 
        else if (command == "EDIT") { serialEdit(); }
        else if (command.startsWith("AUTH")) {
            int slot = command.substring(5).toInt();
            Serial.println(F("TAP CARD..."));
            unsigned long sA = millis();
            while(millis() - sA < 10000) {
                if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
                    for(int i=0; i<4; i++) authorisedUIDs[slot][i] = mfrc522.uid.uidByte[i];
                    Serial.println(F("CARD RECORDED.")); break;
                }
            }
        }
        else if (command == "WRITE") { saveDataToEEPROM(); Serial.println(F("EEPROM UPDATED.")); } 
        else if (command == "RESET") { wipeSystem(); Serial.println(F("SYSTEM WIPED.")); }
        else if (command == "EXIT") { menuLevel = 0; updateMenu(); }
    }
}

// --- CORE ---

void setup() {
    Serial.begin(9600);
    lcd.init(); lcd.backlight();
    pinMode(BUTTON_UP, INPUT_PULLUP);
    pinMode(BUTTON_DOWN, INPUT_PULLUP);
    pinMode(BUTTON_SELECT, INPUT_PULLUP);

    if (digitalRead(BUTTON_UP) == LOW && digitalRead(BUTTON_DOWN) == LOW) {
        lcd.clear(); lcd.print(F("HARDWARE RESET"));
        int c = 0;
        while (c < 3) {
            if (digitalRead(BUTTON_SELECT) == LOW) {
                c++; while(digitalRead(BUTTON_SELECT) == LOW); delay(200);
            }
        }
        wipeSystem();
    }

    SPI.begin(); mfrc522.PCD_Init();
    Keyboard.begin();
    loadDataFromEEPROM();
    updateMenu();
}

void loop() {
    if (menuLevel == 0) {
        if (digitalRead(BUTTON_UP) == LOW && (millis() - lastMove > 200)) {
            currentAppIndex = (currentAppIndex <= 0) ? TOTAL_MENU_COUNT - 1 : currentAppIndex - 1;
            updateMenu(); lastMove = millis();
        }
        if (digitalRead(BUTTON_DOWN) == LOW && (millis() - lastMove > 200)) {
            currentAppIndex = (currentAppIndex >= TOTAL_MENU_COUNT - 1) ? 0 : currentAppIndex + 1;
            updateMenu(); lastMove = millis();
        }
    }

    if (digitalRead(BUTTON_SELECT) == LOW && (millis() - lastMove > 500)) {
        lastMove = millis();
        while(digitalRead(BUTTON_SELECT) == LOW); 

        if (menuLevel == 2) { 
            menuLevel = 0; updateMenu(); 
        } 
        else if (currentAppIndex == CONFIG_INDEX) { 
            bool masterKeySet = false;
            for(int i=0; i<4; i++) { if(authorisedUIDs[0][i] != 0) masterKeySet = true; }
            if (!masterKeySet || isAuthorised()) { 
                menuLevel = 2; 
                lcd.clear(); lcd.setCursor(3, 1); lcd.print(F("SERIAL CONFIG"));
                Serial.println(F("\nPROXI-TYPE CONFIG: DUMP, EDIT, AUTH, WRITE, EXIT"));
            } else { updateMenu(); }
        } 
        else {
            if (isAuthorised()) {
                lcd.clear(); lcd.print(F("PROXI-TYPING..."));
                typeUK(usernames[currentAppIndex]);
                
                if (loginStepDelay[currentAppIndex]) {
                  Keyboard.write(KEY_RETURN); 
                  delay(1200); // Wait for page transition
                } else {
                  Keyboard.write(KEY_TAB); 
                }

                typeUK(passwords[currentAppIndex]); 
                Keyboard.write(KEY_RETURN);
                delay(2000); 
            }
            updateMenu();
        }
    }

    if (menuLevel == 2) configureMode();
    delay(10);
}
