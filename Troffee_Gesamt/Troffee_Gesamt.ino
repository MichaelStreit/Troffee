/**************************************************************************/
/*! 
  TROFFEE

  */
// ################################################################################
// LIBS
// ################################################################################ 
// Allgemein_______________________________________________________________/
#include <string.h>
#include <SPI.h>
// PN532___________________________________________________________________/
#include <Wire.h>
#include <PN532_I2C.h>
#include <NfcAdapter.h>
// RTC DS1307______________________________________________________________/
#include "RTClib.h"
// SD Card Reader__________________________________________________________/
/* Falls Probleme nutze das folgende anstelle SD.h
#include "SdFat.h"
SdFat SD;
*/
 #include <SD.h>
// LCD Display_____________________________________________________________/
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789

// Encoder
#include <Encoder.h>

// ################################################################################
// DEFINITIONEN
// ################################################################################
// SD Kartenleser______________________________________________________/
#define SD_MISO_PIN 50 // Master Input Slave Output
#define SD_MOSI_PIN 51 // Master Output Slave Input
#define SD_SCLK_PIN 52 // Serial Clock
#define SD_CS_PIN 53 //CS (=ChipSelect), manchmal auch SS (=SlaveSelect)

// LCD Display_____________________________________________________________/
#define LCD_CS_PIN 43 //CS (=ChipSelect), manchmal auch SS (=SlaveSelect)
#define LCD_DC_PIN A7 //Data command control PIN
#define LCD_MOSI_PIN 41 // Master Output Slave Input
#define LCD_SCLK_PIN 39 // Serial Clock
#define LCD_RST_PIN A8 // Reset PIN

Adafruit_ST7789 LCD = Adafruit_ST7789(LCD_CS_PIN, LCD_DC_PIN, LCD_MOSI_PIN, LCD_SCLK_PIN, LCD_RST_PIN);
#define LCD_AnzahlBuchstabenXvon8 45 // Zeichen passen in eine Zeile bei der Groesse von Buchstaben
#define LCD_AnzahlBuchstabenXvon5 27 // Zeichen passen in eine Zeile bei der Groesse von Buchstaben
#define LCD_AnzahlBuchstabenHeadline 20
char LCD_TextInZeileXvon8[8][LCD_AnzahlBuchstabenXvon8] = {{" "},{" "},{" "},{" "},{" "},{" "},{" "},{" "}};
char LCD_TextInZeileXvon5[8][LCD_AnzahlBuchstabenXvon5] = {{" "},{" "},{" "},{" "},{" "},{" "},{" "},{" "}};
char LCD_TextInSpalte2ZeileXvon8[8][LCD_AnzahlBuchstabenXvon5] = {{" "},{" "},{" "},{" "},{" "},{" "},{" "},{" "}};
char LCD_HeaderUhrzeitAlt[6] = "00:00";
char LCD_HeaderDatumAlt[11] = "01.01.2001";
char LCD_Headline[LCD_AnzahlBuchstabenHeadline] = "Setup";

// Allgemein_____________________________________________________________/
uint32_t time = 2000;

// PN532__________________________________________________________________/
PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);
// RTC DS1307_____________________________________________________________/
// RTC_DS1307 rtc;
RTC_DS3231 rtc;
// SD Card Reader_________________________________________________________/
File myFile;
#define ZeilenNames 50  // Zeilen des Textfiles "Names.TXT" zum Reservieren von Speicherplatz
#define SpaltenNames 4 // Spalten zum Parsen des Textfiles "Names.TXT"
int AnzahlMappingEintraege = 0;
int ZeileDerAktuellenKarte = 0;
int TagesZaehlerKaffee = 0;
int MonatsZaehlerKaffee = 0;
int VormonatsZaehlerKaffee = 0;
int JahresZaehlerKaffee = 0;

#define LaengeUID 17 // Maximallaenge UID
#define LaengeVorname 15 // Maximallaenge Vorname
#define LaengeNachname 15 // Maximallaenge Nachname
struct NamesTXT
{
  char UID[LaengeUID];
  char Vorname[LaengeVorname];
  char Nachname[LaengeNachname];
  char LieblingsKaffee[2];
};
NamesTXT NameMapping[ZeilenNames];

// Wird übers Config File eingelesen:______________________________
char PreisHalbeTasseLatte[5] = "";
char PreisGanzeTasseLatte[5] = "";
char PreisHalbeTasseSchwarz[5] = "";
char PreisGanzeTasseSchwarz[5] = "";
//_________________________________________________________________
// KY-040 Drehknopf________________________________________________________/
const int ENC_PIN_CLK = 27;
const int ENC_PIN_DT = 25;
const int ENC_PIN_SW = 23;
long encoderPos = 0;
long lastEncoderPos = 0;

Encoder encoder(ENC_PIN_DT,ENC_PIN_CLK);

//__________________________________________________________________
// Bitmaps
extern uint8_t dampf_1[];
extern uint8_t dampf_2[];
extern uint8_t tasse[];

#define Dampf_width 116
#define Dampf_height 54
#define Tasse_width 116
#define Tasse_height 67

bool dampfswitch = false;
//___________________________________________________________________
// Time

uint8_t u8MinuteOld = 0;
uint8_t u8HourOld = 0;
uint8_t u8DayOld = 0;
uint8_t u8MonthOld = 0;
uint16_t u16YearOld = 0;


