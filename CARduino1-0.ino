/***********************************************
 * CARduinoV1-0.ino
 * Basic version of the City Race Game, which
 * operates a motor to drive through a city.
 * As long as the light is green, the motor can
 * be run for points, and higher speeds give
 * more points, but running through a red light
 * loses a life, and after losing the last life,
 * the player gets GAME OVER.
 * Authors: Matt Hawkins and James Libbey
 * Course: EE 10115
 * Last updated: 4/22/2018
 **********************************************/

/***********************************************
 * Set up EEPROM and read hiscore
 **********************************************/

#include <avr/eeprom.h>
#include <LiquidCrystal.h> //include LCD library
LiquidCrystal lcd(11,12,4,5,6,7); //Initialize LCD display
#include <Servo.h>
byte addr=0; // address to store hi score
unsigned int hiscore;

/***********************************************
 * Set pins for sensors and actuators
 **********************************************/

const int greenLED=8;
const int yellowLED=9;
const int redLED=13;
const int blueLED = 2; 
const int startButton = 3;

/***********************************************
 * Set global constants
 **********************************************/
// lowest input signal to count as "moving"
const int driveThreshold=323;

// milliseconds between score counts
const byte scoreInterval=250;

/***********************************************
 * Set global variables
 **********************************************/

unsigned int score=0;
byte difficulty=1;
byte lives=3;

// initialize light timing parameters
unsigned int lightVar[2],lightMin[2];

/***********************************************
 * Core Engine
 **********************************************/

void setup() {
  // initialize pins
  pinMode(greenLED,OUTPUT);
  pinMode(yellowLED,OUTPUT);
  pinMode(redLED,OUTPUT);
  pinMode(blueLED, OUTPUT);
  pinMode(startButton, INPUT);
  lcd.begin(16,2);
  
  randomSeed(analogRead(A1));

  eeprom_read_block(&hiscore,addr,4); // prepare hiscore

  setTimes();

  // show start screen
  lcd.setCursor(0,1);
  lcd.print("PRESS STRT BTN! ");
  lcd.setCursor(0,0);
  lcd.print("CARDUINO 1.0:  ");

  while (!digitalRead(startButton)); // wait for start button

  lcd.clear();
  lcd.print("Welcome!        ");
  delay(2000);
  lcd.setCursor(0,0);
  lcd.print("HISCORE:     ");
  lcd.setCursor(0,1);
  lcd.print(hiscore);
  delay(2000);

  //Countdown before game starts
  digitalWrite(redLED, HIGH);
  lcd.setCursor(0,0);
  lcd.print("Get ready!      ");
  delay(2000);
  digitalWrite(redLED, LOW);
  lcd.setCursor(0,0);
  lcd.print("Set!            ");
  digitalWrite(yellowLED, HIGH);
  delay(2000);
  digitalWrite(yellowLED, LOW); 
  lcd.setCursor(0,0);
  lcd.print("Go!!!!!         ");
  digitalWrite(greenLED, HIGH); 
  delay(1000);
}


void loop() {

  // go through 10 light cycles before increasing difficulty
  for (byte i=0; i<10; i++) {
     gyLight(0);
     gyLight(1);
     redLight();
  }
  
  difficulty++;
  setTimes();
}


/***********************************************
 * Light Functions ("Phases")
 **********************************************/

void gyLight(byte lightMode) { // input which light to use
  if (lightMode==0) {
    digitalWrite(greenLED,HIGH);
    digitalWrite(redLED,LOW);
    lcd.setCursor(0,0);
    lcd.print("Go!!!!!         ");
  }
  else {
    digitalWrite(greenLED,LOW);
    digitalWrite(yellowLED,HIGH);
  }

  unsigned long sysTime=millis();
  
  // set the time that the next phase is entered
  unsigned long nextLight=lightMin[lightMode]+random(lightVar[lightMode])+sysTime;
  
  // set the next time that score is checked
  unsigned long nextScore=scoreInterval+sysTime;

  // repeatedly run drive code until the time exceeds
  // that for the next phase
  while (sysTime<nextLight) {
    driveCheck();
    sysTime=millis();

    // at scoring interval, interrupt drive code with
    // tracking the score and setting the next scoring
    // interval
    if (sysTime>=nextScore) {
      addScore();
      nextScore=scoreInterval+sysTime;
    }
  }
}

