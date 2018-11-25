#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <x10rf.h>

#define senderPower 0
#define switchUp 3
#define switchDown 4
#define statusLED 1
#define tx 2
#define reps 2

volatile uint8_t portbhistory = 0xFF;
volatile boolean running = false;

x10rf myx10 = x10rf(tx, 0, reps);

void setup() {
  pinMode(switchUp, INPUT);
  pinMode(switchDown, INPUT);
  
  flash(true);
} // setup

void flash(boolean slow) {
  for (int k = 0; k < 10; k = k + 1) {
    if (k % 2 == 0) {
      digitalWrite(statusLED, HIGH);
    }
    else {
      digitalWrite(statusLED, LOW);
    }
    delay(slow ? 100 : 50);
  }
}

void sleep() {

  delay(500);
  // shutdown ports
  digitalWrite(statusLED, LOW);
  digitalWrite(senderPower, LOW);
  digitalWrite(tx, LOW);
  pinMode(senderPower, INPUT);
  pinMode(statusLED, INPUT);
  pinMode(tx, INPUT); // need to do this, otherwise RF consumes


//  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  //festlegen des Schlafmoduses
//  sleep_enable();  //ermoeglichen der angegebenen Schlaffunktion
//  sleep_mode();  //starten der Schlaffunktion
//  //hier geht es nach dem Interrupt weiter
//  sleep_disable();  //deaktivieren der Schlaffunktion
//  power_all_enable();  //reaktivieren aller Chips/Funktionen

  GIMSK |= _BV(PCIE);                     // Enable Pin Change Interrupts
  PCMSK |= _BV(PCINT3);                   // Use PB3 as interrupt pin
  PCMSK |= _BV(PCINT4);                   // Use PB3 as interrupt pin
  ADCSRA &= ~_BV(ADEN);                   // ADC off
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);    // replaces above statement
  sleep_enable();                         // Sets the Sleep Enable bit in the MCUCR Register (SE BIT)
  sei();                                  // Enable interrupts
  sleep_cpu();                            // sleep

  cli();                                  // Disable interrupts
  PCMSK &= ~_BV(PCINT3);
  PCMSK &= ~_BV(PCINT4);                  // Turn off interrupt
  sleep_disable();                        // Clear SE bit
  ADCSRA |= _BV(ADEN);                    // ADC on
  sei();                                  // Enable interrupts

  // enable ports
  pinMode(senderPower, OUTPUT);
  pinMode(statusLED, OUTPUT);
  pinMode(tx, OUTPUT);
  digitalWrite(senderPower, HIGH);
  myx10.begin();
  delay(100);
} // sleep


typedef enum { NONE, UP, DOWN } switchType;
volatile switchType button = NONE;


// found at https://sites.google.com/site/qeewiki/books/avr-guide/external-interrupts-on-the-atmega328

ISR(PCINT0_vect) {
  
  uint8_t changedbits;
  changedbits = PINB ^ portbhistory;

   if(changedbits & (1 << PB3)) {  
     button = UP;
   }
   if(changedbits & (1 << PB4)) {  
      button = DOWN;
   }

}

boolean isLongPress(int id) {
  int count = 0;
  while (digitalRead(id) == 0 && count < 6) {
    count++;
    delay(50);
  }
  return count > 5;
}

void loop() {
  sleep();
    
  switch (button) {
    case UP:
     if (isLongPress(switchUp)) {
      myx10.x10Switch('D', 15, ON);
      flash(true);
     } else {
      myx10.x10Switch('C', 15, ON);
      flash(false);
     }
     break;
   case DOWN:
     if (isLongPress(switchDown)) {
       myx10.x10Switch('D', 15, OFF); 
       flash(true);
     } else {
       myx10.x10Switch('C', 15, OFF);
       flash(false);
     }
     break;
   default:
     break;
  }
  

}
