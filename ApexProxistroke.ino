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
#define SYSTEM_NAME "Apex Proxistroke"
#define VERSION_DATE "v1.4 - 14/03/2026"

// --- BUTTON PINS ---
#define BUTTON_UP    7 
#define BUTTON_DOWN  8 
#define BUTTON_SELECT 6

// --- RFID READER DEFINITIONS ---
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);

// --- STORAGE CONSTANTS ---
const int MAX_DATA_LENGTH = 64;
const int APP_COUNT = 5;
const int CONFIG_INDEX = APP_COUNT;
const int TOTAL_MENU_COUNT = APP_COUNT + 1;

// --- AUTHORISATION SLOTS ---
const int AUTH_SLOTS = 3; 
byte authorisedUIDs[AUTH_SLOTS][4]; 

char apps[APP_COUNT + 1][MAX_DATA_LENGTH];
char usernames[APP_COUNT + 1][MAX_DATA_LENGTH]; 
char passwords[APP_COUNT + 1][MAX_DATA_LENGTH];

int currentAppIndex = 0; 
int menuLevel = 0; 

// --- UK KEYBOARD FIX HELPER ---
void typeUK(char* text) {
  for (int i = 0; i < (int)strlen(text); i++) {
    char c = text[i];
    if (c == '@') Keyboard.write('"');      
    else if (c == '"') Keyboard.write('@'); 
    else if (c == '#') Keyboard.write((char)0x32); 
    else Keyboard.write(c);
  }
}

// --- BOOT SEQUENCE ---
void splashScreen() {
    lcd.clear();
    int namePos = (20 - strlen(SYSTEM_NAME)) / 2;
    lcd.setCursor(namePos, 0);
    lcd.print(F(SYSTEM_NAME));
    
    int verPos = (20 - strlen(VERSION_DATE)) / 2;
    lcd.setCursor(verPos, 1);
    lcd.print(F(VERSION_DATE));
    
    lcd.setCursor(3, 3);
    lcd.print(F("Initialising..."));
    
    for (int i = 0; i < 20; i++) {
        lcd.setCursor(i, 2);
        lcd.print(F("."));
        delay(60);
    }
    delay(400);
}

// --- DATA PERSISTENCE ---
void saveDataToEEPROM() {
    int addr = 0;
    for (int i = 0; i < APP_COUNT; i++) {
        EEPROM.put(addr, apps[i]);      addr += MAX_DATA_LENGTH;
        EEPROM.put(addr, usernames[i]); addr += MAX_DATA_LENGTH;
        EEPROM.put(addr, passwords[i]); addr += MAX_DATA_LENGTH;
    }
    addr += 10; 
    for(int s = 0; s < AUTH_SLOTS; s++) {
        for(int b = 0; b < 4; b++) {
            EEPROM.write(addr++, authorisedUIDs[s][b]);
        }
    }
    Serial.println(F(">>> SUCCESS: Settings saved to EEPROM."));
}

void loadDataFromEEPROM() {
    int addr = 0;
    for (int i = 0; i < APP_COUNT; i++) {
        EEPROM.get(addr, apps[i]);      addr += MAX_DATA_LENGTH;
        EEPROM.get(addr, usernames[i]); addr += MAX_DATA_LENGTH;
        EEPROM.get(addr, passwords[i]); addr += MAX_DATA_LENGTH;
    }
    addr += 10;
    for(int s = 0; s < AUTH_SLOTS; s++) {
        for(int b = 0; b < 4; b++) {
            authorisedUIDs[s][b] = EEPROM.read(addr++);
        }
    }
    if (apps[0][0] == 0 || (byte)apps[0][0] == 255) {
        strcpy(apps[0], "Windows Logon");
        strcpy(usernames[0], "user");
        strcpy(passwords[0], "pass");
    }
    strcpy_P(apps[CONFIG_INDEX], PSTR("*** Configure ***"));
}

void setup() {
    Serial.begin(9600);
    lcd.init();
    lcd.backlight();
    splashScreen();
    SPI.begin();
    mfrc522.PCD_Init();
    Keyboard.begin();
    pinMode(BUTTON_UP, INPUT_PULLUP);
    pinMode(BUTTON_DOWN, INPUT_PULLUP);
    pinMode(BUTTON_SELECT, INPUT_PULLUP);
    loadDataFromEEPROM();
    updateMenu();
}

