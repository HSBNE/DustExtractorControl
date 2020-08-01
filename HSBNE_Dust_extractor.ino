const int binsensor = 2;     // the pin of the bin level sensor
const int startbutton =  3;      // the the pin of the start button
const int stopbutton =  4;      // the pin of the stop button
const int extract =  8;      // the pin of the extractor contactor
const int clean =  13;      // the pin of the cleaning contactor
const int red =  10;      // the pin of the bin full indicator
const int yellow = 11;      // the pin of the delay cleaning indicator
const int green = 12;      // the pin of the running indicator

int extractrun = 0; // status of start button, latches until extractor starts
int extractstop = 0; // status of stop button, latches until extractor stops
int binstate = 0; // status of bin - full or empty
int timer = 0;

// Beau's totally newb code 24102019
// note the extract and clean outputs are inverted due to relay board configuration
// 
// 01082020 - bugfix - max run timer was looking for EXTRACTRUN HIGH rather than EXTRACTRUN LOW and causing unresponsiveness issues after an hour

void setup() {
  pinMode(binsensor, INPUT);
  pinMode(startbutton, INPUT);
  pinMode(stopbutton, INPUT);
  pinMode(extract, OUTPUT);
  pinMode(clean, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(yellow, OUTPUT);
  pinMode(green, OUTPUT);
  digitalWrite(clean, HIGH);
  digitalWrite(extract, HIGH);
}

void loop() {
  if (extractstop == LOW){
      extractstop = digitalRead(stopbutton);
  }
  extractrun = digitalRead(startbutton);
  binstate = digitalRead(binsensor);
  digitalWrite (red, binstate);
  delay(100);
  timer++ ;

 // stop extractor after half an hour or reset timer
  if (timer > 18000) {       //600*minutes
    if (extractrun == HIGH) {
      extractstop = HIGH;
    }
    else {
      timer = 0;
    }
  }

 // main extraction loop, check for button presses and start extractor or exit if full
 
  if ((extractrun == HIGH) && (extractstop == LOW) && (digitalRead(clean) == HIGH)) { //start code on start button press
    if ((binstate == HIGH) && (extract == LOW)) { //stop code if bin full
       extractrun = LOW;
    }
    if ((extractrun == HIGH) && (binstate == LOW)) {
      digitalWrite (green, HIGH);
      digitalWrite (extract, LOW);
    }
  }

 // power off extraction loop and clean
    
  if ((digitalRead(extract) == LOW) && (extractstop == HIGH) && (timer < 650) && (timer > 1)){ //minimum run timer of 1 minute - turns on delay-cleaning indicator if in delay on shutdown
    digitalWrite (yellow, HIGH);
  }
  
  if ((digitalRead(extract) == LOW) && (extractstop == HIGH) && (timer > 650)) { //stop blower and advance code if stop button pressed after 60 second cooldown
      digitalWrite (extract, HIGH);
      digitalWrite (green, LOW);
      digitalWrite (yellow, HIGH);


      delay(10000); //10 second delay for extractor to spin down
      timer = 0; //clear timer for use in cleaning timer
      digitalWrite (clean, LOW);
      extractrun = LOW;
  }

  if ((digitalRead(clean) == LOW) && (timer > 100)) { // lets filter clean for 10 seconds
    digitalWrite (clean, HIGH);
    extractstop = LOW;
    timer = 0; //clear timer
    delay(10000); //10 second delay for cleaning motor to stop and dust to settle
    digitalWrite (yellow, LOW);
  }
}
