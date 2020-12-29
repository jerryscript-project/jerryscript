/* run_harness.js */

function do_print()
{
  if (typeof print === 'function') {
    print.apply(null, arguments)
  } else {
    console.log.apply(null, arguments)
  }
}

function Run() {
  BenchmarkSuite.RunSuites({ NotifyStep: ShowProgress,
                             NotifyError: AddError,
                             NotifyResult: AddResult,
                             NotifyScore: AddScore });
}

var harnessErrorCount = 0;

function ShowProgress(name) {
  do_print('PROGRESS', name);
}

function AddError(name, error) {
  do_print('ERROR', name, error);
  do_print(error.stack);
  harnessErrorCount++;
}

function AddResult(name, result) {
  do_print('RESULT', name, result);
}

function AddScore(score) {
  do_print('SCORE', score);
}

try {
  Run();
} catch (e) {
  do_print('*** Run() failed');
  do_print(e.stack || e);
}

if (harnessErrorCount > 0) {
  // Throw an error so that 'duk' has a non-zero exit code which helps
  // automatic testing.
  throw new Error('Benchmark had ' + harnessErrorCount + ' errors');
}