void loop() {
    handleInput();
    if (menuLevel == 2) configureMode();
    delay(10);
}

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
    lcd.setCursor(3, 0); lcd.print(F("AUTHORISATION"));
    lcd.setCursor(2, 1); lcd.print(F("TAP MASTER/USER"));
    delay(400); 
    unsigned long startWait = millis();
    while (millis() - startWait < 10000) {
        int timeLeft = (10000 - (millis() - startWait)) / 1000;
        lcd.setCursor(0, 3);
        lcd.print(F("BTN to Cancel | ")); lcd.print(timeLeft); lcd.print(F("s "));
        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
            int matchedSlot = -1;
            for (int s = 0; s < AUTH_SLOTS; s++) {
                bool match = true;
                for (byte i = 0; i < 4; i++) {
                    if (mfrc522.uid.uidByte[i] != authorisedUIDs[s][i]) match = false;
                }
                if (match) { matchedSlot = s; break; }
            }
            mfrc522.PICC_HaltA();
            if (matchedSlot != -1) {
                lcd.setCursor(2, 2);
                lcd.print(matchedSlot == 0 ? F("MASTER VERIFIED") : F(" USER VERIFIED"));
                delay(1500); return true;
            } else {
                lcd.setCursor(4, 2); lcd.print(F("INVALID KEY!"));
                delay(1500); return false;
            }
        }
        if (digitalRead(BUTTON_SELECT) == LOW) return false;
        delay(50);
    }
    return false;
}

void handleInput() {
    static unsigned long lastMove = 0;
    if (digitalRead(BUTTON_UP) == LOW && millis() - lastMove > 200) {
        if (menuLevel == 0) {
            currentAppIndex = (currentAppIndex <= 0) ? TOTAL_MENU_COUNT - 1 : currentAppIndex - 1;
            updateMenu();
        }
        lastMove = millis();
    }
    if (digitalRead(BUTTON_DOWN) == LOW && millis() - lastMove > 200) {
        if (menuLevel == 0) {
            currentAppIndex = (currentAppIndex >= TOTAL_MENU_COUNT - 1) ? 0 : currentAppIndex + 1;
            updateMenu();
        }
        lastMove = millis();
    }
    if (digitalRead(BUTTON_SELECT) == LOW && millis() - lastMove > 400) {
        lastMove = millis();
        if (menuLevel == 2) {
            menuLevel = 0; updateMenu();
        } else if (currentAppIndex == CONFIG_INDEX) { 
            if (isAuthorised()) {
                menuLevel = 2; lcd.clear();
                lcd.setCursor(3, 1); lcd.print(F("SERIAL CONFIG"));
                lcd.setCursor(4, 2); lcd.print(F("MODE ACTIVE"));
                Serial.println(F("\n========================================"));
                Serial.print(F("   ")); Serial.println(SYSTEM_NAME);
                Serial.print(F("   ")); Serial.println(VERSION_DATE);
                Serial.println(F("   STATUS: AUTHORISED"));
                Serial.println(F("   BAUD RATE: 9600"));
                Serial.println(F("========================================"));
                Serial.println(F("COMMANDS: DUMP, AUTH <0-2>, EDIT, WRITE, EXIT"));
            } else { updateMenu(); }
        } else {
            if (isAuthorised()) {
                lcd.clear(); lcd.setCursor(4, 1); lcd.print(F("LOGGING IN..."));
                typeUK(usernames[currentAppIndex]);
                Keyboard.write(KEY_TAB);
                typeUK(passwords[currentAppIndex]);
                Keyboard.write(KEY_RETURN);
                delay(2000);
            }
            updateMenu();
        }
    }
}

void configureMode() {
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n'); line.trim();
        String command = line; command.toUpperCase();
        if (command.equalsIgnoreCase("DUMP")) {
            Serial.println(F("\n--- CURRENT DATA DUMP ---"));
            for (int i = 0; i < APP_COUNT; i++) {
                Serial.print(i); Serial.print(F(": ")); Serial.print(apps[i]);
                Serial.print(F(" | U: ")); Serial.print(usernames[i]);
                Serial.print(F(" | P: ")); Serial.println(passwords[i]);
            }
        } else if (command.startsWith("AUTH")) {
            int slot = line.substring(5).toInt();
            if (slot < 0 || slot >= AUTH_SLOTS) {
                Serial.println(F("ERROR: Use AUTH 0, 1, or 2"));
            } else {
                Serial.println(F("TAP CARD NOW..."));
                unsigned long startAuth = millis();
                while(millis() - startAuth < 10000) {
                    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
                        for(int i=0; i<4; i++) { authorisedUIDs[slot][i] = mfrc522.uid.uidByte[i]; }
                        Serial.print(F("SUCCESS. Slot ")); Serial.print(slot); Serial.println(F(" Updated. Type WRITE to save."));
                        break;
                    }
                }
            }
        } else if (command.equalsIgnoreCase("WRITE")) { saveDataToEEPROM(); } 
          else if (command.equalsIgnoreCase("EXIT")) { menuLevel = 0; updateMenu(); Serial.println(F("Exiting...")); }
    }
}