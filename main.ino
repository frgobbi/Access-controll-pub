/*     RFID RC522          |      microSD Adapter       |          Pin Pad        |
  RC522 MODULE    MEGA     |   microSD MODULE    MEGA   |    da destra a sinistra |
  SDA              D49     |   CS               D43     |         dal 22 al 29    |
  SCK              D52     |   SCK              D52     |-------------------------|
  MOSI             D51     |   MOSI             D51     |       LCD I2C           |
  MISO             D50     |   MISO             D50     |   LCD MODULE    MEGA    |
  IRQ              N/A     |   GND              GND     |   VCC           5V      |
  GND              GND     |   5V               5V      |   GND           GND     |
  RST              D53     |    NB: Il miso della SD    |   SDA           D20     |
  3.3V             3.3V    |    ha bisogno di una       |   SLC           D21     |
                           |    resistenza 330 Ohm      |                         |
*/
//Librerie
#include <MFRC522.h>
#include <SD.h>
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//PIN DISPOSIVI
# define PIN_CS 43
# define PIN_RST 48
# define PIN_SDA 49
//PIN LED E RELE'
#define LED_G 3
#define LED_B 4
#define LED_R 5
#define RELE 9
//NUM MASSIMO DI CODICI AMMESSI
#define MAXCodici 30
//TEMPI DI ATTESA
#define TIME_OPEN 3000
#define TIME_BLINKY 250
#define TIME_PIN 30000
#define TIME_CHANGE 60000

int numCodici, flagBlock, flagChange, nowTime, TLC, Tchange;
String pin, pinValid;
String arrayCode[MAXCodici];

const String CODE_MASTER = "#########";
const byte RC = 4;

char hexaKeys[RC][RC] = {{'1', '2', '3', 'A'},
                         {'4', '5', '6', 'B'},
                         {'7', '8', '9', 'C'},
                         {'*', '0', '#', 'D'}};
byte rowPins[RC] = {29, 28, 27, 26};
byte colPins[RC] = {25, 24, 23, 22};
Keypad pinpad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, RC, RC);
LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD I2C address
MFRC522 rfid(PIN_SDA, PIN_RST);
File file;

void setup() {
    flagBlock = 0;
    numCodici = 0;
    flagChange = 0;

    Serial.begin(9600);
    SPI.begin();
    Serial.println("Avvio Programma");

    pinMode(LED_G, OUTPUT);
    pinMode(LED_R, OUTPUT);
    pinMode(LED_B, OUTPUT);
    pinMode(RELE, OUTPUT);
    pinMode(PIN_CS, OUTPUT);
    pinMode(PIN_SDA, OUTPUT);
    digitalWrite(RELE, LOW);

    changeShield(0);

    rfid.PCD_Init();// inizializzo l’oggetto rfid
    Serial.println("RFID inizializzato");

    changeShield(1);
    if (!SD.begin(PIN_CS)) {
        Serial.println("SCHEDA NON INSERITA");
    } else {
        Serial.println("SCHEDA INSERITA");
    }

    getCode();
    getPin();
    changeShield(0);

    TLC = millis();
    Tchange = millis();
    lcd.init();
    lcd.backlight();
    Serial.println("Pronto");
    lcd.setCursor(0, 0);
    lcd.print("Controllo Porta");
    lcd.setCursor(0, 1);
    lcd.print("   Brusco Pub   ");

    ledOn();
}

