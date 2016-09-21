/* Copyright (c) 2016 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

var led = 0;

// Setting the pin to 0 turns the LED on
var led_off = 1;
var led_on = 0;

var digital_outs = [];

var leds = [LED1, LED2, LED3, LED4];

function connect_pins() {
    print("Creating new DigitalOuts");
    digital_outs = [];
    for (var i = 0; i < 4; i++) {
        digital_outs.push(DigitalOut(leds[i], led_off));
        if (digital_outs[i].is_connected()) {
            print("LED " + i + " is connected.");
        }
        else {
            print("LED " + i + " is not connected.");
        }
    }
}

connect_pins();

function blink() {
    digital_outs[0].write(led_off);
    digital_outs[1].write(led_off);
    digital_outs[2].write(led_off);
    digital_outs[3].write(led_off);

    digital_outs[led].write(led_on);

    print("Finished with LED " + led);
    led = (led + 1) % 4;
}

// BUTTON2 on NRF52
// USER_BUTTON on NUCLEO
// SW2 on the K64F
// BTN0 on EFM32GG
var button;

/* global BUTTON2, SW2, USER_BUTTON, BTN0 */
if (typeof BUTTON2 !== 'undefined') {
        button = InterruptIn(BUTTON2);
} else if (typeof SW2 !== 'undefined') {
        button = InterruptIn(SW2);
} else if (typeof USER_BUTTON !== 'undefined') {
        button = InterruptIn(USER_BUTTON);  
} else if (typeof BTN0 !== 'undefined') {
        button = InterruptIn(BTN0);    
} else {
        print("no button specified");
}
button.fall(function() {
    print("YOU PUSHED THE BUTTON!");
});

print("flash_leds.js has finished executing.");