// ################################################################################
// SETUP 
// ################################################################################ 
void setup() {
  //Serial.begin(115200);
  SetupLCD();
  LCD_SchreibeHeadline("Setup");
  LCD.drawLine(0, 40, 320, 40, ST77XX_WHITE);

  SetupRTC();
  SetupPN532();
  SetupSDCard();
  ReadNamesTXT();
  ReadConfigTXT();  
  // KY-040 Drehknopf________________________________________________________/  

  //pinMode(ENC_PIN_CLK, INPUT_PULLUP);
  //pinMode(ENC_PIN_DT, INPUT_PULLUP);
  pinMode(ENC_PIN_SW, INPUT_PULLUP);

  LCD_SchreibeTextInZeileXvon8(8,"Setup complete");
  LCD_ResetAlle8Zeilen();
  LCD_UpdateHeaderUhrzeit();
  LCD_SchreibeHeadline ("");

  LCD.drawXBitmap(102, 50 + Dampf_height, tasse, Tasse_width, Tasse_height, ST77XX_WHITE);

} // Ende Setup
// ################################################################################

// LOOP 
// ################################################################################
// ################################################################################
// ################################################################################
void loop(void) {
  if(time + 5000 <= millis()) { //5 sekunden update time 
    // Bei jeder Loop die aktuelle Zeit holen -> eventuell seltener, um Laufzeit zu sparen?
    LCD_UpdateHeaderUhrzeit();
    if (dampfswitch){
      LCD.drawXBitmap(102, 50, dampf_2, Dampf_width, Dampf_height, ST77XX_BLACK);
      LCD.drawXBitmap(102, 50, dampf_1, Dampf_width, Dampf_height, ST77XX_WHITE);
      dampfswitch = false;
    } else {
      LCD.drawXBitmap(102, 50, dampf_1, Dampf_width, Dampf_height, ST77XX_BLACK);
      LCD.drawXBitmap(102, 50, dampf_2, Dampf_width, Dampf_height, ST77XX_WHITE);
      dampfswitch = true;
    }
    time = millis();
  } 

  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  if (success) {
    String UidString ="0x";
    for (uint8_t i=0; i < uidLength; i++) 
    {
      UidString += "0123456789ABCDEF"[uid[i] / 16];
      UidString += "0123456789ABCDEF"[uid[i] % 16];
    }
    if(GetMappingIndexOfCurrentUID(UidString)){
      if (dampfswitch){
        LCD.drawXBitmap(102, 50, dampf_2, Dampf_width, Dampf_height, ST77XX_BLACK);
      } else {
        LCD.drawXBitmap(102, 50, dampf_1, Dampf_width, Dampf_height, ST77XX_BLACK);
      }
      LCD.drawXBitmap(102, 50 + Dampf_height, tasse, Tasse_width, Tasse_height, ST77XX_BLACK); 
      SD_BuchungInFileSpeichern(LCD_Buchungsscreen());
      LCD_BuildUpHomeScreen();
      LCD.drawXBitmap(102, 50 + Dampf_height, tasse, Tasse_width, Tasse_height, ST77XX_WHITE);
      if (dampfswitch){
        LCD.drawXBitmap(102, 50, dampf_2, Dampf_width, Dampf_height, ST77XX_WHITE);
      } else {
        LCD.drawXBitmap(102, 50, dampf_1, Dampf_width, Dampf_height, ST77XX_WHITE);
      }
    }
  }
  // Ende von Karte erfolgreich gelesen  
} // Ende Loop

// ################################################################################
// SUBFUNKTIONS
// ################################################################################
// ################################################################################
// LCD_____________________________________________________________________________
// LCD.setTextSize(1,x); 6 Pixel breit/ Zeichen
// LCD.setTextSize(2,x); 12 Pixel breit / Zeichen ?
// LCD.setTextSize(3,x);
// LCD.setTextSize(x,1); 
// LCD.setTextSize(x,2); 15 Pixel hoch
// LCD.setTextSize(x,3); 20 Pixel hoch


void LCD_DrawGitternetz(bool state) {
  // #########################################
  // true = Gitternetz zeichnen
  // false = Gitternetz löschen
  // #########################################
  if (state == true) {
      LCD.drawLine(0, 40, 320, 40, ST77XX_WHITE);
      LCD.drawLine(0, 65, 320, 65, ST77XX_WHITE);
      LCD.drawLine(0, 90, 320, 90, ST77XX_WHITE);
      LCD.drawLine(0, 115, 320, 115, ST77XX_WHITE);
      LCD.drawLine(0, 140, 320, 140, ST77XX_WHITE);
      LCD.drawLine(0, 165, 320, 165, ST77XX_WHITE);
      LCD.drawLine(0, 190, 320, 190, ST77XX_WHITE);
      LCD.drawLine(0, 215, 320, 215, ST77XX_WHITE);
  }
    if (state == false) {
      LCD.drawLine(0, 40, 320, 40, ST77XX_BLACK);
      LCD.drawLine(0, 65, 320, 65, ST77XX_BLACK);
      LCD.drawLine(0, 90, 320, 90, ST77XX_BLACK);
      LCD.drawLine(0, 115, 320, 115, ST77XX_BLACK);
      LCD.drawLine(0, 140, 320, 140, ST77XX_BLACK);
      LCD.drawLine(0, 165, 320, 165, ST77XX_BLACK);
      LCD.drawLine(0, 190, 320, 190, ST77XX_BLACK);
      LCD.drawLine(0, 215, 320, 215, ST77XX_BLACK);
  }
}
void LCD_ResetAlle8Zeilen() {
  for (int i=1;i<=8;i++){
    LCD_SchreibeTextInZeileXvon8(i," ");
  }
}

void LCD_SchreibeTextInZeileXvon8 (int Zeilennummer, String Textneu) { 
  // #########################################
  // Zeilennummer: Integer Werte von 1 bis 8 
  // Text: Beliebiger Text als String
  // #########################################
  LCD.setTextSize(1,2);
  int yPosition[] = {45,70,95,120,145,170,195,220};
  Zeilennummer--;
  LCD.setCursor(10, yPosition[Zeilennummer]);
  LCD.setTextColor(ST77XX_BLACK);
  LCD.print(LCD_TextInZeileXvon8[Zeilennummer]);
  LCD.setCursor(10, yPosition[Zeilennummer]);
  LCD.setTextColor(ST77XX_WHITE);
  Textneu.toCharArray(LCD_TextInZeileXvon8[Zeilennummer], LCD_AnzahlBuchstabenXvon8);
  LCD.print(LCD_TextInZeileXvon8[Zeilennummer]);
}

