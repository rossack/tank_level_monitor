/*
**
** Class to manage a momentary push button
** https://www.electronicdesign.com/technologies/analog/article/21155418/logiswitch-11-myths-about-switch-bouncedebounce
**
**
*/
#include "button.h"

MPButton::MPButton() {
    buttonState = BTN_NOTREADY;
    event = WAITING;
    doubleClick = false;
    alarm_pool_init_default();
}

MPButton::MPButton(uint gpio, void (*cb)(Event)) {
    gpioSetup(gpio);
    eventHandler(cb);
}

//
// The debounce time is up so the button state can be read
//
int64_t MPButton::alarmCallback(alarm_id_t id, void *uData) {
    MPButton * button = (MPButton *)uData;
    button->bTranTime = time_us_32(); // start to measure duration
    if (gpio_get(button->gpioPin)) button->buttonState = BTN_OPEN;
    else button->buttonState = BTN_CLOSED;
    return(0); //dont reschedule the alarm
} 

//
// Button has changed state. ISR dispatcher calls this method.
// Note the way double clicks work, it results in two events firing within the DBCLICK_TIME
// The first being CLICK and second being DBCLICK. This avoids the need to hold and wait for 
// for the second button press to fire a single event.
//
void MPButton::interrupt(uint32_t hw_evt) {
    uint32_t timeElapsed;
    switch(buttonState) {
        case BTN_OPEN:
            if (hw_evt & GPIO_IRQ_EDGE_FALL) { 
                timeElapsed = (time_us_32() - bTranTime)/1000;
                if (timeElapsed < DBCLICK_TIME) doubleClick = true; 
                add_alarm_in_ms(DEBOUNCE_TIME, &alarmCallback, this, true);
                buttonState = BTN_DEBOUNCE_WINDOW;
            }
            break;
        
        case BTN_DEBOUNCE_WINDOW:
            // ignore interrupts during this period
            break;
        
        case BTN_CLOSED:
            if (hw_evt & GPIO_IRQ_EDGE_RISE) {  
                timeElapsed = (time_us_32() - bTranTime)/1000;
                if (timeElapsed > LPRESS_TIME) event = LPRESS; else
                if (timeElapsed > CLICK_TIME) {
                    if (doubleClick) {
                        event = DBCLICK;
                        doubleClick = false;
                    } else {
                        event = CLICK;
                    }
                }
                //DEBUG_printf("Button time:%u, event: %d\n",timeElapsed, event);
                if (callback) callback(event);
                add_alarm_in_ms(DEBOUNCE_TIME, &alarmCallback, this, true);
                buttonState = BTN_DEBOUNCE_WINDOW;
            }
            break;
    }
}

void MPButton::gpioSetup(uint gpio) {
    // Assign the button to GPIO pin for input, 
    // where Open=High (via the pullup), Closed=Ground
    gpioPin = gpio;
    gpio_init(gpio);
    gpio_pull_up(gpio);
	gpio_set_dir(gpio, GPIO_IN); 
    if (gpio_get(gpio)) {
        buttonState = BTN_OPEN; 
    } else {
        buttonState = BTN_CLOSED;
    }
}

void MPButton::eventHandler(void (*cb)(Event e)) {
    //
    // When button events are determined, this callback will be fired
    // NOTE: This happens from inside the ISR so be careful what the callback 
    // needs to do so deadlocks dont occur. Anything else attached to the ISR 
    // will be held up until the callback is executed. 
    // 
    callback = cb;
}

Event MPButton::getEvent() {
    //
    // if you want to poll for button events instead of using the callback
    // 
    if (event != WAITING) {
        Event tmp;
        tmp = event;
        event = WAITING;
        return(tmp);
    } else return(WAITING);
}

