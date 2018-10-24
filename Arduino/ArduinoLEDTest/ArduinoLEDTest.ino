#define arr_len( x )  ( sizeof( x ) / sizeof( *x ) )

unsigned int  ledPins[]       = { 3,     5,     6,     9   };
char          ledLabels[]     = { 'G',   'B',   'R',   'W' };
byte          ledPowerValue[] = { 255,   255,   0,     0   };

void setup() {
  // Initialize the LED pins
  for (unsigned int i = 0; i < arr_len(ledPins); ++i)
  {
    pinMode(ledPins[i], OUTPUT);
  }
}

void loop() {
  for (unsigned int i = 0; i < arr_len(ledPins); ++i)
  {
    analogWrite(ledPins[i], ledPowerValue[i]);
  }
}

int findPinByLabel(char label) {
  for (unsigned int i = 0; i < arr_len(ledPins); ++i) {
    if (ledLabels[i] == label) {
      return ledPins[i];
    }
  }
}