void LCD_ResetAlle5Zeilen() {
  for (int i=1;i<=5;i++){
    LCD_SchreibeTextInZeileXvon5(i," ");
  }
}
void LCD_SchreibeTextInZeileXvon5 (int Zeilennummer, String Textneu) { 
  // #########################################
  // Zeilennummer: Integer Werte von 1 bis 5 
  // Text: Beliebiger Text als String
  // #########################################
  LCD.setTextSize(2,3);
  int yPosition[] = {50,90,130,170,210};
  Zeilennummer--;
  LCD.setCursor(10, yPosition[Zeilennummer]);
  LCD.setTextColor(ST77XX_BLACK);
  LCD.print(LCD_TextInZeileXvon5[Zeilennummer]);
  LCD.setCursor(10, yPosition[Zeilennummer]);
  LCD.setTextColor(ST77XX_WHITE);
  Textneu.toCharArray(LCD_TextInZeileXvon5[Zeilennummer], LCD_AnzahlBuchstabenXvon5);
  LCD.print(LCD_TextInZeileXvon5[Zeilennummer]);
}


void LCD_SchreibeHeadline (String Textneu) { 
  // #########################################
  // Zeilennummer: Integer Werte von 1 bis 5 
  // Text: Beliebiger Text als String
  // #########################################
  LCD.setTextSize(2,3);
  
  LCD.setCursor(0, 10);
  LCD.setTextColor(ST77XX_BLACK);
  LCD.print(LCD_Headline);
  LCD.setCursor(0, 10);
  LCD.setTextColor(ST77XX_WHITE);
  Textneu.toCharArray(LCD_Headline, LCD_AnzahlBuchstabenHeadline);
  LCD.print(LCD_Headline);
}

void LCD_UpdateHeaderUhrzeit(){ 
  DateTime now = rtc.now();
  
  uint8_t u8Minute = now.minute();
  uint8_t u8Hour = now.hour();
  uint8_t u8Day = now.day();
  uint8_t u8Month = now.month();
  uint16_t u16Year = now.year();

  if (u8Minute != u8MinuteOld){
    u8MinuteOld = LCD_UpdateMinute(u8Minute, u8MinuteOld);
  }
  if (u8Hour != u8HourOld){
    u8HourOld = LCD_UpdateHour(u8Hour, u8HourOld);
  }
  if (u8Day != u8DayOld){
    u8DayOld = LCD_UpdateDay(u8Day, u8DayOld);
  }
  if (u8Month != u8MonthOld){
    u8MonthOld = LCD_UpdateMonth(u8Month, u8MonthOld);
  }
  if (u16Year != u16YearOld){
    u16YearOld = LCD_UpdateYear(u16Year, u16YearOld);
  }
}

uint8_t LCD_UpdateMinute(uint8_t u8Min, uint8_t u8MinOld){
  uint16_t u16Offset = 296;
  LCD_UpdateTime(u8Min, u8MinOld, u16Offset);
  return u8Min;
}

uint8_t LCD_UpdateHour(uint8_t u8Hou, uint8_t u8HouOld){
  uint16_t u16Offset = 260;
  LCD_UpdateTime(u8Hou, u8HouOld, u16Offset);
  return u8Hou;
}

void LCD_UpdateTime(uint8_t u8NewTimeValue, uint8_t u8OldTimeValue, uint16_t u16Offset){
  LCD.setTextSize(2,2);
  LCD.setCursor(u16Offset,7);
  LCD.setTextColor(ST77XX_BLACK);
  if (u8OldTimeValue < 10) {
     LCD.print('0');
     LCD.setCursor(u16Offset + 12, 7);
  }
  LCD.print(u8OldTimeValue);
  LCD.setCursor(u16Offset,7);
  LCD.setTextColor(ST77XX_WHITE);
  if (u8NewTimeValue < 10) {
     LCD.print("0");
     LCD.setCursor(u16Offset + 12, 7);
  }
  LCD.print(u8NewTimeValue);
  //Dots between hour and minute
  char sDouDot[] = ":";
  LCD.setCursor(284, 7);
  LCD.print(sDouDot);
}

uint8_t LCD_UpdateDay(uint8_t u8D, uint8_t u8DOld){
  uint16_t u16Offset = 260;
  LCD_UpdateDate(u8D, u8DOld, u16Offset);
  return u8D;
}

uint8_t LCD_UpdateMonth(uint8_t u8M, uint8_t u8MOld){
  uint16_t u16Offset = 278;
  LCD_UpdateDate(u8M, u8MOld, u16Offset);
  return u8M;
}

uint16_t LCD_UpdateYear(uint16_t u16Y, uint16_t u16YOld){
  uint16_t u16Offset = 296;
  LCD_UpdateDate(u16Y, u16YOld, u16Offset);
  return u16Y;
}

void LCD_UpdateDate(uint16_t u16NewDateValue, uint16_t u16OldDateValue, uint16_t u16Offset){
  LCD.setTextSize(1,1);
  LCD.setCursor(u16Offset,30);
  LCD.setTextColor(ST77XX_BLACK);
  if (u16OldDateValue < 10) {
     LCD.print("0");
     LCD.setCursor(u16Offset + 6, 30);
  }
  LCD.print(u16OldDateValue);
  LCD.setCursor(u16Offset,30);
  LCD.setTextColor(ST77XX_WHITE);
  if (u16NewDateValue < 10) {
     LCD.print('0');
     LCD.setCursor(u16Offset + 6, 30);
  }
  LCD.print(u16NewDateValue);

  //Dots between day month and year
  char sDot[] = ".";
  LCD.setCursor(272, 30);
  LCD.print(sDot);
  LCD.setCursor(290, 30);
  LCD.print(sDot);

}

