#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define PIN_BUTTON   2
#define PIN_AUTOPLAY 1

#define HUMAN_RUN1 1  //human
#define HUMAN_RUN2 2
#define HUMAN_JUMP1 3        
#define HUMAN_JUMP2 4 

#define OBSTACLE   5 //heart
#define OBSTACLE_RIGHT 6 
#define OBSTACLE_LEFT 7

/* POSITONS:
1,2: lower row
3,4: jumping process
5,6, 7, 8: jump upper row
9, 10: landing
11, 12: upper row */

LiquidCrystal_I2C lcd(0x27, 16, 2);
static char upperRow[17];  //width (16) + 1 for null terminator
static char lowerRow[17];
static bool buttonPushed = false;

static unsigned int speed = 500;
static unsigned long lastSpeedIncreaseTime = 0;
static const unsigned int speedIncreaseInterval = 3000;

void initializeGraphics() {
  static byte graphics[] = {
    //Human left leg
    0b01110,
    0b01010,
    0b01110,
    0b00100,
    0b11111,
    0b00100,
    0b01000,
    0b01000,

    //Human right leg
    0b01110,
    0b01010,
    0b01110,
    0b00100,
    0b11111,
    0b00100,
    0b00010,
    0b00010,

    //Human left leg
    0b01110,
    0b01010,
    0b01110,
    0b00100,
    0b11111,
    0b00100,
    0b01000,
    0b01000,

    //Human right leg
    0b01110,
    0b01010,
    0b01110,
    0b00100,
    0b11111,
    0b00100,
    0b00010,
    0b00010,

    //Heart
    0b00000,
    0b01010,
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00000,
    0b00000,

    //Heart right
    0b00000,
    0b01010,
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00000,
    0b00000,

    //Heart left
    0b00000,
    0b01010,
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00000,
    0b00000,
  };
  for (int i = 0; i < 7; i++) {
    lcd.createChar(i + 1, &graphics[i * 8]);
  }
  for (int i = 0; i < 16; ++i) { //width
    upperRow[i] = ' ';
    lowerRow[i] = ' ';
  }
}

// Move the screen (obstacles) to the left:
void moveObstacles(char*   world, byte newWorld) {
  for (int i = 0; i < 16; ++i) {
    char current = world[i];
    char next = (i == 16 - 1) ? newWorld :   world[i + 1];
    switch (current) {
      case ' ':
         world[i] = (next == OBSTACLE) ? OBSTACLE_RIGHT   : ' '; 
         // 1)current element is solid and has terrain to the right.
        break;
      case OBSTACLE:
         world[i] = (next == ' ') ? OBSTACLE_LEFT   : OBSTACLE;
         //1)if next is empty, then current element (obstacle) has obstacle to the left
        break;
      case OBSTACLE_RIGHT:
         world[i] = OBSTACLE;
        break;
      case OBSTACLE_LEFT:
         world[i] = ' ';
        break;
    }
  }
}

void updateSpeed() {
  if(millis() - lastSpeedIncreaseTime > 2000) {
    lastSpeedIncreaseTime = millis();
    if(speed > 30) {
      speed -= 20;
    }
    }
  }