void loop() {

    nowTime = millis();
    if ((nowTime - TLC) >= TIME_PIN) {
        pin = "";
    }

    nowTime = millis();
    if ((nowTime - Tchange) >= TIME_CHANGE) {
        flagChange = 0;
        pin = "";
    }

    char customKey = pinpad.getKey();

    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        String uid = readUID();
        if (uid.compareTo(CODE_MASTER) == 0) {
            allLedOn();
            rfid.PICC_HaltA();
            int aspetta = 1;
            while (aspetta) {
                if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
                    String uid2 = readUID();
                    setArray(uid2);
                    aspetta = 0;
                }
            }
            ledOn();
        } else {
            if (flagBlock == 0) {
                controllaCode(uid);
            } else {
                blinkyThreeLed();
                ledOn();
            }

        }
        rfid.PICC_HaltA();
        delay(5);
    }

    if (customKey) {
        switch (customKey) {
            case 'A':
            Serial.println("A");
                if (flagChange == 0) {
                    if (flagBlock == 0) {
                        openDor(0);
                    }
                }
                break;
            case 'B':
            Serial.println("B");
                if (flagChange == 0) {
                    setPorta();
                }
                break;
            case 'C':
            Serial.println("C");
                pin = "";
                lcdON(0);
                Serial.println("Pin Inserito: " + pin);
                break;
            case 'D':
            Serial.println("D");
                changePin();
                Tchange = millis();
                break;
            default:
                if (pin.length() < 8) {
                    pin += customKey;
                    Serial.println("Pin Inserito: " + pin);
                    lcdON(0);
                    TLC = millis();
                    if (flagChange == 1) {
                        Tchange = millis();
                    }
                    //pin = String(pin);
                }
                break;
        }
    }
}

void changeShield(int b){
    if(b == 0){
        digitalWrite(PIN_CS, HIGH);
        digitalWrite(PIN_SDA, LOW);
    } else {
        digitalWrite(PIN_SDA, HIGH);
        digitalWrite(PIN_CS, LOW);
    }
}

void blinckyOneLed(int led) {
    digitalWrite(LED_G, LOW);
    digitalWrite(LED_R, LOW);
    digitalWrite(LED_B, LOW);
    for (int i = 0; i < 3; i++) {
        digitalWrite(led, HIGH);
        delay(TIME_BLINKY);
        digitalWrite(led, LOW);
        delay(TIME_BLINKY);
    }
}

void blinkyThreeLed() {
    digitalWrite(LED_G, LOW);
    digitalWrite(LED_R, LOW);
    digitalWrite(LED_B, LOW);
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_G, HIGH);
        digitalWrite(LED_R, HIGH);
        digitalWrite(LED_B, HIGH);
        delay(TIME_BLINKY);
        digitalWrite(LED_G, LOW);
        digitalWrite(LED_R, LOW);
        digitalWrite(LED_B, LOW);
        delay(TIME_BLINKY);
    }
}

void openDor(int flag_led) {
    if (flag_led == 1) {
        digitalWrite(LED_B, LOW);
        digitalWrite(LED_G, HIGH);
    }
    digitalWrite(RELE, HIGH);
    delay(TIME_OPEN);
    digitalWrite(RELE, LOW);
}

void error() {
    digitalWrite(LED_B, LOW);
    digitalWrite(LED_R, HIGH);
    delay(TIME_OPEN);
}

void ledOn() {
    digitalWrite(LED_G, LOW);
    digitalWrite(LED_R, LOW);
    digitalWrite(LED_B, HIGH);
}

void allLedOn() {
    digitalWrite(LED_G, HIGH);
    digitalWrite(LED_R, HIGH);
    digitalWrite(LED_B, HIGH);
}

String readUID() {
    String uid = "";
    for (int i = 0; i < rfid.uid.size; i++) {
        uid += rfid.uid.uidByte[i] < 0x10 ? "0" : "";
        uid += String(rfid.uid.uidByte[i], HEX);
    }
    rfid.PICC_HaltA();
    return uid;
}

void controllaCode(String uid) {
    int trovato = 0;
    for (int i = 0; i < numCodici; i++) {
        if (arrayCode[i].compareTo(uid) == 0) {
            trovato = 1;
        }
    }
    if (trovato == 1) {
        openDor(1);
    } else {
        error();
    }
    ledOn();
}