int LCD_Buchungsscreen(){
  // #########################################
  // Argument:
  //
  // #########################################
  // #########################################
  // Return Values:
  // 0: Zurück ohne Buchung
  // 1: Halbe Tasse mit Milch
  // 2: Ganze Tasse mit Milch
  // 3: Halbe Tasse schwarz
  // 4: Ganze Tasse schwarz
  // #########################################
  LCD_ClearCurrentHomeScreen();
  LCD_SchreibeHeadline("");
  int AktionDrehknopf = 0;
  int AktuelleMarkierung = 4;
  uint8_t maxOptions;
  if (NameMapping[ZeileDerAktuellenKarte].LieblingsKaffee[0] == 'L'){
    AktuelleMarkierung = 2;
  }
  // Titelzeile aufbauen mit Namen
  LCD.setTextColor(ST77XX_WHITE);
  LCD.setTextSize(2,3);
  LCD.setCursor(0,10);
  LCD.print(String(NameMapping[ZeileDerAktuellenKarte].Vorname) + " " + String(NameMapping[ZeileDerAktuellenKarte].Nachname));

  // Buchungsscreen aufbauen
  if (String(NameMapping[ZeileDerAktuellenKarte].Vorname) == "Michael" && String(NameMapping[ZeileDerAktuellenKarte].Nachname) == "Streit"){
    LCD_SchreibeTextInZeileXvon8(1, "1/2 Tasse + Milch     "+String(PreisHalbeTasseLatte));
    LCD_SchreibeTextInZeileXvon8(2, "1   Tasse + Milch     "+String(PreisGanzeTasseLatte));
    LCD_SchreibeTextInZeileXvon8(3, "1/2 Tasse schwarz     "+String(PreisHalbeTasseSchwarz));
    LCD_SchreibeTextInZeileXvon8(4, "1   Tasse schwarz     "+String(PreisGanzeTasseSchwarz));
    LCD_SchreibeTextInZeileXvon8(5, "Abbrechen");
    LCD_SchreibeTextInZeileXvon8(6, "Adminmenue");
    maxOptions = 6;
  } else {
    LCD_SchreibeTextInZeileXvon5(1, "1/2 Tasse + Milch     "+String(PreisHalbeTasseLatte));
    LCD_SchreibeTextInZeileXvon5(2, "1   Tasse + Milch     "+String(PreisGanzeTasseLatte));
    LCD_SchreibeTextInZeileXvon5(3, "1/2 Tasse schwarz     "+String(PreisHalbeTasseSchwarz));
    LCD_SchreibeTextInZeileXvon5(4, "1   Tasse schwarz     "+String(PreisGanzeTasseSchwarz));
    LCD_SchreibeTextInZeileXvon5(5, "Abbrechen");
    maxOptions = 5;
  }
  
  LCD_BuchungsscreenMarkierungsLinie(AktuelleMarkierung, maxOptions);
  // Markierungslinie + durchnavigieren 
  while(AktionDrehknopf != 2){ // Bleibe solange in der Schleife, bis ein Knopfdruck kommt
    AktionDrehknopf = getEncoderDirection();
    AktuelleMarkierung = updateValueRotEncoder(AktionDrehknopf, 1, maxOptions, AktuelleMarkierung);
    if (AktionDrehknopf == 1 or AktionDrehknopf == -1) {
      LCD_BuchungsscreenMarkierungsLinie(AktuelleMarkierung, maxOptions);
    }
  }
  // Markierungslinien Clear
  LCD_BuchungsscreenMarkierungsLinieClearAll(maxOptions);
  
  // Test Clear
  LCD_ResetAlle5Zeilen();
  LCD_ResetAlle8Zeilen();
  if (AktuelleMarkierung == 6){
    //Adminmenü
    return -1;
  }
  if (AktuelleMarkierung == 5){
    return 0; // Abrechen
  }
  else{
    return AktuelleMarkierung;
  }
}

void LCD_BuchungsscreenMarkierungsLinie(int iRow, int iLines){

  LCD_BuchungsscreenMarkierungsLinieClearAll(iLines); // Erst alle löschen
  // Gewünschte Zeile Markierungslinie zeichnen
  // Senkrecht
  int iLineDistance = 0;
  int iLineOffset = 0;
  if (iLines <= 5){
    iLineDistance = 40;
    iLineOffset = 0;
  } else {
    iLineDistance = 25;
    iLineOffset = 15;
  }
  Serial.print("LineDistance: "+ iLineDistance);
  LCD.drawLine(0, (iRow+1)*iLineDistance + iLineOffset, 0, ((iRow+1)*iLineDistance)-20 + iLineOffset, ST77XX_WHITE);
  LCD.drawLine(1, (iRow+1)*iLineDistance + iLineOffset, 1, ((iRow+1)*iLineDistance)-20 + iLineOffset, ST77XX_WHITE);
  LCD.drawLine(2, (iRow+1)*iLineDistance + iLineOffset, 2, ((iRow+1)*iLineDistance)-20 + iLineOffset, ST77XX_WHITE);
  // Waagrecht
  LCD.drawLine(0, (iRow+1)*iLineDistance + iLineOffset, 200, (iRow+1)*iLineDistance + iLineOffset, ST77XX_WHITE);
  LCD.drawLine(0, ((iRow+1)*iLineDistance)-1 + iLineOffset, 200, ((iRow+1)*iLineDistance)-1 + iLineOffset, ST77XX_WHITE);
  LCD.drawLine(0, ((iRow+1)*iLineDistance)-2 + iLineOffset, 200, ((iRow+1)*iLineDistance)-2 + iLineOffset, ST77XX_WHITE);

}

