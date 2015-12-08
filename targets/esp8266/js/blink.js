var check = 1;

function blink() {
  var inp = gpio_get(0);
  var blk = (check > 8) ? 1 - inp : inp;
  gpio_set(2, blk);
  check = check >= 10 ? 1 : check+1;
}

// GPIO 0 as input
// GPIO 2 as output
gpio_dir(0, 0);
gpio_dir(2, 1);

print("blink js OK");
