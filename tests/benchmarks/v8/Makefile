all: combined.js

FILES=base.js richards.js deltablue.js crypto.js raytrace.js earley-boyer.js regexp.js splay.js navier-stokes.js run_harness.js
#FILES=base.js richards.js deltablue.js crypto.js raytrace.js earley-boyer.js splay.js navier-stokes.js run_harness.js

combined.js: $(FILES)
	cat $(FILES) >$@

clean:
	rm -f combined.js