void LCD_BuchungsscreenMarkierungsLinieClearAll(int iLines){
  // #########################################
  // Argument: 
  //    actlines = actual number of lines
  // #########################################
  // Function: 
  //    Clears all the lines
  // #########################################
  int iLineDistance = 0;
  int iLineOffset = 0;
  if (iLines <= 5){
    iLineDistance = 40;
    iLineOffset = 0;
  } else {
    iLineDistance = 25;
    iLineOffset = 15;
  }
  
  for (int counter=1;counter<=iLines;counter++){
    LCD.drawLine(0, (counter+1)*iLineDistance + iLineOffset, 0, ((counter+1)*iLineDistance)-20 + iLineOffset, ST77XX_BLACK);
    LCD.drawLine(1, (counter+1)*iLineDistance + iLineOffset, 1, ((counter+1)*iLineDistance)-20 + iLineOffset, ST77XX_BLACK);
    LCD.drawLine(2, (counter+1)*iLineDistance + iLineOffset, 2, ((counter+1)*iLineDistance)-20 + iLineOffset, ST77XX_BLACK);
    // Waagrecht
    LCD.drawLine(0, (counter+1)*iLineDistance + iLineOffset, 200, (counter+1)*iLineDistance + iLineOffset, ST77XX_BLACK);
    LCD.drawLine(0, ((counter+1)*iLineDistance)-1 + iLineOffset, 200, ((counter+1)*iLineDistance)-1 + iLineOffset, ST77XX_BLACK);
    LCD.drawLine(0, ((counter+1)*iLineDistance)-2 + iLineOffset, 200, ((counter+1)*iLineDistance)-2 + iLineOffset, ST77XX_BLACK);
  }
}

void SD_BuchungInFileSpeichern(int nummer){ 
  // #########################################
  // Argument:
  // nummer: 
  //   -1: Adminmenü
  //    0: Zurück ohne Buchung
  //    1: Halbe Tasse mit Milch
  //    2: Ganze Tasse mit Milch
  //    3: Halbe Tasse schwarz
  //    4: Ganze Tasse schwarz
  // #########################################
  // #########################################
  // Return Values:
  // #########################################

  int iActMenuPos = 0;
  int iActRotaryPos = 0;
  int iLines;
  if (nummer == -1){
    LCD_ResetAlle8Zeilen();
    iLines = 2;
    LCD_SchreibeTextInZeileXvon5(1, "Uhrzeit stellen");
    LCD_SchreibeTextInZeileXvon5(2, "Zurueck");
    while(iActRotaryPos != 2){ // Bleibe solange in der Schleife, bis ein Knopfdruck kommt
      iActRotaryPos = getEncoderDirection();
      iActMenuPos = updateValueRotEncoder(iActRotaryPos, 1, iLines, iActMenuPos);
      if (iActRotaryPos == 1 or iActRotaryPos == -1) {
        LCD_BuchungsscreenMarkierungsLinie(iActMenuPos, iLines);
      }
    }
    delay(300);
    if (iActMenuPos = 1){
      updateTimeAndDate();
    }
    if (iActMenuPos = 2){
      nummer = 0;
    }
    LCD_BuchungsscreenMarkierungsLinieClearAll(iLines);
    LCD_ResetAlle5Zeilen();

  }

  if (nummer == 0){ // Falls Abbrechen gedrückt wurde mach nichts
    
  }
  if (nummer > 0){
    // Zeitstempel zusammenbasteln
    DateTime now = rtc.now();
    String Zeit = now.timestamp('TIMESTAMP_DATE');
    String Zeitstempel = Zeit.substring(0,4) + ";" + Zeit.substring(5,7) + ";" + Zeit.substring(8,10) + ";" + Zeit.substring(11,13) + ";" + Zeit.substring(14,16) + ";"+ Zeit.substring(17,19) + ";";  

    String Buchung = "";
    if (nummer == 1){ //Halbe Tasse Latte
      Buchung = String(NameMapping[ZeileDerAktuellenKarte].UID) +";"+NameMapping[ZeileDerAktuellenKarte].Vorname + ";" + NameMapping[ZeileDerAktuellenKarte].Nachname + ";L;H;" + PreisHalbeTasseLatte +";";
    }
    if (nummer == 2){ //Ganze Tasse Latte
      Buchung = String(NameMapping[ZeileDerAktuellenKarte].UID)+";"+NameMapping[ZeileDerAktuellenKarte].Vorname + ";" + NameMapping[ZeileDerAktuellenKarte].Nachname + ";L;G;" + PreisGanzeTasseLatte +";";
    }
    if (nummer == 3){ //Halbe Tasse Schwarz
      Buchung = String(NameMapping[ZeileDerAktuellenKarte].UID)+";"+NameMapping[ZeileDerAktuellenKarte].Vorname + ";" + NameMapping[ZeileDerAktuellenKarte].Nachname + ";S;H;" + PreisHalbeTasseSchwarz +";";
    }
    if (nummer == 4){ //Ganze Tasse Schwarz
      Buchung = String(NameMapping[ZeileDerAktuellenKarte].UID)+";"+NameMapping[ZeileDerAktuellenKarte].Vorname + ";" + NameMapping[ZeileDerAktuellenKarte].Nachname + ";S;G;" + PreisGanzeTasseSchwarz +";";
    }
    Serial.println(Zeitstempel + Buchung);

    
    LCD_SchreibeTextInZeileXvon5(1,"Bitte warten ...");   
    if(nummer == 2 or nummer == 4){
      LCD_SchreibeTextInZeileXvon5(4,"   GANZE Tasse");
      TagesZaehlerKaffee = TagesZaehlerKaffee +1;
    }else{
      LCD_SchreibeTextInZeileXvon5(4,"   HALBE Tasse");
      TagesZaehlerKaffee = TagesZaehlerKaffee +0,5;
    }
    if(nummer == 1 or nummer == 2){
      LCD_SchreibeTextInZeileXvon5(5,"   MIT Milch");
    }else{
      LCD_SchreibeTextInZeileXvon5(5,"   OHNE Milch");
    } 
    

    for (int counter = 1;counter <=3;counter++){ 
      myFile = SD.open(Zeit.substring(0,4) +"_"+ Zeit.substring(5,7)+".txt", FILE_WRITE); // Filename = "Jahr_Monat.txt"
      if (!myFile){
        LCD_SchreibeTextInZeileXvon5(2,"Fehlversuch "+String(counter));
        SD.begin(SD_CS_PIN);
        delay(2000);
        if  (counter == 3) {
        while (1);
        }
      }
      else{
        counter = 4; // for schleife verlassen
      }
    }

    myFile.println(Zeitstempel+Buchung);
    myFile.close();
    LCD_SchreibeTextInZeileXvon5(1, "        VIELEN DANK"); 
    delay(1000); // zum lesen eventuell wieder löschen
    LCD_ResetAlle5Zeilen(); 
  }

  // Titelzeile löschen
  LCD.setTextColor(ST77XX_BLACK);
  LCD.setTextSize(2,3);
  LCD.setCursor(0,10);
  LCD.print(String(NameMapping[ZeileDerAktuellenKarte].Vorname) + " " + String(NameMapping[ZeileDerAktuellenKarte].Nachname));
  ZeileDerAktuellenKarte = 0;
}