void redLight() {
  digitalWrite(yellowLED,LOW);
  digitalWrite(redLED,HIGH);
  lcd.setCursor(0,0);
  lcd.print("STOP!!!         ");

  // subtract a life if still driving
  if (analogRead(A0)>driveThreshold)
    subtractLife();

  delay(2500);
}

/***********************************************
 * Operational Functions
 **********************************************/

// use the pot reading to send a pwm signal that
// adjusts motor speed whenever called
void driveCheck() {
  int potReading=analogRead(A0);

  if (potReading <= driveThreshold) {
    lcd.setCursor(0,0);
    lcd.print("Go! Go! Go!     ");
  }
  else if (potReading < 673) {
    lcd.setCursor(0,0);
    lcd.print("You're cruising!");
  }
  else if (potReading < 1023) {
    lcd.setCursor(0,0);
    lcd.print("Woah! So fast!  ");
  }
  else if (potReading == 1023) {
    lcd.setCursor(0,0);
    lcd.print("MAXIMUM SPEED!");
  }
}


// Scoring formula:
//    2*(% of max speed) (truncated) + 1
// In effect: up to 3 points based on how high pot is
void addScore() {
  // get value of pot reading relative to the threshold
  int potVal=analogRead(A0)-driveThreshold;

  // add to score only if car is going
  if (potVal>=0)
    score+=2*potVal/(1023-driveThreshold)+1;

  // update score display
  lcd.setCursor(0,1);
  lcd.print("Score:          ");
  lcd.setCursor(8,1);
  lcd.print(score);
}

void subtractLife() {
  //deduct a life
  lives--;
  lcd.setCursor(0,0);
  lcd.print("Ran the light!");
  digitalWrite(blueLED, HIGH);
  delay(500);
  digitalWrite(blueLED,LOW);

  // if out of lives, game over
  if (lives==0) {
    lcd.clear();
    lcd.print("GAME OVER!");
    lcd.setCursor(0,1);
    lcd.print("DRIVE BETTER!");

    unsigned long nextDisp=1000+millis();
    // flash lights
    while (millis()<nextDisp) {
      digitalWrite(redLED,HIGH);
      digitalWrite(blueLED,LOW);
      delay(250);
      digitalWrite(redLED,LOW);
      digitalWrite(blueLED,HIGH);
      delay(250);
    }

    // communicate hiscore
    if (score>hiscore) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("NEW HI SCORE!");
      lcd.setCursor(0,1);
      lcd.print(score);
      eeprom_write_block(&score,addr,4);
    }
    else {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("NEXT TIME BEAT");
      lcd.setCursor(0,1);
      lcd.print(hiscore);
    }

    // flash lights forever
    while (1==1) {
      digitalWrite(redLED,HIGH);
      digitalWrite(blueLED,LOW);
      delay(250);
      digitalWrite(redLED,LOW);
      digitalWrite(blueLED,HIGH);
      delay(250);
    }
  }
}

void setTimes() {

  if (difficulty<=5) {
  // set variation in green light length, which is
  // inversely proportional to 6-difficulty, so each
  // step of difficulty gets more variation (for
  // example, 1 to 2 takes variance from 800 ms to
  // 1000 ms but 4 to 5 takes it from 2 seconds to 4)
  lightVar[0]=3000/(6-difficulty);
  // set minimum value such that range has maximum
  // of 5 seconds
  lightMin[0]=4000-lightVar[0];
  }
  else {
    // set green to default settings at values above
    // 5 to prevent glitches with the (6-difficulty)
    lightVar[0]=4000;
    lightMin[0]=0;
  }

  // set minimum value of yellow lights to be inversely
  // proportional to difficulty, so the first two steps
  // in difficulty have the largest decreases in yellow
  // light timing (for example, 1 to 2 takes yellow
  // intervals from 2 seconds to 1, but 4 to 5 takes
  // it from only 500 ms to 400 ms)
  lightMin[1]=1500/(difficulty);

  // set variation in yellow light length to be
  // proportional to yellowMin. This reduces
  // variance with difficulty, but that reduces
  // the chance of randomly getting an
  // exceptionally long yellow light
  lightVar[1]=lightMin[1]/2;
  
}
