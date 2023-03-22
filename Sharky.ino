// include libraries:
#include <LiquidCrystal.h>
#include <Keypad.h>

# define MODE1 1  // 1 FIRE event starts timer, 1 FIRE event ends timer
# define MODE2 2  // 1 FIRE event starts timer, every FIRE event after calculates time from start
# define MODE3 3  // 1 FIRE event starts timer, every FIRE event after calculates lap time from last FIRE event
// *** FIRE event is signal received from an unknown source that indicates Sharky to calculate the time based on the current mode

# define STATE_MODE_SELECTION 0   // select mode
# define STATE_INITIAL 1          // timer not yet started
# define STATE_WORKING 2          // timer calculating based on mode
// *** STATE is used to determine what action is Sharky performing at the moment

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


// initialize the keypad object
const byte KBD_ROWS = 1, KBD_COLS = 4;

byte kbdRowPins[KBD_ROWS] = { 6 };
byte kbdColPins[KBD_COLS] = { 9, 10, 7, 8};
char kbdKeys[KBD_ROWS][KBD_COLS] = {
  {'1', '2', '3', '4'}
};

const int TONE_ERROR = 220;
const int TONE_KEY = 880;
const int TONE_FIRE = 1047;

Keypad keypad = Keypad(makeKeymap(kbdKeys), kbdRowPins, kbdColPins, KBD_ROWS, KBD_COLS);

// dimensions of the display
const int COLS = 20;
const int ROWS = 4;

// input & output pins
const int INPUT_PIN = 15;
const int OUTPUT_PIN = 13;
const int buzzerPin = 19;

// variables that contain info about CURRENT STATE & MODE
int CURR_MODE = -1;
int CURR_STATE = STATE_MODE_SELECTION;

// variables used for input
bool isTriggered = false;  // active on contact
bool isReleased = false;   // activates only once on release
bool isFired = false;      // activates only once on press
bool canBeFired = true;    // activates only after Release

// variables used for time measuring
unsigned long programStartTime = 0;
unsigned long modeStartTime = 0;
unsigned long modeCurrentTime = 0;
unsigned long modeLastTime = 0;

const int Q_SIZE = 4;
unsigned long buffTimesQ[Q_SIZE] = {0, 0, 0, 0};
int buffNumbersQ[Q_SIZE] = {0, 0, 0, 0};
int p = -1;

bool mode1IsFinished = 0;
int entryCount = 0;


const String TITLE = "Sharky";   
// *** Hello, I am Sharky - a primitive time measuring device!
// *** I am happy to help you!

// *** Sharky v1.0 released 22.03.2023
// *** by Drago Ivanov

// main setup function
// initialising input, output & display
//
void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(COLS, ROWS);

  // initiate input & output 
  pinMode(INPUT_PIN, INPUT_PULLUP);
  pinMode(OUTPUT_PIN, OUTPUT);
  
  programStartTime = millis();
  loadingScreen();
  lcd.clear();
  loadState(STATE_MODE_SELECTION);
}

// main loop function
// responsible for running the current STATE of Sharky
//
void loop() {

  checkForFIREevent();
  checkForKeypadEvent();

  if(CURR_STATE == STATE_WORKING)
  {
    switch(CURR_MODE){
      case MODE1:
      mode1();
      break;
      case MODE2:
      mode2();
      break;
      case MODE3:
      mode3();
      break;
    }
  }
  else if(CURR_STATE == STATE_INITIAL)
  {
    if(isFired) loadState(STATE_WORKING);
  }

}

// function responsible for state transition
// and initialisation
//
void loadState(int state)
{
  switch(state)
  {
    case STATE_MODE_SELECTION:
      CURR_STATE = STATE_MODE_SELECTION;
      CURR_MODE = -1;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("    Select Mode!    ");
      lcd.setCursor(0, 1);
      lcd.print("1: start/finish");
      lcd.setCursor(0, 2);
      lcd.print("2: multiple finish");
      lcd.setCursor(0, 3);
      lcd.print("3: track laps");
      break;
    case STATE_INITIAL:
      CURR_STATE = STATE_INITIAL;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("    Mode ");
      lcd.print(CURR_MODE);
      lcd.print(" Ready!");
      lcd.setCursor(0, 1);
      lcd.print("(1): clear");
      lcd.setCursor(0, 2);
      lcd.print("(4): back to select");
      break;
    case STATE_WORKING:
      CURR_STATE = STATE_WORKING;
      lcd.clear();
      modeStartTime = millis();       // the moment the actual measuring starts
      modeCurrentTime = modeStartTime;
      modeLastTime = modeStartTime;
      mode1IsFinished = 0;
      entryCount = 0;
      // TimesQ
      clear();

      break;
    default:
      beep(200, TONE_ERROR);
  }
}

// function responsible for the loading screen
//
void loadingScreen()
{
  lcd.setCursor(0, 0);
  lcd.print("Sharky Time Tracker");
  lcd.setCursor(0, 1);
  lcd.print("(c)2023 version 1.0");
  lcd.setCursor(0, 2);
  lcd.print("Drago Ivanov");
  lcd.setCursor(0, 3);
  lcd.print("SMG, 11th grade");
  delay(8000);
}