int getEncoderDirection() {
  // #########################################
  // Return Values:
  // -1: Gegen den Uhrzeigersinn
  // 0: Keine Aktion
  // 1: Drehung im Uhrzeigersinn
  // 2: Button wurde gedrückt
  // #########################################

  int dir = 0;
  bool encButton = digitalRead(ENC_PIN_SW);
  
  encoderPos = encoder.read();

  if (encoderPos < -3){
    Serial.println("Links");
    encoder.write(0);
    dir = -1;  // gegen den Uhrzeigersinn
  }
  if (encoderPos > 3){
    Serial.println("Rechts");
    encoder.write(0);
    dir = 1;  // Uhrzeigersinn
  }

  if (encButton == 0){
    dir = 2;
  }

  return dir;
}

bool GetMappingIndexOfCurrentUID(String uidgesucht){
  // #########################################
  // Sucht aus dem NamesMapping den Index der aktuellen Karte raus
  // und schreibt den Wert auf die globale Variable
  // #########################################
  for (int i=0; i <= AnzahlMappingEintraege; i++) 
    {
      if(uidgesucht.compareTo(NameMapping[i].UID) == 0){
        Serial.print("Gefunden in der Zeile: ");
        Serial.println(i);
        ZeileDerAktuellenKarte = i;
        return true;
        i = AnzahlMappingEintraege; // Verlasse die for Schleife frühzeitig, wenn UID gefunden wurde
      }      
    }
    if(ZeileDerAktuellenKarte == 0){
      Serial.print("Zu der aufgelegten Karte existiert kein Mapping!!!");
      return false;
    }

}

String GetNameOfUID(String uidgesucht){
  // #########################################
  // Argumente:
  // String uid: Die UID, die in der Names.TXT gesucht werden soll
  // #########################################
  // #########################################
  // Return Values:
  // String "Vorname Nachname"
  // #########################################
  String name = "kein Mapping!!!";
  for (int i=0; i <= AnzahlMappingEintraege; i++) 
  {
    if(uidgesucht.compareTo(NameMapping[i].UID) == 0){
      name = String(NameMapping[i].Vorname) + " " + String(NameMapping[i].Nachname);
      i = AnzahlMappingEintraege; // Verlasse die for Schleife frühzeitig, wenn UID gefunden wurde
    }      
  }
  return name;
}

void LCD_ClearCurrentHomeScreen(){
  // #########################################
  // Argumente: -
  // #########################################
  // #########################################
  // Return Values:-
  // #########################################
  // AktuellerHomeScreen:
  // 0 = Aktueller Monat
  // 1 = Vorheriger Monat 
  // 2 = Tages Statistik
  // 3 = Letzen Buchungen (geheimScreen)

  LCD_ResetAlle8Zeilen();
  LCD_SchreibeHeadline ("");
  
}

void LCD_BuildUpHomeScreen(){
  // #########################################
  // Argumente: -
  // #########################################
  // #########################################
  // Return Values:-
  // #########################################
  // AktuellerHomeScreen:
  // 0 = Aktueller Monat
  // 1 = Vorheriger Monat 
  // 2 = Tages Statistik
  // 3 = Letzen Buchungen (geheimScreen)

}

int updateValueRotEncoder(int direction, uint16_t minPos, uint16_t maxPos, uint16_t initPos){
  uint16_t actPos = initPos;
  if(direction == 1){ // Rechtsdrehung erkannt;
    if(actPos < maxPos){ // Falls noch nicht am Ende angekommen
      actPos = actPos + 1;
      }
      else{
        // mach nichts wenn linie schon ganz unten
        delay(300); // Entprellung
      }
    }
  if(direction == -1){ // Linksdrehung erkannt;
    if(actPos > minPos){ // Falls noch nicht am Anfang angekommen
      actPos = actPos - 1;
    }
    else{
      // mach nichts wenn linie schon ganz oben
      delay(300);
    }
  }
  return actPos;
}

