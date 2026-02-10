#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int knockPin = 2;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- Knock detection ---
int baseline;
int knockCount = 0;
unsigned long firstKnockTime = 0;
unsigned long lastTriggerTime = 0;

const unsigned long doubleKnockWindow = 400; // ms between knocks
const unsigned long cooldownMs = 600;        // ignore noise after trigger
const unsigned long debounceMs = 150;        // prevent one knock counting twice

// --- Paging (vertical-style update) ---
const unsigned long pageIntervalMs = 1200;   // time each "page" stays
unsigned long lastPageTime = 0;
int pageIndex = 0;

bool hasMessage = false;

// Long messages allowed
const char* messages[] = {
  "Knock, knock. \nWho's there? \nLeaf.\nLeaf who?\nLeaf me alone,\nI'm telling \na joke!",
  "Knock Knock. \nWho is there? \nKanga. \nKanga who? \nActually, it's \nKangaroo!",
  "Knock Knock. \nWho is there? \nBoo. \nBoo who? \nDon't cry, it's \njust a joke.",
  "Knock Knock. \nWho is there? \nLettuce. \nLettuce who? \nLettuce in, it's \ncold out here!",
  "Knock, knock. \nWho is there? \nWire. \nWire who? \nWire you asking \nso many \nquestions? "
};
const int messageCount = sizeof(messages) / sizeof(messages[0]);

String currentText = "";  // we store the chosen message here (easier to paginate)

void showDefault() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   Knock  for   ");
  lcd.setCursor(0, 1);
  lcd.print("    a  laugh     "); // clear second line
  hasMessage = false;
  currentText = "";
  pageIndex = 0;
}

String padTo16(String s) {
  if (s.length() > 16) return s.substring(0, 16);
  while (s.length() < 16) s += " ";
  return s;
}

void showPage() {
  if (!hasMessage) return;

  // Split message into lines
  String text = currentText;
  int totalLines = 0;
  String lines[10]; // up to 10 lines max (safe for your jokes)

  int start = 0;
  while (true) {
    int idx = text.indexOf('\n', start);
    if (idx == -1) {
      lines[totalLines++] = text.substring(start);
      break;
    } else {
      lines[totalLines++] = text.substring(start, idx);
      start = idx + 1;
    }
  }

  // Loop pages
  if (pageIndex >= totalLines) pageIndex = 0;

  String line1 = lines[pageIndex];
  String line2 = (pageIndex + 1 < totalLines) ? lines[pageIndex + 1] : "";

  lcd.setCursor(0, 0);
  lcd.print(padTo16(line1));

  lcd.setCursor(0, 1);
  lcd.print(padTo16(line2));

  pageIndex += 2; // move to next "page"
}

void setup() {
  pinMode(knockPin, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  delay(300);
  baseline = digitalRead(knockPin);

  randomSeed(analogRead(A0));

  showDefault();
}

void loop() {
  unsigned long now = millis();
  int state = digitalRead(knockPin);

  // ---- Double knock detection (based on deviation from baseline) ----
  if (state != baseline && (now - lastTriggerTime) > cooldownMs) {

    if (knockCount == 0) {
      knockCount = 1;
      firstKnockTime = now;
    } 
    else if (knockCount == 1 && (now - firstKnockTime) <= doubleKnockWindow) {
      // Valid double knock -> choose a new message and start paging
      int idx = random(messageCount);
      currentText = String(messages[idx]);

      hasMessage = true;
      pageIndex = 0;
      lastPageTime = 0;  // force immediate page display

      knockCount = 0;
      lastTriggerTime = now;
    }

    delay(debounceMs);
  }

  // If second knock doesn't arrive in time, reset
  if (knockCount == 1 && (now - firstKnockTime) > doubleKnockWindow) {
    knockCount = 0;
  }

  // ---- Page-by-page updates (vertical-style) ----
  if (hasMessage && (now - lastPageTime) >= pageIntervalMs) {
    lastPageTime = now;
    showPage();
  }
}