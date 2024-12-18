#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <Sgp4.h>
#include <Ticker.h>
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Configuração do display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pinos dos botões
#define BUTTON_PIN_NEXT 25
#define BUTTON_PIN_PREV 26
#define BUTTON_PIN_SELECT 27

// Constantes e objetos globais
Ticker tkSecond;
TinyGPSPlus gps;
HardwareSerial SerialGPS(1);
Sgp4 sat;
int timezone = -3;
int currentSatelliteIndex = 0;
int currentMenuIndex = 0;
int topMenuIndex = 0;
int year, mon, day, hr, minute;
double sec;
time_t unixtime;

struct SatelliteData {
    char name[20];
    char tle_line1[70];
    char tle_line2[70];
};

// Lista de satélites
SatelliteData satellites[] = {
    {"NOAA 18", "1 28654U 05018A   24317.45338608  .00000595  00000-0  34008-3 0  9990", "2 28654  98.8636  33.1176 0014143   7.1329 353.0042 14.13417218 3991"},
    {"NOAA 19", "1 33591U 09005A   24318.41757082  .00000570  00000-0  32880-3 0  9993", "2 33591  99.0288  15.6704 0013119 228.8371 131.1669 14.13198506 8127"},
    {"NOAA 15", "1 25338U 98030A   24322.12477685  .00000308  00000-0  14484-3 0  9999", "2 25338  98.5561 345.9880 0009361 234.0269 126.0044 14.26797587379247"},
    {"METEOR M2-3", "1 57166U 23091A   24321.45718246  .00000087  00000-0  57049-4 0  9993", "2 57166  98.6936  14.6197 0004840  58.0639 302.1010 14.23951960 7229"},
    {"METEOR M2-4", "1 59051U 24039A   24315.95621167  .00000309  00000-0  15833-3 0  9995", "2 59051  98.6195 274.3780 0008138  62.7356 297.4650 14.22300252 3635"},
    {"METEOR M2-2", "1 44387U 19038A   24316.56942593  .00000613  00000-0  28629-3 0  9996", "2 44387  98.8482 286.5267 0000176 221.6660 138.4505 14.24060007 2783"},
    {"ISS (ZARYA)", "1 25544U 98067A   24321.39105834  .00018803  00000-0  33810-3 0  9993", "2 25544  51.6414 281.3213 0007607 222.9711 273.8988 15.49854462 8215"},
    {"GOES 17", "1 43226U 18022A   23290.00000000  .00000000  00000-0  00000-0 0  9993", "2 43226   0.0150  89.9995 0001650 180.0000 180.0000  1.00270000  0000"},
    {"GOES 16", "1 41866U 16071A   23290.00000000  .00000000  00000-0  00000-0 0  9995", "2 41866   0.0170  89.9999 0001650 180.0000 180.0000  1.00270000  0000"}
};

const int numSatellites = sizeof(satellites) / sizeof(SatelliteData);
const int maxVisibleItems = 3;

// Atualiza dados do GPS e localização do satélite
void processGPSData() {
    if (gps.date.isUpdated() && gps.time.isUpdated()) {
        struct tm timeinfo;
        timeinfo.tm_year = gps.date.year() - 1900;
        timeinfo.tm_mon = gps.date.month() - 1;
        timeinfo.tm_mday = gps.date.day();
        timeinfo.tm_hour = gps.time.hour();
        timeinfo.tm_min = gps.time.minute();
        timeinfo.tm_sec = gps.time.second();
        timeinfo.tm_isdst = 0;

        unixtime = mktime(&timeinfo);

        if (gps.location.isValid() && gps.altitude.isValid()) {
            double lat = gps.location.lat();
            double lon = gps.location.lng();
            double alt = gps.altitude.meters();
            sat.site(lat, lon, alt);
        }

        sat.findsat(static_cast<unsigned long>(unixtime));
    }
}