void updateTimeAndDate(){
  DateTime actualDateTime = rtc.now();
  uint8_t u8CounterTimeAndDate = 0;
  uint32_t u32TimeAndDate = millis(); 
  bool bDisplayLine = true;
  uint16_t u16XPosition;
  uint8_t u8YPosition;
  uint8_t u8Offset;
  uint8_t u8Minute = actualDateTime.minute();
  uint8_t u8Hour = actualDateTime.hour();
  uint8_t u8Day = actualDateTime.day();
  uint8_t u8Month = actualDateTime.month();
  uint16_t u16Year = actualDateTime.year();
  uint8_t u8Months31 [7] = {1,3,5,7,8,10,12};
  uint8_t u8Months30 [4] = {4,6,9,11};
  uint8_t u8Months28 [1] = {2};
  int iActEncoder = 0;
  while (u8CounterTimeAndDate < 5){
    //Minutes
    if (u8CounterTimeAndDate == 0){
      u16XPosition = 296;
      u8YPosition = 22;
      u8Offset = 22;
      iActEncoder = getEncoderDirection();
      u8Minute = updateValueRotEncoder(iActEncoder, 0, 59, u8Minute);
      if (u8Minute != u8MinuteOld){
        u8MinuteOld = LCD_UpdateMinute(u8Minute,u8MinuteOld);
      }
    }
    //Hour
    if (u8CounterTimeAndDate == 1){
      u16XPosition = 260;
      iActEncoder = getEncoderDirection();
      u8Hour = updateValueRotEncoder(iActEncoder, 0, 23, u8Hour);
      if (u8Hour != u8HourOld){
        u8HourOld = LCD_UpdateHour(u8Hour,u8HourOld);
      }
    }
    //Month before as we need to know how much days it have
    if (u8CounterTimeAndDate == 2){
      u16XPosition = 278;
      u8YPosition = 38;
      u8Offset = 10;
      iActEncoder = getEncoderDirection();
      u8Month = updateValueRotEncoder(iActEncoder, 1, 12, u8Month);
      if (u8Month != u8MonthOld){
        u8MonthOld = LCD_UpdateMonth(u8Month,u8MonthOld);
      }
    }
    //Day
    if (u8CounterTimeAndDate == 3){
      u16XPosition = 260;
      iActEncoder = getEncoderDirection();
      u8Day = updateValueRotEncoder(iActEncoder, 1, 31, u8Day);
      if (u8Day != u8DayOld){
        u8DayOld = LCD_UpdateDay(u8Day,u8DayOld);
      }
      
    }
    //Year
    if (u8CounterTimeAndDate == 4){
      u16XPosition = 295;
      u8Offset = 25;
      iActEncoder = getEncoderDirection();
      u16Year = updateValueRotEncoder(iActEncoder, 2000, 2100, u16Year);
      if (u16Year != u16YearOld){
        Serial.println(u16YearOld);
        Serial.println(u16Year);
        u16YearOld = LCD_UpdateYear(u16Year,u16YearOld);
      }
    }

    //Lineblink
    if ((u32TimeAndDate <= millis() - 1000)){
      if (bDisplayLine){
        LCD.drawLine(u16XPosition, u8YPosition, u16XPosition + u8Offset, u8YPosition, ST77XX_WHITE);
        bDisplayLine = false;
      } else {
        LCD.drawLine(u16XPosition, u8YPosition, u16XPosition + u8Offset, u8YPosition, ST77XX_BLACK);
        bDisplayLine = true;
      }
      u32TimeAndDate = millis();
    }
    
    if (getEncoderDirection() == 2){
      u8CounterTimeAndDate++;
      delay(300);
      Serial.print(u8CounterTimeAndDate);
      LCD.drawLine(u16XPosition, u8YPosition, u16XPosition + u8Offset, u8YPosition, ST77XX_BLACK);
    } 
    if (u8CounterTimeAndDate == 5) {
      rtc.adjust(DateTime(u16Year, u8Month, u8Day, u8Hour, u8Minute, 0));
    }
  }
}    

void SetupLCD(){
  Serial.print("Init LCD");
  LCD.init(240, 320);
  LCD.fillScreen(ST77XX_BLACK);
  LCD.setTextWrap(false);
  LCD.setRotation(3);
}

void SetupRTC(){
  // RTC DS3231____________________________________________________________/
  LCD_SchreibeTextInZeileXvon8(1,"Initialisation of RTC started....");
  Serial.print("Init RTC");
  if (! rtc.begin()) {
    Serial.print("Init RTC failed");
    while (1);
  }
  LCD_SchreibeTextInZeileXvon8(1,"Initialisation of RTC complete");
  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
}

void SetupPN532(){
  // PN532_______________________________________________________________/
  LCD_SchreibeTextInZeileXvon8(2,"Initialisation of PN532 started....");
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    // Serial.print("Didn't find PN53x board");
    LCD_SchreibeTextInZeileXvon8(2,"Didn't find PN53x board");
    while (1); // halt
  }
   
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  nfc.setPassiveActivationRetries(0x0);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
  LCD_SchreibeTextInZeileXvon8(2,"Initialisation of PN532 complete");
  Serial.println("Initialisation of PN532 complete");
}

void SetupSDCard(){
  // SD Card Reader__________________________________________________________/
  Serial.print("Initializing SD card...");
  LCD_SchreibeTextInZeileXvon8(3,"Initialisation of SD Card Reader started....");
  
  
  if (!SD.begin(SD_CS_PIN)){
    LCD_SchreibeTextInZeileXvon8(3,"Initialisation of SD Card Reader failed...");
    Serial.print("Failed");
    while (1);
  }
  else{
    LCD_SchreibeTextInZeileXvon8(3,"Initialisation Success");
    Serial.print("Success");
  }
}