void setArray(String uid) {
    int trovato = 0;
    int indiceT = 0;
    String app = "";
    if(uid.compareTo("be032549")!=0){
        for (int i = 0; i < numCodici; i++) {
            if (arrayCode[i].compareTo(uid) == 0) {
                trovato = 1;
                indiceT = i;
            }
        }
    
        if (trovato == 1) { //Codice presente => Eliminazione
            arrayCode[indiceT] = "";
            Serial.println("ELIMINAZIONE CODICE");
            for (int i = indiceT; i < numCodici; i++) {
                if (arrayCode[i] == "" && arrayCode[i + 1] != "") {
                    if (i != MAXCodici) {
                        arrayCode[i] = arrayCode[i + 1];
                        arrayCode[i + 1] = "";
                    }
                }
            }
            numCodici--;
            blinckyOneLed(LED_R);
        } else {
            Serial.println("AGGIUNTA DI UN NUOVO CODICE");
            if (numCodici < 30) {
                arrayCode[numCodici] = uid;
                numCodici++;
                blinckyOneLed(LED_G);
            }
    
        }
        setFileCode();
    }
    /* Serial.println("STAMPA CODICI PERMESSI");
     for (int i = 0; i < numCodici; i++) {
         Serial.println(arrayCode[i]);
     }*/
}

void getCode() {
    String line = "";
    char ch;
    file = SD.open("codici.txt");
    if (file) {
        //Serial.println("Leggo il contenuto di codici.txt :");
        while (file.available()) {
            ch = file.read();
            if (ch == '\n') {
                arrayCode[numCodici] = String(line);
                numCodici++;
                //Serial.println("Codice valido: " + String(line));
                line = "";
            } else {
                line += String(ch);
            }
        }
    } else {
        Serial.println("File codici.txt inesistente");
    }
    file.close();
}

void setFileCode() {
    changeShield(1);
    if (SD.exists("codici.txt")) {
        SD.remove("codici.txt");
        //Serial.println("File già esiste");
    }

    file = SD.open("codici.txt", FILE_WRITE);
    if (file) {
        //Serial.print("Scrivo");
        for (int i = 0; i < numCodici; i++) {
            file.print(arrayCode[i]);
            file.print("\n");
        }
        file.close();
        Serial.println("Fatto");
    }
    changeShield(0);
}

void getPin() {
    String line = "";
    char ch;
    file = SD.open("pin.txt");
    if (file) {
        //Serial.println("Leggo il contenuto di pin.txt :");
        while (file.available()) {
            ch = file.read();
            if(ch != '\0'){
             if (ch == '\n') {
                //Serial.print("IL PIN e': ");
                Serial.println(line);
                pinValid = String(line);
                line = "";
                }else {
                  line += ch;
                }
            } 
        }
        file.close();
    } else {
        Serial.println("File pin.txt inesistente");
    }
}

void lcdON(int change) {
    lcd.clear();
    if (change == 0) {
        if (flagBlock == 1) {
            writeR(0, "Bloccato");
        } else {
            writeR(0, "Aperto");
        }
        if (pin.compareTo("") != 0) {
            writeR(1, pin);
        }
    } else {
        writeR(0, "  Cambia Pin   ");
        writeR(1, pin);
    }
}

void writeR(int riga, String messaggio) {
    if (riga == 0) {
        lcd.setCursor(0, 0);
        lcd.print(messaggio);
    } else {
        lcd.setCursor(0, 1);
        lcd.print("Pin: ");
        int lunghezza = messaggio.length();
        for (int i = 0; i < lunghezza; i++) {
            lcd.print("*");
        }
    }
}

void setPorta() {
  Serial.println("Metodo set porta");
    //pinValid = String(pinValid);
    Serial.println(pinValid+"="+pin);
    if (pinValid.compareTo(pin)==0) {
      Serial.println("uguali");
        if (flagBlock == 0) {
            flagBlock = 1;
        } else {
            flagBlock = 0;
        }
    }
    pin = "";
    lcdON(0);
}

void setPin(String pin) {
    changeShield(1);
    if (SD.exists("pin.txt")) {
        SD.remove("pin.txt");
        //Serial.println("File già esiste");
    }

    file = SD.open("pin.txt", FILE_WRITE);
    if (file) {
        file.print(pin);
        file.print("\n");
        file.close();
        Serial.println("Fatto");
    }
    changeShield(0);
}

void changePin() {
    pin = String(pin);
    if (flagChange == 0) {
        if (pinValid.compareTo(pin) == 0) {
            flagChange = 1;
        }
        Tchange = millis();
        pin = "";
        lcdON(1);
    } else {
        setPin(pin);
        flagChange = 0;
        pin = "";
    }

}