bool  drawHuman(byte position, char* upperRow, char* lowerRow, unsigned int score)   {
  bool collide = false;
  byte upper, lower;
  char upperSave = upperRow[1];
  char lowerSave = lowerRow[1];
   switch (position) {
    case 0:
      upper = lower = ' ';
       break;
    case 1:
      upper = ' ';
      lower = HUMAN_RUN1; //human runs on the ground (left leg)
      break;
    case 2:
      upper = ' ';
      lower = HUMAN_RUN2; //human runs on the ground (right leg)
      break;
     case 3: //jump on upper row
    case 10:  //landing
      upper =   ' ';
      lower = HUMAN_JUMP1; //jumps to the lower row
      break;
    case   4: //half-way up
    case 9:  //half-way down
      upper = '.';
       lower = HUMAN_JUMP2;
      break;
    case 5:
     case 6:
    case 7:
    case 8:
       upper = HUMAN_JUMP1;
      lower = ' ';
      break;
     case 11:
      upper = HUMAN_RUN1;  //human runs on upper row (left leg)
      lower   = ' ';
      break;
    case 12:
       upper = HUMAN_RUN2;  //right leg
      lower = ' ';
      break;
   }
  if (upper != ' ') {
    upperRow[1] = upper; //displaying char in upper row
     collide = (upperSave == ' ') ? false : true; //previous char in the upper row
  }
  if   (lower != ' ') {
    lowerRow[1] = lower; //displaying char in lower row
    collide   |= (lowerSave == ' ') ? false : true;
  }

  byte digits   = (score > 9999) ? 5 : (score > 999) ? 4 : (score > 99) ? 3 : (score > 9) ? 2 :   1;

  // Drawing the full screen
  upperRow[16] = '\\0';
  lowerRow[16] = '\\0';
  char temp = upperRow[16 - digits];
  upperRow[16 - digits]   = '\\0';
  lcd.setCursor(0, 0);
  lcd.print(upperRow);
  upperRow[16  - digits] = temp;
  lcd.setCursor(0, 1);
  lcd.print(lowerRow);

  lcd.setCursor(16 - digits, 0);
  lcd.print(score);

  upperRow[1]   = upperSave;
  lowerRow[1] = lowerSave;
  return   collide;
}


void buttonPush()   {
  buttonPushed = true;
}

void setup() {
  Serial.begin(4000);
  pinMode(PIN_BUTTON,   INPUT);
  digitalWrite(PIN_BUTTON, HIGH);
  pinMode(PIN_AUTOPLAY, OUTPUT);
  digitalWrite(PIN_AUTOPLAY, HIGH);

  // // Digital pin 2 maps to interrupt   0
  attachInterrupt(0, buttonPush, FALLING); //0 pin button

  initializeGraphics();

  lcd.init();
  lcd.backlight();
} 

void loop() {
  static byte HumanPos   = 1;
  static byte newWorldType = 0; //EMPTY
   static byte newWorldDuration = 1;
  static bool playing = false;
  static   bool blink = false;
  static unsigned int distance = 0;

  if (!playing)   {
    drawHuman((blink) ? 0 : HumanPos, upperRow, lowerRow,   distance >> 3);
    if (blink) {
      lcd.setCursor(0, 0); //top left corner of lcd
      lcd.print("Are you ready?");
      lcd.setCursor(0, 1);
      lcd.print("Press Start!");
    }

    delay(100);
    blink = !blink;
    if (buttonPushed)   {
      initializeGraphics();
      HumanPos = 1;
      playing = true;
      buttonPushed = false;
      distance = 0;
      speed = 250;  // Reset speed when starting the game
      lastSpeedIncreaseTime = millis();  // Reset the speed timer
    }
  }else{
      updateSpeed();

  // Shift to the left
  moveObstacles(lowerRow,   newWorldType == 1 ? OBSTACLE : ' ');
  moveObstacles(upperRow, newWorldType == 2 ? OBSTACLE   : ' ');

  // Make new screen to enter on the right
   if (--newWorldDuration == 0) {
    if (newWorldType == 0) {
       newWorldType = (random(3) == 0) ? 2 : 1;
       newWorldDuration = 10 + random(6);
    } else {
      newWorldType   = 0;
      newWorldDuration = 10 + random(6);
    }
  }

   if (buttonPushed) {
    if (HumanPos <= 2) HumanPos = 3;
     buttonPushed = false;
  }

  if (drawHuman(HumanPos, upperRow, lowerRow,   distance >> 3)) {
    playing = false; // The Human collided with something
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Game Over!");
      delay(2000);  // Show game over screen for 2 seconds
      playing = false;
      blink = false;
  } else {
    if (HumanPos   == 2 || HumanPos == 10) {
      HumanPos   = 1;
    } else if ((HumanPos >= 5 &&   HumanPos <= 7) && lowerRow[1] != ' ')   {
      HumanPos = 11;
    } else if (HumanPos >= 11   && lowerRow[1] == ' ') {
      HumanPos   = 7;
    } else if (HumanPos == 12) {
       HumanPos = 11;
    } else {
      ++HumanPos;
     }
    ++distance;
  }
    delay(speed); // Use the speed variable to control game refresh rate
  }
}