void ReadNamesTXT(){
  // Names.txt einlesen _____________________________________________________/
  myFile = SD.open("Names.txt");
  if (myFile) { 
    unsigned int Spaltenzaehler = 0;
    unsigned int Zeilenzaehler = 0;
    char Buchstabensammler = "";
    String Wortsammlung = "";
    while (myFile.available()) { // read from the file until there's nothing else in it:
      //Serial.write(myFile.read());
      Buchstabensammler = myFile.read();
      if (Buchstabensammler == ';'){ // Bei jedem ; wird wegsortiert
        if (Spaltenzaehler == 0){ // Spalte Vorname
          Wortsammlung.remove(0, 2); // \n muss entfernt werden, ansonsten hat man einen Zeilenumbruch mit drin
          Wortsammlung.toCharArray(NameMapping[Zeilenzaehler].Vorname, LaengeVorname);
        }
        if (Spaltenzaehler == 1){ // Spalte Nachname
          Wortsammlung.toCharArray(NameMapping[Zeilenzaehler].Nachname, LaengeNachname);
        }
        if (Spaltenzaehler == 2){ // Spalte UID
          Wortsammlung.toCharArray(NameMapping[Zeilenzaehler].UID, LaengeUID);
        }
        if (Spaltenzaehler == 3){ // Spalte Lieblingskaffee
          // NameMapping[Zeilenzaehler].LieblingsKaffee = Wortsammlung[0];
          Wortsammlung.toCharArray(NameMapping[Zeilenzaehler].LieblingsKaffee, 2);
        }
        if (Spaltenzaehler == SpaltenNames - 1){ // Spalte Lieblingskaffee
          Spaltenzaehler = 0; // wenn man bei der letzten Spalte angekommen ist, muss der Zaehler zurückgesetzt werden
          Zeilenzaehler += 1; // und die Zeile hochgezählt werden
        } else {
          Spaltenzaehler += 1;
        }
        Wortsammlung = "";
      } else {
        Wortsammlung += Buchstabensammler;
      }     
    }
    myFile.close(); // close the file
    AnzahlMappingEintraege = Zeilenzaehler - 1;
    LCD_SchreibeTextInZeileXvon8(4,"File Names.txt complete: Mappings: " + String(AnzahlMappingEintraege));
    Serial.print("File Names.txt complete: Mappings: " + String(AnzahlMappingEintraege));     
  } else {
    // if the file didn't open, print an error:
    LCD_SchreibeTextInZeileXvon8(4,"File Names.txt nicht erfolgreich eingelesen!! ");
    Serial.print("File Names.txt nicht erfolgreich eingelesen!! ");
    while (1);
  }
  // Falls MappingListe zu lang halte an und weise darauf hin
  if (AnzahlMappingEintraege >= ZeilenNames-3){
    LCD_SchreibeTextInZeileXvon8(4,"Zu viele Mapping Eintraege: " + String(AnzahlMappingEintraege)); 
    while (1);
  }
}

void ReadConfigTXT(){
  // Config.txt einlesen _____________________________________________________/
  myFile = SD.open("Config.txt");
  if (myFile) {
    unsigned int Spaltenzaehler = 0;
    unsigned int Zeilenzaehler = 0;
    char Buchstabensammler = "";
    String Wortsammlung = "";
    while (myFile.available()) { // read from the file until there's nothing else in it:
      //Serial.write(myFile.read());
      Buchstabensammler = myFile.read();
      if (Buchstabensammler == ';'){ // Bei jedem ; wird wegsortiert
        if (Spaltenzaehler == 0){ // 
          // Hier steht immer nur die Bezeichnung
        }
        if (Spaltenzaehler == 1){ // Hier steht das relevante
          if (Zeilenzaehler == 0){ 
            Wortsammlung.toCharArray(PreisHalbeTasseLatte, 5);
          }
          if (Zeilenzaehler == 1){
            Wortsammlung.toCharArray(PreisGanzeTasseLatte, 5);
          }
          if (Zeilenzaehler == 2){
            Wortsammlung.toCharArray(PreisHalbeTasseSchwarz, 5);
          }
          if (Zeilenzaehler == 3){
            Wortsammlung.toCharArray(PreisGanzeTasseSchwarz, 5);
          }
        }
        if (Spaltenzaehler == 1){ // Spalte Lieblingskaffee
          Spaltenzaehler = 0; // wenn man bei der letzten Spalte angekommen ist, muss der Zaehler zurückgesetzt werden
          Zeilenzaehler += 1; // und die Zeile hochgezählt werden
        } else {
          Spaltenzaehler += 1;
        }
        Wortsammlung = "";
      } else {
        Wortsammlung += Buchstabensammler;
      }     
    }
    myFile.close();     // close the file
    LCD_SchreibeTextInZeileXvon8(5,"File Config.txt complete");     
  } else {
    // if the file didn't open, print an error:
    LCD_SchreibeTextInZeileXvon8(5,"File Config.txt nicht erfolgreich eingelesen!! ");
    while (1);
  }
}


boolean arrayIncludeElement(uint8_t array[], uint8_t element) {
  for (int i = 0; i < (sizeof(array)/4); i++) {
    if (array[i] == element) {
      return true;
    }
  }
  return false;
}

// ################################################################################
// SAMMLUNG 
void SammlungFunctions(){ // Wird nie aufgerufen!

  // Sonstige Funktionen
  SD_BuchungInFileSpeichern(1);
  getEncoderDirection();
  GetNameOfUID("UID");
  GetMappingIndexOfCurrentUID("uidgesucht");

  // LCD________________________
  LCD_DrawGitternetz(1);
    // Zeilen___________________
    LCD_ResetAlle8Zeilen();
    LCD_ResetAlle5Zeilen();
    LCD_SchreibeTextInZeileXvon8(1,"");
    LCD_SchreibeTextInZeileXvon5 (1,"");
    // Healine________________
    LCD_SchreibeHeadline ("");
    LCD_UpdateHeaderUhrzeit();
    // Ganze Screens__________
    LCD_Buchungsscreen();
    LCD_ClearCurrentHomeScreen();
    LCD_BuildUpHomeScreen();
    // Auswahllinie_____________
    LCD_BuchungsscreenMarkierungsLinie(1,1);
    LCD_BuchungsscreenMarkierungsLinieClearAll(1);


  // SETUP Funktions_____
  SetupLCD();
  SetupRTC();
  SetupPN532();
  SetupSDCard();
  ReadNamesTXT();
  ReadConfigTXT();


}
