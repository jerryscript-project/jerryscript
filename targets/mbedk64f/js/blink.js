var check = 1;

function blink() {
  var blk = (check > 8) ? 1 : 0;
  led(1, blk);
  check = (check >= 10) ? 1 : check+1;
}

print("blink js OK");
