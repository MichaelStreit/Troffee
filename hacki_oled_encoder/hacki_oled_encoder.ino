//Include library
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include<Bounce2.h>
#include "NewEncoder.h"
#define OLED_RESET 4
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(OLED_RESET);

NewEncoder encoder(2, 3, 1, 3, 1, FULL_PULSE);
int16_t prevEncoderValue;

const int ENC_SW = 10;
Bounce sw_enc_push = Bounce();

int displaytime = 0;
int buttontime = 0;
int runtime;
boolean update_display = false;

static String menu_card_detected_admin[]= {"Buchung", "Abruf", "Einstellungen"}; //actual menu = 1
static String menu_card_detected_noadmin[]= {"Buchung", "Abruf", "     "};       //actual menu = 2
static String menu_buchung[]= {"Ganzer", "Halber", "Zurueck"};                   //actual menu = 3
static String menu_einstellungen[]= {"Uhr stellen", "Userliste", "Zurueck"};     //actual menu = 4
int actualMenu = 1;

void setup()
{
    NewEncoder::EncoderState state;
    
    Serial.begin (9600);
    delay(2000);
    Serial.println("Starting");
    
    //Encoder init
    if (!encoder.begin()) {
      Serial.println("Encoder Failed to Start. Check pin assignments and available interrupts. Aborting.");
      while (1) {
      yield();
      }
    } else {
    encoder.getState(state);
    Serial.print("Encoder Successfully Started at value = ");
    prevEncoderValue = state.currentValue;
    Serial.println(prevEncoderValue);
    }

    pinMode(ENC_SW, INPUT_PULLUP);
    sw_enc_push.attach(ENC_SW);
    sw_enc_push.interval(5);
       
    //Initialize display by providing the display type and its I2C address.
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    //Set the text size and color.
    display.setTextSize(1);
    display.setTextColor(WHITE);
}
void loop()
{
  runtime = millis();
  switchState ();

  int16_t currentValue;
  NewEncoder::EncoderState currentEncoderState;

  if (encoder.getState(currentEncoderState)) {
    Serial.print("Encoder: ");
    currentValue = currentEncoderState.currentValue;
    if (currentValue != prevEncoderValue) {
      Serial.print(currentValue);
      Serial.print(" Menu: ");
      Serial.print(actualMenu);
      
      prevEncoderValue = currentValue;
    } else
      switch (currentEncoderState.currentClick) {
        case NewEncoder::UpClick:
          Serial.println("at upper limit.");
          break;

        case NewEncoder::DownClick:
          Serial.println("at lower limit.");
          break;

        default:
          break;
    }
  }
    
  updateDisplay ();
  if ((runtime - displaytime) > 200){
    display.display();
    displaytime = runtime;
  }
}

void switchState ()
{
  sw_enc_push.update();
  if (sw_enc_push.read() == LOW)
  {
    if (actualMenu == 1 && prevEncoderValue == 1) //menu_card_detected_admin -> menu_buchung
    {
      actualMenu = 3;
    }
    else if(actualMenu == 1 && prevEncoderValue == 3) //menu_card_detected_admin -> menu_einstellungen
    {
      actualMenu = 4;
    }
    else if(actualMenu == 3 && prevEncoderValue == 3) //menu_buchung -> menu_card_detected_admin
    {
      actualMenu = 1;
    }
    else if(actualMenu == 4 && prevEncoderValue == 3) //menu_einstellungen -> menu_card_detected_admin
    {
      actualMenu = 1;
    }
  }
}


void updateDisplay ()
{
  
  display.clearDisplay();

  if (actualMenu == 1)
  {
    printMenu(menu_card_detected_admin);
  }
  if (actualMenu == 2)
  {
    printMenu(menu_card_detected_noadmin);
  }
  if (actualMenu == 3)
  {
    printMenu(menu_buchung);
  }
  if (actualMenu == 4)
  {
    printMenu(menu_einstellungen);
  }
    
  display.setCursor(0, prevEncoderValue*8);
  display.print(">");
}

//int readEncoder ()
//{
//  int value = digitalRead(CLK_ENC);
//  if (value != rotEnc)
//  { // we use the DT pin to find out which way we turning.
//    if (digitalRead(DT_ENC) != value && value == 1) {  // Clockwise
//      rotEnc = value;
//      counter ++;
//      Serial.print("Actual counter: ");
//      Serial.println(counter);
//      return 1;    
//    } 
//    else 
//    { //Counterclockwise
//      rotEnc = value;
//      counter --;
//      Serial.print("Actual counter: ");
//      Serial.println(counter);
//      return -1;
//    }
//  }
//  rotEnc = value;
//  return 0;
//}

void printMenu(String menu[3])
{
  display.setCursor(10, 8);
  display.print(menu[0]);    
  display.setCursor(10, 16);
  display.print(menu[1]);
  display.setCursor(10, 24);
  display.print(menu[2]);
}
