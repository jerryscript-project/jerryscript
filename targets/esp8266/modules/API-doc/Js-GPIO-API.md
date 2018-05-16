# GPIO

GPIO is a generic pin on an integrated circuit or computer board whose behavior - including whether it is an input or output pin - is controllable by the user at run time.

### GPIO_PIN
The board contains 11 configurable GPIO pins. To identify them ESP8266-12E uses the next mapping:

| Board pin | GPIO pin |
| ------ | ------ |
| D0 | 16* |
| D1 | 5 |
| D2 | 4 |
| D3 | 0 |
| D4 | 2 |
| D5 | 4 |
| D6 | 2 |
| D7 | 3 |
| D8 | 5 |
| RX | 3 |
| TX | 1 |

\* GPIO 16 can only used for GPIO read/write interrupts are no supported.

## Levels

### GPIO.LOW
  - Indicates logical low voltage.

### GPIO.HIGH
  - Indicates logical high voltage.

## Modes
### GPIO.INPUT
  - Indicates pin as input.

### GPIO.OUTPUT
  - Indicates pin as output.

## Methods
### GPIO.pinMode(pin, mode)
  - `pin` {GPIO_PIN} - Desired pin to set.
  - `mode` {GPIO.INPUT | GPIO.OUTPUT} - Input or output.
  - Returns: {undefined}

  Sets the selected GPIO `pin` to the given `mode`.

**Example**

```js
GPIO.pinMode(1, GPIO.OUTPUT);
```

### GPIO.write(pin, level)
  - `pin` {GPIO_PIN} - Desired pin to write.
  - `level` {GPIO.HIGH | GPIO.LOW} - High or low.
  - Returns: {undefined}

  Pulls the selected GPIO `pin` to the given `level`.

**Example**

```js
GPIO.write(1, GPIO.LOW);
```

### GPIO.read(pin)
  - `pin` {GPIO_PIN} - Desired pin to read.
  - Returns: {integer} - `0` | `1` based on the read data.

  Reads the selected GPIO `pin`.

**Example**

```js
var data = GPIO.read(1);
```