// Sharky Mode 1:
// calculates time between 2 FIRE events -> start and end
//
void mode1()
{
  modeCurrentTime = millis();

  if(mode1IsFinished)
  {
    lcd.setCursor(0, 3);
    lcd.print("Awaiting command!");
    return;
  }

  lcd.setCursor(0, 1);
  lcd.print("TIME: ");
  printTime(modeCurrentTime - modeStartTime, 1);

  if(isFired)  // signal to end the counting
  {
    mode1IsFinished = true;
  }
}

// Sharky Mode 2:
// calculates overrall time of each FIRE event
//
void mode2()
{
  modeCurrentTime = millis();

  if(isFired)
  {
    //timesQ
    addNewTimeEntry(modeCurrentTime - modeStartTime);
    entryCount++;
  }

  for(int itr = 0; itr < (ROWS > Q_SIZE ? Q_SIZE : ROWS); itr++)
  {
    if(buffTimesQ[itr] == 0) return;
    lcd.setCursor(0, itr);
    lcd.print(buffNumbersQ[itr]);
    lcd.print(":    ");
    printTime(buffTimesQ[itr], itr);
  }

}

// Sharky Mode 3:
// calculates time laps between each FIRE event
//
void mode3()
{
  modeCurrentTime = millis();
  
  if(isFired)
  {
    //timesQ
    addNewTimeEntry(modeCurrentTime - modeLastTime);
    modeLastTime = modeCurrentTime;
    entryCount++;
  }

    for(int itr = 0; itr < (ROWS > Q_SIZE ? Q_SIZE : ROWS); itr++)
  {
    if(buffTimesQ[itr] == 0) return;
    lcd.setCursor(0, itr);
    lcd.print(buffNumbersQ[itr]);
    lcd.print(":    ");
    printTime(buffTimesQ[itr], itr);
  }
}

// function responsible for FIRE events: 
// isTrigger, isReleased, isFired
//
void checkForFIREevent()
{
  int read = digitalRead(INPUT_PIN);
  if(read == 0) read = true;   // if triggered make it 1
  else read = false;

  if(isTriggered == 1 && read == 0)
  {
    isReleased = true;
    canBeFired = true;
  }
  else isReleased = false;

  if(canBeFired && read)
  {
    isFired = true;
    canBeFired = false;
    beep(50, TONE_FIRE);
  }
  else isFired = false;

  isTriggered = read;

  digitalWrite(OUTPUT_PIN, isFired);
}

// function responsible for keypad events:
// mode selection, restart, clear
//
void checkForKeypadEvent()
{
  char key = keypad.getKey();
  if (key == NO_KEY) return;

  if(key == '4')
  {
    loadState(STATE_MODE_SELECTION);
    beep(50, TONE_KEY);
    return;    
  }

  if(CURR_STATE == STATE_MODE_SELECTION)
  {
    CURR_MODE = key - '0';
    beep(50, TONE_KEY);
    loadState(STATE_INITIAL);
    return;
  }
  else if(CURR_STATE == STATE_WORKING)
  {
    if(key == '1')
    {
      loadState(STATE_INITIAL);
      beep(50, TONE_KEY);
      return;
    }
  }

  beep(50, TONE_ERROR);
}

// function responsible for time formating:
// seconds, right allign
//
void printTime(unsigned long a, int line)
{
  String time = String(a);
  if(time.length() <= 3) time = "0." + time;
  else time = time.substring(0, time.length() - 3) + "." + time.substring(time.length() - 3);

  lcd.setCursor(20 - time.length(), line);
  lcd.print(time);
}

// function used to produce a buzz sound effect
//
void beep(unsigned char delayms, int frequency){
  tone(buzzerPin,frequency);
  delay(delayms);
  noTone(buzzerPin);
}  

// function that clears a given line from the display
//
void deleteLCDline(int line)
{
  lcd.setCursor(0, line);
  for(int i = 0; i < COLS; i++)
  {
    lcd.print(" ");
  }
}

// QUEUE function:
// returns true if Times Queue is empty
//
bool is_empty()
{
  return p == -1;
}

// QUEUE function:
// inserts an element at the end of the queue
//
void push(unsigned long a){
  if(p >= Q_SIZE-1)
  {
    return;    
  }
  p++;
  buffTimesQ[p] = a;
  if(p == 0) buffNumbersQ[p] = 1;
  else buffNumbersQ[p] = buffNumbersQ[p-1] + 1;
}

// QUEUE function:
// pops the first element of the queue
//
unsigned long pop()
{
  if(p == -1) return 0;
  
  unsigned long res =  buffTimesQ[0];
  p--;
  for(int i = 1; i < Q_SIZE; i++)
  {
    buffTimesQ[i-1] = buffTimesQ[i];
    buffNumbersQ[i-1] = buffNumbersQ[i];    
  }

  return res;
}

// QUEUE function:
// returns the last element of the queue
//
unsigned long back()
{
  return p != -1 ? buffTimesQ[0] : 0;
}

// QUEUE function:
// returns the first element of the queue
//
unsigned long peek()
{
  return p != -1 ?  buffTimesQ[p] : 0;
}

// QUEUE function:
// deletes all elements of the queue
//
void clear()
{
  p = -1;

  for(int i = 0; i < Q_SIZE; i++)
  {
    buffTimesQ[i] = 0;
    buffNumbersQ[i] = 0;
  }
}

// special QUEUE function:
// if queue is full -> pop();
// then push(entry);
//
void addNewTimeEntry(unsigned long a)
{
  if(p >= Q_SIZE-1)
  {
    pop();
  }

  push(a);
}
