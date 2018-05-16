var check = 1;

function blink() {
  var inp = GPIO.read(0);
  var blk = (check > 8) ? 1 - inp : inp;
  GPIO.write(2, blk);
  check = check >= 10 ? 1 : check + 1;
}

// GPIO 0 as input
// GPIO 2 as output
GPIO.pinMode(0, GPIO.INPUT);
GPIO.pinMode(2, GPIO.OUTPUT);

print("blink js OK");
