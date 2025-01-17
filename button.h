/*
**
** Class to handle a momentary push button
** TBD: inherit from an abstract base class InputDevice ??
** 
**
*/
#include <stdio.h>
#include "pico/stdlib.h"

#define DEBOUNCE_TIME 10 //mSec ignore any button interrupts during this period
#define CLICK_TIME 20 //mSec minimum duration to qualify as a click
#define DBCLICK_TIME 500 //mSec need to see two clicks within this time
#define LPRESS_TIME 1000 //mSec minimum duration to qualify as a long press 

enum State { 
    BTN_NOTREADY = -1,
    BTN_OPEN = 0, 
    BTN_CLOSED = 1, 
    BTN_DEBOUNCE_WINDOW = 3,
};

enum Event { 
    WAITING = 0,// Waiting for somethign to happen
    CLICK = 1,  // User action
    DBCLICK = 2,
    LPRESS = 3
};


class MPButton
{
    private:
    volatile Event event = WAITING; // ISR will change this value
    volatile State buttonState; // ISR will change this value
    volatile uint32_t bTranTime; // time at which button transition occurred
    volatile bool doubleClick; // possible doubleClick may/may not occur. 
    uint gpioPin; // what the button is connected to.
    void (*callback)(Event) = NULL; 
    static int64_t alarmCallback(alarm_id_t id, void *uData); // needs to be static See: http://www.gammon.com.au/forum/?id=12983

    public:
    MPButton();
    MPButton(uint gPn, void (*callback)(Event));
    void gpioSetup(uint gPn); // specify what GPIO the button is connected to
    void eventHandler(void (*callback)(Event)); // attach the users callback 
    
    void interrupt(uint32_t hw_evt); // ISR calls this method 
    Event getEvent(); // if you want to poll it, calling this will clear the event back to WAITING
};
