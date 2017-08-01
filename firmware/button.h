#define BUTTON_HOLD_TIME      1500
#define BUTTON_DEBOUNCE_TIME  50


class Button 
{
  int pin;
  bool pressed;
  bool held;
  unsigned long press_time;
  unsigned long release_time;
  
  public:
    Button(int pin, WiringPinMode mode=INPUT_PULLUP) {
        this->pressed = false;
        this->held = false;
        this->press_time = 0;
        this->release_time = 0;
        
        this->pin = pin;
        pinMode(pin, mode);
    }

    void poll(void (*short_press)()=NULL, void (*hold)()=NULL) {
        bool current_pressed = digitalRead(this->pin) == LOW;

        // button state changed
        if (current_pressed != this->pressed) {
            this->pressed = current_pressed;
            
            // press
            if (current_pressed) {
                 this->press_time = millis();
            }
            // release
            else {
                if (millis() -  this->press_time > BUTTON_DEBOUNCE_TIME) {
                     this->release_time = millis();
                    if (!this->held && short_press) {
                       short_press();
                    }
                }
            }
            
            held = false;
        }

        // hold?
        if (this->pressed && !this->held && millis() - this->press_time > BUTTON_HOLD_TIME) {
            held = true;
            if (hold) {
                hold();
            }
        }
    }

    bool isPressed() {
        return this->pressed;
    }

    bool isHeld() {
        return this->held;
    }

    unsigned long getPressTime() {
        return this->press_time;
    }

    unsigned long getReleaseTime() {
        return this->release_time;
    }
};