// Função para exibir o menu com rolagem
void displayMenu() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    if (currentMenuIndex == 0) {
        display.setCursor(0, 0);
        display.print("Selecione Sat:");

        // Ajusta o topMenuIndex para garantir que o satélite selecionado esteja visível
        if (currentSatelliteIndex < topMenuIndex) {
            topMenuIndex = currentSatelliteIndex;
        } else if (currentSatelliteIndex >= topMenuIndex + maxVisibleItems) {
            topMenuIndex = currentSatelliteIndex - maxVisibleItems + 1;
        }

        for (int i = 0; i < maxVisibleItems; i++) {
            int satelliteIndex = topMenuIndex + i;
            if (satelliteIndex >= numSatellites) break;

            display.setCursor(0, 8 + i * 8);
            if (satelliteIndex == currentSatelliteIndex) {
                display.print("> ");
            } else {
                display.print("  ");
            }
            display.print(satellites[satelliteIndex].name);
        }
    } 
              display.display();
}

// Função para atualizar os dados do satélite a cada segundo
void Second_Tick() {

    passinfo overpass; 
    sat.initpredpoint(static_cast<unsigned long>(unixtime), 0.0);

    if (currentMenuIndex == 1) {  
        invjday(sat.satJd, timezone, true, year, mon, day, hr, minute, sec);
        int error = sat.nextpass(&overpass, 20); // Limit of 20 maximums below horizon
 
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);

        display.setCursor(0, 0);
        display.printf("%02d:%02d:%02d", hr, minute, (int)sec);

        display.setCursor(0, 8);
        display.print("Sat: ");
        display.print(satellites[currentSatelliteIndex].name);

        display.setCursor(0, 16);
        display.print("Az:");
        display.print(sat.satAz, 0);
        display.print(" El:");
        display.print(sat.satEl, 0);

        display.setCursor(0, 24);
        if ( error == 1){
          
          invjday(overpass.jdstart ,timezone ,true , year, mon, day, hr, minute, sec);
          display.print(String(hr) + ':' + String(minute) + ':' + String(sec));
                    
        }else{
            display.print("Prediction error");
        }
        display.display();
    }
}


// Função para inicializar os dados do satélite
void initSatellite(int index) {
    sat.init(satellites[index].name, satellites[index].tle_line1, satellites[index].tle_line2);

    double jdC = sat.satrec.jdsatepoch;
    invjday(jdC, timezone, true, year, mon, day, hr, minute, sec);
}


// Função para gerenciar os botões e o menu
void handleButtonPresses() {
    if (digitalRead(BUTTON_PIN_NEXT) == LOW) {
        if (currentMenuIndex == 0) { 
            currentSatelliteIndex = (currentSatelliteIndex + 1) % numSatellites;

            if (currentSatelliteIndex >= topMenuIndex + maxVisibleItems) {
                topMenuIndex = currentSatelliteIndex - maxVisibleItems + 1;
                if (topMenuIndex > numSatellites - maxVisibleItems) {
                    topMenuIndex = numSatellites - maxVisibleItems;
                }
            }
            displayMenu();
        }
        delay(200);
    }

    if (digitalRead(BUTTON_PIN_PREV) == LOW) {
        if (currentMenuIndex == 0) { 
            currentSatelliteIndex = (currentSatelliteIndex - 1 + numSatellites) % numSatellites;

            if (currentSatelliteIndex < topMenuIndex) {
                topMenuIndex = currentSatelliteIndex;
                if (topMenuIndex < 0) {
                    topMenuIndex = 0;
                }
            }
            displayMenu();
        }
        delay(200); 
    }

    if (digitalRead(BUTTON_PIN_SELECT) == LOW) {
        if (currentMenuIndex == 0) { 
            initSatellite(currentSatelliteIndex);
            currentMenuIndex = 1;  
        } else if (currentMenuIndex == 1) {
            currentMenuIndex = 0;
        }
        displayMenu();
        delay(200);
    }
}

void setup() {
    Serial.begin(115200);
    SerialGPS.begin(9600, SERIAL_8N1, 16, 17);

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("Falha na inicialização do display SSD1306"));
        for(;;);
    }
    display.clearDisplay();
    display.display();
    
    pinMode(BUTTON_PIN_NEXT, INPUT_PULLUP);
    pinMode(BUTTON_PIN_PREV, INPUT_PULLUP);
    pinMode(BUTTON_PIN_SELECT, INPUT_PULLUP);

    displayMenu();
    
    // Configura o ticker para atualizar os dados do satélite a cada segundo
    tkSecond.attach(1, Second_Tick);
}

void loop() {
    while (SerialGPS.available() > 0) {
        gps.encode(SerialGPS.read());
        processGPSData();
    }
    handleButtonPresses();
}
