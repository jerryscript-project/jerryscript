"use strict";
"use strip";
// Copyright 2012 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Simple framework for running the benchmark suites and
// computing a score based on the timing measurements.


// A benchmark has a name (string) and a function that will be run to
// do the performance measurement. The optional setup and tearDown
// arguments are functions that will be invoked before and after
// running the benchmark, but the running time of these functions will
// not be accounted for in the benchmark score.
function Benchmark(name, run, setup, tearDown) {
  this.name = name;
  this.run = run;
  this.Setup = setup ? setup : function() { };
  this.TearDown = tearDown ? tearDown : function() { };
}


// Benchmark results hold the benchmark and the measured time used to
// run the benchmark. The benchmark score is computed later once a
// full benchmark suite has run to completion.
function BenchmarkResult(benchmark, time) {
  this.benchmark = benchmark;
  this.time = time;
}


// Automatically convert results to numbers. Used by the geometric
// mean computation.
BenchmarkResult.prototype.valueOf = function() {
  return this.time;
};


// Suites of benchmarks consist of a name and the set of benchmarks in
// addition to the reference timing that the final score will be based
// on. This way, all scores are relative to a reference run and higher
// scores implies better performance.
function BenchmarkSuite(name, reference, benchmarks) {
  this.name = name;
  this.reference = reference;
  this.benchmarks = benchmarks;
  BenchmarkSuite.suites.push(this);
}


// Keep track of all declared benchmark suites.
BenchmarkSuite.suites = [];


// Scores are not comparable across versions. Bump the version if
// you're making changes that will affect that scores, e.g. if you add
// a new benchmark or change an existing one.
BenchmarkSuite.version = '7';


// To make the benchmark results predictable, we replace Math.random
// with a 100% deterministic alternative.
Math.random = (function() {
  var seed = 49734321;
  return function() {
    // Robert Jenkins' 32 bit integer hash function.
    seed = ((seed + 0x7ed55d16) + (seed << 12))  & 0xffffffff;
    seed = ((seed ^ 0xc761c23c) ^ (seed >>> 19)) & 0xffffffff;
    seed = ((seed + 0x165667b1) + (seed << 5))   & 0xffffffff;
    seed = ((seed + 0xd3a2646c) ^ (seed << 9))   & 0xffffffff;
    seed = ((seed + 0xfd7046c5) + (seed << 3))   & 0xffffffff;
    seed = ((seed ^ 0xb55a4f09) ^ (seed >>> 16)) & 0xffffffff;
    return (seed & 0xfffffff) / 0x10000000;
  };
})();


// Runs all registered benchmark suites and optionally yields between
// each individual benchmark to avoid running for too long in the
// context of browsers. Once done, the final score is reported to the
// runner.
BenchmarkSuite.RunSuites = function(runner) {
  var continuation = null;
  var suites = BenchmarkSuite.suites;
  var length = suites.length;
  BenchmarkSuite.scores = [];
  var index = 0;
  function RunStep() {
    while (continuation || index < length) {
      if (continuation) {
        continuation = continuation();
      } else {
        var suite = suites[index++];
        if (runner.NotifyStart) runner.NotifyStart(suite.name);
        continuation = suite.RunStep(runner);
      }
      if (continuation && typeof window != 'undefined' && window.setTimeout) {
        window.setTimeout(RunStep, 25);
        return;
      }
    }
    if (runner.NotifyScore) {
      var score = BenchmarkSuite.GeometricMean(BenchmarkSuite.scores);
      var formatted = BenchmarkSuite.FormatScore(100 * score);
      runner.NotifyScore(formatted);
    }
  }
  RunStep();
};


// Counts the total number of registered benchmarks. Useful for
// showing progress as a percentage.
BenchmarkSuite.CountBenchmarks = function() {
  var result = 0;
  var suites = BenchmarkSuite.suites;
  for (var i = 0; i < suites.length; i++) {
    result += suites[i].benchmarks.length;
  }
  return result;
};


// Computes the geometric mean of a set of numbers.
BenchmarkSuite.GeometricMean = function(numbers) {
  var log = 0;
  for (var i = 0; i < numbers.length; i++) {
    log += Math.log(numbers[i]);
  }
  return Math.pow(Math.E, log / numbers.length);
};


// Converts a score value to a string with at least three significant
// digits.
BenchmarkSuite.FormatScore = function(value) {
  if (value > 100) {
    return value.toFixed(0);
  } else {
    return value.toPrecision(3);
  }
};

// Notifies the runner that we're done running a single benchmark in
// the benchmark suite. This can be useful to report progress.
BenchmarkSuite.prototype.NotifyStep = function(result) {
  this.results.push(result);
  if (this.runner.NotifyStep) this.runner.NotifyStep(result.benchmark.name);
};


// Notifies the runner that we're done with running a suite and that
// we have a result which can be reported to the user if needed.
BenchmarkSuite.prototype.NotifyResult = function() {
  var mean = BenchmarkSuite.GeometricMean(this.results);
  var score = this.reference / mean;
  BenchmarkSuite.scores.push(score);
  if (this.runner.NotifyResult) {
    var formatted = BenchmarkSuite.FormatScore(100 * score);
    this.runner.NotifyResult(this.name, formatted);
  }
};


// Notifies the runner that running a benchmark resulted in an error.
BenchmarkSuite.prototype.NotifyError = function(error) {
  if (this.runner.NotifyError) {
    this.runner.NotifyError(this.name, error);
  }
  if (this.runner.NotifyStep) {
    this.runner.NotifyStep(this.name);
  }
};


// Runs a single benchmark for at least a second and computes the
// average time it takes to run a single iteration.
BenchmarkSuite.prototype.RunSingleBenchmark = function(benchmark, data) {
  function Measure(data) {
    var elapsed = 0;
    var start = new Date();
    for (var n = 0; elapsed < 1000; n++) {
      benchmark.run();
      elapsed = new Date() - start;
    }
    if (data != null) {
      data.runs += n;
      data.elapsed += elapsed;
    }
  }

  if (data == null) {
    // Measure the benchmark once for warm up and throw the result
    // away. Return a fresh data object.
    Measure(null);
    return { runs: 0, elapsed: 0 };
  } else {
    Measure(data);
    // If we've run too few iterations, we continue for another second.
    if (data.runs < 32) return data;
    var usec = (data.elapsed * 1000) / data.runs;
    this.NotifyStep(new BenchmarkResult(benchmark, usec));
    return null;
  }
};


// This function starts running a suite, but stops between each
// individual benchmark in the suite and returns a continuation
// function which can be invoked to run the next benchmark. Once the
// last benchmark has been executed, null is returned.
BenchmarkSuite.prototype.RunStep = function(runner) {
  this.results = [];
  this.runner = runner;
  var length = this.benchmarks.length;
  var index = 0;
  var suite = this;
  var data;

  // Run the setup, the actual benchmark, and the tear down in three
  // separate steps to allow the framework to yield between any of the
  // steps.

  function RunNextSetup() {
    if (index < length) {
      try {
        suite.benchmarks[index].Setup();
      } catch (e) {
        suite.NotifyError(e);
        return null;
      }
      return RunNextBenchmark;
    }
    suite.NotifyResult();
    return null;
  }

  function RunNextBenchmark() {
    try {
      data = suite.RunSingleBenchmark(suite.benchmarks[index], data);
    } catch (e) {
      suite.NotifyError(e);
      return null;
    }
    // If data is null, we're done with this benchmark.
    return (data == null) ? RunNextTearDown : RunNextBenchmark();
  }

  function RunNextTearDown() {
    try {
      suite.benchmarks[index++].TearDown();
    } catch (e) {
      suite.NotifyError(e);
      return null;
    }
    return RunNextSetup;
  }

  // Start out running the setup.
  return RunNextSetup();
};
// Copyright 2006-2008 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


// This is a JavaScript implementation of the Richards
// benchmark from:
//
//    http://www.cl.cam.ac.uk/~mr10/Bench.html
//
// The benchmark was originally implemented in BCPL by
// Martin Richards.


/**
 * The Richards benchmark simulates the task dispatcher of an
 * operating system.
 **/
function runRichards() {
  var scheduler = new Scheduler();
  scheduler.addIdleTask(ID_IDLE, 0, null, COUNT);

  var queue = new Packet(null, ID_WORKER, KIND_WORK);
  queue = new Packet(queue,  ID_WORKER, KIND_WORK);
  scheduler.addWorkerTask(ID_WORKER, 1000, queue);

  queue = new Packet(null, ID_DEVICE_A, KIND_DEVICE);
  queue = new Packet(queue,  ID_DEVICE_A, KIND_DEVICE);
  queue = new Packet(queue,  ID_DEVICE_A, KIND_DEVICE);
  scheduler.addHandlerTask(ID_HANDLER_A, 2000, queue);

  queue = new Packet(null, ID_DEVICE_B, KIND_DEVICE);
  queue = new Packet(queue,  ID_DEVICE_B, KIND_DEVICE);
  queue = new Packet(queue,  ID_DEVICE_B, KIND_DEVICE);
  scheduler.addHandlerTask(ID_HANDLER_B, 3000, queue);

  scheduler.addDeviceTask(ID_DEVICE_A, 4000, null);

  scheduler.addDeviceTask(ID_DEVICE_B, 5000, null);

  scheduler.schedule();

  if (scheduler.queueCount != EXPECTED_QUEUE_COUNT ||
      scheduler.holdCount != EXPECTED_HOLD_COUNT) {
    var msg =
        "Error during execution: queueCount = " + scheduler.queueCount +
        ", holdCount = " + scheduler.holdCount + ".";
    throw new Error(msg);
  }
}

var COUNT = 1000;

/**
 * These two constants specify how many times a packet is queued and
 * how many times a task is put on hold in a correct run of richards.
 * They don't have any meaning a such but are characteristic of a
 * correct run so if the actual queue or hold count is different from
 * the expected there must be a bug in the implementation.
 **/
var EXPECTED_QUEUE_COUNT = 2322;
var EXPECTED_HOLD_COUNT = 928;


/**
 * A scheduler can be used to schedule a set of tasks based on their relative
 * priorities.  Scheduling is done by maintaining a list of task control blocks
 * which holds tasks and the data queue they are processing.
 * @constructor
 */
function Scheduler() {
  this.queueCount = 0;
  this.holdCount = 0;
  this.blocks = new Array(NUMBER_OF_IDS);
  this.list = null;
  this.currentTcb = null;
  this.currentId = null;
}

var ID_IDLE       = 0;
var ID_WORKER     = 1;
var ID_HANDLER_A  = 2;
var ID_HANDLER_B  = 3;
var ID_DEVICE_A   = 4;
var ID_DEVICE_B   = 5;
var NUMBER_OF_IDS = 6;

var KIND_DEVICE   = 0;
var KIND_WORK     = 1;

/**
 * Add an idle task to this scheduler.
 * @param {int} id the identity of the task
 * @param {int} priority the task's priority
 * @param {Packet} queue the queue of work to be processed by the task
 * @param {int} count the number of times to schedule the task
 */
Scheduler.prototype.addIdleTask = function (id, priority, queue, count) {
  this.addRunningTask(id, priority, queue, new IdleTask(this, 1, count));
};

/**
 * Add a work task to this scheduler.
 * @param {int} id the identity of the task
 * @param {int} priority the task's priority
 * @param {Packet} queue the queue of work to be processed by the task
 */
Scheduler.prototype.addWorkerTask = function (id, priority, queue) {
  this.addTask(id, priority, queue, new WorkerTask(this, ID_HANDLER_A, 0));
};

/**
 * Add a handler task to this scheduler.
 * @param {int} id the identity of the task
 * @param {int} priority the task's priority
 * @param {Packet} queue the queue of work to be processed by the task
 */
Scheduler.prototype.addHandlerTask = function (id, priority, queue) {
  this.addTask(id, priority, queue, new HandlerTask(this));
};

/**
 * Add a handler task to this scheduler.
 * @param {int} id the identity of the task
 * @param {int} priority the task's priority
 * @param {Packet} queue the queue of work to be processed by the task
 */
Scheduler.prototype.addDeviceTask = function (id, priority, queue) {
  this.addTask(id, priority, queue, new DeviceTask(this));
};

/**
 * Add the specified task and mark it as running.
 * @param {int} id the identity of the task
 * @param {int} priority the task's priority
 * @param {Packet} queue the queue of work to be processed by the task
 * @param {Task} task the task to add
 */
Scheduler.prototype.addRunningTask = function (id, priority, queue, task) {
  this.addTask(id, priority, queue, task);
  this.currentTcb.setRunning();
};

/**
 * Add the specified task to this scheduler.
 * @param {int} id the identity of the task
 * @param {int} priority the task's priority
 * @param {Packet} queue the queue of work to be processed by the task
 * @param {Task} task the task to add
 */
Scheduler.prototype.addTask = function (id, priority, queue, task) {
  this.currentTcb = new TaskControlBlock(this.list, id, priority, queue, task);
  this.list = this.currentTcb;
  this.blocks[id] = this.currentTcb;
};

/**
 * Execute the tasks managed by this scheduler.
 */
Scheduler.prototype.schedule = function () {
  this.currentTcb = this.list;
  while (this.currentTcb != null) {
    if (this.currentTcb.isHeldOrSuspended()) {
      this.currentTcb = this.currentTcb.link;
    } else {
      this.currentId = this.currentTcb.id;
      this.currentTcb = this.currentTcb.run();
    }
  }
};

/**
 * Release a task that is currently blocked and return the next block to run.
 * @param {int} id the id of the task to suspend
 */
Scheduler.prototype.release = function (id) {
  var tcb = this.blocks[id];
  if (tcb == null) return tcb;
  tcb.markAsNotHeld();
  if (tcb.priority > this.currentTcb.priority) {
    return tcb;
  } else {
    return this.currentTcb;
  }
};

/**
 * Block the currently executing task and return the next task control block
 * to run.  The blocked task will not be made runnable until it is explicitly
 * released, even if new work is added to it.
 */
Scheduler.prototype.holdCurrent = function () {
  this.holdCount++;
  this.currentTcb.markAsHeld();
  return this.currentTcb.link;
};

/**
 * Suspend the currently executing task and return the next task control block
 * to run.  If new work is added to the suspended task it will be made runnable.
 */
Scheduler.prototype.suspendCurrent = function () {
  this.currentTcb.markAsSuspended();
  return this.currentTcb;
};

/**
 * Add the specified packet to the end of the worklist used by the task
 * associated with the packet and make the task runnable if it is currently
 * suspended.
 * @param {Packet} packet the packet to add
 */
Scheduler.prototype.queue = function (packet) {
  var t = this.blocks[packet.id];
  if (t == null) return t;
  this.queueCount++;
  packet.link = null;
  packet.id = this.currentId;
  return t.checkPriorityAdd(this.currentTcb, packet);
};

/**
 * A task control block manages a task and the queue of work packages associated
 * with it.
 * @param {TaskControlBlock} link the preceding block in the linked block list
 * @param {int} id the id of this block
 * @param {int} priority the priority of this block
 * @param {Packet} queue the queue of packages to be processed by the task
 * @param {Task} task the task
 * @constructor
 */
function TaskControlBlock(link, id, priority, queue, task) {
  this.link = link;
  this.id = id;
  this.priority = priority;
  this.queue = queue;
  this.task = task;
  if (queue == null) {
    this.state = STATE_SUSPENDED;
  } else {
    this.state = STATE_SUSPENDED_RUNNABLE;
  }
}

/**
 * The task is running and is currently scheduled.
 */
var STATE_RUNNING = 0;

/**
 * The task has packets left to process.
 */
var STATE_RUNNABLE = 1;

/**
 * The task is not currently running.  The task is not blocked as such and may
* be started by the scheduler.
 */
var STATE_SUSPENDED = 2;

/**
 * The task is blocked and cannot be run until it is explicitly released.
 */
var STATE_HELD = 4;

var STATE_SUSPENDED_RUNNABLE = STATE_SUSPENDED | STATE_RUNNABLE;
var STATE_NOT_HELD = ~STATE_HELD;

TaskControlBlock.prototype.setRunning = function () {
  this.state = STATE_RUNNING;
};

TaskControlBlock.prototype.markAsNotHeld = function () {
  this.state = this.state & STATE_NOT_HELD;
};

TaskControlBlock.prototype.markAsHeld = function () {
  this.state = this.state | STATE_HELD;
};

TaskControlBlock.prototype.isHeldOrSuspended = function () {
  return (this.state & STATE_HELD) != 0 || (this.state == STATE_SUSPENDED);
};

TaskControlBlock.prototype.markAsSuspended = function () {
  this.state = this.state | STATE_SUSPENDED;
};

TaskControlBlock.prototype.markAsRunnable = function () {
  this.state = this.state | STATE_RUNNABLE;
};

/**
 * Runs this task, if it is ready to be run, and returns the next task to run.
 */
TaskControlBlock.prototype.run = function () {
  var packet;
  if (this.state == STATE_SUSPENDED_RUNNABLE) {
    packet = this.queue;
    this.queue = packet.link;
    if (this.queue == null) {
      this.state = STATE_RUNNING;
    } else {
      this.state = STATE_RUNNABLE;
    }
  } else {
    packet = null;
  }
  return this.task.run(packet);
};

/**
 * Adds a packet to the worklist of this block's task, marks this as runnable if
 * necessary, and returns the next runnable object to run (the one
 * with the highest priority).
 */
TaskControlBlock.prototype.checkPriorityAdd = function (task, packet) {
  if (this.queue == null) {
    this.queue = packet;
    this.markAsRunnable();
    if (this.priority > task.priority) return this;
  } else {
    this.queue = packet.addTo(this.queue);
  }
  return task;
};

TaskControlBlock.prototype.toString = function () {
  return "tcb { " + this.task + "@" + this.state + " }";
};

/**
 * An idle task doesn't do any work itself but cycles control between the two
 * device tasks.
 * @param {Scheduler} scheduler the scheduler that manages this task
 * @param {int} v1 a seed value that controls how the device tasks are scheduled
 * @param {int} count the number of times this task should be scheduled
 * @constructor
 */
function IdleTask(scheduler, v1, count) {
  this.scheduler = scheduler;
  this.v1 = v1;
  this.count = count;
}

IdleTask.prototype.run = function (packet) {
  this.count--;
  if (this.count == 0) return this.scheduler.holdCurrent();
  if ((this.v1 & 1) == 0) {
    this.v1 = this.v1 >> 1;
    return this.scheduler.release(ID_DEVICE_A);
  } else {
    this.v1 = (this.v1 >> 1) ^ 0xD008;
    return this.scheduler.release(ID_DEVICE_B);
  }
};

IdleTask.prototype.toString = function () {
  return "IdleTask";
};

/**
 * A task that suspends itself after each time it has been run to simulate
 * waiting for data from an external device.
 * @param {Scheduler} scheduler the scheduler that manages this task
 * @constructor
 */
function DeviceTask(scheduler) {
  this.scheduler = scheduler;
  this.v1 = null;
}

DeviceTask.prototype.run = function (packet) {
  if (packet == null) {
    if (this.v1 == null) return this.scheduler.suspendCurrent();
    var v = this.v1;
    this.v1 = null;
    return this.scheduler.queue(v);
  } else {
    this.v1 = packet;
    return this.scheduler.holdCurrent();
  }
};

DeviceTask.prototype.toString = function () {
  return "DeviceTask";
};

/**
 * A task that manipulates work packets.
 * @param {Scheduler} scheduler the scheduler that manages this task
 * @param {int} v1 a seed used to specify how work packets are manipulated
 * @param {int} v2 another seed used to specify how work packets are manipulated
 * @constructor
 */
function WorkerTask(scheduler, v1, v2) {
  this.scheduler = scheduler;
  this.v1 = v1;
  this.v2 = v2;
}

WorkerTask.prototype.run = function (packet) {
  if (packet == null) {
    return this.scheduler.suspendCurrent();
  } else {
    if (this.v1 == ID_HANDLER_A) {
      this.v1 = ID_HANDLER_B;
    } else {
      this.v1 = ID_HANDLER_A;
    }
    packet.id = this.v1;
    packet.a1 = 0;
    for (var i = 0; i < DATA_SIZE; i++) {
      this.v2++;
      if (this.v2 > 26) this.v2 = 1;
      packet.a2[i] = this.v2;
    }
    return this.scheduler.queue(packet);
  }
};

WorkerTask.prototype.toString = function () {
  return "WorkerTask";
};

/**
 * A task that manipulates work packets and then suspends itself.
 * @param {Scheduler} scheduler the scheduler that manages this task
 * @constructor
 */
function HandlerTask(scheduler) {
  this.scheduler = scheduler;
  this.v1 = null;
  this.v2 = null;
}

HandlerTask.prototype.run = function (packet) {
  if (packet != null) {
    if (packet.kind == KIND_WORK) {
      this.v1 = packet.addTo(this.v1);
    } else {
      this.v2 = packet.addTo(this.v2);
    }
  }
  if (this.v1 != null) {
    var count = this.v1.a1;
    var v;
    if (count < DATA_SIZE) {
      if (this.v2 != null) {
        v = this.v2;
        this.v2 = this.v2.link;
        v.a1 = this.v1.a2[count];
        this.v1.a1 = count + 1;
        return this.scheduler.queue(v);
      }
    } else {
      v = this.v1;
      this.v1 = this.v1.link;
      return this.scheduler.queue(v);
    }
  }
  return this.scheduler.suspendCurrent();
};

HandlerTask.prototype.toString = function () {
  return "HandlerTask";
};

/* --- *
 * P a c k e t
 * --- */

var DATA_SIZE = 4;

/**
 * A simple package of data that is manipulated by the tasks.  The exact layout
 * of the payload data carried by a packet is not importaint, and neither is the
 * nature of the work performed on packets by the tasks.
 *
 * Besides carrying data, packets form linked lists and are hence used both as
 * data and worklists.
 * @param {Packet} link the tail of the linked list of packets
 * @param {int} id an ID for this packet
 * @param {int} kind the type of this packet
 * @constructor
 */
function Packet(link, id, kind) {
  this.link = link;
  this.id = id;
  this.kind = kind;
  this.a1 = 0;
  this.a2 = new Array(DATA_SIZE);
}

/**
 * Add this packet to the end of a worklist, and return the worklist.
 * @param {Packet} queue the worklist to add this packet to
 */
Packet.prototype.addTo = function (queue) {
  this.link = null;
  if (queue == null) return this;
  var peek, next = queue;
  while ((peek = next.link) != null)
    next = peek;
  next.link = this;
  return queue;
};

Packet.prototype.toString = function () {
  return "Packet";
};

var Richards = new BenchmarkSuite('Richards', 35302, [
  new Benchmark("Richards", runRichards)
]);

// Copyright 2008 the V8 project authors. All rights reserved.
// Copyright 1996 John Maloney and Mario Wolczko.

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


// This implementation of the DeltaBlue benchmark is derived
// from the Smalltalk implementation by John Maloney and Mario
// Wolczko. Some parts have been translated directly, whereas
// others have been modified more aggresively to make it feel
// more like a JavaScript program.


/**
 * A JavaScript implementation of the DeltaBlue constraint-solving
 * algorithm, as described in:
 *
 * "The DeltaBlue Algorithm: An Incremental Constraint Hierarchy Solver"
 *   Bjorn N. Freeman-Benson and John Maloney
 *   January 1990 Communications of the ACM,
 *   also available as University of Washington TR 89-08-06.
 *
 * Beware: this benchmark is written in a grotesque style where
 * the constraint model is built by side-effects from constructors.
 * I've kept it this way to avoid deviating too much from the original
 * implementation.
 */


/* --- O b j e c t   M o d e l --- */

Object.prototype.inheritsFrom = function (shuper) {
  function Inheriter() { }
  Inheriter.prototype = shuper.prototype;
  this.prototype = new Inheriter();
  this.superConstructor = shuper;
};

function OrderedCollection() {
  this.elms = new Array();
}

OrderedCollection.prototype.add = function (elm) {
  this.elms.push(elm);
};

OrderedCollection.prototype.at = function (index) {
  return this.elms[index];
};

OrderedCollection.prototype.size = function () {
  return this.elms.length;
};

OrderedCollection.prototype.removeFirst = function () {
  return this.elms.pop();
};

OrderedCollection.prototype.remove = function (elm) {
  var index = 0, skipped = 0;
  for (var i = 0; i < this.elms.length; i++) {
    var value = this.elms[i];
    if (value != elm) {
      this.elms[index] = value;
      index++;
    } else {
      skipped++;
    }
  }
  for (var i = 0; i < skipped; i++)
    this.elms.pop();
};

/* --- *
 * S t r e n g t h
 * --- */

/**
 * Strengths are used to measure the relative importance of constraints.
 * New strengths may be inserted in the strength hierarchy without
 * disrupting current constraints.  Strengths cannot be created outside
 * this class, so pointer comparison can be used for value comparison.
 */
function Strength(strengthValue, name) {
  this.strengthValue = strengthValue;
  this.name = name;
}

Strength.stronger = function (s1, s2) {
  return s1.strengthValue < s2.strengthValue;
};

Strength.weaker = function (s1, s2) {
  return s1.strengthValue > s2.strengthValue;
};

Strength.weakestOf = function (s1, s2) {
  return this.weaker(s1, s2) ? s1 : s2;
};

Strength.strongest = function (s1, s2) {
  return this.stronger(s1, s2) ? s1 : s2;
};

Strength.prototype.nextWeaker = function () {
  switch (this.strengthValue) {
    case 0: return Strength.STRONG_PREFERRED;
    case 1: return Strength.PREFERRED;
    case 2: return Strength.STRONG_DEFAULT;
    case 3: return Strength.NORMAL;
    case 4: return Strength.WEAK_DEFAULT;
    case 5: return Strength.WEAKEST;
  }
};

// Strength constants.
Strength.REQUIRED         = new Strength(0, "required");
Strength.STRONG_PREFERRED = new Strength(1, "strongPreferred");
Strength.PREFERRED        = new Strength(2, "preferred");
Strength.STRONG_DEFAULT   = new Strength(3, "strongDefault");
Strength.NORMAL           = new Strength(4, "normal");
Strength.WEAK_DEFAULT     = new Strength(5, "weakDefault");
Strength.WEAKEST          = new Strength(6, "weakest");

/* --- *
 * C o n s t r a i n t
 * --- */

/**
 * An abstract class representing a system-maintainable relationship
 * (or "constraint") between a set of variables. A constraint supplies
 * a strength instance variable; concrete subclasses provide a means
 * of storing the constrained variables and other information required
 * to represent a constraint.
 */
function Constraint(strength) {
  this.strength = strength;
}

/**
 * Activate this constraint and attempt to satisfy it.
 */
Constraint.prototype.addConstraint = function () {
  this.addToGraph();
  planner.incrementalAdd(this);
};

/**
 * Attempt to find a way to enforce this constraint. If successful,
 * record the solution, perhaps modifying the current dataflow
 * graph. Answer the constraint that this constraint overrides, if
 * there is one, or nil, if there isn't.
 * Assume: I am not already satisfied.
 */
Constraint.prototype.satisfy = function (mark) {
  this.chooseMethod(mark);
  if (!this.isSatisfied()) {
    if (this.strength == Strength.REQUIRED)
      alert("Could not satisfy a required constraint!");
    return null;
  }
  this.markInputs(mark);
  var out = this.output();
  var overridden = out.determinedBy;
  if (overridden != null) overridden.markUnsatisfied();
  out.determinedBy = this;
  if (!planner.addPropagate(this, mark))
    alert("Cycle encountered");
  out.mark = mark;
  return overridden;
};

Constraint.prototype.destroyConstraint = function () {
  if (this.isSatisfied()) planner.incrementalRemove(this);
  else this.removeFromGraph();
};

/**
 * Normal constraints are not input constraints.  An input constraint
 * is one that depends on external state, such as the mouse, the
 * keybord, a clock, or some arbitraty piece of imperative code.
 */
Constraint.prototype.isInput = function () {
  return false;
};

/* --- *
 * U n a r y   C o n s t r a i n t
 * --- */

/**
 * Abstract superclass for constraints having a single possible output
 * variable.
 */
function UnaryConstraint(v, strength) {
  UnaryConstraint.superConstructor.call(this, strength);
  this.myOutput = v;
  this.satisfied = false;
  this.addConstraint();
}

UnaryConstraint.inheritsFrom(Constraint);

/**
 * Adds this constraint to the constraint graph
 */
UnaryConstraint.prototype.addToGraph = function () {
  this.myOutput.addConstraint(this);
  this.satisfied = false;
};

/**
 * Decides if this constraint can be satisfied and records that
 * decision.
 */
UnaryConstraint.prototype.chooseMethod = function (mark) {
  this.satisfied = (this.myOutput.mark != mark)
    && Strength.stronger(this.strength, this.myOutput.walkStrength);
};

/**
 * Returns true if this constraint is satisfied in the current solution.
 */
UnaryConstraint.prototype.isSatisfied = function () {
  return this.satisfied;
};

UnaryConstraint.prototype.markInputs = function (mark) {
  // has no inputs
};

/**
 * Returns the current output variable.
 */
UnaryConstraint.prototype.output = function () {
  return this.myOutput;
};

/**
 * Calculate the walkabout strength, the stay flag, and, if it is
 * 'stay', the value for the current output of this constraint. Assume
 * this constraint is satisfied.
 */
UnaryConstraint.prototype.recalculate = function () {
  this.myOutput.walkStrength = this.strength;
  this.myOutput.stay = !this.isInput();
  if (this.myOutput.stay) this.execute(); // Stay optimization
};

/**
 * Records that this constraint is unsatisfied
 */
UnaryConstraint.prototype.markUnsatisfied = function () {
  this.satisfied = false;
};

UnaryConstraint.prototype.inputsKnown = function () {
  return true;
};

UnaryConstraint.prototype.removeFromGraph = function () {
  if (this.myOutput != null) this.myOutput.removeConstraint(this);
  this.satisfied = false;
};

/* --- *
 * S t a y   C o n s t r a i n t
 * --- */

/**
 * Variables that should, with some level of preference, stay the same.
 * Planners may exploit the fact that instances, if satisfied, will not
 * change their output during plan execution.  This is called "stay
 * optimization".
 */
function StayConstraint(v, str) {
  StayConstraint.superConstructor.call(this, v, str);
}

StayConstraint.inheritsFrom(UnaryConstraint);

StayConstraint.prototype.execute = function () {
  // Stay constraints do nothing
};

/* --- *
 * E d i t   C o n s t r a i n t
 * --- */

/**
 * A unary input constraint used to mark a variable that the client
 * wishes to change.
 */
function EditConstraint(v, str) {
  EditConstraint.superConstructor.call(this, v, str);
}

EditConstraint.inheritsFrom(UnaryConstraint);

/**
 * Edits indicate that a variable is to be changed by imperative code.
 */
EditConstraint.prototype.isInput = function () {
  return true;
};

EditConstraint.prototype.execute = function () {
  // Edit constraints do nothing
};

/* --- *
 * B i n a r y   C o n s t r a i n t
 * --- */

var Direction = new Object();
Direction.NONE     = 0;
Direction.FORWARD  = 1;
Direction.BACKWARD = -1;

/**
 * Abstract superclass for constraints having two possible output
 * variables.
 */
function BinaryConstraint(var1, var2, strength) {
  BinaryConstraint.superConstructor.call(this, strength);
  this.v1 = var1;
  this.v2 = var2;
  this.direction = Direction.NONE;
  this.addConstraint();
}

BinaryConstraint.inheritsFrom(Constraint);

/**
 * Decides if this constraint can be satisfied and which way it
 * should flow based on the relative strength of the variables related,
 * and record that decision.
 */
BinaryConstraint.prototype.chooseMethod = function (mark) {
  if (this.v1.mark == mark) {
    this.direction = (this.v2.mark != mark && Strength.stronger(this.strength, this.v2.walkStrength))
      ? Direction.FORWARD
      : Direction.NONE;
  }
  if (this.v2.mark == mark) {
    this.direction = (this.v1.mark != mark && Strength.stronger(this.strength, this.v1.walkStrength))
      ? Direction.BACKWARD
      : Direction.NONE;
  }
  if (Strength.weaker(this.v1.walkStrength, this.v2.walkStrength)) {
    this.direction = Strength.stronger(this.strength, this.v1.walkStrength)
      ? Direction.BACKWARD
      : Direction.NONE;
  } else {
    this.direction = Strength.stronger(this.strength, this.v2.walkStrength)
      ? Direction.FORWARD
      : Direction.BACKWARD
  }
};

/**
 * Add this constraint to the constraint graph
 */
BinaryConstraint.prototype.addToGraph = function () {
  this.v1.addConstraint(this);
  this.v2.addConstraint(this);
  this.direction = Direction.NONE;
};

/**
 * Answer true if this constraint is satisfied in the current solution.
 */
BinaryConstraint.prototype.isSatisfied = function () {
  return this.direction != Direction.NONE;
};

/**
 * Mark the input variable with the given mark.
 */
BinaryConstraint.prototype.markInputs = function (mark) {
  this.input().mark = mark;
};

/**
 * Returns the current input variable
 */
BinaryConstraint.prototype.input = function () {
  return (this.direction == Direction.FORWARD) ? this.v1 : this.v2;
};

/**
 * Returns the current output variable
 */
BinaryConstraint.prototype.output = function () {
  return (this.direction == Direction.FORWARD) ? this.v2 : this.v1;
};

/**
 * Calculate the walkabout strength, the stay flag, and, if it is
 * 'stay', the value for the current output of this
 * constraint. Assume this constraint is satisfied.
 */
BinaryConstraint.prototype.recalculate = function () {
  var ihn = this.input(), out = this.output();
  out.walkStrength = Strength.weakestOf(this.strength, ihn.walkStrength);
  out.stay = ihn.stay;
  if (out.stay) this.execute();
};

/**
 * Record the fact that this constraint is unsatisfied.
 */
BinaryConstraint.prototype.markUnsatisfied = function () {
  this.direction = Direction.NONE;
};

BinaryConstraint.prototype.inputsKnown = function (mark) {
  var i = this.input();
  return i.mark == mark || i.stay || i.determinedBy == null;
};

BinaryConstraint.prototype.removeFromGraph = function () {
  if (this.v1 != null) this.v1.removeConstraint(this);
  if (this.v2 != null) this.v2.removeConstraint(this);
  this.direction = Direction.NONE;
};

/* --- *
 * S c a l e   C o n s t r a i n t
 * --- */

/**
 * Relates two variables by the linear scaling relationship: "v2 =
 * (v1 * scale) + offset". Either v1 or v2 may be changed to maintain
 * this relationship but the scale factor and offset are considered
 * read-only.
 */
function ScaleConstraint(src, scale, offset, dest, strength) {
  this.direction = Direction.NONE;
  this.scale = scale;
  this.offset = offset;
  ScaleConstraint.superConstructor.call(this, src, dest, strength);
};

ScaleConstraint.inheritsFrom(BinaryConstraint);

/**
 * Adds this constraint to the constraint graph.
 */
ScaleConstraint.prototype.addToGraph = function () {
  ScaleConstraint.superConstructor.prototype.addToGraph.call(this);
  this.scale.addConstraint(this);
  this.offset.addConstraint(this);
};

ScaleConstraint.prototype.removeFromGraph = function () {
  ScaleConstraint.superConstructor.prototype.removeFromGraph.call(this);
  if (this.scale != null) this.scale.removeConstraint(this);
  if (this.offset != null) this.offset.removeConstraint(this);
};

ScaleConstraint.prototype.markInputs = function (mark) {
  ScaleConstraint.superConstructor.prototype.markInputs.call(this, mark);
  this.scale.mark = this.offset.mark = mark;
};

/**
 * Enforce this constraint. Assume that it is satisfied.
 */
ScaleConstraint.prototype.execute = function () {
  if (this.direction == Direction.FORWARD) {
    this.v2.value = this.v1.value * this.scale.value + this.offset.value;
  } else {
    this.v1.value = (this.v2.value - this.offset.value) / this.scale.value;
  }
}

/**
 * Calculate the walkabout strength, the stay flag, and, if it is
 * 'stay', the value for the current output of this constraint. Assume
 * this constraint is satisfied.
 */
ScaleConstraint.prototype.recalculate = function () {
  var ihn = this.input(), out = this.output();
  out.walkStrength = Strength.weakestOf(this.strength, ihn.walkStrength);
  out.stay = ihn.stay && this.scale.stay && this.offset.stay;
  if (out.stay) this.execute();
}

/* --- *
 * E q u a l i t  y   C o n s t r a i n t
 * --- */

/**
 * Constrains two variables to have the same value.
 */
function EqualityConstraint(var1, var2, strength) {
  EqualityConstraint.superConstructor.call(this, var1, var2, strength);
}

EqualityConstraint.inheritsFrom(BinaryConstraint);

/**
 * Enforce this constraint. Assume that it is satisfied.
 */
EqualityConstraint.prototype.execute = function () {
  this.output().value = this.input().value;
}

/* --- *
 * V a r i a b l e
 * --- */

/**
 * A constrained variable. In addition to its value, it maintain the
 * structure of the constraint graph, the current dataflow graph, and
 * various parameters of interest to the DeltaBlue incremental
 * constraint solver.
 **/
function Variable(name, initialValue) {
  this.value = initialValue || 0;
  this.constraints = new OrderedCollection();
  this.determinedBy = null;
  this.mark = 0;
  this.walkStrength = Strength.WEAKEST;
  this.stay = true;
  this.name = name;
}

/**
 * Add the given constraint to the set of all constraints that refer
 * this variable.
 */
Variable.prototype.addConstraint = function (c) {
  this.constraints.add(c);
}

/**
 * Removes all traces of c from this variable.
 */
Variable.prototype.removeConstraint = function (c) {
  this.constraints.remove(c);
  if (this.determinedBy == c) this.determinedBy = null;
}

/* --- *
 * P l a n n e r
 * --- */

/**
 * The DeltaBlue planner
 */
function Planner() {
  this.currentMark = 0;
}

/**
 * Attempt to satisfy the given constraint and, if successful,
 * incrementally update the dataflow graph.  Details: If satifying
 * the constraint is successful, it may override a weaker constraint
 * on its output. The algorithm attempts to resatisfy that
 * constraint using some other method. This process is repeated
 * until either a) it reaches a variable that was not previously
 * determined by any constraint or b) it reaches a constraint that
 * is too weak to be satisfied using any of its methods. The
 * variables of constraints that have been processed are marked with
 * a unique mark value so that we know where we've been. This allows
 * the algorithm to avoid getting into an infinite loop even if the
 * constraint graph has an inadvertent cycle.
 */
Planner.prototype.incrementalAdd = function (c) {
  var mark = this.newMark();
  var overridden = c.satisfy(mark);
  while (overridden != null)
    overridden = overridden.satisfy(mark);
}

/**
 * Entry point for retracting a constraint. Remove the given
 * constraint and incrementally update the dataflow graph.
 * Details: Retracting the given constraint may allow some currently
 * unsatisfiable downstream constraint to be satisfied. We therefore collect
 * a list of unsatisfied downstream constraints and attempt to
 * satisfy each one in turn. This list is traversed by constraint
 * strength, strongest first, as a heuristic for avoiding
 * unnecessarily adding and then overriding weak constraints.
 * Assume: c is satisfied.
 */
Planner.prototype.incrementalRemove = function (c) {
  var out = c.output();
  c.markUnsatisfied();
  c.removeFromGraph();
  var unsatisfied = this.removePropagateFrom(out);
  var strength = Strength.REQUIRED;
  do {
    for (var i = 0; i < unsatisfied.size(); i++) {
      var u = unsatisfied.at(i);
      if (u.strength == strength)
        this.incrementalAdd(u);
    }
    strength = strength.nextWeaker();
  } while (strength != Strength.WEAKEST);
}

/**
 * Select a previously unused mark value.
 */
Planner.prototype.newMark = function () {
  return ++this.currentMark;
}

/**
 * Extract a plan for resatisfaction starting from the given source
 * constraints, usually a set of input constraints. This method
 * assumes that stay optimization is desired; the plan will contain
 * only constraints whose output variables are not stay. Constraints
 * that do no computation, such as stay and edit constraints, are
 * not included in the plan.
 * Details: The outputs of a constraint are marked when it is added
 * to the plan under construction. A constraint may be appended to
 * the plan when all its input variables are known. A variable is
 * known if either a) the variable is marked (indicating that has
 * been computed by a constraint appearing earlier in the plan), b)
 * the variable is 'stay' (i.e. it is a constant at plan execution
 * time), or c) the variable is not determined by any
 * constraint. The last provision is for past states of history
 * variables, which are not stay but which are also not computed by
 * any constraint.
 * Assume: sources are all satisfied.
 */
Planner.prototype.makePlan = function (sources) {
  var mark = this.newMark();
  var plan = new Plan();
  var todo = sources;
  while (todo.size() > 0) {
    var c = todo.removeFirst();
    if (c.output().mark != mark && c.inputsKnown(mark)) {
      plan.addConstraint(c);
      c.output().mark = mark;
      this.addConstraintsConsumingTo(c.output(), todo);
    }
  }
  return plan;
}

/**
 * Extract a plan for resatisfying starting from the output of the
 * given constraints, usually a set of input constraints.
 */
Planner.prototype.extractPlanFromConstraints = function (constraints) {
  var sources = new OrderedCollection();
  for (var i = 0; i < constraints.size(); i++) {
    var c = constraints.at(i);
    if (c.isInput() && c.isSatisfied())
      // not in plan already and eligible for inclusion
      sources.add(c);
  }
  return this.makePlan(sources);
}

/**
 * Recompute the walkabout strengths and stay flags of all variables
 * downstream of the given constraint and recompute the actual
 * values of all variables whose stay flag is true. If a cycle is
 * detected, remove the given constraint and answer
 * false. Otherwise, answer true.
 * Details: Cycles are detected when a marked variable is
 * encountered downstream of the given constraint. The sender is
 * assumed to have marked the inputs of the given constraint with
 * the given mark. Thus, encountering a marked node downstream of
 * the output constraint means that there is a path from the
 * constraint's output to one of its inputs.
 */
Planner.prototype.addPropagate = function (c, mark) {
  var todo = new OrderedCollection();
  todo.add(c);
  while (todo.size() > 0) {
    var d = todo.removeFirst();
    if (d.output().mark == mark) {
      this.incrementalRemove(c);
      return false;
    }
    d.recalculate();
    this.addConstraintsConsumingTo(d.output(), todo);
  }
  return true;
}


/**
 * Update the walkabout strengths and stay flags of all variables
 * downstream of the given constraint. Answer a collection of
 * unsatisfied constraints sorted in order of decreasing strength.
 */
Planner.prototype.removePropagateFrom = function (out) {
  out.determinedBy = null;
  out.walkStrength = Strength.WEAKEST;
  out.stay = true;
  var unsatisfied = new OrderedCollection();
  var todo = new OrderedCollection();
  todo.add(out);
  while (todo.size() > 0) {
    var v = todo.removeFirst();
    for (var i = 0; i < v.constraints.size(); i++) {
      var c = v.constraints.at(i);
      if (!c.isSatisfied())
        unsatisfied.add(c);
    }
    var determining = v.determinedBy;
    for (var i = 0; i < v.constraints.size(); i++) {
      var next = v.constraints.at(i);
      if (next != determining && next.isSatisfied()) {
        next.recalculate();
        todo.add(next.output());
      }
    }
  }
  return unsatisfied;
}

Planner.prototype.addConstraintsConsumingTo = function (v, coll) {
  var determining = v.determinedBy;
  var cc = v.constraints;
  for (var i = 0; i < cc.size(); i++) {
    var c = cc.at(i);
    if (c != determining && c.isSatisfied())
      coll.add(c);
  }
}

/* --- *
 * P l a n
 * --- */

/**
 * A Plan is an ordered list of constraints to be executed in sequence
 * to resatisfy all currently satisfiable constraints in the face of
 * one or more changing inputs.
 */
function Plan() {
  this.v = new OrderedCollection();
}

Plan.prototype.addConstraint = function (c) {
  this.v.add(c);
}

Plan.prototype.size = function () {
  return this.v.size();
}

Plan.prototype.constraintAt = function (index) {
  return this.v.at(index);
}

Plan.prototype.execute = function () {
  for (var i = 0; i < this.size(); i++) {
    var c = this.constraintAt(i);
    c.execute();
  }
}

/* --- *
 * M a i n
 * --- */

/**
 * This is the standard DeltaBlue benchmark. A long chain of equality
 * constraints is constructed with a stay constraint on one end. An
 * edit constraint is then added to the opposite end and the time is
 * measured for adding and removing this constraint, and extracting
 * and executing a constraint satisfaction plan. There are two cases.
 * In case 1, the added constraint is stronger than the stay
 * constraint and values must propagate down the entire length of the
 * chain. In case 2, the added constraint is weaker than the stay
 * constraint so it cannot be accomodated. The cost in this case is,
 * of course, very low. Typical situations lie somewhere between these
 * two extremes.
 */
function chainTest(n) {
  planner = new Planner();
  var prev = null, first = null, last = null;

  // Build chain of n equality constraints
  for (var i = 0; i <= n; i++) {
    var name = "v" + i;
    var v = new Variable(name);
    if (prev != null)
      new EqualityConstraint(prev, v, Strength.REQUIRED);
    if (i == 0) first = v;
    if (i == n) last = v;
    prev = v;
  }

  new StayConstraint(last, Strength.STRONG_DEFAULT);
  var edit = new EditConstraint(first, Strength.PREFERRED);
  var edits = new OrderedCollection();
  edits.add(edit);
  var plan = planner.extractPlanFromConstraints(edits);
  for (var i = 0; i < 100; i++) {
    first.value = i;
    plan.execute();
    if (last.value != i)
      alert("Chain test failed.");
  }
}

/**
 * This test constructs a two sets of variables related to each
 * other by a simple linear transformation (scale and offset). The
 * time is measured to change a variable on either side of the
 * mapping and to change the scale and offset factors.
 */
function projectionTest(n) {
  planner = new Planner();
  var scale = new Variable("scale", 10);
  var offset = new Variable("offset", 1000);
  var src = null, dst = null;

  var dests = new OrderedCollection();
  for (var i = 0; i < n; i++) {
    src = new Variable("src" + i, i);
    dst = new Variable("dst" + i, i);
    dests.add(dst);
    new StayConstraint(src, Strength.NORMAL);
    new ScaleConstraint(src, scale, offset, dst, Strength.REQUIRED);
  }

  change(src, 17);
  if (dst.value != 1170) alert("Projection 1 failed");
  change(dst, 1050);
  if (src.value != 5) alert("Projection 2 failed");
  change(scale, 5);
  for (var i = 0; i < n - 1; i++) {
    if (dests.at(i).value != i * 5 + 1000)
      alert("Projection 3 failed");
  }
  change(offset, 2000);
  for (var i = 0; i < n - 1; i++) {
    if (dests.at(i).value != i * 5 + 2000)
      alert("Projection 4 failed");
  }
}

function change(v, newValue) {
  var edit = new EditConstraint(v, Strength.PREFERRED);
  var edits = new OrderedCollection();
  edits.add(edit);
  var plan = planner.extractPlanFromConstraints(edits);
  for (var i = 0; i < 10; i++) {
    v.value = newValue;
    plan.execute();
  }
  edit.destroyConstraint();
}

// Global variable holding the current planner.
var planner = null;

function deltaBlue() {
  chainTest(100);
  projectionTest(100);
}

var DeltaBlue = new BenchmarkSuite('DeltaBlue', 66118, [
  new Benchmark('DeltaBlue', deltaBlue)
]);
/*
 * Copyright (c) 2003-2005  Tom Wu
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL TOM WU BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF
 * THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * In addition, the following condition applies:
 *
 * All redistributions must retain an intact copy of this copyright notice
 * and disclaimer.
 */


// Basic JavaScript BN library - subset useful for RSA encryption.

// Bits per digit
var dbits;
var BI_DB;
var BI_DM;
var BI_DV;

var BI_FP;
var BI_FV;
var BI_F1;
var BI_F2;

// JavaScript engine analysis
var canary = 0xdeadbeefcafe;
var j_lm = ((canary&0xffffff)==0xefcafe);

// (public) Constructor
function BigInteger(a,b,c) {
  this.array = new Array();
  if(a != null)
    if("number" == typeof a) this.fromNumber(a,b,c);
    else if(b == null && "string" != typeof a) this.fromString(a,256);
    else this.fromString(a,b);
}

// return new, unset BigInteger
function nbi() { return new BigInteger(null); }

// am: Compute w_j += (x*this_i), propagate carries,
// c is initial carry, returns final carry.
// c < 3*dvalue, x < 2*dvalue, this_i < dvalue
// We need to select the fastest one that works in this environment.

// am1: use a single mult and divide to get the high bits,
// max digit bits should be 26 because
// max internal value = 2*dvalue^2-2*dvalue (< 2^53)
function am1(i,x,w,j,c,n) {
  var this_array = this.array;
  var w_array    = w.array;
  while(--n >= 0) {
    var v = x*this_array[i++]+w_array[j]+c;
    c = Math.floor(v/0x4000000);
    w_array[j++] = v&0x3ffffff;
  }
  return c;
}

// am2 avoids a big mult-and-extract completely.
// Max digit bits should be <= 30 because we do bitwise ops
// on values up to 2*hdvalue^2-hdvalue-1 (< 2^31)
function am2(i,x,w,j,c,n) {
  var this_array = this.array;
  var w_array    = w.array;
  var xl = x&0x7fff, xh = x>>15;
  while(--n >= 0) {
    var l = this_array[i]&0x7fff;
    var h = this_array[i++]>>15;
    var m = xh*l+h*xl;
    l = xl*l+((m&0x7fff)<<15)+w_array[j]+(c&0x3fffffff);
    c = (l>>>30)+(m>>>15)+xh*h+(c>>>30);
    w_array[j++] = l&0x3fffffff;
  }
  return c;
}

// Alternately, set max digit bits to 28 since some
// browsers slow down when dealing with 32-bit numbers.
function am3(i,x,w,j,c,n) {
  var this_array = this.array;
  var w_array    = w.array;

  var xl = x&0x3fff, xh = x>>14;
  while(--n >= 0) {
    var l = this_array[i]&0x3fff;
    var h = this_array[i++]>>14;
    var m = xh*l+h*xl;
    l = xl*l+((m&0x3fff)<<14)+w_array[j]+c;
    c = (l>>28)+(m>>14)+xh*h;
    w_array[j++] = l&0xfffffff;
  }
  return c;
}

// This is tailored to VMs with 2-bit tagging. It makes sure
// that all the computations stay within the 29 bits available.
function am4(i,x,w,j,c,n) {
  var this_array = this.array;
  var w_array    = w.array;

  var xl = x&0x1fff, xh = x>>13;
  while(--n >= 0) {
    var l = this_array[i]&0x1fff;
    var h = this_array[i++]>>13;
    var m = xh*l+h*xl;
    l = xl*l+((m&0x1fff)<<13)+w_array[j]+c;
    c = (l>>26)+(m>>13)+xh*h;
    w_array[j++] = l&0x3ffffff;
  }
  return c;
}

// am3/28 is best for SM, Rhino, but am4/26 is best for v8.
// Kestrel (Opera 9.5) gets its best result with am4/26.
// IE7 does 9% better with am3/28 than with am4/26.
// Firefox (SM) gets 10% faster with am3/28 than with am4/26.

var setupEngine = function(fn, bits) {
  BigInteger.prototype.am = fn;
  dbits = bits;

  BI_DB = dbits;
  BI_DM = ((1<<dbits)-1);
  BI_DV = (1<<dbits);

  BI_FP = 52;
  BI_FV = Math.pow(2,BI_FP);
  BI_F1 = BI_FP-dbits;
  BI_F2 = 2*dbits-BI_FP;
}


// Digit conversions
var BI_RM = "0123456789abcdefghijklmnopqrstuvwxyz";
var BI_RC = new Array();
var rr,vv;
rr = "0".charCodeAt(0);
for(vv = 0; vv <= 9; ++vv) BI_RC[rr++] = vv;
rr = "a".charCodeAt(0);
for(vv = 10; vv < 36; ++vv) BI_RC[rr++] = vv;
rr = "A".charCodeAt(0);
for(vv = 10; vv < 36; ++vv) BI_RC[rr++] = vv;

function int2char(n) { return BI_RM.charAt(n); }
function intAt(s,i) {
  var c = BI_RC[s.charCodeAt(i)];
  return (c==null)?-1:c;
}

// (protected) copy this to r
function bnpCopyTo(r) {
  var this_array = this.array;
  var r_array    = r.array;

  for(var i = this.t-1; i >= 0; --i) r_array[i] = this_array[i];
  r.t = this.t;
  r.s = this.s;
}

// (protected) set from integer value x, -DV <= x < DV
function bnpFromInt(x) {
  var this_array = this.array;
  this.t = 1;
  this.s = (x<0)?-1:0;
  if(x > 0) this_array[0] = x;
  else if(x < -1) this_array[0] = x+DV;
  else this.t = 0;
}

// return bigint initialized to value
function nbv(i) { var r = nbi(); r.fromInt(i); return r; }

// (protected) set from string and radix
function bnpFromString(s,b) {
  var this_array = this.array;
  var k;
  if(b == 16) k = 4;
  else if(b == 8) k = 3;
  else if(b == 256) k = 8; // byte array
  else if(b == 2) k = 1;
  else if(b == 32) k = 5;
  else if(b == 4) k = 2;
  else { this.fromRadix(s,b); return; }
  this.t = 0;
  this.s = 0;
  var i = s.length, mi = false, sh = 0;
  while(--i >= 0) {
    var x = (k==8)?s[i]&0xff:intAt(s,i);
    if(x < 0) {
      if(s.charAt(i) == "-") mi = true;
      continue;
    }
    mi = false;
    if(sh == 0)
      this_array[this.t++] = x;
    else if(sh+k > BI_DB) {
      this_array[this.t-1] |= (x&((1<<(BI_DB-sh))-1))<<sh;
      this_array[this.t++] = (x>>(BI_DB-sh));
    }
    else
      this_array[this.t-1] |= x<<sh;
    sh += k;
    if(sh >= BI_DB) sh -= BI_DB;
  }
  if(k == 8 && (s[0]&0x80) != 0) {
    this.s = -1;
    if(sh > 0) this_array[this.t-1] |= ((1<<(BI_DB-sh))-1)<<sh;
  }
  this.clamp();
  if(mi) BigInteger.ZERO.subTo(this,this);
}

// (protected) clamp off excess high words
function bnpClamp() {
  var this_array = this.array;
  var c = this.s&BI_DM;
  while(this.t > 0 && this_array[this.t-1] == c) --this.t;
}

// (public) return string representation in given radix
function bnToString(b) {
  var this_array = this.array;
  if(this.s < 0) return "-"+this.negate().toString(b);
  var k;
  if(b == 16) k = 4;
  else if(b == 8) k = 3;
  else if(b == 2) k = 1;
  else if(b == 32) k = 5;
  else if(b == 4) k = 2;
  else return this.toRadix(b);
  var km = (1<<k)-1, d, m = false, r = "", i = this.t;
  var p = BI_DB-(i*BI_DB)%k;
  if(i-- > 0) {
    if(p < BI_DB && (d = this_array[i]>>p) > 0) { m = true; r = int2char(d); }
    while(i >= 0) {
      if(p < k) {
        d = (this_array[i]&((1<<p)-1))<<(k-p);
        d |= this_array[--i]>>(p+=BI_DB-k);
      }
      else {
        d = (this_array[i]>>(p-=k))&km;
        if(p <= 0) { p += BI_DB; --i; }
      }
      if(d > 0) m = true;
      if(m) r += int2char(d);
    }
  }
  return m?r:"0";
}

// (public) -this
function bnNegate() { var r = nbi(); BigInteger.ZERO.subTo(this,r); return r; }

// (public) |this|
function bnAbs() { return (this.s<0)?this.negate():this; }

// (public) return + if this > a, - if this < a, 0 if equal
function bnCompareTo(a) {
  var this_array = this.array;
  var a_array = a.array;

  var r = this.s-a.s;
  if(r != 0) return r;
  var i = this.t;
  r = i-a.t;
  if(r != 0) return r;
  while(--i >= 0) if((r=this_array[i]-a_array[i]) != 0) return r;
  return 0;
}

// returns bit length of the integer x
function nbits(x) {
  var r = 1, t;
  if((t=x>>>16) != 0) { x = t; r += 16; }
  if((t=x>>8) != 0) { x = t; r += 8; }
  if((t=x>>4) != 0) { x = t; r += 4; }
  if((t=x>>2) != 0) { x = t; r += 2; }
  if((t=x>>1) != 0) { x = t; r += 1; }
  return r;
}

// (public) return the number of bits in "this"
function bnBitLength() {
  var this_array = this.array;
  if(this.t <= 0) return 0;
  return BI_DB*(this.t-1)+nbits(this_array[this.t-1]^(this.s&BI_DM));
}

// (protected) r = this << n*DB
function bnpDLShiftTo(n,r) {
  var this_array = this.array;
  var r_array = r.array;
  var i;
  for(i = this.t-1; i >= 0; --i) r_array[i+n] = this_array[i];
  for(i = n-1; i >= 0; --i) r_array[i] = 0;
  r.t = this.t+n;
  r.s = this.s;
}

// (protected) r = this >> n*DB
function bnpDRShiftTo(n,r) {
  var this_array = this.array;
  var r_array = r.array;
  for(var i = n; i < this.t; ++i) r_array[i-n] = this_array[i];
  r.t = Math.max(this.t-n,0);
  r.s = this.s;
}

// (protected) r = this << n
function bnpLShiftTo(n,r) {
  var this_array = this.array;
  var r_array = r.array;
  var bs = n%BI_DB;
  var cbs = BI_DB-bs;
  var bm = (1<<cbs)-1;
  var ds = Math.floor(n/BI_DB), c = (this.s<<bs)&BI_DM, i;
  for(i = this.t-1; i >= 0; --i) {
    r_array[i+ds+1] = (this_array[i]>>cbs)|c;
    c = (this_array[i]&bm)<<bs;
  }
  for(i = ds-1; i >= 0; --i) r_array[i] = 0;
  r_array[ds] = c;
  r.t = this.t+ds+1;
  r.s = this.s;
  r.clamp();
}

// (protected) r = this >> n
function bnpRShiftTo(n,r) {
  var this_array = this.array;
  var r_array = r.array;
  r.s = this.s;
  var ds = Math.floor(n/BI_DB);
  if(ds >= this.t) { r.t = 0; return; }
  var bs = n%BI_DB;
  var cbs = BI_DB-bs;
  var bm = (1<<bs)-1;
  r_array[0] = this_array[ds]>>bs;
  for(var i = ds+1; i < this.t; ++i) {
    r_array[i-ds-1] |= (this_array[i]&bm)<<cbs;
    r_array[i-ds] = this_array[i]>>bs;
  }
  if(bs > 0) r_array[this.t-ds-1] |= (this.s&bm)<<cbs;
  r.t = this.t-ds;
  r.clamp();
}

// (protected) r = this - a
function bnpSubTo(a,r) {
  var this_array = this.array;
  var r_array = r.array;
  var a_array = a.array;
  var i = 0, c = 0, m = Math.min(a.t,this.t);
  while(i < m) {
    c += this_array[i]-a_array[i];
    r_array[i++] = c&BI_DM;
    c >>= BI_DB;
  }
  if(a.t < this.t) {
    c -= a.s;
    while(i < this.t) {
      c += this_array[i];
      r_array[i++] = c&BI_DM;
      c >>= BI_DB;
    }
    c += this.s;
  }
  else {
    c += this.s;
    while(i < a.t) {
      c -= a_array[i];
      r_array[i++] = c&BI_DM;
      c >>= BI_DB;
    }
    c -= a.s;
  }
  r.s = (c<0)?-1:0;
  if(c < -1) r_array[i++] = BI_DV+c;
  else if(c > 0) r_array[i++] = c;
  r.t = i;
  r.clamp();
}

// (protected) r = this * a, r != this,a (HAC 14.12)
// "this" should be the larger one if appropriate.
function bnpMultiplyTo(a,r) {
  var this_array = this.array;
  var r_array = r.array;
  var x = this.abs(), y = a.abs();
  var y_array = y.array;

  var i = x.t;
  r.t = i+y.t;
  while(--i >= 0) r_array[i] = 0;
  for(i = 0; i < y.t; ++i) r_array[i+x.t] = x.am(0,y_array[i],r,i,0,x.t);
  r.s = 0;
  r.clamp();
  if(this.s != a.s) BigInteger.ZERO.subTo(r,r);
}

// (protected) r = this^2, r != this (HAC 14.16)
function bnpSquareTo(r) {
  var x = this.abs();
  var x_array = x.array;
  var r_array = r.array;

  var i = r.t = 2*x.t;
  while(--i >= 0) r_array[i] = 0;
  for(i = 0; i < x.t-1; ++i) {
    var c = x.am(i,x_array[i],r,2*i,0,1);
    if((r_array[i+x.t]+=x.am(i+1,2*x_array[i],r,2*i+1,c,x.t-i-1)) >= BI_DV) {
      r_array[i+x.t] -= BI_DV;
      r_array[i+x.t+1] = 1;
    }
  }
  if(r.t > 0) r_array[r.t-1] += x.am(i,x_array[i],r,2*i,0,1);
  r.s = 0;
  r.clamp();
}

// (protected) divide this by m, quotient and remainder to q, r (HAC 14.20)
// r != q, this != m.  q or r may be null.
function bnpDivRemTo(m,q,r) {
  var pm = m.abs();
  if(pm.t <= 0) return;
  var pt = this.abs();
  if(pt.t < pm.t) {
    if(q != null) q.fromInt(0);
    if(r != null) this.copyTo(r);
    return;
  }
  if(r == null) r = nbi();
  var y = nbi(), ts = this.s, ms = m.s;
  var pm_array = pm.array;
  var nsh = BI_DB-nbits(pm_array[pm.t-1]);	// normalize modulus
  if(nsh > 0) { pm.lShiftTo(nsh,y); pt.lShiftTo(nsh,r); }
  else { pm.copyTo(y); pt.copyTo(r); }
  var ys = y.t;

  var y_array = y.array;
  var y0 = y_array[ys-1];
  if(y0 == 0) return;
  var yt = y0*(1<<BI_F1)+((ys>1)?y_array[ys-2]>>BI_F2:0);
  var d1 = BI_FV/yt, d2 = (1<<BI_F1)/yt, e = 1<<BI_F2;
  var i = r.t, j = i-ys, t = (q==null)?nbi():q;
  y.dlShiftTo(j,t);

  var r_array = r.array;
  if(r.compareTo(t) >= 0) {
    r_array[r.t++] = 1;
    r.subTo(t,r);
  }
  BigInteger.ONE.dlShiftTo(ys,t);
  t.subTo(y,y);	// "negative" y so we can replace sub with am later
  while(y.t < ys) y_array[y.t++] = 0;
  while(--j >= 0) {
    // Estimate quotient digit
    var qd = (r_array[--i]==y0)?BI_DM:Math.floor(r_array[i]*d1+(r_array[i-1]+e)*d2);
    if((r_array[i]+=y.am(0,qd,r,j,0,ys)) < qd) {	// Try it out
      y.dlShiftTo(j,t);
      r.subTo(t,r);
      while(r_array[i] < --qd) r.subTo(t,r);
    }
  }
  if(q != null) {
    r.drShiftTo(ys,q);
    if(ts != ms) BigInteger.ZERO.subTo(q,q);
  }
  r.t = ys;
  r.clamp();
  if(nsh > 0) r.rShiftTo(nsh,r);	// Denormalize remainder
  if(ts < 0) BigInteger.ZERO.subTo(r,r);
}

// (public) this mod a
function bnMod(a) {
  var r = nbi();
  this.abs().divRemTo(a,null,r);
  if(this.s < 0 && r.compareTo(BigInteger.ZERO) > 0) a.subTo(r,r);
  return r;
}

// Modular reduction using "classic" algorithm
function Classic(m) { this.m = m; }
function cConvert(x) {
  if(x.s < 0 || x.compareTo(this.m) >= 0) return x.mod(this.m);
  else return x;
}
function cRevert(x) { return x; }
function cReduce(x) { x.divRemTo(this.m,null,x); }
function cMulTo(x,y,r) { x.multiplyTo(y,r); this.reduce(r); }
function cSqrTo(x,r) { x.squareTo(r); this.reduce(r); }

Classic.prototype.convert = cConvert;
Classic.prototype.revert = cRevert;
Classic.prototype.reduce = cReduce;
Classic.prototype.mulTo = cMulTo;
Classic.prototype.sqrTo = cSqrTo;

// (protected) return "-1/this % 2^DB"; useful for Mont. reduction
// justification:
//         xy == 1 (mod m)
//         xy =  1+km
//   xy(2-xy) = (1+km)(1-km)
// x[y(2-xy)] = 1-k^2m^2
// x[y(2-xy)] == 1 (mod m^2)
// if y is 1/x mod m, then y(2-xy) is 1/x mod m^2
// should reduce x and y(2-xy) by m^2 at each step to keep size bounded.
// JS multiply "overflows" differently from C/C++, so care is needed here.
function bnpInvDigit() {
  var this_array = this.array;
  if(this.t < 1) return 0;
  var x = this_array[0];
  if((x&1) == 0) return 0;
  var y = x&3;		// y == 1/x mod 2^2
  y = (y*(2-(x&0xf)*y))&0xf;	// y == 1/x mod 2^4
  y = (y*(2-(x&0xff)*y))&0xff;	// y == 1/x mod 2^8
  y = (y*(2-(((x&0xffff)*y)&0xffff)))&0xffff;	// y == 1/x mod 2^16
  // last step - calculate inverse mod DV directly;
  // assumes 16 < DB <= 32 and assumes ability to handle 48-bit ints
  y = (y*(2-x*y%BI_DV))%BI_DV;		// y == 1/x mod 2^dbits
  // we really want the negative inverse, and -DV < y < DV
  return (y>0)?BI_DV-y:-y;
}

// Montgomery reduction
function Montgomery(m) {
  this.m = m;
  this.mp = m.invDigit();
  this.mpl = this.mp&0x7fff;
  this.mph = this.mp>>15;
  this.um = (1<<(BI_DB-15))-1;
  this.mt2 = 2*m.t;
}

// xR mod m
function montConvert(x) {
  var r = nbi();
  x.abs().dlShiftTo(this.m.t,r);
  r.divRemTo(this.m,null,r);
  if(x.s < 0 && r.compareTo(BigInteger.ZERO) > 0) this.m.subTo(r,r);
  return r;
}

// x/R mod m
function montRevert(x) {
  var r = nbi();
  x.copyTo(r);
  this.reduce(r);
  return r;
}

// x = x/R mod m (HAC 14.32)
function montReduce(x) {
  var x_array = x.array;
  while(x.t <= this.mt2)	// pad x so am has enough room later
    x_array[x.t++] = 0;
  for(var i = 0; i < this.m.t; ++i) {
    // faster way of calculating u0 = x[i]*mp mod DV
    var j = x_array[i]&0x7fff;
    var u0 = (j*this.mpl+(((j*this.mph+(x_array[i]>>15)*this.mpl)&this.um)<<15))&BI_DM;
    // use am to combine the multiply-shift-add into one call
    j = i+this.m.t;
    x_array[j] += this.m.am(0,u0,x,i,0,this.m.t);
    // propagate carry
    while(x_array[j] >= BI_DV) { x_array[j] -= BI_DV; x_array[++j]++; }
  }
  x.clamp();
  x.drShiftTo(this.m.t,x);
  if(x.compareTo(this.m) >= 0) x.subTo(this.m,x);
}

// r = "x^2/R mod m"; x != r
function montSqrTo(x,r) { x.squareTo(r); this.reduce(r); }

// r = "xy/R mod m"; x,y != r
function montMulTo(x,y,r) { x.multiplyTo(y,r); this.reduce(r); }

Montgomery.prototype.convert = montConvert;
Montgomery.prototype.revert = montRevert;
Montgomery.prototype.reduce = montReduce;
Montgomery.prototype.mulTo = montMulTo;
Montgomery.prototype.sqrTo = montSqrTo;

// (protected) true iff this is even
function bnpIsEven() {
  var this_array = this.array;
  return ((this.t>0)?(this_array[0]&1):this.s) == 0;
}

// (protected) this^e, e < 2^32, doing sqr and mul with "r" (HAC 14.79)
function bnpExp(e,z) {
  if(e > 0xffffffff || e < 1) return BigInteger.ONE;
  var r = nbi(), r2 = nbi(), g = z.convert(this), i = nbits(e)-1;
  g.copyTo(r);
  while(--i >= 0) {
    z.sqrTo(r,r2);
    if((e&(1<<i)) > 0) z.mulTo(r2,g,r);
    else { var t = r; r = r2; r2 = t; }
  }
  return z.revert(r);
}

// (public) this^e % m, 0 <= e < 2^32
function bnModPowInt(e,m) {
  var z;
  if(e < 256 || m.isEven()) z = new Classic(m); else z = new Montgomery(m);
  return this.exp(e,z);
}

// protected
BigInteger.prototype.copyTo = bnpCopyTo;
BigInteger.prototype.fromInt = bnpFromInt;
BigInteger.prototype.fromString = bnpFromString;
BigInteger.prototype.clamp = bnpClamp;
BigInteger.prototype.dlShiftTo = bnpDLShiftTo;
BigInteger.prototype.drShiftTo = bnpDRShiftTo;
BigInteger.prototype.lShiftTo = bnpLShiftTo;
BigInteger.prototype.rShiftTo = bnpRShiftTo;
BigInteger.prototype.subTo = bnpSubTo;
BigInteger.prototype.multiplyTo = bnpMultiplyTo;
BigInteger.prototype.squareTo = bnpSquareTo;
BigInteger.prototype.divRemTo = bnpDivRemTo;
BigInteger.prototype.invDigit = bnpInvDigit;
BigInteger.prototype.isEven = bnpIsEven;
BigInteger.prototype.exp = bnpExp;

// public
BigInteger.prototype.toString = bnToString;
BigInteger.prototype.negate = bnNegate;
BigInteger.prototype.abs = bnAbs;
BigInteger.prototype.compareTo = bnCompareTo;
BigInteger.prototype.bitLength = bnBitLength;
BigInteger.prototype.mod = bnMod;
BigInteger.prototype.modPowInt = bnModPowInt;

// "constants"
BigInteger.ZERO = nbv(0);
BigInteger.ONE = nbv(1);
// Copyright (c) 2005  Tom Wu
// All Rights Reserved.
// See "LICENSE" for details.

// Extended JavaScript BN functions, required for RSA private ops.

// (public)
function bnClone() { var r = nbi(); this.copyTo(r); return r; }

// (public) return value as integer
function bnIntValue() {
  var this_array = this.array;
  if(this.s < 0) {
    if(this.t == 1) return this_array[0]-BI_DV;
    else if(this.t == 0) return -1;
  }
  else if(this.t == 1) return this_array[0];
  else if(this.t == 0) return 0;
  // assumes 16 < DB < 32
  return ((this_array[1]&((1<<(32-BI_DB))-1))<<BI_DB)|this_array[0];
}

// (public) return value as byte
function bnByteValue() {
  var this_array = this.array;
  return (this.t==0)?this.s:(this_array[0]<<24)>>24;
}

// (public) return value as short (assumes DB>=16)
function bnShortValue() {
  var this_array = this.array;
  return (this.t==0)?this.s:(this_array[0]<<16)>>16;
}

// (protected) return x s.t. r^x < DV
function bnpChunkSize(r) { return Math.floor(Math.LN2*BI_DB/Math.log(r)); }

// (public) 0 if this == 0, 1 if this > 0
function bnSigNum() {
  var this_array = this.array;
  if(this.s < 0) return -1;
  else if(this.t <= 0 || (this.t == 1 && this_array[0] <= 0)) return 0;
  else return 1;
}

// (protected) convert to radix string
function bnpToRadix(b) {
  if(b == null) b = 10;
  if(this.signum() == 0 || b < 2 || b > 36) return "0";
  var cs = this.chunkSize(b);
  var a = Math.pow(b,cs);
  var d = nbv(a), y = nbi(), z = nbi(), r = "";
  this.divRemTo(d,y,z);
  while(y.signum() > 0) {
    r = (a+z.intValue()).toString(b).substr(1) + r;
    y.divRemTo(d,y,z);
  }
  return z.intValue().toString(b) + r;
}

// (protected) convert from radix string
function bnpFromRadix(s,b) {
  this.fromInt(0);
  if(b == null) b = 10;
  var cs = this.chunkSize(b);
  var d = Math.pow(b,cs), mi = false, j = 0, w = 0;
  for(var i = 0; i < s.length; ++i) {
    var x = intAt(s,i);
    if(x < 0) {
      if(s.charAt(i) == "-" && this.signum() == 0) mi = true;
      continue;
    }
    w = b*w+x;
    if(++j >= cs) {
      this.dMultiply(d);
      this.dAddOffset(w,0);
      j = 0;
      w = 0;
    }
  }
  if(j > 0) {
    this.dMultiply(Math.pow(b,j));
    this.dAddOffset(w,0);
  }
  if(mi) BigInteger.ZERO.subTo(this,this);
}

// (protected) alternate constructor
function bnpFromNumber(a,b,c) {
  if("number" == typeof b) {
    // new BigInteger(int,int,RNG)
    if(a < 2) this.fromInt(1);
    else {
      this.fromNumber(a,c);
      if(!this.testBit(a-1))	// force MSB set
        this.bitwiseTo(BigInteger.ONE.shiftLeft(a-1),op_or,this);
      if(this.isEven()) this.dAddOffset(1,0); // force odd
      while(!this.isProbablePrime(b)) {
        this.dAddOffset(2,0);
        if(this.bitLength() > a) this.subTo(BigInteger.ONE.shiftLeft(a-1),this);
      }
    }
  }
  else {
    // new BigInteger(int,RNG)
    var x = new Array(), t = a&7;
    x.length = (a>>3)+1;
    b.nextBytes(x);
    if(t > 0) x[0] &= ((1<<t)-1); else x[0] = 0;
    this.fromString(x,256);
  }
}

// (public) convert to bigendian byte array
function bnToByteArray() {
  var this_array = this.array;
  var i = this.t, r = new Array();
  r[0] = this.s;
  var p = BI_DB-(i*BI_DB)%8, d, k = 0;
  if(i-- > 0) {
    if(p < BI_DB && (d = this_array[i]>>p) != (this.s&BI_DM)>>p)
      r[k++] = d|(this.s<<(BI_DB-p));
    while(i >= 0) {
      if(p < 8) {
        d = (this_array[i]&((1<<p)-1))<<(8-p);
        d |= this_array[--i]>>(p+=BI_DB-8);
      }
      else {
        d = (this_array[i]>>(p-=8))&0xff;
        if(p <= 0) { p += BI_DB; --i; }
      }
      if((d&0x80) != 0) d |= -256;
      if(k == 0 && (this.s&0x80) != (d&0x80)) ++k;
      if(k > 0 || d != this.s) r[k++] = d;
    }
  }
  return r;
}

function bnEquals(a) { return(this.compareTo(a)==0); }
function bnMin(a) { return(this.compareTo(a)<0)?this:a; }
function bnMax(a) { return(this.compareTo(a)>0)?this:a; }

// (protected) r = this op a (bitwise)
function bnpBitwiseTo(a,op,r) {
  var this_array = this.array;
  var a_array    = a.array;
  var r_array    = r.array;
  var i, f, m = Math.min(a.t,this.t);
  for(i = 0; i < m; ++i) r_array[i] = op(this_array[i],a_array[i]);
  if(a.t < this.t) {
    f = a.s&BI_DM;
    for(i = m; i < this.t; ++i) r_array[i] = op(this_array[i],f);
    r.t = this.t;
  }
  else {
    f = this.s&BI_DM;
    for(i = m; i < a.t; ++i) r_array[i] = op(f,a_array[i]);
    r.t = a.t;
  }
  r.s = op(this.s,a.s);
  r.clamp();
}

// (public) this & a
function op_and(x,y) { return x&y; }
function bnAnd(a) { var r = nbi(); this.bitwiseTo(a,op_and,r); return r; }

// (public) this | a
function op_or(x,y) { return x|y; }
function bnOr(a) { var r = nbi(); this.bitwiseTo(a,op_or,r); return r; }

// (public) this ^ a
function op_xor(x,y) { return x^y; }
function bnXor(a) { var r = nbi(); this.bitwiseTo(a,op_xor,r); return r; }

// (public) this & ~a
function op_andnot(x,y) { return x&~y; }
function bnAndNot(a) { var r = nbi(); this.bitwiseTo(a,op_andnot,r); return r; }

// (public) ~this
function bnNot() {
  var this_array = this.array;
  var r = nbi();
  var r_array = r.array;

  for(var i = 0; i < this.t; ++i) r_array[i] = BI_DM&~this_array[i];
  r.t = this.t;
  r.s = ~this.s;
  return r;
}

// (public) this << n
function bnShiftLeft(n) {
  var r = nbi();
  if(n < 0) this.rShiftTo(-n,r); else this.lShiftTo(n,r);
  return r;
}

// (public) this >> n
function bnShiftRight(n) {
  var r = nbi();
  if(n < 0) this.lShiftTo(-n,r); else this.rShiftTo(n,r);
  return r;
}

// return index of lowest 1-bit in x, x < 2^31
function lbit(x) {
  if(x == 0) return -1;
  var r = 0;
  if((x&0xffff) == 0) { x >>= 16; r += 16; }
  if((x&0xff) == 0) { x >>= 8; r += 8; }
  if((x&0xf) == 0) { x >>= 4; r += 4; }
  if((x&3) == 0) { x >>= 2; r += 2; }
  if((x&1) == 0) ++r;
  return r;
}

// (public) returns index of lowest 1-bit (or -1 if none)
function bnGetLowestSetBit() {
  var this_array = this.array;
  for(var i = 0; i < this.t; ++i)
    if(this_array[i] != 0) return i*BI_DB+lbit(this_array[i]);
  if(this.s < 0) return this.t*BI_DB;
  return -1;
}

// return number of 1 bits in x
function cbit(x) {
  var r = 0;
  while(x != 0) { x &= x-1; ++r; }
  return r;
}

// (public) return number of set bits
function bnBitCount() {
  var r = 0, x = this.s&BI_DM;
  for(var i = 0; i < this.t; ++i) r += cbit(this_array[i]^x);
  return r;
}

// (public) true iff nth bit is set
function bnTestBit(n) {
  var this_array = this.array;
  var j = Math.floor(n/BI_DB);
  if(j >= this.t) return(this.s!=0);
  return((this_array[j]&(1<<(n%BI_DB)))!=0);
}

// (protected) this op (1<<n)
function bnpChangeBit(n,op) {
  var r = BigInteger.ONE.shiftLeft(n);
  this.bitwiseTo(r,op,r);
  return r;
}

// (public) this | (1<<n)
function bnSetBit(n) { return this.changeBit(n,op_or); }

// (public) this & ~(1<<n)
function bnClearBit(n) { return this.changeBit(n,op_andnot); }

// (public) this ^ (1<<n)
function bnFlipBit(n) { return this.changeBit(n,op_xor); }

// (protected) r = this + a
function bnpAddTo(a,r) {
  var this_array = this.array;
  var a_array = a.array;
  var r_array = r.array;
  var i = 0, c = 0, m = Math.min(a.t,this.t);
  while(i < m) {
    c += this_array[i]+a_array[i];
    r_array[i++] = c&BI_DM;
    c >>= BI_DB;
  }
  if(a.t < this.t) {
    c += a.s;
    while(i < this.t) {
      c += this_array[i];
      r_array[i++] = c&BI_DM;
      c >>= BI_DB;
    }
    c += this.s;
  }
  else {
    c += this.s;
    while(i < a.t) {
      c += a_array[i];
      r_array[i++] = c&BI_DM;
      c >>= BI_DB;
    }
    c += a.s;
  }
  r.s = (c<0)?-1:0;
  if(c > 0) r_array[i++] = c;
  else if(c < -1) r_array[i++] = BI_DV+c;
  r.t = i;
  r.clamp();
}

// (public) this + a
function bnAdd(a) { var r = nbi(); this.addTo(a,r); return r; }

// (public) this - a
function bnSubtract(a) { var r = nbi(); this.subTo(a,r); return r; }

// (public) this * a
function bnMultiply(a) { var r = nbi(); this.multiplyTo(a,r); return r; }

// (public) this / a
function bnDivide(a) { var r = nbi(); this.divRemTo(a,r,null); return r; }

// (public) this % a
function bnRemainder(a) { var r = nbi(); this.divRemTo(a,null,r); return r; }

// (public) [this/a,this%a]
function bnDivideAndRemainder(a) {
  var q = nbi(), r = nbi();
  this.divRemTo(a,q,r);
  return new Array(q,r);
}

// (protected) this *= n, this >= 0, 1 < n < DV
function bnpDMultiply(n) {
  var this_array = this.array;
  this_array[this.t] = this.am(0,n-1,this,0,0,this.t);
  ++this.t;
  this.clamp();
}

// (protected) this += n << w words, this >= 0
function bnpDAddOffset(n,w) {
  var this_array = this.array;
  while(this.t <= w) this_array[this.t++] = 0;
  this_array[w] += n;
  while(this_array[w] >= BI_DV) {
    this_array[w] -= BI_DV;
    if(++w >= this.t) this_array[this.t++] = 0;
    ++this_array[w];
  }
}

// A "null" reducer
function NullExp() {}
function nNop(x) { return x; }
function nMulTo(x,y,r) { x.multiplyTo(y,r); }
function nSqrTo(x,r) { x.squareTo(r); }

NullExp.prototype.convert = nNop;
NullExp.prototype.revert = nNop;
NullExp.prototype.mulTo = nMulTo;
NullExp.prototype.sqrTo = nSqrTo;

// (public) this^e
function bnPow(e) { return this.exp(e,new NullExp()); }

// (protected) r = lower n words of "this * a", a.t <= n
// "this" should be the larger one if appropriate.
function bnpMultiplyLowerTo(a,n,r) {
  var r_array = r.array;
  var a_array = a.array;
  var i = Math.min(this.t+a.t,n);
  r.s = 0; // assumes a,this >= 0
  r.t = i;
  while(i > 0) r_array[--i] = 0;
  var j;
  for(j = r.t-this.t; i < j; ++i) r_array[i+this.t] = this.am(0,a_array[i],r,i,0,this.t);
  for(j = Math.min(a.t,n); i < j; ++i) this.am(0,a_array[i],r,i,0,n-i);
  r.clamp();
}

// (protected) r = "this * a" without lower n words, n > 0
// "this" should be the larger one if appropriate.
function bnpMultiplyUpperTo(a,n,r) {
  var r_array = r.array;
  var a_array = a.array;
  --n;
  var i = r.t = this.t+a.t-n;
  r.s = 0; // assumes a,this >= 0
  while(--i >= 0) r_array[i] = 0;
  for(i = Math.max(n-this.t,0); i < a.t; ++i)
    r_array[this.t+i-n] = this.am(n-i,a_array[i],r,0,0,this.t+i-n);
  r.clamp();
  r.drShiftTo(1,r);
}

// Barrett modular reduction
function Barrett(m) {
  // setup Barrett
  this.r2 = nbi();
  this.q3 = nbi();
  BigInteger.ONE.dlShiftTo(2*m.t,this.r2);
  this.mu = this.r2.divide(m);
  this.m = m;
}

function barrettConvert(x) {
  if(x.s < 0 || x.t > 2*this.m.t) return x.mod(this.m);
  else if(x.compareTo(this.m) < 0) return x;
  else { var r = nbi(); x.copyTo(r); this.reduce(r); return r; }
}

function barrettRevert(x) { return x; }

// x = x mod m (HAC 14.42)
function barrettReduce(x) {
  x.drShiftTo(this.m.t-1,this.r2);
  if(x.t > this.m.t+1) { x.t = this.m.t+1; x.clamp(); }
  this.mu.multiplyUpperTo(this.r2,this.m.t+1,this.q3);
  this.m.multiplyLowerTo(this.q3,this.m.t+1,this.r2);
  while(x.compareTo(this.r2) < 0) x.dAddOffset(1,this.m.t+1);
  x.subTo(this.r2,x);
  while(x.compareTo(this.m) >= 0) x.subTo(this.m,x);
}

// r = x^2 mod m; x != r
function barrettSqrTo(x,r) { x.squareTo(r); this.reduce(r); }

// r = x*y mod m; x,y != r
function barrettMulTo(x,y,r) { x.multiplyTo(y,r); this.reduce(r); }

Barrett.prototype.convert = barrettConvert;
Barrett.prototype.revert = barrettRevert;
Barrett.prototype.reduce = barrettReduce;
Barrett.prototype.mulTo = barrettMulTo;
Barrett.prototype.sqrTo = barrettSqrTo;

// (public) this^e % m (HAC 14.85)
function bnModPow(e,m) {
  var e_array = e.array;
  var i = e.bitLength(), k, r = nbv(1), z;
  if(i <= 0) return r;
  else if(i < 18) k = 1;
  else if(i < 48) k = 3;
  else if(i < 144) k = 4;
  else if(i < 768) k = 5;
  else k = 6;
  if(i < 8)
    z = new Classic(m);
  else if(m.isEven())
    z = new Barrett(m);
  else
    z = new Montgomery(m);

  // precomputation
  var g = new Array(), n = 3, k1 = k-1, km = (1<<k)-1;
  g[1] = z.convert(this);
  if(k > 1) {
    var g2 = nbi();
    z.sqrTo(g[1],g2);
    while(n <= km) {
      g[n] = nbi();
      z.mulTo(g2,g[n-2],g[n]);
      n += 2;
    }
  }

  var j = e.t-1, w, is1 = true, r2 = nbi(), t;
  i = nbits(e_array[j])-1;
  while(j >= 0) {
    if(i >= k1) w = (e_array[j]>>(i-k1))&km;
    else {
      w = (e_array[j]&((1<<(i+1))-1))<<(k1-i);
      if(j > 0) w |= e_array[j-1]>>(BI_DB+i-k1);
    }

    n = k;
    while((w&1) == 0) { w >>= 1; --n; }
    if((i -= n) < 0) { i += BI_DB; --j; }
    if(is1) {	// ret == 1, don't bother squaring or multiplying it
      g[w].copyTo(r);
      is1 = false;
    }
    else {
      while(n > 1) { z.sqrTo(r,r2); z.sqrTo(r2,r); n -= 2; }
      if(n > 0) z.sqrTo(r,r2); else { t = r; r = r2; r2 = t; }
      z.mulTo(r2,g[w],r);
    }

    while(j >= 0 && (e_array[j]&(1<<i)) == 0) {
      z.sqrTo(r,r2); t = r; r = r2; r2 = t;
      if(--i < 0) { i = BI_DB-1; --j; }
    }
  }
  return z.revert(r);
}

// (public) gcd(this,a) (HAC 14.54)
function bnGCD(a) {
  var x = (this.s<0)?this.negate():this.clone();
  var y = (a.s<0)?a.negate():a.clone();
  if(x.compareTo(y) < 0) { var t = x; x = y; y = t; }
  var i = x.getLowestSetBit(), g = y.getLowestSetBit();
  if(g < 0) return x;
  if(i < g) g = i;
  if(g > 0) {
    x.rShiftTo(g,x);
    y.rShiftTo(g,y);
  }
  while(x.signum() > 0) {
    if((i = x.getLowestSetBit()) > 0) x.rShiftTo(i,x);
    if((i = y.getLowestSetBit()) > 0) y.rShiftTo(i,y);
    if(x.compareTo(y) >= 0) {
      x.subTo(y,x);
      x.rShiftTo(1,x);
    }
    else {
      y.subTo(x,y);
      y.rShiftTo(1,y);
    }
  }
  if(g > 0) y.lShiftTo(g,y);
  return y;
}

// (protected) this % n, n < 2^26
function bnpModInt(n) {
  var this_array = this.array;
  if(n <= 0) return 0;
  var d = BI_DV%n, r = (this.s<0)?n-1:0;
  if(this.t > 0)
    if(d == 0) r = this_array[0]%n;
    else for(var i = this.t-1; i >= 0; --i) r = (d*r+this_array[i])%n;
  return r;
}

// (public) 1/this % m (HAC 14.61)
function bnModInverse(m) {
  var ac = m.isEven();
  if((this.isEven() && ac) || m.signum() == 0) return BigInteger.ZERO;
  var u = m.clone(), v = this.clone();
  var a = nbv(1), b = nbv(0), c = nbv(0), d = nbv(1);
  while(u.signum() != 0) {
    while(u.isEven()) {
      u.rShiftTo(1,u);
      if(ac) {
        if(!a.isEven() || !b.isEven()) { a.addTo(this,a); b.subTo(m,b); }
        a.rShiftTo(1,a);
      }
      else if(!b.isEven()) b.subTo(m,b);
      b.rShiftTo(1,b);
    }
    while(v.isEven()) {
      v.rShiftTo(1,v);
      if(ac) {
        if(!c.isEven() || !d.isEven()) { c.addTo(this,c); d.subTo(m,d); }
        c.rShiftTo(1,c);
      }
      else if(!d.isEven()) d.subTo(m,d);
      d.rShiftTo(1,d);
    }
    if(u.compareTo(v) >= 0) {
      u.subTo(v,u);
      if(ac) a.subTo(c,a);
      b.subTo(d,b);
    }
    else {
      v.subTo(u,v);
      if(ac) c.subTo(a,c);
      d.subTo(b,d);
    }
  }
  if(v.compareTo(BigInteger.ONE) != 0) return BigInteger.ZERO;
  if(d.compareTo(m) >= 0) return d.subtract(m);
  if(d.signum() < 0) d.addTo(m,d); else return d;
  if(d.signum() < 0) return d.add(m); else return d;
}

var lowprimes = [2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97,101,103,107,109,113,127,131,137,139,149,151,157,163,167,173,179,181,191,193,197,199,211,223,227,229,233,239,241,251,257,263,269,271,277,281,283,293,307,311,313,317,331,337,347,349,353,359,367,373,379,383,389,397,401,409,419,421,431,433,439,443,449,457,461,463,467,479,487,491,499,503,509];
var lplim = (1<<26)/lowprimes[lowprimes.length-1];

// (public) test primality with certainty >= 1-.5^t
function bnIsProbablePrime(t) {
  var i, x = this.abs();
  var x_array = x.array;
  if(x.t == 1 && x_array[0] <= lowprimes[lowprimes.length-1]) {
    for(i = 0; i < lowprimes.length; ++i)
      if(x_array[0] == lowprimes[i]) return true;
    return false;
  }
  if(x.isEven()) return false;
  i = 1;
  while(i < lowprimes.length) {
    var m = lowprimes[i], j = i+1;
    while(j < lowprimes.length && m < lplim) m *= lowprimes[j++];
    m = x.modInt(m);
    while(i < j) if(m%lowprimes[i++] == 0) return false;
  }
  return x.millerRabin(t);
}

// (protected) true if probably prime (HAC 4.24, Miller-Rabin)
function bnpMillerRabin(t) {
  var n1 = this.subtract(BigInteger.ONE);
  var k = n1.getLowestSetBit();
  if(k <= 0) return false;
  var r = n1.shiftRight(k);
  t = (t+1)>>1;
  if(t > lowprimes.length) t = lowprimes.length;
  var a = nbi();
  for(var i = 0; i < t; ++i) {
    a.fromInt(lowprimes[i]);
    var y = a.modPow(r,this);
    if(y.compareTo(BigInteger.ONE) != 0 && y.compareTo(n1) != 0) {
      var j = 1;
      while(j++ < k && y.compareTo(n1) != 0) {
        y = y.modPowInt(2,this);
        if(y.compareTo(BigInteger.ONE) == 0) return false;
      }
      if(y.compareTo(n1) != 0) return false;
    }
  }
  return true;
}

// protected
BigInteger.prototype.chunkSize = bnpChunkSize;
BigInteger.prototype.toRadix = bnpToRadix;
BigInteger.prototype.fromRadix = bnpFromRadix;
BigInteger.prototype.fromNumber = bnpFromNumber;
BigInteger.prototype.bitwiseTo = bnpBitwiseTo;
BigInteger.prototype.changeBit = bnpChangeBit;
BigInteger.prototype.addTo = bnpAddTo;
BigInteger.prototype.dMultiply = bnpDMultiply;
BigInteger.prototype.dAddOffset = bnpDAddOffset;
BigInteger.prototype.multiplyLowerTo = bnpMultiplyLowerTo;
BigInteger.prototype.multiplyUpperTo = bnpMultiplyUpperTo;
BigInteger.prototype.modInt = bnpModInt;
BigInteger.prototype.millerRabin = bnpMillerRabin;

// public
BigInteger.prototype.clone = bnClone;
BigInteger.prototype.intValue = bnIntValue;
BigInteger.prototype.byteValue = bnByteValue;
BigInteger.prototype.shortValue = bnShortValue;
BigInteger.prototype.signum = bnSigNum;
BigInteger.prototype.toByteArray = bnToByteArray;
BigInteger.prototype.equals = bnEquals;
BigInteger.prototype.min = bnMin;
BigInteger.prototype.max = bnMax;
BigInteger.prototype.and = bnAnd;
BigInteger.prototype.or = bnOr;
BigInteger.prototype.xor = bnXor;
BigInteger.prototype.andNot = bnAndNot;
BigInteger.prototype.not = bnNot;
BigInteger.prototype.shiftLeft = bnShiftLeft;
BigInteger.prototype.shiftRight = bnShiftRight;
BigInteger.prototype.getLowestSetBit = bnGetLowestSetBit;
BigInteger.prototype.bitCount = bnBitCount;
BigInteger.prototype.testBit = bnTestBit;
BigInteger.prototype.setBit = bnSetBit;
BigInteger.prototype.clearBit = bnClearBit;
BigInteger.prototype.flipBit = bnFlipBit;
BigInteger.prototype.add = bnAdd;
BigInteger.prototype.subtract = bnSubtract;
BigInteger.prototype.multiply = bnMultiply;
BigInteger.prototype.divide = bnDivide;
BigInteger.prototype.remainder = bnRemainder;
BigInteger.prototype.divideAndRemainder = bnDivideAndRemainder;
BigInteger.prototype.modPow = bnModPow;
BigInteger.prototype.modInverse = bnModInverse;
BigInteger.prototype.pow = bnPow;
BigInteger.prototype.gcd = bnGCD;
BigInteger.prototype.isProbablePrime = bnIsProbablePrime;

// BigInteger interfaces not implemented in jsbn:

// BigInteger(int signum, byte[] magnitude)
// double doubleValue()
// float floatValue()
// int hashCode()
// long longValue()
// static BigInteger valueOf(long val)
// prng4.js - uses Arcfour as a PRNG

function Arcfour() {
  this.i = 0;
  this.j = 0;
  this.S = new Array();
}

// Initialize arcfour context from key, an array of ints, each from [0..255]
function ARC4init(key) {
  var i, j, t;
  for(i = 0; i < 256; ++i)
    this.S[i] = i;
  j = 0;
  for(i = 0; i < 256; ++i) {
    j = (j + this.S[i] + key[i % key.length]) & 255;
    t = this.S[i];
    this.S[i] = this.S[j];
    this.S[j] = t;
  }
  this.i = 0;
  this.j = 0;
}

function ARC4next() {
  var t;
  this.i = (this.i + 1) & 255;
  this.j = (this.j + this.S[this.i]) & 255;
  t = this.S[this.i];
  this.S[this.i] = this.S[this.j];
  this.S[this.j] = t;
  return this.S[(t + this.S[this.i]) & 255];
}

Arcfour.prototype.init = ARC4init;
Arcfour.prototype.next = ARC4next;

// Plug in your RNG constructor here
function prng_newstate() {
  return new Arcfour();
}

// Pool size must be a multiple of 4 and greater than 32.
// An array of bytes the size of the pool will be passed to init()
var rng_psize = 256;
// Random number generator - requires a PRNG backend, e.g. prng4.js

// For best results, put code like
// <body onClick='rng_seed_time();' onKeyPress='rng_seed_time();'>
// in your main HTML document.

var rng_state;
var rng_pool;
var rng_pptr;

// Mix in a 32-bit integer into the pool
function rng_seed_int(x) {
  rng_pool[rng_pptr++] ^= x & 255;
  rng_pool[rng_pptr++] ^= (x >> 8) & 255;
  rng_pool[rng_pptr++] ^= (x >> 16) & 255;
  rng_pool[rng_pptr++] ^= (x >> 24) & 255;
  if(rng_pptr >= rng_psize) rng_pptr -= rng_psize;
}

// Mix in the current time (w/milliseconds) into the pool
function rng_seed_time() {
  // Use pre-computed date to avoid making the benchmark
  // results dependent on the current date.
  rng_seed_int(1122926989487);
}

// Initialize the pool with junk if needed.
if(rng_pool == null) {
  rng_pool = new Array();
  rng_pptr = 0;
  var t;
  while(rng_pptr < rng_psize) {  // extract some randomness from Math.random()
    t = Math.floor(65536 * Math.random());
    rng_pool[rng_pptr++] = t >>> 8;
    rng_pool[rng_pptr++] = t & 255;
  }
  rng_pptr = 0;
  rng_seed_time();
  //rng_seed_int(window.screenX);
  //rng_seed_int(window.screenY);
}

function rng_get_byte() {
  if(rng_state == null) {
    rng_seed_time();
    rng_state = prng_newstate();
    rng_state.init(rng_pool);
    for(rng_pptr = 0; rng_pptr < rng_pool.length; ++rng_pptr)
      rng_pool[rng_pptr] = 0;
    rng_pptr = 0;
    //rng_pool = null;
  }
  // TODO: allow reseeding after first request
  return rng_state.next();
}

function rng_get_bytes(ba) {
  var i;
  for(i = 0; i < ba.length; ++i) ba[i] = rng_get_byte();
}

function SecureRandom() {}

SecureRandom.prototype.nextBytes = rng_get_bytes;
// Depends on jsbn.js and rng.js

// convert a (hex) string to a bignum object
function parseBigInt(str,r) {
  return new BigInteger(str,r);
}

function linebrk(s,n) {
  var ret = "";
  var i = 0;
  while(i + n < s.length) {
    ret += s.substring(i,i+n) + "\n";
    i += n;
  }
  return ret + s.substring(i,s.length);
}

function byte2Hex(b) {
  if(b < 0x10)
    return "0" + b.toString(16);
  else
    return b.toString(16);
}

// PKCS#1 (type 2, random) pad input string s to n bytes, and return a bigint
function pkcs1pad2(s,n) {
  if(n < s.length + 11) {
    alert("Message too long for RSA");
    return null;
  }
  var ba = new Array();
  var i = s.length - 1;
  while(i >= 0 && n > 0) ba[--n] = s.charCodeAt(i--);
  ba[--n] = 0;
  var rng = new SecureRandom();
  var x = new Array();
  while(n > 2) { // random non-zero pad
    x[0] = 0;
    while(x[0] == 0) rng.nextBytes(x);
    ba[--n] = x[0];
  }
  ba[--n] = 2;
  ba[--n] = 0;
  return new BigInteger(ba);
}

// "empty" RSA key constructor
function RSAKey() {
  this.n = null;
  this.e = 0;
  this.d = null;
  this.p = null;
  this.q = null;
  this.dmp1 = null;
  this.dmq1 = null;
  this.coeff = null;
}

// Set the public key fields N and e from hex strings
function RSASetPublic(N,E) {
  if(N != null && E != null && N.length > 0 && E.length > 0) {
    this.n = parseBigInt(N,16);
    this.e = parseInt(E,16);
  }
  else
    alert("Invalid RSA public key");
}

// Perform raw public operation on "x": return x^e (mod n)
function RSADoPublic(x) {
  return x.modPowInt(this.e, this.n);
}

// Return the PKCS#1 RSA encryption of "text" as an even-length hex string
function RSAEncrypt(text) {
  var m = pkcs1pad2(text,(this.n.bitLength()+7)>>3);
  if(m == null) return null;
  var c = this.doPublic(m);
  if(c == null) return null;
  var h = c.toString(16);
  if((h.length & 1) == 0) return h; else return "0" + h;
}

// Return the PKCS#1 RSA encryption of "text" as a Base64-encoded string
//function RSAEncryptB64(text) {
//  var h = this.encrypt(text);
//  if(h) return hex2b64(h); else return null;
//}

// protected
RSAKey.prototype.doPublic = RSADoPublic;

// public
RSAKey.prototype.setPublic = RSASetPublic;
RSAKey.prototype.encrypt = RSAEncrypt;
//RSAKey.prototype.encrypt_b64 = RSAEncryptB64;
// Depends on rsa.js and jsbn2.js

// Undo PKCS#1 (type 2, random) padding and, if valid, return the plaintext
function pkcs1unpad2(d,n) {
  var b = d.toByteArray();
  var i = 0;
  while(i < b.length && b[i] == 0) ++i;
  if(b.length-i != n-1 || b[i] != 2)
    return null;
  ++i;
  while(b[i] != 0)
    if(++i >= b.length) return null;
  var ret = "";
  while(++i < b.length)
    ret += String.fromCharCode(b[i]);
  return ret;
}

// Set the private key fields N, e, and d from hex strings
function RSASetPrivate(N,E,D) {
  if(N != null && E != null && N.length > 0 && E.length > 0) {
    this.n = parseBigInt(N,16);
    this.e = parseInt(E,16);
    this.d = parseBigInt(D,16);
  }
  else
    alert("Invalid RSA private key");
}

// Set the private key fields N, e, d and CRT params from hex strings
function RSASetPrivateEx(N,E,D,P,Q,DP,DQ,C) {
  if(N != null && E != null && N.length > 0 && E.length > 0) {
    this.n = parseBigInt(N,16);
    this.e = parseInt(E,16);
    this.d = parseBigInt(D,16);
    this.p = parseBigInt(P,16);
    this.q = parseBigInt(Q,16);
    this.dmp1 = parseBigInt(DP,16);
    this.dmq1 = parseBigInt(DQ,16);
    this.coeff = parseBigInt(C,16);
  }
  else
    alert("Invalid RSA private key");
}

// Generate a new random private key B bits long, using public expt E
function RSAGenerate(B,E) {
  var rng = new SecureRandom();
  var qs = B>>1;
  this.e = parseInt(E,16);
  var ee = new BigInteger(E,16);
  for(;;) {
    for(;;) {
      this.p = new BigInteger(B-qs,1,rng);
      if(this.p.subtract(BigInteger.ONE).gcd(ee).compareTo(BigInteger.ONE) == 0 && this.p.isProbablePrime(10)) break;
    }
    for(;;) {
      this.q = new BigInteger(qs,1,rng);
      if(this.q.subtract(BigInteger.ONE).gcd(ee).compareTo(BigInteger.ONE) == 0 && this.q.isProbablePrime(10)) break;
    }
    if(this.p.compareTo(this.q) <= 0) {
      var t = this.p;
      this.p = this.q;
      this.q = t;
    }
    var p1 = this.p.subtract(BigInteger.ONE);
    var q1 = this.q.subtract(BigInteger.ONE);
    var phi = p1.multiply(q1);
    if(phi.gcd(ee).compareTo(BigInteger.ONE) == 0) {
      this.n = this.p.multiply(this.q);
      this.d = ee.modInverse(phi);
      this.dmp1 = this.d.mod(p1);
      this.dmq1 = this.d.mod(q1);
      this.coeff = this.q.modInverse(this.p);
      break;
    }
  }
}

// Perform raw private operation on "x": return x^d (mod n)
function RSADoPrivate(x) {
  if(this.p == null || this.q == null)
    return x.modPow(this.d, this.n);

  // TODO: re-calculate any missing CRT params
  var xp = x.mod(this.p).modPow(this.dmp1, this.p);
  var xq = x.mod(this.q).modPow(this.dmq1, this.q);

  while(xp.compareTo(xq) < 0)
    xp = xp.add(this.p);
  return xp.subtract(xq).multiply(this.coeff).mod(this.p).multiply(this.q).add(xq);
}

// Return the PKCS#1 RSA decryption of "ctext".
// "ctext" is an even-length hex string and the output is a plain string.
function RSADecrypt(ctext) {
  var c = parseBigInt(ctext, 16);
  var m = this.doPrivate(c);
  if(m == null) return null;
  return pkcs1unpad2(m, (this.n.bitLength()+7)>>3);
}

// Return the PKCS#1 RSA decryption of "ctext".
// "ctext" is a Base64-encoded string and the output is a plain string.
//function RSAB64Decrypt(ctext) {
//  var h = b64tohex(ctext);
//  if(h) return this.decrypt(h); else return null;
//}

// protected
RSAKey.prototype.doPrivate = RSADoPrivate;

// public
RSAKey.prototype.setPrivate = RSASetPrivate;
RSAKey.prototype.setPrivateEx = RSASetPrivateEx;
RSAKey.prototype.generate = RSAGenerate;
RSAKey.prototype.decrypt = RSADecrypt;
//RSAKey.prototype.b64_decrypt = RSAB64Decrypt;


var nValue="a5261939975948bb7a58dffe5ff54e65f0498f9175f5a09288810b8975871e99af3b5dd94057b0fc07535f5f97444504fa35169d461d0d30cf0192e307727c065168c788771c561a9400fb49175e9e6aa4e23fe11af69e9412dd23b0cb6684c4c2429bce139e848ab26d0829073351f4acd36074eafd036a5eb83359d2a698d3";
var eValue="10001";
var dValue="8e9912f6d3645894e8d38cb58c0db81ff516cf4c7e5a14c7f1eddb1459d2cded4d8d293fc97aee6aefb861859c8b6a3d1dfe710463e1f9ddc72048c09751971c4a580aa51eb523357a3cc48d31cfad1d4a165066ed92d4748fb6571211da5cb14bc11b6e2df7c1a559e6d5ac1cd5c94703a22891464fba23d0d965086277a161";
var pValue="d090ce58a92c75233a6486cb0a9209bf3583b64f540c76f5294bb97d285eed33aec220bde14b2417951178ac152ceab6da7090905b478195498b352048f15e7d";
var qValue="cab575dc652bb66df15a0359609d51d1db184750c00c6698b90ef3465c99655103edbf0d54c56aec0ce3c4d22592338092a126a0cc49f65a4a30d222b411e58f";
var dmp1Value="1a24bca8e273df2f0e47c199bbf678604e7df7215480c77c8db39f49b000ce2cf7500038acfff5433b7d582a01f1826e6f4d42e1c57f5e1fef7b12aabc59fd25";
var dmq1Value="3d06982efbbe47339e1f6d36b1216b8a741d410b0c662f54f7118b27b9a4ec9d914337eb39841d8666f3034408cf94f5b62f11c402fc994fe15a05493150d9fd";
var coeffValue="3a3e731acd8960b7ff9eb81a7ff93bd1cfa74cbd56987db58b4594fb09c09084db1734c8143f98b602b981aaa9243ca28deb69b5b280ee8dcee0fd2625e53250";

setupEngine(am3, 28);

var TEXT = "The quick brown fox jumped over the extremely lazy frog! " +
    "Now is the time for all good men to come to the party.";
var encrypted;

function encrypt() {
  var RSA = new RSAKey();
  RSA.setPublic(nValue, eValue);
  RSA.setPrivateEx(nValue, eValue, dValue, pValue, qValue, dmp1Value, dmq1Value, coeffValue);
  encrypted = RSA.encrypt(TEXT);
}

function decrypt() {
  var RSA = new RSAKey();
  RSA.setPublic(nValue, eValue);
  RSA.setPrivateEx(nValue, eValue, dValue, pValue, qValue, dmp1Value, dmq1Value, coeffValue);
  var decrypted = RSA.decrypt(encrypted);
  if (decrypted != TEXT) {
    throw new Error("Crypto operation failed");
  }
}

// The code has been adapted for use as a benchmark by Google.
var Crypto = new BenchmarkSuite('Crypto', 266181, [
  new Benchmark("Encrypt", encrypt),
  new Benchmark("Decrypt", decrypt)
]);
// The ray tracer code in this file is written by Adam Burmister. It
// is available in its original form from:
//
//   http://labs.flog.nz.co/raytracer/
//
// It has been modified slightly by Google to work as a standalone
// benchmark, but the all the computational code remains
// untouched. This file also contains a copy of parts of the Prototype
// JavaScript framework which is used by the ray tracer.

// Variable used to hold a number that can be used to verify that
// the scene was ray traced correctly.
var checkNumber;


// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

// The following is a copy of parts of the Prototype JavaScript library:

// Prototype JavaScript framework, version 1.5.0
// (c) 2005-2007 Sam Stephenson
//
// Prototype is freely distributable under the terms of an MIT-style license.
// For details, see the Prototype web site: http://prototype.conio.net/

var Class = {
  create: function() {
    return function() {
      this.initialize.apply(this, arguments);
    }
  }
};


Object.extend = function(destination, source) {
  for (var property in source) {
    destination[property] = source[property];
  }
  return destination;
};


// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

// The rest of this file is the actual ray tracer written by Adam
// Burmister. It's a concatenation of the following files:
//
//   flog/color.js
//   flog/light.js
//   flog/vector.js
//   flog/ray.js
//   flog/scene.js
//   flog/material/basematerial.js
//   flog/material/solid.js
//   flog/material/chessboard.js
//   flog/shape/baseshape.js
//   flog/shape/sphere.js
//   flog/shape/plane.js
//   flog/intersectioninfo.js
//   flog/camera.js
//   flog/background.js
//   flog/engine.js


/* Fake a Flog.* namespace */
if(typeof(Flog) == 'undefined') var Flog = {};
if(typeof(Flog.RayTracer) == 'undefined') Flog.RayTracer = {};

Flog.RayTracer.Color = Class.create();

Flog.RayTracer.Color.prototype = {
    red : 0.0,
    green : 0.0,
    blue : 0.0,

    initialize : function(r, g, b) {
        if(!r) r = 0.0;
        if(!g) g = 0.0;
        if(!b) b = 0.0;

        this.red = r;
        this.green = g;
        this.blue = b;
    },

    add : function(c1, c2){
        var result = new Flog.RayTracer.Color(0,0,0);

        result.red = c1.red + c2.red;
        result.green = c1.green + c2.green;
        result.blue = c1.blue + c2.blue;

        return result;
    },

    addScalar: function(c1, s){
        var result = new Flog.RayTracer.Color(0,0,0);

        result.red = c1.red + s;
        result.green = c1.green + s;
        result.blue = c1.blue + s;

        result.limit();

        return result;
    },

    subtract: function(c1, c2){
        var result = new Flog.RayTracer.Color(0,0,0);

        result.red = c1.red - c2.red;
        result.green = c1.green - c2.green;
        result.blue = c1.blue - c2.blue;

        return result;
    },

    multiply : function(c1, c2) {
        var result = new Flog.RayTracer.Color(0,0,0);

        result.red = c1.red * c2.red;
        result.green = c1.green * c2.green;
        result.blue = c1.blue * c2.blue;

        return result;
    },

    multiplyScalar : function(c1, f) {
        var result = new Flog.RayTracer.Color(0,0,0);

        result.red = c1.red * f;
        result.green = c1.green * f;
        result.blue = c1.blue * f;

        return result;
    },

    divideFactor : function(c1, f) {
        var result = new Flog.RayTracer.Color(0,0,0);

        result.red = c1.red / f;
        result.green = c1.green / f;
        result.blue = c1.blue / f;

        return result;
    },

    limit: function(){
        this.red = (this.red > 0.0) ? ( (this.red > 1.0) ? 1.0 : this.red ) : 0.0;
        this.green = (this.green > 0.0) ? ( (this.green > 1.0) ? 1.0 : this.green ) : 0.0;
        this.blue = (this.blue > 0.0) ? ( (this.blue > 1.0) ? 1.0 : this.blue ) : 0.0;
    },

    distance : function(color) {
        var d = Math.abs(this.red - color.red) + Math.abs(this.green - color.green) + Math.abs(this.blue - color.blue);
        return d;
    },

    blend: function(c1, c2, w){
        var result = new Flog.RayTracer.Color(0,0,0);
        result = Flog.RayTracer.Color.prototype.add(
                    Flog.RayTracer.Color.prototype.multiplyScalar(c1, 1 - w),
                    Flog.RayTracer.Color.prototype.multiplyScalar(c2, w)
                  );
        return result;
    },

    brightness : function() {
        var r = Math.floor(this.red*255);
        var g = Math.floor(this.green*255);
        var b = Math.floor(this.blue*255);
        return (r * 77 + g * 150 + b * 29) >> 8;
    },

    toString : function () {
        var r = Math.floor(this.red*255);
        var g = Math.floor(this.green*255);
        var b = Math.floor(this.blue*255);

        return "rgb("+ r +","+ g +","+ b +")";
    }
}
/* Fake a Flog.* namespace */
if(typeof(Flog) == 'undefined') var Flog = {};
if(typeof(Flog.RayTracer) == 'undefined') Flog.RayTracer = {};

Flog.RayTracer.Light = Class.create();

Flog.RayTracer.Light.prototype = {
    position: null,
    color: null,
    intensity: 10.0,

    initialize : function(pos, color, intensity) {
        this.position = pos;
        this.color = color;
        this.intensity = (intensity ? intensity : 10.0);
    },

    toString : function () {
        return 'Light [' + this.position.x + ',' + this.position.y + ',' + this.position.z + ']';
    }
}
/* Fake a Flog.* namespace */
if(typeof(Flog) == 'undefined') var Flog = {};
if(typeof(Flog.RayTracer) == 'undefined') Flog.RayTracer = {};

Flog.RayTracer.Vector = Class.create();

Flog.RayTracer.Vector.prototype = {
    x : 0.0,
    y : 0.0,
    z : 0.0,

    initialize : function(x, y, z) {
        this.x = (x ? x : 0);
        this.y = (y ? y : 0);
        this.z = (z ? z : 0);
    },

    copy: function(vector){
        this.x = vector.x;
        this.y = vector.y;
        this.z = vector.z;
    },

    normalize : function() {
        var m = this.magnitude();
        return new Flog.RayTracer.Vector(this.x / m, this.y / m, this.z / m);
    },

    magnitude : function() {
        return Math.sqrt((this.x * this.x) + (this.y * this.y) + (this.z * this.z));
    },

    cross : function(w) {
        return new Flog.RayTracer.Vector(
                                            -this.z * w.y + this.y * w.z,
                                           this.z * w.x - this.x * w.z,
                                          -this.y * w.x + this.x * w.y);
    },

    dot : function(w) {
        return this.x * w.x + this.y * w.y + this.z * w.z;
    },

    add : function(v, w) {
        return new Flog.RayTracer.Vector(w.x + v.x, w.y + v.y, w.z + v.z);
    },

    subtract : function(v, w) {
        if(!w || !v) throw 'Vectors must be defined [' + v + ',' + w + ']';
        return new Flog.RayTracer.Vector(v.x - w.x, v.y - w.y, v.z - w.z);
    },

    multiplyVector : function(v, w) {
        return new Flog.RayTracer.Vector(v.x * w.x, v.y * w.y, v.z * w.z);
    },

    multiplyScalar : function(v, w) {
        return new Flog.RayTracer.Vector(v.x * w, v.y * w, v.z * w);
    },

    toString : function () {
        return 'Vector [' + this.x + ',' + this.y + ',' + this.z + ']';
    }
}
/* Fake a Flog.* namespace */
if(typeof(Flog) == 'undefined') var Flog = {};
if(typeof(Flog.RayTracer) == 'undefined') Flog.RayTracer = {};

Flog.RayTracer.Ray = Class.create();

Flog.RayTracer.Ray.prototype = {
    position : null,
    direction : null,
    initialize : function(pos, dir) {
        this.position = pos;
        this.direction = dir;
    },

    toString : function () {
        return 'Ray [' + this.position + ',' + this.direction + ']';
    }
}
/* Fake a Flog.* namespace */
if(typeof(Flog) == 'undefined') var Flog = {};
if(typeof(Flog.RayTracer) == 'undefined') Flog.RayTracer = {};

Flog.RayTracer.Scene = Class.create();

Flog.RayTracer.Scene.prototype = {
    camera : null,
    shapes : [],
    lights : [],
    background : null,

    initialize : function() {
        this.camera = new Flog.RayTracer.Camera(
            new Flog.RayTracer.Vector(0,0,-5),
            new Flog.RayTracer.Vector(0,0,1),
            new Flog.RayTracer.Vector(0,1,0)
        );
        this.shapes = new Array();
        this.lights = new Array();
        this.background = new Flog.RayTracer.Background(new Flog.RayTracer.Color(0,0,0.5), 0.2);
    }
}
/* Fake a Flog.* namespace */
if(typeof(Flog) == 'undefined') var Flog = {};
if(typeof(Flog.RayTracer) == 'undefined') Flog.RayTracer = {};
if(typeof(Flog.RayTracer.Material) == 'undefined') Flog.RayTracer.Material = {};

Flog.RayTracer.Material.BaseMaterial = Class.create();

Flog.RayTracer.Material.BaseMaterial.prototype = {

    gloss: 2.0,             // [0...infinity] 0 = matt
    transparency: 0.0,      // 0=opaque
    reflection: 0.0,        // [0...infinity] 0 = no reflection
    refraction: 0.50,
    hasTexture: false,

    initialize : function() {

    },

    getColor: function(u, v){

    },

    wrapUp: function(t){
        t = t % 2.0;
        if(t < -1) t += 2.0;
        if(t >= 1) t -= 2.0;
        return t;
    },

    toString : function () {
        return 'Material [gloss=' + this.gloss + ', transparency=' + this.transparency + ', hasTexture=' + this.hasTexture +']';
    }
}
/* Fake a Flog.* namespace */
if(typeof(Flog) == 'undefined') var Flog = {};
if(typeof(Flog.RayTracer) == 'undefined') Flog.RayTracer = {};

Flog.RayTracer.Material.Solid = Class.create();

Flog.RayTracer.Material.Solid.prototype = Object.extend(
    new Flog.RayTracer.Material.BaseMaterial(), {
        initialize : function(color, reflection, refraction, transparency, gloss) {
            this.color = color;
            this.reflection = reflection;
            this.transparency = transparency;
            this.gloss = gloss;
            this.hasTexture = false;
        },

        getColor: function(u, v){
            return this.color;
        },

        toString : function () {
            return 'SolidMaterial [gloss=' + this.gloss + ', transparency=' + this.transparency + ', hasTexture=' + this.hasTexture +']';
        }
    }
);
/* Fake a Flog.* namespace */
if(typeof(Flog) == 'undefined') var Flog = {};
if(typeof(Flog.RayTracer) == 'undefined') Flog.RayTracer = {};

Flog.RayTracer.Material.Chessboard = Class.create();

Flog.RayTracer.Material.Chessboard.prototype = Object.extend(
    new Flog.RayTracer.Material.BaseMaterial(), {
        colorEven: null,
        colorOdd: null,
        density: 0.5,

        initialize : function(colorEven, colorOdd, reflection, transparency, gloss, density) {
            this.colorEven = colorEven;
            this.colorOdd = colorOdd;
            this.reflection = reflection;
            this.transparency = transparency;
            this.gloss = gloss;
            this.density = density;
            this.hasTexture = true;
        },

        getColor: function(u, v){
            var t = this.wrapUp(u * this.density) * this.wrapUp(v * this.density);

            if(t < 0.0)
                return this.colorEven;
            else
                return this.colorOdd;
        },

        toString : function () {
            return 'ChessMaterial [gloss=' + this.gloss + ', transparency=' + this.transparency + ', hasTexture=' + this.hasTexture +']';
        }
    }
);
/* Fake a Flog.* namespace */
if(typeof(Flog) == 'undefined') var Flog = {};
if(typeof(Flog.RayTracer) == 'undefined') Flog.RayTracer = {};
if(typeof(Flog.RayTracer.Shape) == 'undefined') Flog.RayTracer.Shape = {};

Flog.RayTracer.Shape.Sphere = Class.create();

Flog.RayTracer.Shape.Sphere.prototype = {
    initialize : function(pos, radius, material) {
        this.radius = radius;
        this.position = pos;
        this.material = material;
    },

    intersect: function(ray){
        var info = new Flog.RayTracer.IntersectionInfo();
        info.shape = this;

        var dst = Flog.RayTracer.Vector.prototype.subtract(ray.position, this.position);

        var B = dst.dot(ray.direction);
        var C = dst.dot(dst) - (this.radius * this.radius);
        var D = (B * B) - C;

        if(D > 0){ // intersection!
            info.isHit = true;
            info.distance = (-B) - Math.sqrt(D);
            info.position = Flog.RayTracer.Vector.prototype.add(
                                                ray.position,
                                                Flog.RayTracer.Vector.prototype.multiplyScalar(
                                                    ray.direction,
                                                    info.distance
                                                )
                                            );
            info.normal = Flog.RayTracer.Vector.prototype.subtract(
                                            info.position,
                                            this.position
                                        ).normalize();

            info.color = this.material.getColor(0,0);
        } else {
            info.isHit = false;
        }
        return info;
    },

    toString : function () {
        return 'Sphere [position=' + this.position + ', radius=' + this.radius + ']';
    }
}
/* Fake a Flog.* namespace */
if(typeof(Flog) == 'undefined') var Flog = {};
if(typeof(Flog.RayTracer) == 'undefined') Flog.RayTracer = {};
if(typeof(Flog.RayTracer.Shape) == 'undefined') Flog.RayTracer.Shape = {};

Flog.RayTracer.Shape.Plane = Class.create();

Flog.RayTracer.Shape.Plane.prototype = {
    d: 0.0,

    initialize : function(pos, d, material) {
        this.position = pos;
        this.d = d;
        this.material = material;
    },

    intersect: function(ray){
        var info = new Flog.RayTracer.IntersectionInfo();

        var Vd = this.position.dot(ray.direction);
        if(Vd == 0) return info; // no intersection

        var t = -(this.position.dot(ray.position) + this.d) / Vd;
        if(t <= 0) return info;

        info.shape = this;
        info.isHit = true;
        info.position = Flog.RayTracer.Vector.prototype.add(
                                            ray.position,
                                            Flog.RayTracer.Vector.prototype.multiplyScalar(
                                                ray.direction,
                                                t
                                            )
                                        );
        info.normal = this.position;
        info.distance = t;

        if(this.material.hasTexture){
            var vU = new Flog.RayTracer.Vector(this.position.y, this.position.z, -this.position.x);
            var vV = vU.cross(this.position);
            var u = info.position.dot(vU);
            var v = info.position.dot(vV);
            info.color = this.material.getColor(u,v);
        } else {
            info.color = this.material.getColor(0,0);
        }

        return info;
    },

    toString : function () {
        return 'Plane [' + this.position + ', d=' + this.d + ']';
    }
}
/* Fake a Flog.* namespace */
if(typeof(Flog) == 'undefined') var Flog = {};
if(typeof(Flog.RayTracer) == 'undefined') Flog.RayTracer = {};

Flog.RayTracer.IntersectionInfo = Class.create();

Flog.RayTracer.IntersectionInfo.prototype = {
    isHit: false,
    hitCount: 0,
    shape: null,
    position: null,
    normal: null,
    color: null,
    distance: null,

    initialize : function() {
        this.color = new Flog.RayTracer.Color(0,0,0);
    },

    toString : function () {
        return 'Intersection [' + this.position + ']';
    }
}
/* Fake a Flog.* namespace */
if(typeof(Flog) == 'undefined') var Flog = {};
if(typeof(Flog.RayTracer) == 'undefined') Flog.RayTracer = {};

Flog.RayTracer.Camera = Class.create();

Flog.RayTracer.Camera.prototype = {
    position: null,
    lookAt: null,
    equator: null,
    up: null,
    screen: null,

    initialize : function(pos, lookAt, up) {
        this.position = pos;
        this.lookAt = lookAt;
        this.up = up;
        this.equator = lookAt.normalize().cross(this.up);
        this.screen = Flog.RayTracer.Vector.prototype.add(this.position, this.lookAt);
    },

    getRay: function(vx, vy){
        var pos = Flog.RayTracer.Vector.prototype.subtract(
            this.screen,
            Flog.RayTracer.Vector.prototype.subtract(
                Flog.RayTracer.Vector.prototype.multiplyScalar(this.equator, vx),
                Flog.RayTracer.Vector.prototype.multiplyScalar(this.up, vy)
            )
        );
        pos.y = pos.y * -1;
        var dir = Flog.RayTracer.Vector.prototype.subtract(
            pos,
            this.position
        );

        var ray = new Flog.RayTracer.Ray(pos, dir.normalize());

        return ray;
    },

    toString : function () {
        return 'Ray []';
    }
}
/* Fake a Flog.* namespace */
if(typeof(Flog) == 'undefined') var Flog = {};
if(typeof(Flog.RayTracer) == 'undefined') Flog.RayTracer = {};

Flog.RayTracer.Background = Class.create();

Flog.RayTracer.Background.prototype = {
    color : null,
    ambience : 0.0,

    initialize : function(color, ambience) {
        this.color = color;
        this.ambience = ambience;
    }
}
/* Fake a Flog.* namespace */
if(typeof(Flog) == 'undefined') var Flog = {};
if(typeof(Flog.RayTracer) == 'undefined') Flog.RayTracer = {};

Flog.RayTracer.Engine = Class.create();

Flog.RayTracer.Engine.prototype = {
    canvas: null, /* 2d context we can render to */

    initialize: function(options){
        this.options = Object.extend({
                canvasHeight: 100,
                canvasWidth: 100,
                pixelWidth: 2,
                pixelHeight: 2,
                renderDiffuse: false,
                renderShadows: false,
                renderHighlights: false,
                renderReflections: false,
                rayDepth: 2
            }, options || {});

        this.options.canvasHeight /= this.options.pixelHeight;
        this.options.canvasWidth /= this.options.pixelWidth;

        /* TODO: dynamically include other scripts */
    },

    setPixel: function(x, y, color){
        var pxW, pxH;
        pxW = this.options.pixelWidth;
        pxH = this.options.pixelHeight;

        if (this.canvas) {
          this.canvas.fillStyle = color.toString();
          this.canvas.fillRect (x * pxW, y * pxH, pxW, pxH);
        } else {
          if (x ===  y) {
            checkNumber += color.brightness();
          }
          // print(x * pxW, y * pxH, pxW, pxH);
        }
    },

    renderScene: function(scene, canvas){
        checkNumber = 0;
        /* Get canvas */
        if (canvas) {
          this.canvas = canvas.getContext("2d");
        } else {
          this.canvas = null;
        }

        var canvasHeight = this.options.canvasHeight;
        var canvasWidth = this.options.canvasWidth;

        for(var y=0; y < canvasHeight; y++){
            for(var x=0; x < canvasWidth; x++){
                var yp = y * 1.0 / canvasHeight * 2 - 1;
          		var xp = x * 1.0 / canvasWidth * 2 - 1;

          		var ray = scene.camera.getRay(xp, yp);

          		var color = this.getPixelColor(ray, scene);

            	this.setPixel(x, y, color);
            }
        }
        if (checkNumber !== 2321) {
          throw new Error("Scene rendered incorrectly");
        }
    },

    getPixelColor: function(ray, scene){
        var info = this.testIntersection(ray, scene, null);
        if(info.isHit){
            var color = this.rayTrace(info, ray, scene, 0);
            return color;
        }
        return scene.background.color;
    },

    testIntersection: function(ray, scene, exclude){
        var hits = 0;
        var best = new Flog.RayTracer.IntersectionInfo();
        best.distance = 2000;

        for(var i=0; i<scene.shapes.length; i++){
            var shape = scene.shapes[i];

            if(shape != exclude){
                var info = shape.intersect(ray);
                if(info.isHit && info.distance >= 0 && info.distance < best.distance){
                    best = info;
                    hits++;
                }
            }
        }
        best.hitCount = hits;
        return best;
    },

    getReflectionRay: function(P,N,V){
        var c1 = -N.dot(V);
        var R1 = Flog.RayTracer.Vector.prototype.add(
            Flog.RayTracer.Vector.prototype.multiplyScalar(N, 2*c1),
            V
        );
        return new Flog.RayTracer.Ray(P, R1);
    },

    rayTrace: function(info, ray, scene, depth){
        // Calc ambient
        var color = Flog.RayTracer.Color.prototype.multiplyScalar(info.color, scene.background.ambience);
        var oldColor = color;
        var shininess = Math.pow(10, info.shape.material.gloss + 1);

        for(var i=0; i<scene.lights.length; i++){
            var light = scene.lights[i];

            // Calc diffuse lighting
            var v = Flog.RayTracer.Vector.prototype.subtract(
                                light.position,
                                info.position
                            ).normalize();

            if(this.options.renderDiffuse){
                var L = v.dot(info.normal);
                if(L > 0.0){
                    color = Flog.RayTracer.Color.prototype.add(
                                        color,
                                        Flog.RayTracer.Color.prototype.multiply(
                                            info.color,
                                            Flog.RayTracer.Color.prototype.multiplyScalar(
                                                light.color,
                                                L
                                            )
                                        )
                                    );
                }
            }

            // The greater the depth the more accurate the colours, but
            // this is exponentially (!) expensive
            if(depth <= this.options.rayDepth){
          // calculate reflection ray
          if(this.options.renderReflections && info.shape.material.reflection > 0)
          {
              var reflectionRay = this.getReflectionRay(info.position, info.normal, ray.direction);
              var refl = this.testIntersection(reflectionRay, scene, info.shape);

              if (refl.isHit && refl.distance > 0){
                  refl.color = this.rayTrace(refl, reflectionRay, scene, depth + 1);
              } else {
                  refl.color = scene.background.color;
                        }

                  color = Flog.RayTracer.Color.prototype.blend(
                    color,
                    refl.color,
                    info.shape.material.reflection
                  );
          }

                // Refraction
                /* TODO */
            }

            /* Render shadows and highlights */

            var shadowInfo = new Flog.RayTracer.IntersectionInfo();

            if(this.options.renderShadows){
                var shadowRay = new Flog.RayTracer.Ray(info.position, v);

                shadowInfo = this.testIntersection(shadowRay, scene, info.shape);
                if(shadowInfo.isHit && shadowInfo.shape != info.shape /*&& shadowInfo.shape.type != 'PLANE'*/){
                    var vA = Flog.RayTracer.Color.prototype.multiplyScalar(color, 0.5);
                    var dB = (0.5 * Math.pow(shadowInfo.shape.material.transparency, 0.5));
                    color = Flog.RayTracer.Color.prototype.addScalar(vA,dB);
                }
            }

      // Phong specular highlights
      if(this.options.renderHighlights && !shadowInfo.isHit && info.shape.material.gloss > 0){
        var Lv = Flog.RayTracer.Vector.prototype.subtract(
                            info.shape.position,
                            light.position
                        ).normalize();

        var E = Flog.RayTracer.Vector.prototype.subtract(
                            scene.camera.position,
                            info.shape.position
                        ).normalize();

        var H = Flog.RayTracer.Vector.prototype.subtract(
                            E,
                            Lv
                        ).normalize();

        var glossWeight = Math.pow(Math.max(info.normal.dot(H), 0), shininess);
        color = Flog.RayTracer.Color.prototype.add(
                            Flog.RayTracer.Color.prototype.multiplyScalar(light.color, glossWeight),
                            color
                        );
      }
        }
        color.limit();
        return color;
    }
};


function renderScene(){
    var scene = new Flog.RayTracer.Scene();

    scene.camera = new Flog.RayTracer.Camera(
                        new Flog.RayTracer.Vector(0, 0, -15),
                        new Flog.RayTracer.Vector(-0.2, 0, 5),
                        new Flog.RayTracer.Vector(0, 1, 0)
                    );

    scene.background = new Flog.RayTracer.Background(
                                new Flog.RayTracer.Color(0.5, 0.5, 0.5),
                                0.4
                            );

    var sphere = new Flog.RayTracer.Shape.Sphere(
        new Flog.RayTracer.Vector(-1.5, 1.5, 2),
        1.5,
        new Flog.RayTracer.Material.Solid(
            new Flog.RayTracer.Color(0,0.5,0.5),
            0.3,
            0.0,
            0.0,
            2.0
        )
    );

    var sphere1 = new Flog.RayTracer.Shape.Sphere(
        new Flog.RayTracer.Vector(1, 0.25, 1),
        0.5,
        new Flog.RayTracer.Material.Solid(
            new Flog.RayTracer.Color(0.9,0.9,0.9),
            0.1,
            0.0,
            0.0,
            1.5
        )
    );

    var plane = new Flog.RayTracer.Shape.Plane(
                                new Flog.RayTracer.Vector(0.1, 0.9, -0.5).normalize(),
                                1.2,
                                new Flog.RayTracer.Material.Chessboard(
                                    new Flog.RayTracer.Color(1,1,1),
                                    new Flog.RayTracer.Color(0,0,0),
                                    0.2,
                                    0.0,
                                    1.0,
                                    0.7
                                )
                            );

    scene.shapes.push(plane);
    scene.shapes.push(sphere);
    scene.shapes.push(sphere1);

    var light = new Flog.RayTracer.Light(
        new Flog.RayTracer.Vector(5, 10, -1),
        new Flog.RayTracer.Color(0.8, 0.8, 0.8)
    );

    var light1 = new Flog.RayTracer.Light(
        new Flog.RayTracer.Vector(-3, 5, -15),
        new Flog.RayTracer.Color(0.8, 0.8, 0.8),
        100
    );

    scene.lights.push(light);
    scene.lights.push(light1);

    var imageWidth = 100; // $F('imageWidth');
    var imageHeight = 100; // $F('imageHeight');
    var pixelSize = "5,5".split(','); //  $F('pixelSize').split(',');
    var renderDiffuse = true; // $F('renderDiffuse');
    var renderShadows = true; // $F('renderShadows');
    var renderHighlights = true; // $F('renderHighlights');
    var renderReflections = true; // $F('renderReflections');
    var rayDepth = 2;//$F('rayDepth');

    var raytracer = new Flog.RayTracer.Engine(
        {
            canvasWidth: imageWidth,
            canvasHeight: imageHeight,
            pixelWidth: pixelSize[0],
            pixelHeight: pixelSize[1],
            "renderDiffuse": renderDiffuse,
            "renderHighlights": renderHighlights,
            "renderShadows": renderShadows,
            "renderReflections": renderReflections,
            "rayDepth": rayDepth
        }
    );

    raytracer.renderScene(scene, null, 0);
}

var RayTrace = new BenchmarkSuite('RayTrace', 739989, [
  new Benchmark('RayTrace', renderScene)
]);


// This file is automatically generated by scheme2js, except for the
// benchmark harness code at the beginning and end of the file.

/************* GENERATED FILE - DO NOT EDIT *************/
/************* GENERATED FILE - DO NOT EDIT *************/
/************* GENERATED FILE - DO NOT EDIT *************/
/************* GENERATED FILE - DO NOT EDIT *************/
/************* GENERATED FILE - DO NOT EDIT *************/
/************* GENERATED FILE - DO NOT EDIT *************/
/************* GENERATED FILE - DO NOT EDIT *************/
/************* GENERATED FILE - DO NOT EDIT *************/
/*
 * To use write/prints/... the default-output port has to be set first.
 * Simply setting SC_DEFAULT_OUT and SC_ERROR_OUT to the desired values
 * should do the trick.
 * In the following example the std-out and error-port are redirected to
 * a DIV.
function initRuntime() {
    function escapeHTML(s) {
	var tmp = s;
	tmp = tmp.replace(/&/g, "&amp;");
	tmp = tmp.replace(/</g, "&lt;");
	tmp = tmp.replace(/>/g, "&gt;");
	tmp = tmp.replace(/ /g, "&nbsp;");
	tmp = tmp.replace(/\n/g, "<br />");
	tmp = tmp.replace(/\t/g, "&nbsp;&nbsp;&nbsp;&nbsp");
	return tmp;
	
    }

    document.write("<div id='stdout'></div>");
    SC_DEFAULT_OUT = new sc_GenericOutputPort(
	function(s) {
	    var stdout = document.getElementById('stdout');
	    stdout.innerHTML = stdout.innerHTML + escapeHTML(s);
	});
    SC_ERROR_OUT = SC_DEFAULT_OUT;
}
*/


function sc_print_debug() {
    sc_print.apply(null, arguments);
}
/*** META ((export *js*)) */
var sc_JS_GLOBALS = this;

var __sc_LINE=-1;
var __sc_FILE="";

/*** META ((export #t)) */
function sc_alert() {
   var len = arguments.length;
   var s = "";
   var i;

   for( i = 0; i < len; i++ ) {
       s += sc_toDisplayString(arguments[ i ]);
   }

   return alert( s );
}

/*** META ((export #t)) */
function sc_typeof( x ) {
   return typeof x;
}

/*** META ((export #t)) */
function sc_error() {
    var a = [sc_jsstring2symbol("*error*")];
    for (var i = 0; i < arguments.length; i++) {
	a[i+1] = arguments[i];
    }
    throw a;
}

/*** META ((export #t)
           (peephole (prefix "throw ")))
*/
function sc_raise(obj) {
    throw obj;
}

/*** META ((export with-handler-lambda)) */
function sc_withHandlerLambda(handler, body) {
    try {
	return body();
    } catch(e) {
	if (!e._internalException)
	    return handler(e);
	else
	    throw e;
    }
}

var sc_properties = new Object();

/*** META ((export #t)) */
function sc_putpropBang(sym, key, val) {
    var ht = sc_properties[sym];
    if (!ht) {
	ht = new Object();
	sc_properties[sym] = ht;
    }
    ht[key] = val;
}

/*** META ((export #t)) */
function sc_getprop(sym, key) {
    var ht = sc_properties[sym];
    if (ht) {
	if (key in ht)
	    return ht[key];
	else
	    return false;
    } else
	return false;
}

/*** META ((export #t)) */
function sc_rempropBang(sym, key) {
    var ht = sc_properties[sym];
    if (ht)
	delete ht[key];
}

/*** META ((export #t)) */
function sc_any2String(o) {
    return jsstring2string(sc_toDisplayString(o));
}

/*** META ((export #t)
           (peephole (infix 2 2 "==="))
           (type bool))
*/
function sc_isEqv(o1, o2) {
    return (o1 === o2);
}

/*** META ((export #t)
           (peephole (infix 2 2 "==="))
           (type bool))
*/
function sc_isEq(o1, o2) {
    return (o1 === o2);
}

/*** META ((export #t)
           (type bool))
*/
function sc_isNumber(n) {
    return (typeof n === "number");
}

/*** META ((export #t)
           (type bool))
*/
function sc_isComplex(n) {
    return sc_isNumber(n);
}

/*** META ((export #t)
           (type bool))
*/
function sc_isReal(n) {
    return sc_isNumber(n);
}

/*** META ((export #t)
           (type bool))
*/
function sc_isRational(n) {
    return sc_isReal(n);
}

/*** META ((export #t)
           (type bool))
*/
function sc_isInteger(n) {
    return (parseInt(n) === n);
}

/*** META ((export #t)
           (type bool)
           (peephole (postfix ", false")))
*/
// we don't have exact numbers...
function sc_isExact(n) {
    return false;
}

/*** META ((export #t)
           (peephole (postfix ", true"))
	   (type bool))
*/
function sc_isInexact(n) {
    return true;
}

/*** META ((export = =fx =fl)
           (type bool)
           (peephole (infix 2 2 "===")))
*/
function sc_equal(x) {
    for (var i = 1; i < arguments.length; i++)
	if (x !== arguments[i])
	    return false;
    return true;
}

/*** META ((export < <fx <fl)
           (type bool)
           (peephole (infix 2 2 "<")))
*/
function sc_less(x) {
    for (var i = 1; i < arguments.length; i++) {
	if (x >= arguments[i])
	    return false;
	x = arguments[i];
    }
    return true;
}

/*** META ((export > >fx >fl)
           (type bool)
           (peephole (infix 2 2 ">")))
*/
function sc_greater(x, y) {
    for (var i = 1; i < arguments.length; i++) {
	if (x <= arguments[i])
	    return false;
	x = arguments[i];
    }
    return true;
}

/*** META ((export <= <=fx <=fl)
           (type bool)
           (peephole (infix 2 2 "<=")))
*/
function sc_lessEqual(x, y) {
    for (var i = 1; i < arguments.length; i++) {
	if (x > arguments[i])
	    return false;
	x = arguments[i];
    }
    return true;
}

/*** META ((export >= >=fl >=fx)
           (type bool)
           (peephole (infix 2 2 ">=")))
*/
function sc_greaterEqual(x, y) {
    for (var i = 1; i < arguments.length; i++) {
	if (x < arguments[i])
	    return false;
	x = arguments[i];
    }
    return true;
}

/*** META ((export #t)
           (type bool)
           (peephole (postfix "=== 0")))
*/
function sc_isZero(x) {
    return (x === 0);
}

/*** META ((export #t)
           (type bool)
           (peephole (postfix "> 0")))
*/
function sc_isPositive(x) {
    return (x > 0);
}

/*** META ((export #t)
           (type bool)
           (peephole (postfix "< 0")))
*/
function sc_isNegative(x) {
    return (x < 0);
}

/*** META ((export #t)
           (type bool)
           (peephole (postfix "%2===1")))
*/
function sc_isOdd(x) {
    return (x % 2 === 1);
}

/*** META ((export #t)
           (type bool)
           (peephole (postfix "%2===0")))
*/
function sc_isEven(x) {
    return (x % 2 === 0);
}

/*** META ((export #t)) */
var sc_max = Math.max;
/*** META ((export #t)) */
var sc_min = Math.min;

/*** META ((export + +fx +fl)
           (peephole (infix 0 #f "+" "0")))
*/
function sc_plus() {
    var sum = 0;
    for (var i = 0; i < arguments.length; i++)
	sum += arguments[i];
    return sum;
}

/*** META ((export * *fx *fl)
           (peephole (infix 0 #f "*" "1")))
*/
function sc_multi() {
    var product = 1;
    for (var i = 0; i < arguments.length; i++)
	product *= arguments[i];
    return product;
}

/*** META ((export - -fx -fl)
           (peephole (minus)))
*/
function sc_minus(x) {
    if (arguments.length === 1)
	return -x;
    else {
	var res = x;
	for (var i = 1; i < arguments.length; i++)
	    res -= arguments[i];
	return res;
    }
}

/*** META ((export / /fl)
           (peephole (div)))
*/
function sc_div(x) {
    if (arguments.length === 1)
	return 1/x;
    else {
	var res = x;
	for (var i = 1; i < arguments.length; i++)
	    res /= arguments[i];
	return res;
    }
}

/*** META ((export #t)) */
var sc_abs = Math.abs;

/*** META ((export quotient /fx)
           (peephole (hole 2 "parseInt(" x "/" y ")")))
*/
function sc_quotient(x, y) {
    return parseInt(x / y);
}

/*** META ((export #t)
           (peephole (infix 2 2 "%")))
*/
function sc_remainder(x, y) {
    return x % y;
}

/*** META ((export #t)
           (peephole (modulo)))
*/
function sc_modulo(x, y) {
    var remainder = x % y;
    // if they don't have the same sign
    if ((remainder * y) < 0)
	return remainder + y;
    else
	return remainder;
}

function sc_euclid_gcd(a, b) {
    var temp;
    if (a === 0) return b;
    if (b === 0) return a;
    if (a < 0) {a = -a;};
    if (b < 0) {b = -b;};
    if (b > a) {temp = a; a = b; b = temp;};
    while (true) {
	a %= b;
	if(a === 0) {return b;};
	b %= a;
	if(b === 0) {return a;};
    };
    return b;
}

/*** META ((export #t)) */
function sc_gcd() {
    var gcd = 0;
    for (var i = 0; i < arguments.length; i++)
	gcd = sc_euclid_gcd(gcd, arguments[i]);
    return gcd;
}

/*** META ((export #t)) */
function sc_lcm() {
    var lcm = 1;
    for (var i = 0; i < arguments.length; i++) {
	var f = Math.round(arguments[i] / sc_euclid_gcd(arguments[i], lcm));
	lcm *= Math.abs(f);
    }
    return lcm;
}

// LIMITATION: numerator and denominator don't make sense in floating point world.
//var SC_MAX_DECIMALS = 1000000
//
// function sc_numerator(x) {
//     var rounded = Math.round(x * SC_MAX_DECIMALS);
//     return Math.round(rounded / sc_euclid_gcd(rounded, SC_MAX_DECIMALS));
// }

// function sc_denominator(x) {
//     var rounded = Math.round(x * SC_MAX_DECIMALS);
//     return Math.round(SC_MAX_DECIMALS / sc_euclid_gcd(rounded, SC_MAX_DECIMALS));
// }

/*** META ((export #t)) */
var sc_floor = Math.floor;
/*** META ((export #t)) */
var sc_ceiling = Math.ceil;
/*** META ((export #t)) */
var sc_truncate = parseInt;
/*** META ((export #t)) */
var sc_round = Math.round;

// LIMITATION: sc_rationalize doesn't make sense in a floating point world.

/*** META ((export #t)) */
var sc_exp = Math.exp;
/*** META ((export #t)) */
var sc_log = Math.log;
/*** META ((export #t)) */
var sc_sin = Math.sin;
/*** META ((export #t)) */
var sc_cos = Math.cos;
/*** META ((export #t)) */
var sc_tan = Math.tan;
/*** META ((export #t)) */
var sc_asin = Math.asin;
/*** META ((export #t)) */
var sc_acos = Math.acos;
/*** META ((export #t)) */
var sc_atan = Math.atan;

/*** META ((export #t)) */
var sc_sqrt = Math.sqrt;
/*** META ((export #t)) */
var sc_expt = Math.pow;

// LIMITATION: we don't have complex numbers.
// LIMITATION: the following functions are hence not implemented.
// LIMITATION: make-rectangular, make-polar, real-part, imag-part, magnitude, angle
// LIMITATION: 2 argument atan

/*** META ((export #t)
           (peephole (id)))
*/
function sc_exact2inexact(x) {
    return x;
}

/*** META ((export #t)
           (peephole (id)))
*/
function sc_inexact2exact(x) {
    return x;
}

function sc_number2jsstring(x, radix) {
    if (radix)
	return x.toString(radix);
    else
	return x.toString();
}

function sc_jsstring2number(s, radix) {
    if (s === "") return false;

    if (radix) {
	var t = parseInt(s, radix);
	if (!t && t !== 0) return false;
	// verify that each char is in range. (parseInt ignores leading
	// white and trailing chars)
	var allowedChars = "01234567890abcdefghijklmnopqrstuvwxyz".substring(0, radix+1);
	if ((new RegExp("^["+allowedChars+"]*$", "i")).test(s))
	    return t;
	else return false;
    } else {
	var t = +s; // does not ignore trailing chars.
	if (!t && t !== 0) return false;
	// simply verify that first char is not whitespace.
	var c = s.charAt(0);
	// if +c is 0, but the char is not "0", then we have a whitespace.
	if (+c === 0 && c !== "0") return false;
	return t;
    }
}

/*** META ((export #t)
           (type bool)
           (peephole (not)))
*/
function sc_not(b) {
    return b === false;
}

/*** META ((export #t)
           (type bool))
*/
function sc_isBoolean(b) {
    return (b === true) || (b === false);
}

function sc_Pair(car, cdr) {
    this.car = car;
    this.cdr = cdr;
}

sc_Pair.prototype.toString = function() {
    return sc_toDisplayString(this);
};
sc_Pair.prototype.sc_toWriteOrDisplayString = function(writeOrDisplay) {
    var current = this;

    var res = "(";

    while(true) {
	res += writeOrDisplay(current.car);
	if (sc_isPair(current.cdr)) {
	    res += " ";
	    current = current.cdr;
	} else if (current.cdr !== null) {
	    res += " . " + writeOrDisplay(current.cdr);
	    break;
	} else // current.cdr == null
	    break;
    }
	
    res += ")";

    return res;
};
sc_Pair.prototype.sc_toDisplayString = function() {
    return this.sc_toWriteOrDisplayString(sc_toDisplayString);
};
sc_Pair.prototype.sc_toWriteString = function() {
    return this.sc_toWriteOrDisplayString(sc_toWriteString);
};
// sc_Pair.prototype.sc_toWriteCircleString in IO.js

/*** META ((export #t)
           (type bool)
           (peephole (postfix " instanceof sc_Pair")))
*/
function sc_isPair(p) {
    return (p instanceof sc_Pair);
}

function sc_isPairEqual(p1, p2, comp) {
    return (comp(p1.car, p2.car) && comp(p1.cdr, p2.cdr));
}

/*** META ((export #t)
           (peephole (hole 2 "new sc_Pair(" car ", " cdr ")")))
*/
function sc_cons(car, cdr) {
    return new sc_Pair(car, cdr);
}

/*** META ((export cons*)) */
function sc_consStar() {
    var res = arguments[arguments.length - 1];
    for (var i = arguments.length-2; i >= 0; i--)
	res = new sc_Pair(arguments[i], res);
    return res;
}

/*** META ((export #t)
           (peephole (postfix ".car")))
*/
function sc_car(p) {
    return p.car;
}

/*** META ((export #t)
           (peephole (postfix ".cdr")))
*/
function sc_cdr(p) {
    return p.cdr;
}

/*** META ((export #t)
           (peephole (hole 2 p ".car = " val)))
*/
function sc_setCarBang(p, val) {
    p.car = val;
}

/*** META ((export #t)
           (peephole (hole 2 p ".cdr = " val)))
*/
function sc_setCdrBang(p, val) {
    p.cdr = val;
}

/*** META ((export #t)
           (peephole (postfix ".car.car")))
*/
function sc_caar(p) { return p.car.car; }
/*** META ((export #t)
           (peephole (postfix ".cdr.car")))
*/
function sc_cadr(p) { return p.cdr.car; }
/*** META ((export #t)
           (peephole (postfix ".car.cdr")))
*/
function sc_cdar(p) { return p.car.cdr; }
/*** META ((export #t)
           (peephole (postfix ".cdr.cdr")))
*/
function sc_cddr(p) { return p.cdr.cdr; }
/*** META ((export #t)
           (peephole (postfix ".car.car.car")))
*/
function sc_caaar(p) { return p.car.car.car; }
/*** META ((export #t)
           (peephole (postfix ".car.cdr.car")))
*/
function sc_cadar(p) { return p.car.cdr.car; }
/*** META ((export #t)
           (peephole (postfix ".cdr.car.car")))
*/
function sc_caadr(p) { return p.cdr.car.car; }
/*** META ((export #t)
           (peephole (postfix ".cdr.cdr.car")))
*/
function sc_caddr(p) { return p.cdr.cdr.car; }
/*** META ((export #t)
           (peephole (postfix ".car.car.cdr")))
*/
function sc_cdaar(p) { return p.car.car.cdr; }
/*** META ((export #t)
           (peephole (postfix ".cdr.car.cdr")))
*/
function sc_cdadr(p) { return p.cdr.car.cdr; }
/*** META ((export #t)
           (peephole (postfix ".car.cdr.cdr")))
*/
function sc_cddar(p) { return p.car.cdr.cdr; }
/*** META ((export #t)
           (peephole (postfix ".cdr.cdr.cdr")))
*/
function sc_cdddr(p) { return p.cdr.cdr.cdr; }
/*** META ((export #t)
           (peephole (postfix ".car.car.car.car")))
*/
function sc_caaaar(p) { return p.car.car.car.car; }
/*** META ((export #t)
           (peephole (postfix ".car.cdr.car.car")))
*/
function sc_caadar(p) { return p.car.cdr.car.car; }
/*** META ((export #t)
           (peephole (postfix ".cdr.car.car.car")))
*/
function sc_caaadr(p) { return p.cdr.car.car.car; }
/*** META ((export #t)
           (peephole (postfix ".cdr.cdr.car.car")))
*/
function sc_caaddr(p) { return p.cdr.cdr.car.car; }
/*** META ((export #t)
           (peephole (postfix ".car.car.car.cdr")))
*/
function sc_cdaaar(p) { return p.car.car.car.cdr; }
/*** META ((export #t)
           (peephole (postfix ".car.cdr.car.cdr")))
*/
function sc_cdadar(p) { return p.car.cdr.car.cdr; }
/*** META ((export #t)
           (peephole (postfix ".cdr.car.car.cdr")))
*/
function sc_cdaadr(p) { return p.cdr.car.car.cdr; }
/*** META ((export #t)
           (peephole (postfix ".cdr.cdr.car.cdr")))
*/
function sc_cdaddr(p) { return p.cdr.cdr.car.cdr; }
/*** META ((export #t)
           (peephole (postfix ".car.car.cdr.car")))
*/
function sc_cadaar(p) { return p.car.car.cdr.car; }
/*** META ((export #t)
           (peephole (postfix ".car.cdr.cdr.car")))
*/
function sc_caddar(p) { return p.car.cdr.cdr.car; }
/*** META ((export #t)
           (peephole (postfix ".cdr.car.cdr.car")))
*/
function sc_cadadr(p) { return p.cdr.car.cdr.car; }
/*** META ((export #t)
           (peephole (postfix ".cdr.cdr.cdr.car")))
*/
function sc_cadddr(p) { return p.cdr.cdr.cdr.car; }
/*** META ((export #t)
           (peephole (postfix ".car.car.cdr.cdr")))
*/
function sc_cddaar(p) { return p.car.car.cdr.cdr; }
/*** META ((export #t)
           (peephole (postfix ".car.cdr.cdr.cdr")))
*/
function sc_cdddar(p) { return p.car.cdr.cdr.cdr; }
/*** META ((export #t)
           (peephole (postfix ".cdr.car.cdr.cdr")))
*/
function sc_cddadr(p) { return p.cdr.car.cdr.cdr; }
/*** META ((export #t)
           (peephole (postfix ".cdr.cdr.cdr.cdr")))
*/
function sc_cddddr(p) { return p.cdr.cdr.cdr.cdr; }

/*** META ((export #t)) */
function sc_lastPair(l) {
    if (!sc_isPair(l)) sc_error("sc_lastPair: pair expected");
    var res = l;
    var cdr = l.cdr;
    while (sc_isPair(cdr)) {
	res = cdr;
	cdr = res.cdr;
    }
    return res;
}

/*** META ((export #t)
           (type bool)
           (peephole (postfix " === null")))
*/
function sc_isNull(o) {
    return (o === null);
}

/*** META ((export #t)
           (type bool))
*/
function sc_isList(o) {
    var rabbit;
    var turtle;

    var rabbit = o;
    var turtle = o;
    while (true) {
	if (rabbit === null ||
	    (rabbit instanceof sc_Pair && rabbit.cdr === null))
	    return true;  // end of list
	else if ((rabbit instanceof sc_Pair) &&
		 (rabbit.cdr instanceof sc_Pair)) {
	    rabbit = rabbit.cdr.cdr;
	    turtle = turtle.cdr;
	    if (rabbit === turtle) return false; // cycle
	} else
	    return false; // not pair
    }
}

/*** META ((export #t)) */
function sc_list() {
    var res = null;
    var a = arguments;
    for (var i = a.length-1; i >= 0; i--)
	res = new sc_Pair(a[i], res);
    return res;
}

/*** META ((export #t)) */
function sc_iota(num, init) {
   var res = null;
   if (!init) init = 0;
   for (var i = num - 1; i >= 0; i--)
      res = new sc_Pair(i + init, res);
   return res;
}

/*** META ((export #t)) */
function sc_makeList(nbEls, fill) {
    var res = null;
    for (var i = 0; i < nbEls; i++)
	res = new sc_Pair(fill, res);
    return res;
}

/*** META ((export #t)) */
function sc_length(l) {
    var res = 0;
    while (l !== null) {
	res++;
	l = l.cdr;
    }
    return res;
}

/*** META ((export #t)) */
function sc_remq(o, l) {
    var dummy = { cdr : null };
    var tail = dummy;
    while (l !== null) {
	if (l.car !== o) {
	    tail.cdr = sc_cons(l.car, null);
	    tail = tail.cdr;
	}
	l = l.cdr;
    }
    return dummy.cdr;
}

/*** META ((export #t)) */
function sc_remqBang(o, l) {
    var dummy = { cdr : null };
    var tail = dummy;
    var needsAssig = true;
    while (l !== null) {
	if (l.car === o) {
	    needsAssig = true;
	} else {
	    if (needsAssig) {
		tail.cdr = l;
		needsAssig = false;
	    }
	    tail = l;
	}
	l = l.cdr;
    }
    tail.cdr = null;
    return dummy.cdr;
}

/*** META ((export #t)) */
function sc_delete(o, l) {
    var dummy = { cdr : null };
    var tail = dummy;
    while (l !== null) {
	if (!sc_isEqual(l.car, o)) {
	    tail.cdr = sc_cons(l.car, null);
	    tail = tail.cdr;
	}
	l = l.cdr;
    }
    return dummy.cdr;
}

/*** META ((export #t)) */
function sc_deleteBang(o, l) {
    var dummy = { cdr : null };
    var tail = dummy;
    var needsAssig = true;
    while (l !== null) {
	if (sc_isEqual(l.car, o)) {
	    needsAssig = true;
	} else {
	    if (needsAssig) {
		tail.cdr = l;
		needsAssig = false;
	    }
	    tail = l;
	}
	l = l.cdr;
    }
    tail.cdr = null;
    return dummy.cdr;
}

function sc_reverseAppendBang(l1, l2) {
    var res = l2;
    while (l1 !== null) {
	var tmp = res;
	res = l1;
	l1 = l1.cdr;
	res.cdr = tmp;
    }
    return res;
}
	
function sc_dualAppend(l1, l2) {
    if (l1 === null) return l2;
    if (l2 === null) return l1;
    var rev = sc_reverse(l1);
    return sc_reverseAppendBang(rev, l2);
}

/*** META ((export #t)) */
function sc_append() {
    if (arguments.length === 0)
	return null;
    var res = arguments[arguments.length - 1];
    for (var i = arguments.length - 2; i >= 0; i--)
	res = sc_dualAppend(arguments[i], res);
    return res;
}

function sc_dualAppendBang(l1, l2) {
    if (l1 === null) return l2;
    if (l2 === null) return l1;
    var tmp = l1;
    while (tmp.cdr !== null) tmp=tmp.cdr;
    tmp.cdr = l2;
    return l1;
}

/*** META ((export #t)) */
function sc_appendBang() {
    var res = null;
    for (var i = 0; i < arguments.length; i++)
	res = sc_dualAppendBang(res, arguments[i]);
    return res;
}

/*** META ((export #t)) */
function sc_reverse(l1) {
    var res = null;
    while (l1 !== null) {
	res = sc_cons(l1.car, res);
	l1 = l1.cdr;
    }
    return res;
}

/*** META ((export #t)) */
function sc_reverseBang(l) {
    return sc_reverseAppendBang(l, null);
}

/*** META ((export #t)) */
function sc_listTail(l, k) {
    var res = l;
    for (var i = 0; i < k; i++) {
	res = res.cdr;
    }
    return res;
}

/*** META ((export #t)) */
function sc_listRef(l, k) {
    return sc_listTail(l, k).car;
}

/* // unoptimized generic versions
function sc_memX(o, l, comp) {
    while (l != null) {
	if (comp(l.car, o))
	    return l;
	l = l.cdr;
    }
    return false;
}
function sc_memq(o, l) { return sc_memX(o, l, sc_isEq); }
function sc_memv(o, l) { return sc_memX(o, l, sc_isEqv); }
function sc_member(o, l) { return sc_memX(o, l, sc_isEqual); }
*/

/* optimized versions */
/*** META ((export #t)) */
function sc_memq(o, l) {
    while (l !== null) {
	if (l.car === o)
	    return l;
	l = l.cdr;
    }
    return false;
}
/*** META ((export #t)) */
function sc_memv(o, l) {
    while (l !== null) {
	if (l.car === o)
	    return l;
	l = l.cdr;
    }
    return false;
}
/*** META ((export #t)) */
function sc_member(o, l) {
    while (l !== null) {
	if (sc_isEqual(l.car,o))
	    return l;
	l = l.cdr;
    }
    return false;
}

/* // generic unoptimized versions
function sc_assX(o, al, comp) {
    while (al != null) {
	if (comp(al.car.car, o))
	    return al.car;
	al = al.cdr;
    }
    return false;
}
function sc_assq(o, al) { return sc_assX(o, al, sc_isEq); }
function sc_assv(o, al) { return sc_assX(o, al, sc_isEqv); }
function sc_assoc(o, al) { return sc_assX(o, al, sc_isEqual); }
*/
// optimized versions
/*** META ((export #t)) */
function sc_assq(o, al) {
    while (al !== null) {
	if (al.car.car === o)
	    return al.car;
	al = al.cdr;
    }
    return false;
}
/*** META ((export #t)) */
function sc_assv(o, al) {
    while (al !== null) {
	if (al.car.car === o)
	    return al.car;
	al = al.cdr;
    }
    return false;
}
/*** META ((export #t)) */
function sc_assoc(o, al) {
    while (al !== null) {
	if (sc_isEqual(al.car.car, o))
	    return al.car;
	al = al.cdr;
    }
    return false;
}

/* can be used for mutable strings and characters */
function sc_isCharStringEqual(cs1, cs2) { return cs1.val === cs2.val; }
function sc_isCharStringLess(cs1, cs2) { return cs1.val < cs2.val; }
function sc_isCharStringGreater(cs1, cs2) { return cs1.val > cs2.val; }
function sc_isCharStringLessEqual(cs1, cs2) { return cs1.val <= cs2.val; }
function sc_isCharStringGreaterEqual(cs1, cs2) { return cs1.val >= cs2.val; }
function sc_isCharStringCIEqual(cs1, cs2)
    { return cs1.val.toLowerCase() === cs2.val.toLowerCase(); }
function sc_isCharStringCILess(cs1, cs2)
    { return cs1.val.toLowerCase() < cs2.val.toLowerCase(); }
function sc_isCharStringCIGreater(cs1, cs2)
    { return cs1.val.toLowerCase() > cs2.val.toLowerCase(); }
function sc_isCharStringCILessEqual(cs1, cs2)
    { return cs1.val.toLowerCase() <= cs2.val.toLowerCase(); }
function sc_isCharStringCIGreaterEqual(cs1, cs2)
    { return cs1.val.toLowerCase() >= cs2.val.toLowerCase(); }




function sc_Char(c) {
    var cached = sc_Char.lazy[c];
    if (cached)
	return cached;
    this.val = c;
    sc_Char.lazy[c] = this;
    // add return, so FF does not complain.
    return undefined;
}
sc_Char.lazy = new Object();
// thanks to Eric
sc_Char.char2readable = {
    "\u0000": "#\\null",
    "\u0007": "#\\bell",
    "\b": "#\\backspace",
    "\t": "#\\tab",
    "\n": "#\\newline",
    "\f": "#\\page",
    "\r": "#\\return",
    "\u001b": "#\\esc",
    " ": "#\\space",
    "\u007f": "#\\delete",
/* poeticless names */
    "\u0001": "#\\soh",
    "\u0002": "#\\stx",
    "\u0003": "#\\etx",
    "\u0004": "#\\eot",
    "\u0005": "#\\enq",
    "\u0006": "#\\ack",
    "\u000b": "#\\vt",
    "\u000e": "#\\so",
    "\u000f": "#\\si",
    "\u0010": "#\\dle",
    "\u0011": "#\\dc1",
    "\u0012": "#\\dc2",
    "\u0013": "#\\dc3",
    "\u0014": "#\\dc4",
    "\u0015": "#\\nak",
    "\u0016": "#\\syn",
    "\u0017": "#\\etb",
    "\u0018": "#\\can",
    "\u0019": "#\\em",
    "\u001a": "#\\sub",
    "\u001c": "#\\fs",
    "\u001d": "#\\gs",
    "\u001e": "#\\rs",
    "\u001f": "#\\us",
};

sc_Char.readable2char = {
    "null": "\u0000",
    "bell": "\u0007",
    "backspace": "\b",
    "tab": "\t",
    "newline": "\n",
    "page": "\f",
    "return": "\r",
    "escape": "\u001b",
    "space": " ",
    "delete": "\u0000",
    "soh": "\u0001",
    "stx": "\u0002",
    "etx": "\u0003",
    "eot": "\u0004",
    "enq": "\u0005",
    "ack": "\u0006",
    "bel": "\u0007",
    "bs": "\b",
    "ht": "\t",
    "nl": "\n",
    "vt": "\u000b",
    "np": "\f",
    "cr": "\r",
    "so": "\u000e",
    "si": "\u000f",
    "dle": "\u0010",
    "dc1": "\u0011",
    "dc2": "\u0012",
    "dc3": "\u0013",
    "dc4": "\u0014",
    "nak": "\u0015",
    "syn": "\u0016",
    "etb": "\u0017",
    "can": "\u0018",
    "em": "\u0019",
    "sub": "\u001a",
    "esc": "\u001b",
    "fs": "\u001c",
    "gs": "\u001d",
    "rs": "\u001e",
    "us": "\u001f",
    "sp": " ",
    "del": "\u007f",
};

sc_Char.prototype.toString = function() {
    return this.val;
};
// sc_toDisplayString == toString
sc_Char.prototype.sc_toWriteString = function() {
    var entry = sc_Char.char2readable[this.val];
    if (entry)
	return entry;
    else
	return "#\\" + this.val;
};

/*** META ((export #t)
           (type bool)
           (peephole (postfix "instanceof sc_Char")))
*/
function sc_isChar(c) {
    return (c instanceof sc_Char);
}

/*** META ((export char=?)
           (type bool)
           (peephole (hole 2 c1 ".val === " c2 ".val")))
*/
var sc_isCharEqual = sc_isCharStringEqual;
/*** META ((export char<?)
           (type bool)
           (peephole (hole 2 c1 ".val < " c2 ".val")))
*/
var sc_isCharLess = sc_isCharStringLess;
/*** META ((export char>?)
           (type bool)
           (peephole (hole 2 c1 ".val > " c2 ".val")))
*/
var sc_isCharGreater = sc_isCharStringGreater;
/*** META ((export char<=?)
           (type bool)
           (peephole (hole 2 c1 ".val <= " c2 ".val")))
*/
var sc_isCharLessEqual = sc_isCharStringLessEqual;
/*** META ((export char>=?)
           (type bool)
           (peephole (hole 2 c1 ".val >= " c2 ".val")))
*/
var sc_isCharGreaterEqual = sc_isCharStringGreaterEqual;
/*** META ((export char-ci=?)
           (type bool)
           (peephole (hole 2 c1 ".val.toLowerCase() === " c2 ".val.toLowerCase()")))
*/
var sc_isCharCIEqual = sc_isCharStringCIEqual;
/*** META ((export char-ci<?)
           (type bool)
           (peephole (hole 2 c1 ".val.toLowerCase() < " c2 ".val.toLowerCase()")))
*/
var sc_isCharCILess = sc_isCharStringCILess;
/*** META ((export char-ci>?)
           (type bool)
           (peephole (hole 2 c1 ".val.toLowerCase() > " c2 ".val.toLowerCase()")))
*/
var sc_isCharCIGreater = sc_isCharStringCIGreater;
/*** META ((export char-ci<=?)
           (type bool)
           (peephole (hole 2 c1 ".val.toLowerCase() <= " c2 ".val.toLowerCase()")))
*/
var sc_isCharCILessEqual = sc_isCharStringCILessEqual;
/*** META ((export char-ci>=?)
           (type bool)
           (peephole (hole 2 c1 ".val.toLowerCase() >= " c2 ".val.toLowerCase()")))
*/
var sc_isCharCIGreaterEqual = sc_isCharStringCIGreaterEqual;

var SC_NUMBER_CLASS = "0123456789";
var SC_WHITESPACE_CLASS = ' \r\n\t\f';
var SC_LOWER_CLASS = 'abcdefghijklmnopqrstuvwxyz';
var SC_UPPER_CLASS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';

function sc_isCharOfClass(c, cl) { return (cl.indexOf(c) != -1); }
/*** META ((export #t)
           (type bool))
*/
function sc_isCharAlphabetic(c)
    { return sc_isCharOfClass(c.val, SC_LOWER_CLASS) ||
	  sc_isCharOfClass(c.val, SC_UPPER_CLASS); }
/*** META ((export #t)
           (type bool)
           (peephole (hole 1 "SC_NUMBER_CLASS.indexOf(" c ".val) != -1")))
*/
function sc_isCharNumeric(c)
    { return sc_isCharOfClass(c.val, SC_NUMBER_CLASS); }
/*** META ((export #t)
           (type bool))
*/
function sc_isCharWhitespace(c) {
    var tmp = c.val;
    return tmp === " " || tmp === "\r" || tmp === "\n" || tmp === "\t" || tmp === "\f";
}
/*** META ((export #t)
           (type bool)
           (peephole (hole 1 "SC_UPPER_CLASS.indexOf(" c ".val) != -1")))
*/
function sc_isCharUpperCase(c)
    { return sc_isCharOfClass(c.val, SC_UPPER_CLASS); }
/*** META ((export #t)
           (type bool)
           (peephole (hole 1 "SC_LOWER_CLASS.indexOf(" c ".val) != -1")))
*/
function sc_isCharLowerCase(c)
    { return sc_isCharOfClass(c.val, SC_LOWER_CLASS); }

/*** META ((export #t)
           (peephole (postfix ".val.charCodeAt(0)")))
*/
function sc_char2integer(c)
    { return c.val.charCodeAt(0); }
/*** META ((export #t)
           (peephole (hole 1 "new sc_Char(String.fromCharCode(" n "))")))
*/
function sc_integer2char(n)
    { return new sc_Char(String.fromCharCode(n)); }

/*** META ((export #t)
           (peephole (hole 1 "new sc_Char(" c ".val.toUpperCase())")))
*/
function sc_charUpcase(c)
    { return new sc_Char(c.val.toUpperCase()); }
/*** META ((export #t)
           (peephole (hole 1 "new sc_Char(" c ".val.toLowerCase())")))
*/
function sc_charDowncase(c)
    { return new sc_Char(c.val.toLowerCase()); }

function sc_makeJSStringOfLength(k, c) {
    var fill;
    if (c === undefined)
	fill = " ";
    else
	fill = c;
    var res = "";
    var len = 1;
    // every round doubles the size of fill.
    while (k >= len) {
	if (k & len)
	    res = res.concat(fill);
	fill = fill.concat(fill);
	len *= 2;
    }
    return res;
}

function sc_makejsString(k, c) {
    var fill;
    if (c)
	fill = c.val;
    else
	fill = " ";
    return sc_makeJSStringOfLength(k, fill);
}

function sc_jsstring2list(s) {
    var res = null;
    for (var i = s.length - 1; i >= 0; i--)
	res = sc_cons(new sc_Char(s.charAt(i)), res);
    return res;
}

function sc_list2jsstring(l) {
    var a = new Array();
    while(l !== null) {
	a.push(l.car.val);
	l = l.cdr;
    }
    return "".concat.apply("", a);
}

var sc_Vector = Array;

sc_Vector.prototype.sc_toWriteOrDisplayString = function(writeOrDisplay) {
    if (this.length === 0) return "#()";

    var res = "#(" + writeOrDisplay(this[0]);
    for (var i = 1; i < this.length; i++)
	res += " " + writeOrDisplay(this[i]);
    res += ")";
    return res;
};
sc_Vector.prototype.sc_toDisplayString = function() {
    return this.sc_toWriteOrDisplayString(sc_toDisplayString);
};
sc_Vector.prototype.sc_toWriteString = function() {
    return this.sc_toWriteOrDisplayString(sc_toWriteString);
};

/*** META ((export vector? array?)
           (type bool)
           (peephole (postfix " instanceof sc_Vector")))
*/
function sc_isVector(v) {
    return (v instanceof sc_Vector);
}

// only applies to vectors
function sc_isVectorEqual(v1, v2, comp) {
    if (v1.length !== v2.length) return false;
    for (var i = 0; i < v1.length; i++)
	if (!comp(v1[i], v2[i])) return false;
    return true;
}

/*** META ((export make-vector make-array)) */
function sc_makeVector(size, fill) {
    var a = new sc_Vector(size);
    if (fill !== undefined)
	sc_vectorFillBang(a, fill);
    return a;
}

/*** META ((export vector array)
           (peephole (vector)))
*/
function sc_vector() {
    var a = new sc_Vector();
    for (var i = 0; i < arguments.length; i++)
	a.push(arguments[i]);
    return a;
}

/*** META ((export vector-length array-length)
           (peephole (postfix ".length")))
*/
function sc_vectorLength(v) {
    return v.length;
}

/*** META ((export vector-ref array-ref)
           (peephole (hole 2 v "[" pos "]")))
*/
function sc_vectorRef(v, pos) {
    return v[pos];
}

/*** META ((export vector-set! array-set!)
           (peephole (hole 3 v "[" pos "] = " val)))
*/
function sc_vectorSetBang(v, pos, val) {
    v[pos] = val;
}

/*** META ((export vector->list array->list)) */
function sc_vector2list(a) {
    var res = null;
    for (var i = a.length-1; i >= 0; i--)
	res = sc_cons(a[i], res);
    return res;
}

/*** META ((export list->vector list->array)) */
function sc_list2vector(l) {
    var a = new sc_Vector();
    while(l !== null) {
	a.push(l.car);
	l = l.cdr;
    }
    return a;
}

/*** META ((export vector-fill! array-fill!)) */
function sc_vectorFillBang(a, fill) {
    for (var i = 0; i < a.length; i++)
	a[i] = fill;
}


/*** META ((export #t)) */
function sc_copyVector(a, len) {
    if (len <= a.length)
	return a.slice(0, len);
    else {
	var tmp = a.concat();
	tmp.length = len;
	return tmp;
    }
}

/*** META ((export #t)
           (peephole (hole 3 a ".slice(" start "," end ")")))
*/
function sc_vectorCopy(a, start, end) {
    return a.slice(start, end);
}

/*** META ((export #t)) */
function sc_vectorCopyBang(target, tstart, source, sstart, send) {
    if (!sstart) sstart = 0;
    if (!send) send = source.length;

    // if target == source we don't want to overwrite not yet copied elements.
    if (tstart <= sstart) {
	for (var i = tstart, j = sstart; j < send; i++, j++) {
	    target[i] = source[j];
	}
    } else {
	var diff = send - sstart;
	for (var i = tstart + diff - 1, j = send - 1;
	     j >= sstart;
	     i--, j--) {
	    target[i] = source[j];
	}
    }
    return target;
}

/*** META ((export #t)
           (type bool)
           (peephole (hole 1 "typeof " o " === 'function'")))
*/
function sc_isProcedure(o) {
    return (typeof o === "function");
}

/*** META ((export #t)) */
function sc_apply(proc) {
    var args = new Array();
    // first part of arguments are not in list-form.
    for (var i = 1; i < arguments.length - 1; i++)
	args.push(arguments[i]);
    var l = arguments[arguments.length - 1];
    while (l !== null) {
	args.push(l.car);
	l = l.cdr;
    }
    return proc.apply(null, args);
}

/*** META ((export #t)) */
function sc_map(proc, l1) {
    if (l1 === undefined)
	return null;
    // else
    var nbApplyArgs = arguments.length - 1;
    var applyArgs = new Array(nbApplyArgs);
    var revres = null;
    while (l1 !== null) {
	for (var i = 0; i < nbApplyArgs; i++) {
	    applyArgs[i] = arguments[i + 1].car;
	    arguments[i + 1] = arguments[i + 1].cdr;
	}
	revres = sc_cons(proc.apply(null, applyArgs), revres);
    }
    return sc_reverseAppendBang(revres, null);
}

/*** META ((export #t)) */
function sc_mapBang(proc, l1) {
    if (l1 === undefined)
	return null;
    // else
    var l1_orig = l1;
    var nbApplyArgs = arguments.length - 1;
    var applyArgs = new Array(nbApplyArgs);
    while (l1 !== null) {
	var tmp = l1;
	for (var i = 0; i < nbApplyArgs; i++) {
	    applyArgs[i] = arguments[i + 1].car;
	    arguments[i + 1] = arguments[i + 1].cdr;
	}
	tmp.car = proc.apply(null, applyArgs);
    }
    return l1_orig;
}

/*** META ((export #t)) */
function sc_forEach(proc, l1) {
    if (l1 === undefined)
	return undefined;
    // else
    var nbApplyArgs = arguments.length - 1;
    var applyArgs = new Array(nbApplyArgs);
    while (l1 !== null) {
	for (var i = 0; i < nbApplyArgs; i++) {
	    applyArgs[i] = arguments[i + 1].car;
	    arguments[i + 1] = arguments[i + 1].cdr;
	}
	proc.apply(null, applyArgs);
    }
    // add return so FF does not complain.
    return undefined;
}

/*** META ((export #t)) */
function sc_filter(proc, l1) {
    var dummy = { cdr : null };
    var tail = dummy;
    while (l1 !== null) {
	if (proc(l1.car) !== false) {
	    tail.cdr = sc_cons(l1.car, null);
	    tail = tail.cdr;
	}
	l1 = l1.cdr;
    }
    return dummy.cdr;
}

/*** META ((export #t)) */
function sc_filterBang(proc, l1) {
    var head = sc_cons("dummy", l1);
    var it = head;
    var next = l1;
    while (next !== null) {
        if (proc(next.car) !== false) {
	    it.cdr = next
	    it = next;
	}
	next = next.cdr;
    }
    it.cdr = null;
    return head.cdr;
}

function sc_filterMap1(proc, l1) {
    var revres = null;
    while (l1 !== null) {
        var tmp = proc(l1.car)
        if (tmp !== false) revres = sc_cons(tmp, revres);
        l1 = l1.cdr;
    }
    return sc_reverseAppendBang(revres, null);
}
function sc_filterMap2(proc, l1, l2) {
    var revres = null;
    while (l1 !== null) {
        var tmp = proc(l1.car, l2.car);
        if(tmp !== false) revres = sc_cons(tmp, revres);
	l1 = l1.cdr;
	l2 = l2.cdr
    }
    return sc_reverseAppendBang(revres, null);
}

/*** META ((export #t)) */
function sc_filterMap(proc, l1, l2, l3) {
    if (l2 === undefined)
	return sc_filterMap1(proc, l1);
    else if (l3 === undefined)
	return sc_filterMap2(proc, l1, l2);
    // else
    var nbApplyArgs = arguments.length - 1;
    var applyArgs = new Array(nbApplyArgs);
    var revres = null;
    while (l1 !== null) {
	for (var i = 0; i < nbApplyArgs; i++) {
	    applyArgs[i] = arguments[i + 1].car;
	    arguments[i + 1] = arguments[i + 1].cdr;
	}
	var tmp = proc.apply(null, applyArgs);
	if(tmp !== false) revres = sc_cons(tmp, revres);
    }
    return sc_reverseAppendBang(revres, null);
}

/*** META ((export #t)) */
function sc_any(proc, l) {
    var revres = null;
    while (l !== null) {
        var tmp = proc(l.car);
        if(tmp !== false) return tmp;
	l = l.cdr;
    }
    return false;
}

/*** META ((export any?)
           (peephole (hole 2 "sc_any(" proc "," l ") !== false")))
*/
function sc_anyPred(proc, l) {
    return sc_any(proc, l)!== false;
}

/*** META ((export #t)) */
function sc_every(proc, l) {
    var revres = null;
    var tmp = true;
    while (l !== null) {
        tmp = proc(l.car);
        if (tmp === false) return false;
	l = l.cdr;
    }
    return tmp;
}

/*** META ((export every?)
           (peephole (hole 2 "sc_every(" proc "," l ") !== false")))
*/
function sc_everyPred(proc, l) {
    var tmp = sc_every(proc, l);
    if (tmp !== false) return true;
    return false;
}

/*** META ((export #t)
           (peephole (postfix "()")))
*/
function sc_force(o) {
    return o();
}

/*** META ((export #t)) */
function sc_makePromise(proc) {
    var isResultReady = false;
    var result = undefined;
    return function() {
	if (!isResultReady) {
	    var tmp = proc();
	    if (!isResultReady) {
		isResultReady = true;
		result = tmp;
	    }
	}
	return result;
    };
}

function sc_Values(values) {
    this.values = values;
}

/*** META ((export #t)
           (peephole (values)))
*/
function sc_values() {
    if (arguments.length === 1)
	return arguments[0];
    else
	return new sc_Values(arguments);
}

/*** META ((export #t)) */
function sc_callWithValues(producer, consumer) {
    var produced = producer();
    if (produced instanceof sc_Values)
	return consumer.apply(null, produced.values);
    else
	return consumer(produced);
}

/*** META ((export #t)) */
function sc_dynamicWind(before, thunk, after) {
    before();
    try {
	var res = thunk();
	return res;
    } finally {
	after();
    }
}


// TODO: eval/scheme-report-environment/null-environment/interaction-environment

// LIMITATION: 'load' doesn't exist without files.
// LIMITATION: transcript-on/transcript-off doesn't exist without files.


function sc_Struct(name) {
    this.name = name;
}
sc_Struct.prototype.sc_toDisplayString = function() {
    return "#<struct" + sc_hash(this) + ">";
};
sc_Struct.prototype.sc_toWriteString = sc_Struct.prototype.sc_toDisplayString;

/*** META ((export #t)
           (peephole (hole 1 "new sc_Struct(" name ")")))
*/
function sc_makeStruct(name) {
    return new sc_Struct(name);
}

/*** META ((export #t)
           (type bool)
           (peephole (postfix " instanceof sc_Struct")))
*/
function sc_isStruct(o) {
    return (o instanceof sc_Struct);
}

/*** META ((export #t)
           (type bool)
           (peephole (hole 2 "(" 1 " instanceof sc_Struct) && ( " 1 ".name === " 0 ")")))
*/
function sc_isStructNamed(name, s) {
    return ((s instanceof sc_Struct) && (s.name === name));
}

/*** META ((export struct-field)
           (peephole (hole 3 0 "[" 2 "]")))
*/
function sc_getStructField(s, name, field) {
    return s[field];
}

/*** META ((export struct-field-set!)
           (peephole (hole 4 0 "[" 2 "] = " 3)))
*/
function sc_setStructFieldBang(s, name, field, val) {
    s[field] = val;
}

/*** META ((export #t)
           (peephole (prefix "~")))
*/
function sc_bitNot(x) {
    return ~x;
}

/*** META ((export #t)
           (peephole (infix 2 2 "&")))
*/
function sc_bitAnd(x, y) {
    return x & y;
}

/*** META ((export #t)
           (peephole (infix 2 2 "|")))
*/
function sc_bitOr(x, y) {
    return x | y;
}

/*** META ((export #t)
           (peephole (infix 2 2 "^")))
*/
function sc_bitXor(x, y) {
    return x ^ y;
}

/*** META ((export #t)
           (peephole (infix 2 2 "<<")))
*/
function sc_bitLsh(x, y) {
    return x << y;
}

/*** META ((export #t)
           (peephole (infix 2 2 ">>")))
*/
function sc_bitRsh(x, y) {
    return x >> y;
}

/*** META ((export #t)
           (peephole (infix 2 2 ">>>")))
*/
function sc_bitUrsh(x, y) {
    return x >>> y;
}

/*** META ((export js-field js-property)
           (peephole (hole 2 o "[" field "]")))
*/
function sc_jsField(o, field) {
    return o[field];
}

/*** META ((export js-field-set! js-property-set!)
           (peephole (hole 3 o "[" field "] = " val)))
*/
function sc_setJsFieldBang(o, field, val) {
    return o[field] = val;
}

/*** META ((export js-field-delete! js-property-delete!)
           (peephole (hole 2 "delete" o "[" field "]")))
*/
function sc_deleteJsFieldBang(o, field) {
    delete o[field];
}

/*** META ((export #t)
           (peephole (jsCall)))
*/
function sc_jsCall(o, fun) {
    var args = new Array();
    for (var i = 2; i < arguments.length; i++)
	args[i-2] = arguments[i];
    return fun.apply(o, args);
}

/*** META ((export #t)
           (peephole (jsMethodCall)))
*/
function sc_jsMethodCall(o, field) {
    var args = new Array();
    for (var i = 2; i < arguments.length; i++)
	args[i-2] = arguments[i];
    return o[field].apply(o, args);
}

/*** META ((export new js-new)
           (peephole (jsNew)))
*/
function sc_jsNew(c) {
    var evalStr = "new c(";
    evalStr +=arguments.length > 1? "arguments[1]": "";
    for (var i = 2; i < arguments.length; i++)
	evalStr += ", arguments[" + i + "]";
    evalStr +=")";
    return eval(evalStr);
}

// ======================== RegExp ====================
/*** META ((export #t)) */
function sc_pregexp(re) {
    return new RegExp(sc_string2jsstring(re));
}

/*** META ((export #t)) */
function sc_pregexpMatch(re, s) {
    var reg = (re instanceof RegExp) ? re : sc_pregexp(re);
    var tmp = reg.exec(sc_string2jsstring(s));

    if (tmp == null) return false;

    var res = null;
    for (var i = tmp.length-1; i >= 0; i--) {
	if (tmp[i] !== null) {
	    res = sc_cons(sc_jsstring2string(tmp[i]), res);
	} else {
	    res = sc_cons(false, res);
	}
    }
    return res;
}

/*** META ((export #t)) */
function sc_pregexpReplace(re, s1, s2) {
   var reg;
   var jss1 = sc_string2jsstring(s1);
   var jss2 = sc_string2jsstring(s2);

   if (re instanceof RegExp) {
       if (re.global)
	   reg = re;
       else
	   reg = new RegExp(re.source);
   } else {
       reg = new RegExp(sc_string2jsstring(re));
   }

   return jss1.replace(reg, jss2);
}

/*** META ((export pregexp-replace*)) */
function sc_pregexpReplaceAll(re, s1, s2) {
   var reg;
   var jss1 = sc_string2jsstring(s1);
   var jss2 = sc_string2jsstring(s2);

   if (re instanceof RegExp) {
      if (re.global)
	  reg = re;
      else
	  reg = new RegExp(re.source, "g");
   } else {
       reg = new RegExp(sc_string2jsstring(re), "g");
   }

   return jss1.replace(reg, jss2);
}

/*** META ((export #t)) */
function sc_pregexpSplit(re, s) {
   var reg = ((re instanceof RegExp) ?
	      re :
	      new RegExp(sc_string2jsstring(re)));
   var jss = sc_string2jsstring(s);
   var tmp = jss.split(reg);

   if (tmp == null) return false;

   return sc_vector2list(tmp);
}


/* =========================================================================== */
/* Other library stuff */
/* =========================================================================== */

/*** META ((export #t)
           (peephole (hole 1 "Math.floor(Math.random()*" 'n ")")))
*/
function sc_random(n) {
    return Math.floor(Math.random()*n);
}

/*** META ((export current-date)
           (peephole (hole 0 "new Date()")))
*/
function sc_currentDate() {
   return new Date();
}

function sc_Hashtable() {
}
sc_Hashtable.prototype.toString = function() {
    return "#{%hashtable}";
};
// sc_toWriteString == sc_toDisplayString == toString

function sc_HashtableElement(key, val) {
    this.key = key;
    this.val = val;
}

/*** META ((export #t)
           (peephole (hole 0 "new sc_Hashtable()")))
*/
function sc_makeHashtable() {
    return new sc_Hashtable();
}

/*** META ((export #t)) */
function sc_hashtablePutBang(ht, key, val) {
    var hash = sc_hash(key);
    ht[hash] = new sc_HashtableElement(key, val);
}

/*** META ((export #t)) */
function sc_hashtableGet(ht, key) {
    var hash = sc_hash(key);
    if (hash in ht)
	return ht[hash].val;
    else
	return false;
}

/*** META ((export #t)) */
function sc_hashtableForEach(ht, f) {
    for (var v in ht) {
	if (ht[v] instanceof sc_HashtableElement)
	    f(ht[v].key, ht[v].val);
    }
}

/*** META ((export hashtable-contains?)
           (peephole (hole 2 "sc_hash(" 1 ") in " 0)))
*/
function sc_hashtableContains(ht, key) {
    var hash = sc_hash(key);
    if (hash in ht)
	return true;
    else
	return false;
}

var SC_HASH_COUNTER = 0;

function sc_hash(o) {
    if (o === null)
	return "null";
    else if (o === undefined)
	return "undefined";
    else if (o === true)
	return "true";
    else if (o === false)
	return "false";
    else if (typeof o === "number")
	return "num-" + o;
    else if (typeof o === "string")
	return "jsstr-" + o;
    else if (o.sc_getHash)
	return o.sc_getHash();
    else
	return sc_counterHash.call(o);
}
function sc_counterHash() {
    if (!this.sc_hash) {
	this.sc_hash = "hash-" + SC_HASH_COUNTER;
	SC_HASH_COUNTER++;
    }
    return this.sc_hash;
}

function sc_Trampoline(args, maxTailCalls) {
    this['__trampoline return__'] = true;
    this.args = args;
    this.MAX_TAIL_CALLs = maxTailCalls;
}
// TODO: call/cc stuff
sc_Trampoline.prototype.restart = function() {
    var o = this;
    while (true) {
	// set both globals.
	SC_TAIL_OBJECT.calls = o.MAX_TAIL_CALLs-1;
	var fun = o.args.callee;
	var res = fun.apply(SC_TAIL_OBJECT, o.args);
	if (res instanceof sc_Trampoline)
	    o = res;
	else
	    return res;
    }
}

/*** META ((export bind-exit-lambda)) */
function sc_bindExitLambda(proc) {
    var escape_obj = new sc_BindExitException();
    var escape = function(res) {
	escape_obj.res = res;
	throw escape_obj;
    };
    try {
	return proc(escape);
    } catch(e) {
	if (e === escape_obj) {
	    return e.res;
	}
	throw e;
    }
}
function sc_BindExitException() {
    this._internalException = true;
}

var SC_SCM2JS_GLOBALS = new Object();

// default tail-call depth.
// normally the program should set it again. but just in case...
var SC_TAIL_OBJECT = new Object();
SC_SCM2JS_GLOBALS.TAIL_OBJECT = SC_TAIL_OBJECT;
// ======================== I/O =======================

/*------------------------------------------------------------------*/

function sc_EOF() {
}
var SC_EOF_OBJECT = new sc_EOF();

function sc_Port() {
}

/* --------------- Input ports -------------------------------------*/

function sc_InputPort() {
}
sc_InputPort.prototype = new sc_Port();

sc_InputPort.prototype.peekChar = function() {
    if (!("peeked" in this))
	this.peeked = this.getNextChar();
    return this.peeked;
}
sc_InputPort.prototype.readChar = function() {
    var tmp = this.peekChar();
    delete this.peeked;
    return tmp;
}
sc_InputPort.prototype.isCharReady = function() {
    return true;
}
sc_InputPort.prototype.close = function() {
    // do nothing
}

/* .............. String port ..........................*/
function sc_ErrorInputPort() {
};
sc_ErrorInputPort.prototype = new sc_InputPort();
sc_ErrorInputPort.prototype.getNextChar = function() {
    throw "can't read from error-port.";
};
sc_ErrorInputPort.prototype.isCharReady = function() {
    return false;
};


/* .............. String port ..........................*/

function sc_StringInputPort(jsStr) {
    // we are going to do some charAts on the str.
    // instead of recreating all the time a String-object, we
    // create one in the beginning. (not sure, if this is really an optim)
    this.str = new String(jsStr);
    this.pos = 0;
}
sc_StringInputPort.prototype = new sc_InputPort();
sc_StringInputPort.prototype.getNextChar = function() {
    if (this.pos >= this.str.length)
	return SC_EOF_OBJECT;
    return this.str.charAt(this.pos++);
};

/* ------------- Read and other lib-funs  -------------------------------*/
function sc_Token(type, val, pos) {
    this.type = type;
    this.val = val;
    this.pos = pos;
}
sc_Token.EOF = 0/*EOF*/;
sc_Token.OPEN_PAR = 1/*OPEN_PAR*/;
sc_Token.CLOSE_PAR = 2/*CLOSE_PAR*/;
sc_Token.OPEN_BRACE = 3/*OPEN_BRACE*/;
sc_Token.CLOSE_BRACE = 4/*CLOSE_BRACE*/;
sc_Token.OPEN_BRACKET = 5/*OPEN_BRACKET*/;
sc_Token.CLOSE_BRACKET = 6/*CLOSE_BRACKET*/;
sc_Token.WHITESPACE = 7/*WHITESPACE*/;
sc_Token.QUOTE = 8/*QUOTE*/;
sc_Token.ID = 9/*ID*/;
sc_Token.DOT = 10/*DOT*/;
sc_Token.STRING = 11/*STRING*/;
sc_Token.NUMBER = 12/*NUMBER*/;
sc_Token.ERROR = 13/*ERROR*/;
sc_Token.VECTOR_BEGIN = 14/*VECTOR_BEGIN*/;
sc_Token.TRUE = 15/*TRUE*/;
sc_Token.FALSE = 16/*FALSE*/;
sc_Token.UNSPECIFIED = 17/*UNSPECIFIED*/;
sc_Token.REFERENCE = 18/*REFERENCE*/;
sc_Token.STORE = 19/*STORE*/;
sc_Token.CHAR = 20/*CHAR*/;

var SC_ID_CLASS = SC_LOWER_CLASS + SC_UPPER_CLASS + "!$%*+-./:<=>?@^_~";
function sc_Tokenizer(port) {
    this.port = port;
}
sc_Tokenizer.prototype.peekToken = function() {
    if (this.peeked)
	return this.peeked;
    var newToken = this.nextToken();
    this.peeked = newToken;
    return newToken;
};
sc_Tokenizer.prototype.readToken = function() {
    var tmp = this.peekToken();
    delete this.peeked;
    return tmp;
};
sc_Tokenizer.prototype.nextToken = function() {
    var port = this.port;

    function isNumberChar(c) {
	return (c >= "0" && c <= "9");
    };
    function isIdOrNumberChar(c) {
	return SC_ID_CLASS.indexOf(c) != -1 || // ID-char
	    (c >= "0" && c <= "9");
    }
    function isWhitespace(c) {
	return c === " " || c === "\r" || c === "\n" || c === "\t" || c === "\f";
    };
    function isWhitespaceOrEOF(c) {
	return isWhitespace(c) || c === SC_EOF_OBJECT;
    };

    function readString() {
	res = "";
	while (true) {
	    var c = port.readChar();
	    switch (c) {
	    case '"':
		return new sc_Token(11/*STRING*/, res);
	    case "\\":
		var tmp = port.readChar();
		switch (tmp) {
		case '0': res += "\0"; break;
		case 'a': res += "\a"; break;
		case 'b': res += "\b"; break;
		case 'f': res += "\f"; break;
		case 'n': res += "\n"; break;
		case 'r': res += "\r"; break;
		case 't': res += "\t"; break;
		case 'v': res += "\v"; break;
		case '"': res += '"'; break;
		case '\\': res += '\\'; break;
		case 'x':
		    /* hexa-number */
		    var nb = 0;
		    while (true) {
			var hexC = port.peekChar();
			if (hexC >= '0' && hexC <= '9') {
			    port.readChar();
			    nb = nb * 16 + hexC.charCodeAt(0) - '0'.charCodeAt(0);
			} else if (hexC >= 'a' && hexC <= 'f') {
			    port.readChar();
			    nb = nb * 16 + hexC.charCodeAt(0) - 'a'.charCodeAt(0);
			} else if (hexC >= 'A' && hexC <= 'F') {
			    port.readChar();
			    nb = nb * 16 + hexC.charCodeAt(0) - 'A'.charCodeAt(0);
			} else {
			    // next char isn't part of hex.
			    res += String.fromCharCode(nb);
			    break;
			}
		    }
		    break;
		default:
		    if (tmp === SC_EOF_OBJECT) {
			return new sc_Token(13/*ERROR*/, "unclosed string-literal" + res);
		    }
		    res += tmp;
		}
		break;
	    default:
		if (c === SC_EOF_OBJECT) {
		    return new sc_Token(13/*ERROR*/, "unclosed string-literal" + res);
		}
		res += c;
	    }
	}
    };
    function readIdOrNumber(firstChar) {
	var res = firstChar;
	while (isIdOrNumberChar(port.peekChar()))
	    res += port.readChar();
	if (isNaN(res))
	    return new sc_Token(9/*ID*/, res);
	else
	    return new sc_Token(12/*NUMBER*/, res - 0);
    };

    function skipWhitespaceAndComments() {
	var done = false;
	while (!done) {
	    done = true;
	    while (isWhitespace(port.peekChar()))
		port.readChar();
	    if (port.peekChar() === ';') {
		port.readChar();
		done = false;
		while (true) {
		    curChar = port.readChar();
		    if (curChar === SC_EOF_OBJECT ||
			curChar === '\n')
			break;
		}
	    }
	}
    };

    function readDot() {
	if (isWhitespace(port.peekChar()))
	    return new sc_Token(10/*DOT*/);
	else
	    return readIdOrNumber(".");
    };

    function readSharp() {
	var c = port.readChar();
	if (isWhitespace(c))
	    return new sc_Token(13/*ERROR*/, "bad #-pattern0.");

	// reference
	if (isNumberChar(c)) {
	    var nb = c - 0;
	    while (isNumberChar(port.peekChar()))
		nb = nb*10 + (port.readChar() - 0);
	    switch (port.readChar()) {
	    case '#':
		return new sc_Token(18/*REFERENCE*/, nb);
	    case '=':
		return new sc_Token(19/*STORE*/, nb);
	    default:
		return new sc_Token(13/*ERROR*/, "bad #-pattern1." + nb);
	    }
	}

	if (c === "(")
	    return new sc_Token(14/*VECTOR_BEGIN*/);
	
	if (c === "\\") { // character
	    var tmp = ""
	    while (!isWhitespaceOrEOF(port.peekChar()))
		tmp += port.readChar();
	    switch (tmp.length) {
	    case 0: // it's escaping a whitespace char:
		if (sc_isEOFObject(port.peekChar))
		    return new sc_Token(13/*ERROR*/, "bad #-pattern2.");
		else
		    return new sc_Token(20/*CHAR*/, port.readChar());
	    case 1:
		return new sc_Token(20/*CHAR*/, tmp);
	    default:
		var entry = sc_Char.readable2char[tmp.toLowerCase()];
		if (entry)
		    return new sc_Token(20/*CHAR*/, entry);
		else
		    return new sc_Token(13/*ERROR*/, "unknown character description: #\\" + tmp);
	    }
	}

	// some constants (#t, #f, #unspecified)
	var res;
	var needing;
	switch (c) {
	case 't': res = new sc_Token(15/*TRUE*/, true); needing = ""; break;
	case 'f': res = new sc_Token(16/*FALSE*/, false); needing = ""; break;
	case 'u': res = new sc_Token(17/*UNSPECIFIED*/, undefined); needing = "nspecified"; break;
	default:
	    return new sc_Token(13/*ERROR*/, "bad #-pattern3: " + c);
	}
	while(true) {
	    c = port.peekChar();
	    if ((isWhitespaceOrEOF(c) || c === ')') &&
		needing == "")
		return res;
	    else if (isWhitespace(c) || needing == "")
		return new sc_Token(13/*ERROR*/, "bad #-pattern4 " + c + " " + needing);
	    else if (needing.charAt(0) == c) {
		port.readChar(); // consume
		needing = needing.slice(1);
	    } else
		return new sc_Token(13/*ERROR*/, "bad #-pattern5");
	}
	
    };

    skipWhitespaceAndComments();
    var curChar = port.readChar();
    if (curChar === SC_EOF_OBJECT)
	return new sc_Token(0/*EOF*/, curChar);
    switch (curChar)
    {
    case " ":
    case "\n":
    case "\t":
	return readWhitespace();
    case "(":
	return new sc_Token(1/*OPEN_PAR*/);
    case ")":
	return new sc_Token(2/*CLOSE_PAR*/);
    case "{":
	return new sc_Token(3/*OPEN_BRACE*/);
    case "}":
	return new sc_Token(4/*CLOSE_BRACE*/);
    case "[":
	return new sc_Token(5/*OPEN_BRACKET*/);
    case "]":
	return new sc_Token(6/*CLOSE_BRACKET*/);
    case "'":
	return new sc_Token(8/*QUOTE*/);
    case "#":
	return readSharp();
    case ".":
	return readDot();
    case '"':
	return readString();
    default:
	if (isIdOrNumberChar(curChar))
	    return readIdOrNumber(curChar);
	throw "unexpected character: " + curChar;
    }
};

function sc_Reader(tokenizer) {
    this.tokenizer = tokenizer;
    this.backref = new Array();
}
sc_Reader.prototype.read = function() {
    function readList(listBeginType) {
	function matchesPeer(open, close) {
	    return open === 1/*OPEN_PAR*/ && close === 2/*CLOSE_PAR*/
	    	|| open === 3/*OPEN_BRACE*/ && close === 4/*CLOSE_BRACE*/
		|| open === 5/*OPEN_BRACKET*/ && close === 6/*CLOSE_BRACKET*/;
	};
	var res = null;

	while (true) {
	    var token = tokenizer.peekToken();

	    switch (token.type) {
	    case 2/*CLOSE_PAR*/:
	    case 4/*CLOSE_BRACE*/:
	    case 6/*CLOSE_BRACKET*/:
		if (matchesPeer(listBeginType, token.type)) {
		    tokenizer.readToken(); // consume token
		    return sc_reverseBang(res);
		} else
		    throw "closing par doesn't match: " + listBeginType
			+ " " + listEndType;

	    case 0/*EOF*/:
		throw "unexpected end of file";

	    case 10/*DOT*/:
		tokenizer.readToken(); // consume token
		var cdr = this.read();
		var par = tokenizer.readToken();
		if (!matchesPeer(listBeginType, par.type))
		    throw "closing par doesn't match: " + listBeginType
			+ " " + par.type;
		else
		    return sc_reverseAppendBang(res, cdr);
		

	    default:
		res = sc_cons(this.read(), res);
	    }
	}
    };
    function readQuote() {
	return sc_cons("quote", sc_cons(this.read(), null));
    };
    function readVector() {
	// opening-parenthesis is already consumed
	var a = new Array();
	while (true) {
	    var token = tokenizer.peekToken();
	    switch (token.type) {
	    case 2/*CLOSE_PAR*/:
		tokenizer.readToken();
		return a;
		
	    default:
		a.push(this.read());
	    }
	}
    };

    function storeRefence(nb) {
	var tmp = this.read();
	this.backref[nb] = tmp;
	return tmp;
    };
	
    function readReference(nb) {
	if (nb in this.backref)
	    return this.backref[nb];
	else
	    throw "bad reference: " + nb;
    };

    var tokenizer = this.tokenizer;

    var token = tokenizer.readToken();

    // handle error
    if (token.type === 13/*ERROR*/)
	throw token.val;

    switch (token.type) {
    case 1/*OPEN_PAR*/:
    case 3/*OPEN_BRACE*/:
    case 5/*OPEN_BRACKET*/:
	return readList.call(this, token.type);
    case 8/*QUOTE*/:
	return readQuote.call(this);
    case 11/*STRING*/:
	return sc_jsstring2string(token.val);
    case 20/*CHAR*/:
	return new sc_Char(token.val);
    case 14/*VECTOR_BEGIN*/:
	return readVector.call(this);
    case 18/*REFERENCE*/:
	return readReference.call(this, token.val);
    case 19/*STORE*/:
	return storeRefence.call(this, token.val);
    case 9/*ID*/:
	return sc_jsstring2symbol(token.val);
    case 0/*EOF*/:
    case 12/*NUMBER*/:
    case 15/*TRUE*/:
    case 16/*FALSE*/:
    case 17/*UNSPECIFIED*/:
	return token.val;
    default:
	throw "unexpected token " + token.type + " " + token.val;
    }
};

/*** META ((export #t)) */
function sc_read(port) {
    if (port === undefined) // we assume the port hasn't been given.
	port = SC_DEFAULT_IN; // THREAD: shared var...
    var reader = new sc_Reader(new sc_Tokenizer(port));
    return reader.read();
}
/*** META ((export #t)) */
function sc_readChar(port) {
    if (port === undefined) // we assume the port hasn't been given.
	port = SC_DEFAULT_IN; // THREAD: shared var...
    var t = port.readChar();
    return t === SC_EOF_OBJECT? t: new sc_Char(t);
}
/*** META ((export #t)) */
function sc_peekChar(port) {
    if (port === undefined) // we assume the port hasn't been given.
	port = SC_DEFAULT_IN; // THREAD: shared var...
    var t = port.peekChar();
    return t === SC_EOF_OBJECT? t: new sc_Char(t);
}
/*** META ((export #t)
           (type bool))
*/
function sc_isCharReady(port) {
    if (port === undefined) // we assume the port hasn't been given.
	port = SC_DEFAULT_IN; // THREAD: shared var...
    return port.isCharReady();
}
/*** META ((export #t)
           (peephole (postfix ".close()")))
*/
function sc_closeInputPort(p) {
    return p.close();
}

/*** META ((export #t)
           (type bool)
           (peephole (postfix " instanceof sc_InputPort")))
*/
function sc_isInputPort(o) {
    return (o instanceof sc_InputPort);
}

/*** META ((export eof-object?)
           (type bool)
           (peephole (postfix " === SC_EOF_OBJECT")))
*/
function sc_isEOFObject(o) {
    return o === SC_EOF_OBJECT;
}

/*** META ((export #t)
           (peephole (hole 0 "SC_DEFAULT_IN")))
*/
function sc_currentInputPort() {
    return SC_DEFAULT_IN;
}

/* ------------ file operations are not supported -----------*/
/*** META ((export #t)) */
function sc_callWithInputFile(s, proc) {
    throw "can't open " + s;
}

/*** META ((export #t)) */
function sc_callWithOutputFile(s, proc) {
    throw "can't open " + s;
}

/*** META ((export #t)) */
function sc_withInputFromFile(s, thunk) {
    throw "can't open " + s;
}

/*** META ((export #t)) */
function sc_withOutputToFile(s, thunk) {
    throw "can't open " + s;
}

/*** META ((export #t)) */
function sc_openInputFile(s) {
    throw "can't open " + s;
}

/*** META ((export #t)) */
function sc_openOutputFile(s) {
    throw "can't open " + s;
}

/* ----------------------------------------------------------------------------*/
/*** META ((export #t)) */
function sc_basename(p) {
   var i = p.lastIndexOf('/');

   if(i >= 0)
      return p.substring(i + 1, p.length);
   else
      return '';
}

/*** META ((export #t)) */
function sc_dirname(p) {
   var i = p.lastIndexOf('/');

   if(i >= 0)
      return p.substring(0, i);
   else
      return '';
}

/* ----------------------------------------------------------------------------*/

/*** META ((export #t)) */
function sc_withInputFromPort(p, thunk) {
    try {
	var tmp = SC_DEFAULT_IN; // THREAD: shared var.
	SC_DEFAULT_IN = p;
	return thunk();
    } finally {
	SC_DEFAULT_IN = tmp;
    }
}

/*** META ((export #t)) */
function sc_withInputFromString(s, thunk) {
    return sc_withInputFromPort(new sc_StringInputPort(sc_string2jsstring(s)), thunk);
}

/*** META ((export #t)) */
function sc_withOutputToPort(p, thunk) {
    try {
	var tmp = SC_DEFAULT_OUT; // THREAD: shared var.
	SC_DEFAULT_OUT = p;
	return thunk();
    } finally {
	SC_DEFAULT_OUT = tmp;
    }
}

/*** META ((export #t)) */
function sc_withOutputToString(thunk) {
    var p = new sc_StringOutputPort();
    sc_withOutputToPort(p, thunk);
    return p.close();
}

/*** META ((export #t)) */
function sc_withOutputToProcedure(proc, thunk) {
    var t = function(s) { proc(sc_jsstring2string(s)); };
    return sc_withOutputToPort(new sc_GenericOutputPort(t), thunk);
}

/*** META ((export #t)
           (peephole (hole 0 "new sc_StringOutputPort()")))
*/
function sc_openOutputString() {
    return new sc_StringOutputPort();
}

/*** META ((export #t)) */
function sc_openInputString(str) {
    return new sc_StringInputPort(sc_string2jsstring(str));
}

/* ----------------------------------------------------------------------------*/

function sc_OutputPort() {
}
sc_OutputPort.prototype = new sc_Port();
sc_OutputPort.prototype.appendJSString = function(obj) {
    /* do nothing */
}
sc_OutputPort.prototype.close = function() {
    /* do nothing */
}

function sc_StringOutputPort() {
    this.res = "";
}
sc_StringOutputPort.prototype = new sc_OutputPort();
sc_StringOutputPort.prototype.appendJSString = function(s) {
    this.res += s;
}
sc_StringOutputPort.prototype.close = function() {
    return sc_jsstring2string(this.res);
}

/*** META ((export #t)) */
function sc_getOutputString(sp) {
    return sc_jsstring2string(sp.res);
}


function sc_ErrorOutputPort() {
}
sc_ErrorOutputPort.prototype = new sc_OutputPort();
sc_ErrorOutputPort.prototype.appendJSString = function(s) {
    throw "don't write on ErrorPort!";
}
sc_ErrorOutputPort.prototype.close = function() {
    /* do nothing */
}

function sc_GenericOutputPort(appendJSString, close) {
    this.appendJSString = appendJSString;
    if (close)
	this.close = close;
}
sc_GenericOutputPort.prototype = new sc_OutputPort();

/*** META ((export #t)
           (type bool)
           (peephole (postfix " instanceof sc_OutputPort")))
*/
function sc_isOutputPort(o) {
    return (o instanceof sc_OutputPort);
}

/*** META ((export #t)
           (peephole (postfix ".close()")))
*/
function sc_closeOutputPort(p) {
    return p.close();
}

/* ------------------ write ---------------------------------------------------*/

/*** META ((export #t)) */
function sc_write(o, p) {
    if (p === undefined) // we assume not given
	p = SC_DEFAULT_OUT;
    p.appendJSString(sc_toWriteString(o));
}

function sc_toWriteString(o) {
    if (o === null)
	return "()";
    else if (o === true)
	return "#t";
    else if (o === false)
	return "#f";
    else if (o === undefined)
	return "#unspecified";
    else if (typeof o === 'function')
	return "#<procedure " + sc_hash(o) + ">";
    else if (o.sc_toWriteString)
	return o.sc_toWriteString();
    else
	return o.toString();
}

function sc_escapeWriteString(s) {
    var res = "";
    var j = 0;
    for (i = 0; i < s.length; i++) {
	switch (s.charAt(i)) {
	case "\0": res += s.substring(j, i) + "\\0"; j = i + 1; break;
	case "\b": res += s.substring(j, i) + "\\b"; j = i + 1; break;
	case "\f": res += s.substring(j, i) + "\\f"; j = i + 1; break;
	case "\n": res += s.substring(j, i) + "\\n"; j = i + 1; break;
	case "\r": res += s.substring(j, i) + "\\r"; j = i + 1; break;
	case "\t": res += s.substring(j, i) + "\\t"; j = i + 1; break;
	case "\v": res += s.substring(j, i) + "\\v"; j = i + 1; break;
	case '"': res += s.substring(j, i) + '\\"'; j = i + 1; break;
	case "\\": res += s.substring(j, i) + "\\\\"; j = i + 1; break;
	default:
	    var c = s.charAt(i);
	    if ("\a" !== "a" && c == "\a") {
		res += s.substring(j, i) + "\\a"; j = i + 1; continue;
	    }
	    if ("\v" !== "v" && c == "\v") {
		res += s.substring(j, i) + "\\v"; j = i + 1; continue;
	    }
	    //if (s.charAt(i) < ' ' || s.charCodeAt(i) > 127) {
	    // CARE: Manuel is this OK with HOP?
	    if (s.charAt(i) < ' ') {
		/* non printable character and special chars */
		res += s.substring(j, i) + "\\x" + s.charCodeAt(i).toString(16);
		j = i + 1;
	    }
	    // else just let i increase...
	}
    }
    res += s.substring(j, i);
    return res;
}

/* ------------------ display ---------------------------------------------------*/

/*** META ((export #t)) */
function sc_display(o, p) {
    if (p === undefined) // we assume not given
	p = SC_DEFAULT_OUT;
    p.appendJSString(sc_toDisplayString(o));
}

function sc_toDisplayString(o) {
    if (o === null)
	return "()";
    else if (o === true)
	return "#t";
    else if (o === false)
	return "#f";
    else if (o === undefined)
	return "#unspecified";
    else if (typeof o === 'function')
	return "#<procedure " + sc_hash(o) + ">";
    else if (o.sc_toDisplayString)
	return o.sc_toDisplayString();
    else
	return o.toString();
}

/* ------------------ newline ---------------------------------------------------*/

/*** META ((export #t)) */
function sc_newline(p) {
    if (p === undefined) // we assume not given
	p = SC_DEFAULT_OUT;
    p.appendJSString("\n");
}

/* ------------------ write-char ---------------------------------------------------*/

/*** META ((export #t)) */
function sc_writeChar(c, p) {
    if (p === undefined) // we assume not given
	p = SC_DEFAULT_OUT;
    p.appendJSString(c.val);
}

/* ------------------ write-circle ---------------------------------------------------*/

/*** META ((export #t)) */
function sc_writeCircle(o, p) {
    if (p === undefined) // we assume not given
	p = SC_DEFAULT_OUT;
    p.appendJSString(sc_toWriteCircleString(o));
}

function sc_toWriteCircleString(o) {
    var symb = sc_gensym("writeCircle");
    var nbPointer = new Object();
    nbPointer.nb = 0;
    sc_prepWriteCircle(o, symb, nbPointer);
    return sc_genToWriteCircleString(o, symb);
}

function sc_prepWriteCircle(o, symb, nbPointer) {
    // TODO sc_Struct
    if (o instanceof sc_Pair ||
	o instanceof sc_Vector) {
	if (o[symb] !== undefined) {
	    // not the first visit.
	    o[symb]++;
	    // unless there is already a number, assign one.
	    if (!o[symb + "nb"]) o[symb + "nb"] = nbPointer.nb++;
	    return;
	}
	o[symb] = 0;
	if (o instanceof sc_Pair) {
	    sc_prepWriteCircle(o.car, symb, nbPointer);
	    sc_prepWriteCircle(o.cdr, symb, nbPointer);
	} else {
	    for (var i = 0; i < o.length; i++)
		sc_prepWriteCircle(o[i], symb, nbPointer);
	}
    }
}

function sc_genToWriteCircleString(o, symb) {
    if (!(o instanceof sc_Pair ||
	  o instanceof sc_Vector))
	return sc_toWriteString(o);
    return o.sc_toWriteCircleString(symb);
}
sc_Pair.prototype.sc_toWriteCircleString = function(symb, inList) {
    if (this[symb + "use"]) { // use-flag is set. Just use it.
	var nb = this[symb + "nb"];
	if (this[symb]-- === 0) { // if we are the last use. remove all fields.
	    delete this[symb];
	    delete this[symb + "nb"];
	    delete this[symb + "use"];
	}
	if (inList)
	    return '. #' + nb + '#';
	else
	    return '#' + nb + '#';
    }
    if (this[symb]-- === 0) { // if we are the last use. remove all fields.
	delete this[symb];
	delete this[symb + "nb"];
	delete this[symb + "use"];
    }

    var res = "";

    if (this[symb] !== undefined) { // implies > 0
	this[symb + "use"] = true;
	if (inList)
	    res += '. #' + this[symb + "nb"] + '=';
	else
	    res += '#' + this[symb + "nb"] + '=';
	inList = false;
    }

    if (!inList)
	res += "(";

    // print car
    res += sc_genToWriteCircleString(this.car, symb);

    if (sc_isPair(this.cdr)) {
	res += " " + this.cdr.sc_toWriteCircleString(symb, true);
    } else if (this.cdr !== null) {
	res += " . " + sc_genToWriteCircleString(this.cdr, symb);
    }
    if (!inList)
	res += ")";
    return res;
};
sc_Vector.prototype.sc_toWriteCircleString = function(symb) {
    if (this[symb + "use"]) { // use-flag is set. Just use it.
	var nb = this[symb + "nb"];
	if (this[symb]-- === 0) { // if we are the last use. remove all fields.
	    delete this[symb];
	    delete this[symb + "nb"];
	    delete this[symb + "use"];
	}
	return '#' + nb + '#';
    }
    if (this[symb]-- === 0) { // if we are the last use. remove all fields.
	delete this[symb];
	delete this[symb + "nb"];
	delete this[symb + "use"];
    }

    var res = "";
    if (this[symb] !== undefined) { // implies > 0
	this[symb + "use"] = true;
	res += '#' + this[symb + "nb"] + '=';
    }
    res += "#(";
    for (var i = 0; i < this.length; i++) {
	res += sc_genToWriteCircleString(this[i], symb);
	if (i < this.length - 1) res += " ";
    }
    res += ")";
    return res;
};


/* ------------------ print ---------------------------------------------------*/

/*** META ((export #t)) */
function sc_print(s) {
    if (arguments.length === 1) {
	sc_display(s);
	sc_newline();
    }
    else {
	for (var i = 0; i < arguments.length; i++)
	    sc_display(arguments[i]);
	sc_newline();
    }
}

/* ------------------ format ---------------------------------------------------*/
/*** META ((export #t)) */
function sc_format(s, args) {
   var len = s.length;
   var p = new sc_StringOutputPort();
   var i = 0, j = 1;

   while( i < len ) {
      var i2 = s.indexOf("~", i);

      if (i2 == -1) {
	  p.appendJSString( s.substring( i, len ) );
	  return p.close();
      } else {
	 if (i2 > i) {
	    if (i2 == (len - 1)) {
		p.appendJSString(s.substring(i, len));
		return p.close();
	    } else {
	       p.appendJSString(s.substring(i, i2));
	       i = i2;
	    }
	 }

	 switch(s.charCodeAt(i2 + 1)) {
	    case 65:
	    case 97:
	       // a
	       sc_display(arguments[j], p);
	       i += 2; j++;
	       break;

	    case 83:
	    case 115:
	       // s
	       sc_write(arguments[j], p);
	       i += 2; j++;
	       break;

	    case 86:
	    case 118:
	       // v
	       sc_display(arguments[j], p);
	       p.appendJSString("\n");
	       i += 2; j++;
	       break;

	    case 67:
	    case 99:
	       // c
	       p.appendJSString(String.fromCharCode(arguments[j]));
	       i += 2; j++;
	       break;

	    case 88:
	    case 120:
	       // x
	       p.appendJSString(arguments[j].toString(6));
	       i += 2; j++;
	       break;

	    case 79:
	    case 111:
	       // o
	       p.appendJSString(arguments[j].toString(8));
	       i += 2; j++;
	       break;

	    case 66:
	    case 98:
	       // b
	       p.appendJSString(arguments[j].toString(2));
	       i += 2; j++;
	       break;

	    case 37:
	    case 110:
	       // %, n
	       p.appendJSString("\n");
	       i += 2; break;

	    case 114:
	       // r
	       p.appendJSString("\r");
	       i += 2; break;

	    case 126:
	       // ~
	       p.appendJSString("~");
	       i += 2; break;

	    default:
	       sc_error( "format: illegal ~"
			 + String.fromCharCode(s.charCodeAt(i2 + 1))
			 + " sequence" );
	       return "";
	 }
      }
   }

   return p.close();
}

/* ------------------ global ports ---------------------------------------------------*/

var SC_DEFAULT_IN = new sc_ErrorInputPort();
var SC_DEFAULT_OUT = new sc_ErrorOutputPort();
var SC_ERROR_OUT = new sc_ErrorOutputPort();

var sc_SYMBOL_PREFIX = "\u1E9C";
var sc_KEYWORD_PREFIX = "\u1E9D";

/*** META ((export #t)
           (peephole (id))) */
function sc_jsstring2string(s) {
    return s;
}

/*** META ((export #t)
           (peephole (prefix "'\\u1E9C' +")))
*/
function sc_jsstring2symbol(s) {
    return sc_SYMBOL_PREFIX + s;
}

/*** META ((export #t)
           (peephole (id)))
*/
function sc_string2jsstring(s) {
    return s;
}

/*** META ((export #t)
           (peephole (symbol2jsstring_immutable)))
*/
function sc_symbol2jsstring(s) {
    return s.slice(1);
}

/*** META ((export #t)
           (peephole (postfix ".slice(1)")))
*/
function sc_keyword2jsstring(k) {
    return k.slice(1);
}

/*** META ((export #t)
           (peephole (prefix "'\\u1E9D' +")))
*/
function sc_jsstring2keyword(s) {
    return sc_KEYWORD_PREFIX + s;
}

/*** META ((export #t)
           (type bool))
*/
function sc_isKeyword(s) {
    return (typeof s === "string") &&
	(s.charAt(0) === sc_KEYWORD_PREFIX);
}


/*** META ((export #t)) */
var sc_gensym = function() {
    var counter = 1000;
    return function(sym) {
	counter++;
	if (!sym) sym = sc_SYMBOL_PREFIX;
	return sym + "s" + counter + "~" + "^sC-GeNsYm ";
    };
}();


/*** META ((export #t)
           (type bool))
*/
function sc_isEqual(o1, o2) {
    return ((o1 === o2) ||
	    (sc_isPair(o1) && sc_isPair(o2)
	     && sc_isPairEqual(o1, o2, sc_isEqual)) ||
	    (sc_isVector(o1) && sc_isVector(o2)
	     && sc_isVectorEqual(o1, o2, sc_isEqual)));
}

/*** META ((export number->symbol integer->symbol)) */
function sc_number2symbol(x, radix) {
    return sc_SYMBOL_PREFIX + sc_number2jsstring(x, radix);
}

/*** META ((export number->string integer->string)) */
var sc_number2string = sc_number2jsstring;

/*** META ((export #t)) */
function sc_symbol2number(s, radix) {
    return sc_jsstring2number(s.slice(1), radix);
}

/*** META ((export #t)) */
var sc_string2number = sc_jsstring2number;

/*** META ((export #t)
           (peephole (prefix "+" s)))
           ;; peephole will only apply if no radix is given.
*/
function sc_string2integer(s, radix) {
    if (!radix) return +s;
    return parseInt(s, radix);
}

/*** META ((export #t)
           (peephole (prefix "+")))
*/
function sc_string2real(s) {
    return +s;
}


/*** META ((export #t)
           (type bool))
*/
function sc_isSymbol(s) {
    return (typeof s === "string") &&
	(s.charAt(0) === sc_SYMBOL_PREFIX);
}

/*** META ((export #t)
           (peephole (symbol2string_immutable)))
*/
function sc_symbol2string(s) {
    return s.slice(1);
}

/*** META ((export #t)
           (peephole (prefix "'\\u1E9C' +")))
*/
function sc_string2symbol(s) {
    return sc_SYMBOL_PREFIX + s;
}

/*** META ((export symbol-append)
           (peephole (symbolAppend_immutable)))
*/
function sc_symbolAppend() {
    var res = sc_SYMBOL_PREFIX;
    for (var i = 0; i < arguments.length; i++)
	res += arguments[i].slice(1);
    return res;
}

/*** META ((export #t)
           (peephole (postfix ".val")))
*/
function sc_char2string(c) { return c.val; }

/*** META ((export #t)
           (peephole (hole 1 "'\\u1E9C' + " c ".val")))
*/
function sc_char2symbol(c) { return sc_SYMBOL_PREFIX + c.val; }

/*** META ((export #t)
           (type bool))
*/
function sc_isString(s) {
    return (typeof s === "string") &&
	(s.charAt(0) !== sc_SYMBOL_PREFIX);
}

/*** META ((export #t)) */
var sc_makeString = sc_makejsString;


/*** META ((export #t)) */
function sc_string() {
    for (var i = 0; i < arguments.length; i++)
	arguments[i] = arguments[i].val;
    return "".concat.apply("", arguments);
}

/*** META ((export #t)
           (peephole (postfix ".length")))
*/
function sc_stringLength(s) { return s.length; }

/*** META ((export #t)) */
function sc_stringRef(s, k) {
    return new sc_Char(s.charAt(k));
}

/* there's no stringSet in the immutable version
function sc_stringSet(s, k, c)
*/


/*** META ((export string=?)
	   (type bool)
           (peephole (hole 2 str1 " === " str2)))
*/
function sc_isStringEqual(s1, s2) {
    return s1 === s2;
}
/*** META ((export string<?)
	   (type bool)
           (peephole (hole 2 str1 " < " str2)))
*/
function sc_isStringLess(s1, s2) {
    return s1 < s2;
}
/*** META ((export string>?)
	   (type bool)
           (peephole (hole 2 str1 " > " str2)))
*/
function sc_isStringGreater(s1, s2) {
    return s1 > s2;
}
/*** META ((export string<=?)
	   (type bool)
           (peephole (hole 2 str1 " <= " str2)))
*/
function sc_isStringLessEqual(s1, s2) {
    return s1 <= s2;
}
/*** META ((export string>=?)
	   (type bool)
           (peephole (hole 2 str1 " >= " str2)))
*/
function sc_isStringGreaterEqual(s1, s2) {
    return s1 >= s2;
}
/*** META ((export string-ci=?)
	   (type bool)
           (peephole (hole 2 str1 ".toLowerCase() === " str2 ".toLowerCase()")))
*/
function sc_isStringCIEqual(s1, s2) {
    return s1.toLowerCase() === s2.toLowerCase();
}
/*** META ((export string-ci<?)
	   (type bool)
           (peephole (hole 2 str1 ".toLowerCase() < " str2 ".toLowerCase()")))
*/
function sc_isStringCILess(s1, s2) {
    return s1.toLowerCase() < s2.toLowerCase();
}
/*** META ((export string-ci>?)
	   (type bool)
           (peephole (hole 2 str1 ".toLowerCase() > " str2 ".toLowerCase()")))
*/
function sc_isStringCIGreater(s1, s2) {
    return s1.toLowerCase() > s2.toLowerCase();
}
/*** META ((export string-ci<=?)
	   (type bool)
           (peephole (hole 2 str1 ".toLowerCase() <= " str2 ".toLowerCase()")))
*/
function sc_isStringCILessEqual(s1, s2) {
    return s1.toLowerCase() <= s2.toLowerCase();
}
/*** META ((export string-ci>=?)
	   (type bool)
           (peephole (hole 2 str1 ".toLowerCase() >= " str2 ".toLowerCase()")))
*/
function sc_isStringCIGreaterEqual(s1, s2) {
    return s1.toLowerCase() >= s2.toLowerCase();
}

/*** META ((export #t)
           (peephole (hole 3 s ".substring(" start ", " end ")")))
*/
function sc_substring(s, start, end) {
    return s.substring(start, end);
}

/*** META ((export #t))
*/
function sc_isSubstring_at(s1, s2, i) {
    return s2 == s1.substring(i, i+ s2.length);
}

/*** META ((export #t)
           (peephole (infix 0 #f "+" "''")))
*/
function sc_stringAppend() {
    return "".concat.apply("", arguments);
}

/*** META ((export #t)) */
var sc_string2list = sc_jsstring2list;

/*** META ((export #t)) */
var sc_list2string = sc_list2jsstring;

/*** META ((export #t)
           (peephole (id)))
*/
function sc_stringCopy(s) {
    return s;
}

/* there's no string-fill in the immutable version
function sc_stringFill(s, c)
*/

/*** META ((export #t)
           (peephole (postfix ".slice(1)")))
*/
function sc_keyword2string(o) {
    return o.slice(1);
}

/*** META ((export #t)
           (peephole (prefix "'\\u1E9D' +")))
*/
function sc_string2keyword(o) {
    return sc_KEYWORD_PREFIX + o;
}

String.prototype.sc_toDisplayString = function() {
    if (this.charAt(0) === sc_SYMBOL_PREFIX)
	// TODO: care for symbols with spaces (escape-chars symbols).
	return this.slice(1);
    else if (this.charAt(0) === sc_KEYWORD_PREFIX)
	return ":" + this.slice(1);
    else
	return this.toString();
};

String.prototype.sc_toWriteString = function() {
    if (this.charAt(0) === sc_SYMBOL_PREFIX)
	// TODO: care for symbols with spaces (escape-chars symbols).
	return this.slice(1);
    else if (this.charAt(0) === sc_KEYWORD_PREFIX)
	return ":" + this.slice(1);
    else
	return '"' + sc_escapeWriteString(this) + '"';
};
/* Exported Variables */
var BgL_testzd2boyerzd2;
var BgL_nboyerzd2benchmarkzd2;
var BgL_setupzd2boyerzd2;
/* End Exports */

var translate_term_nboyer;
var translate_args_nboyer;
var untranslate_term_nboyer;
var BgL_sc_symbolzd2ze3symbolzd2record_1ze3_nboyer;
var BgL_sc_za2symbolzd2recordszd2alistza2_2z00_nboyer;
var translate_alist_nboyer;
var apply_subst_nboyer;
var apply_subst_lst_nboyer;
var tautologyp_nboyer;
var if_constructor_nboyer;
var rewrite_count_nboyer;
var rewrite_nboyer;
var rewrite_args_nboyer;
var unify_subst_nboyer;
var one_way_unify1_nboyer;
var false_term_nboyer;
var true_term_nboyer;
var trans_of_implies1_nboyer;
var is_term_equal_nboyer;
var is_term_member_nboyer;
var const_nboyer;
var sc_const_3_nboyer;
var sc_const_4_nboyer;
{
    (sc_const_4_nboyer = (new sc_Pair("\u1E9Cimplies",(new sc_Pair((new sc_Pair("\u1E9Cand",(new sc_Pair((new sc_Pair("\u1E9Cimplies",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cand",(new sc_Pair((new sc_Pair("\u1E9Cimplies",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cz",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cand",(new sc_Pair((new sc_Pair("\u1E9Cimplies",(new sc_Pair("\u1E9Cz",(new sc_Pair("\u1E9Cu",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cimplies",(new sc_Pair("\u1E9Cu",(new sc_Pair("\u1E9Cw",null)))))),null)))))),null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cimplies",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cw",null)))))),null)))))));
    (sc_const_3_nboyer = sc_list((new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Ccompile",(new sc_Pair("\u1E9Cform",null)))),(new sc_Pair((new sc_Pair("\u1E9Creverse",(new sc_Pair((new sc_Pair("\u1E9Ccodegen",(new sc_Pair((new sc_Pair("\u1E9Coptimize",(new sc_Pair("\u1E9Cform",null)))),(new sc_Pair((new sc_Pair("\u1E9Cnil",null)),null)))))),null)))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Ceqp",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cfix",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Cfix",(new sc_Pair("\u1E9Cy",null)))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cgreaterp",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cx",null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Clesseqp",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cx",null)))))),null)))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cgreatereqp",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cboolean",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Cor",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Ct",null)),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Cf",null)),null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Ciff",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cand",(new sc_Pair((new sc_Pair("\u1E9Cimplies",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cimplies",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cx",null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Ceven1",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair((new sc_Pair("\u1E9Czerop",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Ct",null)),(new sc_Pair((new sc_Pair("\u1E9Codd",(new sc_Pair((new sc_Pair("\u1E9Csub1",(new sc_Pair("\u1E9Cx",null)))),null)))),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Ccountps-",(new sc_Pair("\u1E9Cl",(new sc_Pair("\u1E9Cpred",null)))))),(new sc_Pair((new sc_Pair("\u1E9Ccountps-loop",(new sc_Pair("\u1E9Cl",(new sc_Pair("\u1E9Cpred",(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cfact-",(new sc_Pair("\u1E9Ci",null)))),(new sc_Pair((new sc_Pair("\u1E9Cfact-loop",(new sc_Pair("\u1E9Ci",(new sc_Pair((1),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Creverse-",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Creverse-loop",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Cnil",null)),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cdivides",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Czerop",(new sc_Pair((new sc_Pair("\u1E9Cremainder",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cx",null)))))),null)))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cassume-true",(new sc_Pair("\u1E9Cvar",(new sc_Pair("\u1E9Calist",null)))))),(new sc_Pair((new sc_Pair("\u1E9Ccons",(new sc_Pair((new sc_Pair("\u1E9Ccons",(new sc_Pair("\u1E9Cvar",(new sc_Pair((new sc_Pair("\u1E9Ct",null)),null)))))),(new sc_Pair("\u1E9Calist",null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cassume-false",(new sc_Pair("\u1E9Cvar",(new sc_Pair("\u1E9Calist",null)))))),(new sc_Pair((new sc_Pair("\u1E9Ccons",(new sc_Pair((new sc_Pair("\u1E9Ccons",(new sc_Pair("\u1E9Cvar",(new sc_Pair((new sc_Pair("\u1E9Cf",null)),null)))))),(new sc_Pair("\u1E9Calist",null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Ctautology-checker",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Ctautologyp",(new sc_Pair((new sc_Pair("\u1E9Cnormalize",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Cnil",null)),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cfalsify",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Cfalsify1",(new sc_Pair((new sc_Pair("\u1E9Cnormalize",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Cnil",null)),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cprime",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Cand",(new sc_Pair((new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Czerop",(new sc_Pair("\u1E9Cx",null)))),null)))),(new sc_Pair((new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Cadd1",(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))),null)))))),null)))),(new sc_Pair((new sc_Pair("\u1E9Cprime1",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Csub1",(new sc_Pair("\u1E9Cx",null)))),null)))))),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cand",(new sc_Pair("\u1E9Cp",(new sc_Pair("\u1E9Cq",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair("\u1E9Cp",(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair("\u1E9Cq",(new sc_Pair((new sc_Pair("\u1E9Ct",null)),(new sc_Pair((new sc_Pair("\u1E9Cf",null)),null)))))))),(new sc_Pair((new sc_Pair("\u1E9Cf",null)),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cor",(new sc_Pair("\u1E9Cp",(new sc_Pair("\u1E9Cq",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair("\u1E9Cp",(new sc_Pair((new sc_Pair("\u1E9Ct",null)),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair("\u1E9Cq",(new sc_Pair((new sc_Pair("\u1E9Ct",null)),(new sc_Pair((new sc_Pair("\u1E9Cf",null)),null)))))))),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cnot",(new sc_Pair("\u1E9Cp",null)))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair("\u1E9Cp",(new sc_Pair((new sc_Pair("\u1E9Cf",null)),(new sc_Pair((new sc_Pair("\u1E9Ct",null)),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cimplies",(new sc_Pair("\u1E9Cp",(new sc_Pair("\u1E9Cq",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair("\u1E9Cp",(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair("\u1E9Cq",(new sc_Pair((new sc_Pair("\u1E9Ct",null)),(new sc_Pair((new sc_Pair("\u1E9Cf",null)),null)))))))),(new sc_Pair((new sc_Pair("\u1E9Ct",null)),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cfix",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair((new sc_Pair("\u1E9Cnumberp",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Cb",(new sc_Pair("\u1E9Cc",null)))))))),(new sc_Pair("\u1E9Cd",(new sc_Pair("\u1E9Ce",null)))))))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair("\u1E9Ca",(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair("\u1E9Cb",(new sc_Pair("\u1E9Cd",(new sc_Pair("\u1E9Ce",null)))))))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair("\u1E9Cc",(new sc_Pair("\u1E9Cd",(new sc_Pair("\u1E9Ce",null)))))))),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Czerop",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Cor",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Cnumberp",(new sc_Pair("\u1E9Cx",null)))),null)))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair("\u1E9Cz",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cz",null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Cb",null)))))),(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cand",(new sc_Pair((new sc_Pair("\u1E9Czerop",(new sc_Pair("\u1E9Ca",null)))),(new sc_Pair((new sc_Pair("\u1E9Czerop",(new sc_Pair("\u1E9Cb",null)))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cdifference",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cx",null)))))),(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Cb",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Cc",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cfix",(new sc_Pair("\u1E9Cb",null)))),(new sc_Pair((new sc_Pair("\u1E9Cfix",(new sc_Pair("\u1E9Cc",null)))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Czero",null)),(new sc_Pair((new sc_Pair("\u1E9Cdifference",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cx",null)))))),null)))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Cdifference",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cand",(new sc_Pair((new sc_Pair("\u1E9Cnumberp",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Cor",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))),(new sc_Pair((new sc_Pair("\u1E9Czerop",(new sc_Pair("\u1E9Cy",null)))),null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cmeaning",(new sc_Pair((new sc_Pair("\u1E9Cplus-tree",(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))),(new sc_Pair("\u1E9Ca",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair((new sc_Pair("\u1E9Cmeaning",(new sc_Pair((new sc_Pair("\u1E9Cplus-tree",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair("\u1E9Ca",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cmeaning",(new sc_Pair((new sc_Pair("\u1E9Cplus-tree",(new sc_Pair("\u1E9Cy",null)))),(new sc_Pair("\u1E9Ca",null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cmeaning",(new sc_Pair((new sc_Pair("\u1E9Cplus-tree",(new sc_Pair((new sc_Pair("\u1E9Cplus-fringe",(new sc_Pair("\u1E9Cx",null)))),null)))),(new sc_Pair("\u1E9Ca",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cfix",(new sc_Pair((new sc_Pair("\u1E9Cmeaning",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Ca",null)))))),null)))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair("\u1E9Cz",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cz",null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Creverse",(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Cb",null)))))),null)))),(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair((new sc_Pair("\u1E9Creverse",(new sc_Pair("\u1E9Cb",null)))),(new sc_Pair((new sc_Pair("\u1E9Creverse",(new sc_Pair("\u1E9Ca",null)))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cz",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cz",null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair("\u1E9Cz",null)))))),(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cz",null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cor",(new sc_Pair((new sc_Pair("\u1E9Czerop",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Czerop",(new sc_Pair("\u1E9Cy",null)))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cexec",(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair("\u1E9Cpds",(new sc_Pair("\u1E9Cenvrn",null)))))))),(new sc_Pair((new sc_Pair("\u1E9Cexec",(new sc_Pair("\u1E9Cy",(new sc_Pair((new sc_Pair("\u1E9Cexec",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cpds",(new sc_Pair("\u1E9Cenvrn",null)))))))),(new sc_Pair("\u1E9Cenvrn",null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cmc-flatten",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair((new sc_Pair("\u1E9Cflatten",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair("\u1E9Cy",null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cmember",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Cb",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cor",(new sc_Pair((new sc_Pair("\u1E9Cmember",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Ca",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cmember",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cb",null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cmember",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Creverse",(new sc_Pair("\u1E9Cy",null)))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cmember",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Clength",(new sc_Pair((new sc_Pair("\u1E9Creverse",(new sc_Pair("\u1E9Cx",null)))),null)))),(new sc_Pair((new sc_Pair("\u1E9Clength",(new sc_Pair("\u1E9Cx",null)))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cmember",(new sc_Pair("\u1E9Ca",(new sc_Pair((new sc_Pair("\u1E9Cintersect",(new sc_Pair("\u1E9Cb",(new sc_Pair("\u1E9Cc",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cand",(new sc_Pair((new sc_Pair("\u1E9Cmember",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Cb",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cmember",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Cc",null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cnth",(new sc_Pair((new sc_Pair("\u1E9Czero",null)),(new sc_Pair("\u1E9Ci",null)))))),(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cexp",(new sc_Pair("\u1E9Ci",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cj",(new sc_Pair("\u1E9Ck",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair((new sc_Pair("\u1E9Cexp",(new sc_Pair("\u1E9Ci",(new sc_Pair("\u1E9Cj",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cexp",(new sc_Pair("\u1E9Ci",(new sc_Pair("\u1E9Ck",null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cexp",(new sc_Pair("\u1E9Ci",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cj",(new sc_Pair("\u1E9Ck",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cexp",(new sc_Pair((new sc_Pair("\u1E9Cexp",(new sc_Pair("\u1E9Ci",(new sc_Pair("\u1E9Cj",null)))))),(new sc_Pair("\u1E9Ck",null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Creverse-loop",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair((new sc_Pair("\u1E9Creverse",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair("\u1E9Cy",null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Creverse-loop",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Cnil",null)),null)))))),(new sc_Pair((new sc_Pair("\u1E9Creverse",(new sc_Pair("\u1E9Cx",null)))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Ccount-list",(new sc_Pair("\u1E9Cz",(new sc_Pair((new sc_Pair("\u1E9Csort-lp",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair((new sc_Pair("\u1E9Ccount-list",(new sc_Pair("\u1E9Cz",(new sc_Pair("\u1E9Cx",null)))))),(new sc_Pair((new sc_Pair("\u1E9Ccount-list",(new sc_Pair("\u1E9Cz",(new sc_Pair("\u1E9Cy",null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Cb",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Cc",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Cb",(new sc_Pair("\u1E9Cc",null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair((new sc_Pair("\u1E9Cremainder",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cy",(new sc_Pair((new sc_Pair("\u1E9Cquotient",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cfix",(new sc_Pair("\u1E9Cx",null)))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cpower-eval",(new sc_Pair((new sc_Pair("\u1E9Cbig-plus1",(new sc_Pair("\u1E9Cl",(new sc_Pair("\u1E9Ci",(new sc_Pair("\u1E9Cbase",null)))))))),(new sc_Pair("\u1E9Cbase",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair((new sc_Pair("\u1E9Cpower-eval",(new sc_Pair("\u1E9Cl",(new sc_Pair("\u1E9Cbase",null)))))),(new sc_Pair("\u1E9Ci",null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cpower-eval",(new sc_Pair((new sc_Pair("\u1E9Cbig-plus",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Ci",(new sc_Pair("\u1E9Cbase",null)))))))))),(new sc_Pair("\u1E9Cbase",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Ci",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair((new sc_Pair("\u1E9Cpower-eval",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cbase",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cpower-eval",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cbase",null)))))),null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cremainder",(new sc_Pair("\u1E9Cy",(new sc_Pair((1),null)))))),(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair((new sc_Pair("\u1E9Cremainder",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Czerop",(new sc_Pair("\u1E9Cy",null)))),null)))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cremainder",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cx",null)))))),(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair((new sc_Pair("\u1E9Cquotient",(new sc_Pair("\u1E9Ci",(new sc_Pair("\u1E9Cj",null)))))),(new sc_Pair("\u1E9Ci",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cand",(new sc_Pair((new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Czerop",(new sc_Pair("\u1E9Ci",null)))),null)))),(new sc_Pair((new sc_Pair("\u1E9Cor",(new sc_Pair((new sc_Pair("\u1E9Czerop",(new sc_Pair("\u1E9Cj",null)))),(new sc_Pair((new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Cj",(new sc_Pair((1),null)))))),null)))),null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair((new sc_Pair("\u1E9Cremainder",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair("\u1E9Cx",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cand",(new sc_Pair((new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Czerop",(new sc_Pair("\u1E9Cy",null)))),null)))),(new sc_Pair((new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Czerop",(new sc_Pair("\u1E9Cx",null)))),null)))),(new sc_Pair((new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cpower-eval",(new sc_Pair((new sc_Pair("\u1E9Cpower-rep",(new sc_Pair("\u1E9Ci",(new sc_Pair("\u1E9Cbase",null)))))),(new sc_Pair("\u1E9Cbase",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cfix",(new sc_Pair("\u1E9Ci",null)))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cpower-eval",(new sc_Pair((new sc_Pair("\u1E9Cbig-plus",(new sc_Pair((new sc_Pair("\u1E9Cpower-rep",(new sc_Pair("\u1E9Ci",(new sc_Pair("\u1E9Cbase",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cpower-rep",(new sc_Pair("\u1E9Cj",(new sc_Pair("\u1E9Cbase",null)))))),(new sc_Pair((new sc_Pair("\u1E9Czero",null)),(new sc_Pair("\u1E9Cbase",null)))))))))),(new sc_Pair("\u1E9Cbase",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Ci",(new sc_Pair("\u1E9Cj",null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cgcd",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cgcd",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cx",null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cnth",(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Cb",null)))))),(new sc_Pair("\u1E9Ci",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair((new sc_Pair("\u1E9Cnth",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Ci",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cnth",(new sc_Pair("\u1E9Cb",(new sc_Pair((new sc_Pair("\u1E9Cdifference",(new sc_Pair("\u1E9Ci",(new sc_Pair((new sc_Pair("\u1E9Clength",(new sc_Pair("\u1E9Ca",null)))),null)))))),null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cdifference",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair("\u1E9Cx",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cfix",(new sc_Pair("\u1E9Cy",null)))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cdifference",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cx",null)))))),(new sc_Pair("\u1E9Cx",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cfix",(new sc_Pair("\u1E9Cy",null)))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cdifference",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cz",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cdifference",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cz",null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Cdifference",(new sc_Pair("\u1E9Cc",(new sc_Pair("\u1E9Cw",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cdifference",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cc",(new sc_Pair("\u1E9Cx",null)))))),(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cw",(new sc_Pair("\u1E9Cx",null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cremainder",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cz",null)))))),(new sc_Pair("\u1E9Cz",null)))))),(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cdifference",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cb",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Cc",null)))))),null)))))),(new sc_Pair("\u1E9Ca",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cb",(new sc_Pair("\u1E9Cc",null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cdifference",(new sc_Pair((new sc_Pair("\u1E9Cadd1",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cz",null)))))),null)))),(new sc_Pair("\u1E9Cz",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cadd1",(new sc_Pair("\u1E9Cy",null)))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cz",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cz",null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cz",null)))))),(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cz",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cand",(new sc_Pair((new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Czerop",(new sc_Pair("\u1E9Cz",null)))),null)))),(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair("\u1E9Cy",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Czerop",(new sc_Pair("\u1E9Cx",null)))),null)))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cgcd",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cz",null)))))),(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cz",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cz",(new sc_Pair((new sc_Pair("\u1E9Cgcd",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cvalue",(new sc_Pair((new sc_Pair("\u1E9Cnormalize",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair("\u1E9Ca",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cvalue",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Ca",null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cflatten",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Ccons",(new sc_Pair("\u1E9Cy",(new sc_Pair((new sc_Pair("\u1E9Cnil",null)),null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cand",(new sc_Pair((new sc_Pair("\u1E9Cnlistp",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Clistp",(new sc_Pair((new sc_Pair("\u1E9Cgopher",(new sc_Pair("\u1E9Cx",null)))),null)))),(new sc_Pair((new sc_Pair("\u1E9Clistp",(new sc_Pair("\u1E9Cx",null)))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Csamefringe",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cflatten",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Cflatten",(new sc_Pair("\u1E9Cy",null)))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cgreatest-factor",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cand",(new sc_Pair((new sc_Pair("\u1E9Cor",(new sc_Pair((new sc_Pair("\u1E9Czerop",(new sc_Pair("\u1E9Cy",null)))),(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Cy",(new sc_Pair((1),null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cgreatest-factor",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((1),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Cx",(new sc_Pair((1),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cnumberp",(new sc_Pair((new sc_Pair("\u1E9Cgreatest-factor",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))),(new sc_Pair((new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Cand",(new sc_Pair((new sc_Pair("\u1E9Cor",(new sc_Pair((new sc_Pair("\u1E9Czerop",(new sc_Pair("\u1E9Cy",null)))),(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Cy",(new sc_Pair((1),null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Cnumberp",(new sc_Pair("\u1E9Cx",null)))),null)))),null)))))),null)))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Ctimes-list",(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))),(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair((new sc_Pair("\u1E9Ctimes-list",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Ctimes-list",(new sc_Pair("\u1E9Cy",null)))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cprime-list",(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))),(new sc_Pair((new sc_Pair("\u1E9Cand",(new sc_Pair((new sc_Pair("\u1E9Cprime-list",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Cprime-list",(new sc_Pair("\u1E9Cy",null)))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Cz",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cw",(new sc_Pair("\u1E9Cz",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cand",(new sc_Pair((new sc_Pair("\u1E9Cnumberp",(new sc_Pair("\u1E9Cz",null)))),(new sc_Pair((new sc_Pair("\u1E9Cor",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Cz",(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Cw",(new sc_Pair((1),null)))))),null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cgreatereqp",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cor",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cand",(new sc_Pair((new sc_Pair("\u1E9Cnumberp",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Cy",(new sc_Pair((1),null)))))),null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cremainder",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cx",null)))))),(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Cb",null)))))),(new sc_Pair((1),null)))))),(new sc_Pair(sc_list("\u1E9Cand", (new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Ca",(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))),null)))), (new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair("\u1E9Cb",(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))),null)))), (new sc_Pair("\u1E9Cnumberp",(new sc_Pair("\u1E9Ca",null)))), (new sc_Pair("\u1E9Cnumberp",(new sc_Pair("\u1E9Cb",null)))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Csub1",(new sc_Pair("\u1E9Ca",null)))),(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Csub1",(new sc_Pair("\u1E9Cb",null)))),(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair((new sc_Pair("\u1E9Clength",(new sc_Pair((new sc_Pair("\u1E9Cdelete",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cl",null)))))),null)))),(new sc_Pair((new sc_Pair("\u1E9Clength",(new sc_Pair("\u1E9Cl",null)))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cmember",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cl",null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Csort2",(new sc_Pair((new sc_Pair("\u1E9Cdelete",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cl",null)))))),null)))),(new sc_Pair((new sc_Pair("\u1E9Cdelete",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Csort2",(new sc_Pair("\u1E9Cl",null)))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cdsort",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Csort2",(new sc_Pair("\u1E9Cx",null)))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Clength",(new sc_Pair((new sc_Pair("\u1E9Ccons",(new sc_Pair("\u1E9Cx1",(new sc_Pair((new sc_Pair("\u1E9Ccons",(new sc_Pair("\u1E9Cx2",(new sc_Pair((new sc_Pair("\u1E9Ccons",(new sc_Pair("\u1E9Cx3",(new sc_Pair((new sc_Pair("\u1E9Ccons",(new sc_Pair("\u1E9Cx4",(new sc_Pair((new sc_Pair("\u1E9Ccons",(new sc_Pair("\u1E9Cx5",(new sc_Pair((new sc_Pair("\u1E9Ccons",(new sc_Pair("\u1E9Cx6",(new sc_Pair("\u1E9Cx7",null)))))),null)))))),null)))))),null)))))),null)))))),null)))))),null)))),(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair((6),(new sc_Pair((new sc_Pair("\u1E9Clength",(new sc_Pair("\u1E9Cx7",null)))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cdifference",(new sc_Pair((new sc_Pair("\u1E9Cadd1",(new sc_Pair((new sc_Pair("\u1E9Cadd1",(new sc_Pair("\u1E9Cx",null)))),null)))),(new sc_Pair((2),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cfix",(new sc_Pair("\u1E9Cx",null)))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cquotient",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))))),(new sc_Pair((2),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Cquotient",(new sc_Pair("\u1E9Cy",(new sc_Pair((2),null)))))),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Csigma",(new sc_Pair((new sc_Pair("\u1E9Czero",null)),(new sc_Pair("\u1E9Ci",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cquotient",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Ci",(new sc_Pair((new sc_Pair("\u1E9Cadd1",(new sc_Pair("\u1E9Ci",null)))),null)))))),(new sc_Pair((2),null)))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Cadd1",(new sc_Pair("\u1E9Cy",null)))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair((new sc_Pair("\u1E9Cnumberp",(new sc_Pair("\u1E9Cy",null)))),(new sc_Pair((new sc_Pair("\u1E9Cadd1",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))),(new sc_Pair((new sc_Pair("\u1E9Cadd1",(new sc_Pair("\u1E9Cx",null)))),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cdifference",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cdifference",(new sc_Pair("\u1E9Cz",(new sc_Pair("\u1E9Cy",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cz",null)))))),null)))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair("\u1E9Cz",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cnot",(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cx",null)))))),null)))),(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cfix",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Cfix",(new sc_Pair("\u1E9Cz",null)))),null)))))),null)))))))),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cmeaning",(new sc_Pair((new sc_Pair("\u1E9Cplus-tree",(new sc_Pair((new sc_Pair("\u1E9Cdelete",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))),(new sc_Pair("\u1E9Ca",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair((new sc_Pair("\u1E9Cmember",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cdifference",(new sc_Pair((new sc_Pair("\u1E9Cmeaning",(new sc_Pair((new sc_Pair("\u1E9Cplus-tree",(new sc_Pair("\u1E9Cy",null)))),(new sc_Pair("\u1E9Ca",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cmeaning",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Ca",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cmeaning",(new sc_Pair((new sc_Pair("\u1E9Cplus-tree",(new sc_Pair("\u1E9Cy",null)))),(new sc_Pair("\u1E9Ca",null)))))),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Cadd1",(new sc_Pair("\u1E9Cy",null)))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair((new sc_Pair("\u1E9Cnumberp",(new sc_Pair("\u1E9Cy",null)))),(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cfix",(new sc_Pair("\u1E9Cx",null)))),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cnth",(new sc_Pair((new sc_Pair("\u1E9Cnil",null)),(new sc_Pair("\u1E9Ci",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair((new sc_Pair("\u1E9Czerop",(new sc_Pair("\u1E9Ci",null)))),(new sc_Pair((new sc_Pair("\u1E9Cnil",null)),(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Clast",(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Cb",null)))))),null)))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair((new sc_Pair("\u1E9Clistp",(new sc_Pair("\u1E9Cb",null)))),(new sc_Pair((new sc_Pair("\u1E9Clast",(new sc_Pair("\u1E9Cb",null)))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair((new sc_Pair("\u1E9Clistp",(new sc_Pair("\u1E9Ca",null)))),(new sc_Pair((new sc_Pair("\u1E9Ccons",(new sc_Pair((new sc_Pair("\u1E9Ccar",(new sc_Pair((new sc_Pair("\u1E9Clast",(new sc_Pair("\u1E9Ca",null)))),null)))),(new sc_Pair("\u1E9Cb",null)))))),(new sc_Pair("\u1E9Cb",null)))))))),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair("\u1E9Cz",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair((new sc_Pair("\u1E9Clessp",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Ct",null)),(new sc_Pair("\u1E9Cz",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cf",null)),(new sc_Pair("\u1E9Cz",null)))))),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cassignment",(new sc_Pair("\u1E9Cx",(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Cb",null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair((new sc_Pair("\u1E9Cassignedp",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Ca",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cassignment",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Ca",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cassignment",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cb",null)))))),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Ccar",(new sc_Pair((new sc_Pair("\u1E9Cgopher",(new sc_Pair("\u1E9Cx",null)))),null)))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair((new sc_Pair("\u1E9Clistp",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Ccar",(new sc_Pair((new sc_Pair("\u1E9Cflatten",(new sc_Pair("\u1E9Cx",null)))),null)))),(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cflatten",(new sc_Pair((new sc_Pair("\u1E9Ccdr",(new sc_Pair((new sc_Pair("\u1E9Cgopher",(new sc_Pair("\u1E9Cx",null)))),null)))),null)))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair((new sc_Pair("\u1E9Clistp",(new sc_Pair("\u1E9Cx",null)))),(new sc_Pair((new sc_Pair("\u1E9Ccdr",(new sc_Pair((new sc_Pair("\u1E9Cflatten",(new sc_Pair("\u1E9Cx",null)))),null)))),(new sc_Pair((new sc_Pair("\u1E9Ccons",(new sc_Pair((new sc_Pair("\u1E9Czero",null)),(new sc_Pair((new sc_Pair("\u1E9Cnil",null)),null)))))),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cquotient",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cx",null)))))),(new sc_Pair("\u1E9Cy",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair((new sc_Pair("\u1E9Czerop",(new sc_Pair("\u1E9Cy",null)))),(new sc_Pair((new sc_Pair("\u1E9Czero",null)),(new sc_Pair((new sc_Pair("\u1E9Cfix",(new sc_Pair("\u1E9Cx",null)))),null)))))))),null)))))), (new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cget",(new sc_Pair("\u1E9Cj",(new sc_Pair((new sc_Pair("\u1E9Cset",(new sc_Pair("\u1E9Ci",(new sc_Pair("\u1E9Cval",(new sc_Pair("\u1E9Cmem",null)))))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cif",(new sc_Pair((new sc_Pair("\u1E9Ceqp",(new sc_Pair("\u1E9Cj",(new sc_Pair("\u1E9Ci",null)))))),(new sc_Pair("\u1E9Cval",(new sc_Pair((new sc_Pair("\u1E9Cget",(new sc_Pair("\u1E9Cj",(new sc_Pair("\u1E9Cmem",null)))))),null)))))))),null))))))));
    (const_nboyer = (new sc_Pair((new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cf",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Cb",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cc",(new sc_Pair((new sc_Pair("\u1E9Czero",null)),null)))))),null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cy",(new sc_Pair("\u1E9Cf",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair((new sc_Pair("\u1E9Ctimes",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Cb",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Cc",(new sc_Pair("\u1E9Cd",null)))))),null)))))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cz",(new sc_Pair("\u1E9Cf",(new sc_Pair((new sc_Pair("\u1E9Creverse",(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair((new sc_Pair("\u1E9Cappend",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Cb",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cnil",null)),null)))))),null)))),null)))))),(new sc_Pair((new sc_Pair("\u1E9Cu",(new sc_Pair("\u1E9Cequal",(new sc_Pair((new sc_Pair("\u1E9Cplus",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Cb",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cdifference",(new sc_Pair("\u1E9Cx",(new sc_Pair("\u1E9Cy",null)))))),null)))))))),(new sc_Pair((new sc_Pair("\u1E9Cw",(new sc_Pair("\u1E9Clessp",(new sc_Pair((new sc_Pair("\u1E9Cremainder",(new sc_Pair("\u1E9Ca",(new sc_Pair("\u1E9Cb",null)))))),(new sc_Pair((new sc_Pair("\u1E9Cmember",(new sc_Pair("\u1E9Ca",(new sc_Pair((new sc_Pair("\u1E9Clength",(new sc_Pair("\u1E9Cb",null)))),null)))))),null)))))))),null)))))))))));
    BgL_nboyerzd2benchmarkzd2 = function() {
        var args = null;
        for (var sc_tmp = arguments.length - 1; sc_tmp >= 0; sc_tmp--) {
            args = sc_cons(arguments[sc_tmp], args);
        }
        var n;
        return ((n = ((args === null)?(0):(args.car))), (BgL_setupzd2boyerzd2()), (BgL_runzd2benchmarkzd2(("nboyer"+(sc_number2string(n))), (1), function() {
            return (BgL_testzd2boyerzd2(n));
        }, function(rewrites) {
            if ((sc_isNumber(rewrites)))
                switch (n) {
                case (0):
                    return (rewrites===(95024));
                    break;
                case (1):
                    return (rewrites===(591777));
                    break;
                case (2):
                    return (rewrites===(1813975));
                    break;
                case (3):
                    return (rewrites===(5375678));
                    break;
                case (4):
                    return (rewrites===(16445406));
                    break;
                case (5):
                    return (rewrites===(51507739));
                    break;
                default:
                    return true;
                    break;
                }
            else
                return false;
        })));
    };
    BgL_setupzd2boyerzd2 = function() {
        return true;
    };
    BgL_testzd2boyerzd2 = function() {
        return true;
    };
    translate_term_nboyer = function(term) {
        var lst;
        return (!(term instanceof sc_Pair)?term:(new sc_Pair((BgL_sc_symbolzd2ze3symbolzd2record_1ze3_nboyer((term.car))), ((lst = (term.cdr)), ((lst === null)?null:(new sc_Pair((translate_term_nboyer((lst.car))), (translate_args_nboyer((lst.cdr))))))))));
    };
    translate_args_nboyer = function(lst) {
        var sc_lst_5;
        var term;
        return ((lst === null)?null:(new sc_Pair(((term = (lst.car)), (!(term instanceof sc_Pair)?term:(new sc_Pair((BgL_sc_symbolzd2ze3symbolzd2record_1ze3_nboyer((term.car))), (translate_args_nboyer((term.cdr))))))), ((sc_lst_5 = (lst.cdr)), ((sc_lst_5 === null)?null:(new sc_Pair((translate_term_nboyer((sc_lst_5.car))), (translate_args_nboyer((sc_lst_5.cdr))))))))));
    };
    untranslate_term_nboyer = function(term) {
        var optrOpnd;
        var tail1131;
        var L1127;
        var falseHead1130;
        var symbol_record;
        if (!(term instanceof sc_Pair))
            return term;
        else
            {
                (falseHead1130 = (new sc_Pair(null, null)));
                (L1127 = (term.cdr));
                (tail1131 = falseHead1130);
                while (!(L1127 === null)) {
                    {
                        (tail1131.cdr = (new sc_Pair((untranslate_term_nboyer((L1127.car))), null)));
                        (tail1131 = (tail1131.cdr));
                        (L1127 = (L1127.cdr));
                    }
                }
                (optrOpnd = (falseHead1130.cdr));
                return (new sc_Pair(((symbol_record = (term.car)), (symbol_record[(0)])), optrOpnd));
            }
    };
    BgL_sc_symbolzd2ze3symbolzd2record_1ze3_nboyer = function(sym) {
        var r;
        var x;
        return ((x = (sc_assq(sym, BgL_sc_za2symbolzd2recordszd2alistza2_2z00_nboyer))), ((x!== false)?(x.cdr):((r = [sym, null]), (BgL_sc_za2symbolzd2recordszd2alistza2_2z00_nboyer = (new sc_Pair((new sc_Pair(sym, r)), BgL_sc_za2symbolzd2recordszd2alistza2_2z00_nboyer))), r)));
    };
    (BgL_sc_za2symbolzd2recordszd2alistza2_2z00_nboyer = null);
    translate_alist_nboyer = function(alist) {
        var sc_alist_6;
        var term;
        return ((alist === null)?null:(new sc_Pair((new sc_Pair((alist.car.car), ((term = (alist.car.cdr)), (!(term instanceof sc_Pair)?term:(new sc_Pair((BgL_sc_symbolzd2ze3symbolzd2record_1ze3_nboyer((term.car))), (translate_args_nboyer((term.cdr))))))))), ((sc_alist_6 = (alist.cdr)), ((sc_alist_6 === null)?null:(new sc_Pair((new sc_Pair((sc_alist_6.car.car), (translate_term_nboyer((sc_alist_6.car.cdr))))), (translate_alist_nboyer((sc_alist_6.cdr))))))))));
    };
    apply_subst_nboyer = function(alist, term) {
        var lst;
        var temp_temp;
        return (!(term instanceof sc_Pair)?((temp_temp = (sc_assq(term, alist))), ((temp_temp!== false)?(temp_temp.cdr):term)):(new sc_Pair((term.car), ((lst = (term.cdr)), ((lst === null)?null:(new sc_Pair((apply_subst_nboyer(alist, (lst.car))), (apply_subst_lst_nboyer(alist, (lst.cdr))))))))));
    };
    apply_subst_lst_nboyer = function(alist, lst) {
        var sc_lst_7;
        return ((lst === null)?null:(new sc_Pair((apply_subst_nboyer(alist, (lst.car))), ((sc_lst_7 = (lst.cdr)), ((sc_lst_7 === null)?null:(new sc_Pair((apply_subst_nboyer(alist, (sc_lst_7.car))), (apply_subst_lst_nboyer(alist, (sc_lst_7.cdr))))))))));
    };
    tautologyp_nboyer = function(sc_x_11, true_lst, false_lst) {
        var tmp1125;
        var x;
        var tmp1126;
        var sc_x_8;
        var sc_tmp1125_9;
        var sc_tmp1126_10;
        var sc_x_11;
        var true_lst;
        var false_lst;
        while (true) {
            if ((((sc_tmp1126_10 = (is_term_equal_nboyer(sc_x_11, true_term_nboyer))), ((sc_tmp1126_10!== false)?sc_tmp1126_10:(is_term_member_nboyer(sc_x_11, true_lst))))!== false))
                return true;
            else
                if ((((sc_tmp1125_9 = (is_term_equal_nboyer(sc_x_11, false_term_nboyer))), ((sc_tmp1125_9!== false)?sc_tmp1125_9:(is_term_member_nboyer(sc_x_11, false_lst))))!== false))
                    return false;
                else
                    if (!(sc_x_11 instanceof sc_Pair))
                        return false;
                    else
                        if (((sc_x_11.car)===if_constructor_nboyer))
                            if ((((sc_x_8 = (sc_x_11.cdr.car)), (tmp1126 = (is_term_equal_nboyer(sc_x_8, true_term_nboyer))), ((tmp1126!== false)?tmp1126:(is_term_member_nboyer(sc_x_8, true_lst))))!== false))
                                (sc_x_11 = (sc_x_11.cdr.cdr.car));
                            else
                                if ((((x = (sc_x_11.cdr.car)), (tmp1125 = (is_term_equal_nboyer(x, false_term_nboyer))), ((tmp1125!== false)?tmp1125:(is_term_member_nboyer(x, false_lst))))!== false))
                                    (sc_x_11 = (sc_x_11.cdr.cdr.cdr.car));
                                else
                                    if (((tautologyp_nboyer((sc_x_11.cdr.cdr.car), (new sc_Pair((sc_x_11.cdr.car), true_lst)), false_lst))!== false))
                                        {
                                            (false_lst = (new sc_Pair((sc_x_11.cdr.car), false_lst)));
                                            (sc_x_11 = (sc_x_11.cdr.cdr.cdr.car));
                                        }
                                    else
                                        return false;
                        else
                            return false;
        }
    };
    (if_constructor_nboyer = "\u1E9C*");
    (rewrite_count_nboyer = (0));
    rewrite_nboyer = function(term) {
        var term2;
        var sc_term_12;
        var lst;
        var symbol_record;
        var sc_lst_13;
        {
            (++rewrite_count_nboyer);
            if (!(term instanceof sc_Pair))
                return term;
            else
                {
                    (sc_term_12 = (new sc_Pair((term.car), ((sc_lst_13 = (term.cdr)), ((sc_lst_13 === null)?null:(new sc_Pair((rewrite_nboyer((sc_lst_13.car))), (rewrite_args_nboyer((sc_lst_13.cdr))))))))));
                    (lst = ((symbol_record = (term.car)), (symbol_record[(1)])));
                    while (true) {
                        if ((lst === null))
                            return sc_term_12;
                        else
                            if ((((term2 = ((lst.car).cdr.car)), (unify_subst_nboyer = null), (one_way_unify1_nboyer(sc_term_12, term2)))!== false))
                                return (rewrite_nboyer((apply_subst_nboyer(unify_subst_nboyer, ((lst.car).cdr.cdr.car)))));
                            else
                                (lst = (lst.cdr));
                    }
                }
        }
    };
    rewrite_args_nboyer = function(lst) {
        var sc_lst_14;
        return ((lst === null)?null:(new sc_Pair((rewrite_nboyer((lst.car))), ((sc_lst_14 = (lst.cdr)), ((sc_lst_14 === null)?null:(new sc_Pair((rewrite_nboyer((sc_lst_14.car))), (rewrite_args_nboyer((sc_lst_14.cdr))))))))));
    };
    (unify_subst_nboyer = "\u1E9C*");
    one_way_unify1_nboyer = function(term1, term2) {
        var lst1;
        var lst2;
        var temp_temp;
        if (!(term2 instanceof sc_Pair))
            {
                (temp_temp = (sc_assq(term2, unify_subst_nboyer)));
                if ((temp_temp!== false))
                    return (is_term_equal_nboyer(term1, (temp_temp.cdr)));
                else
                    if ((sc_isNumber(term2)))
                        return (sc_isEqual(term1, term2));
                    else
                        {
                            (unify_subst_nboyer = (new sc_Pair((new sc_Pair(term2, term1)), unify_subst_nboyer)));
                            return true;
                        }
            }
        else
            if (!(term1 instanceof sc_Pair))
                return false;
            else
                if (((term1.car)===(term2.car)))
                    {
                        (lst1 = (term1.cdr));
                        (lst2 = (term2.cdr));
                        while (true) {
                            if ((lst1 === null))
                                return (lst2 === null);
                            else
                                if ((lst2 === null))
                                    return false;
                                else
                                    if (((one_way_unify1_nboyer((lst1.car), (lst2.car)))!== false))
                                        {
                                            (lst1 = (lst1.cdr));
                                            (lst2 = (lst2.cdr));
                                        }
                                    else
                                        return false;
                        }
                    }
                else
                    return false;
    };
    (false_term_nboyer = "\u1E9C*");
    (true_term_nboyer = "\u1E9C*");
    trans_of_implies1_nboyer = function(n) {
        var sc_n_15;
        return ((sc_isEqual(n, (1)))?(sc_list("\u1E9Cimplies", (0), (1))):(sc_list("\u1E9Cand", (sc_list("\u1E9Cimplies", (n-(1)), n)), ((sc_n_15 = (n-(1))), ((sc_isEqual(sc_n_15, (1)))?(sc_list("\u1E9Cimplies", (0), (1))):(sc_list("\u1E9Cand", (sc_list("\u1E9Cimplies", (sc_n_15-(1)), sc_n_15)), (trans_of_implies1_nboyer((sc_n_15-(1)))))))))));
    };
    is_term_equal_nboyer = function(x, y) {
        var lst1;
        var lst2;
        var r2;
        var r1;
        if ((x instanceof sc_Pair))
            if ((y instanceof sc_Pair))
                if ((((r1 = (x.car)), (r2 = (y.car)), (r1===r2))!== false))
                    {
                        (lst1 = (x.cdr));
                        (lst2 = (y.cdr));
                        while (true) {
                            if ((lst1 === null))
                                return (lst2 === null);
                            else
                                if ((lst2 === null))
                                    return false;
                                else
                                    if (((is_term_equal_nboyer((lst1.car), (lst2.car)))!== false))
                                        {
                                            (lst1 = (lst1.cdr));
                                            (lst2 = (lst2.cdr));
                                        }
                                    else
                                        return false;
                        }
                    }
                else
                    return false;
            else
                return false;
        else
            return (sc_isEqual(x, y));
    };
    is_term_member_nboyer = function(x, lst) {
        var x;
        var lst;
        while (true) {
            if ((lst === null))
                return false;
            else
                if (((is_term_equal_nboyer(x, (lst.car)))!== false))
                    return true;
                else
                    (lst = (lst.cdr));
        }
    };
    BgL_setupzd2boyerzd2 = function() {
        var symbol_record;
        var value;
        var BgL_sc_symbolzd2record_16zd2;
        var sym;
        var sc_sym_17;
        var term;
        var lst;
        var sc_term_18;
        var sc_term_19;
        {
            (BgL_sc_za2symbolzd2recordszd2alistza2_2z00_nboyer = null);
            (if_constructor_nboyer = (BgL_sc_symbolzd2ze3symbolzd2record_1ze3_nboyer("\u1E9Cif")));
            (false_term_nboyer = ((sc_term_19 = (new sc_Pair("\u1E9Cf",null))), (!(sc_term_19 instanceof sc_Pair)?sc_term_19:(new sc_Pair((BgL_sc_symbolzd2ze3symbolzd2record_1ze3_nboyer((sc_term_19.car))), (translate_args_nboyer((sc_term_19.cdr))))))));
            (true_term_nboyer = ((sc_term_18 = (new sc_Pair("\u1E9Ct",null))), (!(sc_term_18 instanceof sc_Pair)?sc_term_18:(new sc_Pair((BgL_sc_symbolzd2ze3symbolzd2record_1ze3_nboyer((sc_term_18.car))), (translate_args_nboyer((sc_term_18.cdr))))))));
            (lst = sc_const_3_nboyer);
            while (!(lst === null)) {
                {
                    (term = (lst.car));
                    if (((term instanceof sc_Pair)&&(((term.car)==="\u1E9Cequal")&&((term.cdr.car) instanceof sc_Pair))))
                        {
                            (sc_sym_17 = ((term.cdr.car).car));
                            (value = (new sc_Pair((!(term instanceof sc_Pair)?term:(new sc_Pair((BgL_sc_symbolzd2ze3symbolzd2record_1ze3_nboyer((term.car))), (translate_args_nboyer((term.cdr)))))), ((sym = ((term.cdr.car).car)), (BgL_sc_symbolzd2record_16zd2 = (BgL_sc_symbolzd2ze3symbolzd2record_1ze3_nboyer(sym))), (BgL_sc_symbolzd2record_16zd2[(1)])))));
                            (symbol_record = (BgL_sc_symbolzd2ze3symbolzd2record_1ze3_nboyer(sc_sym_17)));
                            (symbol_record[(1)] = value);
                        }
                    else
                        (sc_error("ADD-LEMMA did not like term:  ", term));
                    (lst = (lst.cdr));
                }
            }
            return true;
        }
    };
    BgL_testzd2boyerzd2 = function(n) {
        var optrOpnd;
        var term;
        var sc_n_20;
        var answer;
        var sc_term_21;
        var sc_term_22;
        {
            (rewrite_count_nboyer = (0));
            (term = sc_const_4_nboyer);
            (sc_n_20 = n);
            while (!(sc_n_20=== 0)) {
                {
                    (term = (sc_list("\u1E9Cor", term, (new sc_Pair("\u1E9Cf",null)))));
                    (--sc_n_20);
                }
            }
            (sc_term_22 = term);
            if (!(sc_term_22 instanceof sc_Pair))
                (optrOpnd = sc_term_22);
            else
                (optrOpnd = (new sc_Pair((BgL_sc_symbolzd2ze3symbolzd2record_1ze3_nboyer((sc_term_22.car))), (translate_args_nboyer((sc_term_22.cdr))))));
            (sc_term_21 = (apply_subst_nboyer(((const_nboyer === null)?null:(new sc_Pair((new sc_Pair((const_nboyer.car.car), (translate_term_nboyer((const_nboyer.car.cdr))))), (translate_alist_nboyer((const_nboyer.cdr)))))), optrOpnd)));
            (answer = (tautologyp_nboyer((rewrite_nboyer(sc_term_21)), null, null)));
            (sc_write(rewrite_count_nboyer));
            (sc_display(" rewrites"));
            (sc_newline());
            if ((answer!== false))
                return rewrite_count_nboyer;
            else
                return false;
        }
    };
}
/* Exported Variables */
var BgL_parsezd2ze3nbzd2treesze3;
var BgL_earleyzd2benchmarkzd2;
var BgL_parsezd2ze3parsedzf3zc2;
var test;
var BgL_parsezd2ze3treesz31;
var BgL_makezd2parserzd2;
/* End Exports */

var const_earley;
{
    (const_earley = (new sc_Pair((new sc_Pair("\u1E9Cs",(new sc_Pair((new sc_Pair("\u1E9Ca",null)),(new sc_Pair((new sc_Pair("\u1E9Cs",(new sc_Pair("\u1E9Cs",null)))),null)))))),null)));
    BgL_makezd2parserzd2 = function(grammar, lexer) {
        var i;
        var parser_descr;
        var def_loop;
        var nb_nts;
        var names;
        var steps;
        var predictors;
        var enders;
        var starters;
        var nts;
        var sc_names_1;
        var sc_steps_2;
        var sc_predictors_3;
        var sc_enders_4;
        var sc_starters_5;
        var nb_confs;
        var BgL_sc_defzd2loop_6zd2;
        var BgL_sc_nbzd2nts_7zd2;
        var sc_nts_8;
        var BgL_sc_defzd2loop_9zd2;
        var ind;
        {
            ind = function(nt, sc_nts_10) {
                var i;
                {
                    (i = ((sc_nts_10.length)-(1)));
                    while (true) {
                        if ((i>=(0)))
                            if ((sc_isEqual((sc_nts_10[i]), nt)))
                                return i;
                            else
                                (--i);
                        else
                            return false;
                    }
                }
            };
            (sc_nts_8 = ((BgL_sc_defzd2loop_9zd2 = function(defs, sc_nts_11) {
                var rule_loop;
                var head;
                var def;
                return ((defs instanceof sc_Pair)?((def = (defs.car)), (head = (def.car)), (rule_loop = function(rules, sc_nts_12) {
                    var nt;
                    var l;
                    var sc_nts_13;
                    var rule;
                    if ((rules instanceof sc_Pair))
                        {
                            (rule = (rules.car));
                            (l = rule);
                            (sc_nts_13 = sc_nts_12);
                            while ((l instanceof sc_Pair)) {
                                {
                                    (nt = (l.car));
                                    (l = (l.cdr));
                                    (sc_nts_13 = (((sc_member(nt, sc_nts_13))!== false)?sc_nts_13:(new sc_Pair(nt, sc_nts_13))));
                                }
                            }
                            return (rule_loop((rules.cdr), sc_nts_13));
                        }
                    else
                        return (BgL_sc_defzd2loop_9zd2((defs.cdr), sc_nts_12));
                }), (rule_loop((def.cdr), (((sc_member(head, sc_nts_11))!== false)?sc_nts_11:(new sc_Pair(head, sc_nts_11)))))):(sc_list2vector((sc_reverse(sc_nts_11)))));
            }), (BgL_sc_defzd2loop_9zd2(grammar, null))));
            (BgL_sc_nbzd2nts_7zd2 = (sc_nts_8.length));
            (nb_confs = (((BgL_sc_defzd2loop_6zd2 = function(defs, BgL_sc_nbzd2confs_14zd2) {
                var rule_loop;
                var def;
                return ((defs instanceof sc_Pair)?((def = (defs.car)), (rule_loop = function(rules, BgL_sc_nbzd2confs_15zd2) {
                    var l;
                    var BgL_sc_nbzd2confs_16zd2;
                    var rule;
                    if ((rules instanceof sc_Pair))
                        {
                            (rule = (rules.car));
                            (l = rule);
                            (BgL_sc_nbzd2confs_16zd2 = BgL_sc_nbzd2confs_15zd2);
                            while ((l instanceof sc_Pair)) {
                                {
                                    (l = (l.cdr));
                                    (++BgL_sc_nbzd2confs_16zd2);
                                }
                            }
                            return (rule_loop((rules.cdr), (BgL_sc_nbzd2confs_16zd2+(1))));
                        }
                    else
                        return (BgL_sc_defzd2loop_6zd2((defs.cdr), BgL_sc_nbzd2confs_15zd2));
                }), (rule_loop((def.cdr), BgL_sc_nbzd2confs_14zd2))):BgL_sc_nbzd2confs_14zd2);
            }), (BgL_sc_defzd2loop_6zd2(grammar, (0))))+BgL_sc_nbzd2nts_7zd2));
            (sc_starters_5 = (sc_makeVector(BgL_sc_nbzd2nts_7zd2, null)));
            (sc_enders_4 = (sc_makeVector(BgL_sc_nbzd2nts_7zd2, null)));
            (sc_predictors_3 = (sc_makeVector(BgL_sc_nbzd2nts_7zd2, null)));
            (sc_steps_2 = (sc_makeVector(nb_confs, false)));
            (sc_names_1 = (sc_makeVector(nb_confs, false)));
            (nts = sc_nts_8);
            (starters = sc_starters_5);
            (enders = sc_enders_4);
            (predictors = sc_predictors_3);
            (steps = sc_steps_2);
            (names = sc_names_1);
            (nb_nts = (sc_nts_8.length));
            (i = (nb_nts-(1)));
            while ((i>=(0))) {
                {
                    (sc_steps_2[i] = (i-nb_nts));
                    (sc_names_1[i] = (sc_list((sc_nts_8[i]), (0))));
                    (sc_enders_4[i] = (sc_list(i)));
                    (--i);
                }
            }
            def_loop = function(defs, conf) {
                var rule_loop;
                var head;
                var def;
                return ((defs instanceof sc_Pair)?((def = (defs.car)), (head = (def.car)), (rule_loop = function(rules, conf, rule_num) {
                    var i;
                    var sc_i_17;
                    var nt;
                    var l;
                    var sc_conf_18;
                    var sc_i_19;
                    var rule;
                    if ((rules instanceof sc_Pair))
                        {
                            (rule = (rules.car));
                            (names[conf] = (sc_list(head, rule_num)));
                            (sc_i_19 = (ind(head, nts)));
                            (starters[sc_i_19] = (new sc_Pair(conf, (starters[sc_i_19]))));
                            (l = rule);
                            (sc_conf_18 = conf);
                            while ((l instanceof sc_Pair)) {
                                {
                                    (nt = (l.car));
                                    (steps[sc_conf_18] = (ind(nt, nts)));
                                    (sc_i_17 = (ind(nt, nts)));
                                    (predictors[sc_i_17] = (new sc_Pair(sc_conf_18, (predictors[sc_i_17]))));
                                    (l = (l.cdr));
                                    (++sc_conf_18);
                                }
                            }
                            (steps[sc_conf_18] = ((ind(head, nts))-nb_nts));
                            (i = (ind(head, nts)));
                            (enders[i] = (new sc_Pair(sc_conf_18, (enders[i]))));
                            return (rule_loop((rules.cdr), (sc_conf_18+(1)), (rule_num+(1))));
                        }
                    else
                        return (def_loop((defs.cdr), conf));
                }), (rule_loop((def.cdr), conf, (1)))):undefined);
            };
            (def_loop(grammar, (sc_nts_8.length)));
            (parser_descr = [lexer, sc_nts_8, sc_starters_5, sc_enders_4, sc_predictors_3, sc_steps_2, sc_names_1]);
            return function(input) {
                var optrOpnd;
                var sc_optrOpnd_20;
                var sc_optrOpnd_21;
                var sc_optrOpnd_22;
                var loop1;
                var BgL_sc_stateza2_23za2;
                var toks;
                var BgL_sc_nbzd2nts_24zd2;
                var sc_steps_25;
                var sc_enders_26;
                var state_num;
                var BgL_sc_statesza2_27za2;
                var states;
                var i;
                var conf;
                var l;
                var tok_nts;
                var sc_i_28;
                var sc_i_29;
                var l1;
                var l2;
                var tok;
                var tail1129;
                var L1125;
                var goal_enders;
                var BgL_sc_statesza2_30za2;
                var BgL_sc_nbzd2nts_31zd2;
                var BgL_sc_nbzd2confs_32zd2;
                var nb_toks;
                var goal_starters;
                var sc_states_33;
                var BgL_sc_nbzd2confs_34zd2;
                var BgL_sc_nbzd2toks_35zd2;
                var sc_toks_36;
                var falseHead1128;
                var sc_names_37;
                var sc_steps_38;
                var sc_predictors_39;
                var sc_enders_40;
                var sc_starters_41;
                var sc_nts_42;
                var lexer;
                var sc_ind_43;
                var make_states;
                var BgL_sc_confzd2setzd2getza2_44za2;
                var conf_set_merge_new_bang;
                var conf_set_adjoin;
                var BgL_sc_confzd2setzd2adjoinza2_45za2;
                var BgL_sc_confzd2setzd2adjoinza2za2_46z00;
                var conf_set_union;
                var forw;
                var is_parsed;
                var deriv_trees;
                var BgL_sc_derivzd2treesza2_47z70;
                var nb_deriv_trees;
                var BgL_sc_nbzd2derivzd2treesza2_48za2;
                {
                    sc_ind_43 = function(nt, sc_nts_49) {
                        var i;
                        {
                            (i = ((sc_nts_49.length)-(1)));
                            while (true) {
                                if ((i>=(0)))
                                    if ((sc_isEqual((sc_nts_49[i]), nt)))
                                        return i;
                                    else
                                        (--i);
                                else
                                    return false;
                            }
                        }
                    };
                    make_states = function(BgL_sc_nbzd2toks_50zd2, BgL_sc_nbzd2confs_51zd2) {
                        var v;
                        var i;
                        var sc_states_52;
                        {
                            (sc_states_52 = (sc_makeVector((BgL_sc_nbzd2toks_50zd2+(1)), false)));
                            (i = BgL_sc_nbzd2toks_50zd2);
                            while ((i>=(0))) {
                                {
                                    (v = (sc_makeVector((BgL_sc_nbzd2confs_51zd2+(1)), false)));
                                    (v[(0)] = (-1));
                                    (sc_states_52[i] = v);
                                    (--i);
                                }
                            }
                            return sc_states_52;
                        }
                    };
                    BgL_sc_confzd2setzd2getza2_44za2 = function(state, BgL_sc_statezd2num_53zd2, sc_conf_54) {
                        var conf_set;
                        var BgL_sc_confzd2set_55zd2;
                        return ((BgL_sc_confzd2set_55zd2 = (state[(sc_conf_54+(1))])), ((BgL_sc_confzd2set_55zd2!== false)?BgL_sc_confzd2set_55zd2:((conf_set = (sc_makeVector((BgL_sc_statezd2num_53zd2+(6)), false))), (conf_set[(1)] = (-3)), (conf_set[(2)] = (-1)), (conf_set[(3)] = (-1)), (conf_set[(4)] = (-1)), (state[(sc_conf_54+(1))] = conf_set), conf_set)));
                    };
                    conf_set_merge_new_bang = function(conf_set) {
                        return ((conf_set[((conf_set[(1)])+(5))] = (conf_set[(4)])), (conf_set[(1)] = (conf_set[(3)])), (conf_set[(3)] = (-1)), (conf_set[(4)] = (-1)));
                    };
                    conf_set_adjoin = function(state, conf_set, sc_conf_56, i) {
                        var tail;
                        return ((tail = (conf_set[(3)])), (conf_set[(i+(5))] = (-1)), (conf_set[(tail+(5))] = i), (conf_set[(3)] = i), ((tail<(0))?((conf_set[(0)] = (state[(0)])), (state[(0)] = sc_conf_56)):undefined));
                    };
                    BgL_sc_confzd2setzd2adjoinza2_45za2 = function(sc_states_57, BgL_sc_statezd2num_58zd2, l, i) {
                        var conf_set;
                        var sc_conf_59;
                        var l1;
                        var state;
                        {
                            (state = (sc_states_57[BgL_sc_statezd2num_58zd2]));
                            (l1 = l);
                            while ((l1 instanceof sc_Pair)) {
                                {
                                    (sc_conf_59 = (l1.car));
                                    (conf_set = (BgL_sc_confzd2setzd2getza2_44za2(state, BgL_sc_statezd2num_58zd2, sc_conf_59)));
                                    if (((conf_set[(i+(5))])=== false))
                                        {
                                            (conf_set_adjoin(state, conf_set, sc_conf_59, i));
                                            (l1 = (l1.cdr));
                                        }
                                    else
                                        (l1 = (l1.cdr));
                                }
                            }
                            return undefined;
                        }
                    };
                    BgL_sc_confzd2setzd2adjoinza2za2_46z00 = function(sc_states_60, BgL_sc_statesza2_61za2, BgL_sc_statezd2num_62zd2, sc_conf_63, i) {
                        var BgL_sc_confzd2setza2_64z70;
                        var BgL_sc_stateza2_65za2;
                        var conf_set;
                        var state;
                        return ((state = (sc_states_60[BgL_sc_statezd2num_62zd2])), ((((conf_set = (state[(sc_conf_63+(1))])), ((conf_set!== false)?(conf_set[(i+(5))]):false))!== false)?((BgL_sc_stateza2_65za2 = (BgL_sc_statesza2_61za2[BgL_sc_statezd2num_62zd2])), (BgL_sc_confzd2setza2_64z70 = (BgL_sc_confzd2setzd2getza2_44za2(BgL_sc_stateza2_65za2, BgL_sc_statezd2num_62zd2, sc_conf_63))), (((BgL_sc_confzd2setza2_64z70[(i+(5))])=== false)?(conf_set_adjoin(BgL_sc_stateza2_65za2, BgL_sc_confzd2setza2_64z70, sc_conf_63, i)):undefined), true):false));
                    };
                    conf_set_union = function(state, conf_set, sc_conf_66, other_set) {
                        var i;
                        {
                            (i = (other_set[(2)]));
                            while ((i>=(0))) {
                                if (((conf_set[(i+(5))])=== false))
                                    {
                                        (conf_set_adjoin(state, conf_set, sc_conf_66, i));
                                        (i = (other_set[(i+(5))]));
                                    }
                                else
                                    (i = (other_set[(i+(5))]));
                            }
                            return undefined;
                        }
                    };
                    forw = function(sc_states_67, BgL_sc_statezd2num_68zd2, sc_starters_69, sc_enders_70, sc_predictors_71, sc_steps_72, sc_nts_73) {
                        var next_set;
                        var next;
                        var conf_set;
                        var ender;
                        var l;
                        var starter_set;
                        var starter;
                        var sc_l_74;
                        var sc_loop1_75;
                        var head;
                        var BgL_sc_confzd2set_76zd2;
                        var BgL_sc_statezd2num_77zd2;
                        var state;
                        var sc_states_78;
                        var preds;
                        var BgL_sc_confzd2set_79zd2;
                        var step;
                        var sc_conf_80;
                        var BgL_sc_nbzd2nts_81zd2;
                        var sc_state_82;
                        {
                            (sc_state_82 = (sc_states_67[BgL_sc_statezd2num_68zd2]));
                            (BgL_sc_nbzd2nts_81zd2 = (sc_nts_73.length));
                            while (true) {
                                {
                                    (sc_conf_80 = (sc_state_82[(0)]));
                                    if ((sc_conf_80>=(0)))
                                        {
                                            (step = (sc_steps_72[sc_conf_80]));
                                            (BgL_sc_confzd2set_79zd2 = (sc_state_82[(sc_conf_80+(1))]));
                                            (head = (BgL_sc_confzd2set_79zd2[(4)]));
                                            (sc_state_82[(0)] = (BgL_sc_confzd2set_79zd2[(0)]));
                                            (conf_set_merge_new_bang(BgL_sc_confzd2set_79zd2));
                                            if ((step>=(0)))
                                                {
                                                    (sc_l_74 = (sc_starters_69[step]));
                                                    while ((sc_l_74 instanceof sc_Pair)) {
                                                        {
                                                            (starter = (sc_l_74.car));
                                                            (starter_set = (BgL_sc_confzd2setzd2getza2_44za2(sc_state_82, BgL_sc_statezd2num_68zd2, starter)));
                                                            if (((starter_set[(BgL_sc_statezd2num_68zd2+(5))])=== false))
                                                                {
                                                                    (conf_set_adjoin(sc_state_82, starter_set, starter, BgL_sc_statezd2num_68zd2));
                                                                    (sc_l_74 = (sc_l_74.cdr));
                                                                }
                                                            else
                                                                (sc_l_74 = (sc_l_74.cdr));
                                                        }
                                                    }
                                                    (l = (sc_enders_70[step]));
                                                    while ((l instanceof sc_Pair)) {
                                                        {
                                                            (ender = (l.car));
                                                            if ((((conf_set = (sc_state_82[(ender+(1))])), ((conf_set!== false)?(conf_set[(BgL_sc_statezd2num_68zd2+(5))]):false))!== false))
                                                                {
                                                                    (next = (sc_conf_80+(1)));
                                                                    (next_set = (BgL_sc_confzd2setzd2getza2_44za2(sc_state_82, BgL_sc_statezd2num_68zd2, next)));
                                                                    (conf_set_union(sc_state_82, next_set, next, BgL_sc_confzd2set_79zd2));
                                                                    (l = (l.cdr));
                                                                }
                                                            else
                                                                (l = (l.cdr));
                                                        }
                                                    }
                                                }
                                            else
                                                {
                                                    (preds = (sc_predictors_71[(step+BgL_sc_nbzd2nts_81zd2)]));
                                                    (sc_states_78 = sc_states_67);
                                                    (state = sc_state_82);
                                                    (BgL_sc_statezd2num_77zd2 = BgL_sc_statezd2num_68zd2);
                                                    (BgL_sc_confzd2set_76zd2 = BgL_sc_confzd2set_79zd2);
                                                    sc_loop1_75 = function(l) {
                                                        var sc_state_83;
                                                        var BgL_sc_nextzd2set_84zd2;
                                                        var sc_next_85;
                                                        var pred_set;
                                                        var i;
                                                        var pred;
                                                        if ((l instanceof sc_Pair))
                                                            {
                                                                (pred = (l.car));
                                                                (i = head);
                                                                while ((i>=(0))) {
                                                                    {
                                                                        (pred_set = ((sc_state_83 = (sc_states_78[i])), (sc_state_83[(pred+(1))])));
                                                                        if ((pred_set!== false))
                                                                            {
                                                                                (sc_next_85 = (pred+(1)));
                                                                                (BgL_sc_nextzd2set_84zd2 = (BgL_sc_confzd2setzd2getza2_44za2(state, BgL_sc_statezd2num_77zd2, sc_next_85)));
                                                                                (conf_set_union(state, BgL_sc_nextzd2set_84zd2, sc_next_85, pred_set));
                                                                            }
                                                                        (i = (BgL_sc_confzd2set_76zd2[(i+(5))]));
                                                                    }
                                                                }
                                                                return (sc_loop1_75((l.cdr)));
                                                            }
                                                        else
                                                            return undefined;
                                                    };
                                                    (sc_loop1_75(preds));
                                                }
                                        }
                                    else
                                        return undefined;
                                }
                            }
                        }
                    };
                    is_parsed = function(nt, i, j, sc_nts_86, sc_enders_87, sc_states_88) {
                        var conf_set;
                        var state;
                        var sc_conf_89;
                        var l;
                        var BgL_sc_ntza2_90za2;
                        {
                            (BgL_sc_ntza2_90za2 = (sc_ind_43(nt, sc_nts_86)));
                            if ((BgL_sc_ntza2_90za2!== false))
                                {
                                    (sc_nts_86.length);
                                    (l = (sc_enders_87[BgL_sc_ntza2_90za2]));
                                    while (true) {
                                        if ((l instanceof sc_Pair))
                                            {
                                                (sc_conf_89 = (l.car));
                                                if ((((state = (sc_states_88[j])), (conf_set = (state[(sc_conf_89+(1))])), ((conf_set!== false)?(conf_set[(i+(5))]):false))!== false))
                                                    return true;
                                                else
                                                    (l = (l.cdr));
                                            }
                                        else
                                            return false;
                                    }
                                }
                            else
                                return false;
                        }
                    };
                    deriv_trees = function(sc_conf_91, i, j, sc_enders_92, sc_steps_93, sc_names_94, sc_toks_95, sc_states_96, BgL_sc_nbzd2nts_97zd2) {
                        var sc_loop1_98;
                        var prev;
                        var name;
                        return ((name = (sc_names_94[sc_conf_91])), ((name!== false)?((sc_conf_91<BgL_sc_nbzd2nts_97zd2)?(sc_list((sc_list(name, ((sc_toks_95[i]).car))))):(sc_list((sc_list(name))))):((prev = (sc_conf_91-(1))), (sc_loop1_98 = function(l1, l2) {
                            var loop2;
                            var ender_set;
                            var state;
                            var ender;
                            var l1;
                            var l2;
                            while (true) {
                                if ((l1 instanceof sc_Pair))
                                    {
                                        (ender = (l1.car));
                                        (ender_set = ((state = (sc_states_96[j])), (state[(ender+(1))])));
                                        if ((ender_set!== false))
                                            {
                                                loop2 = function(k, l2) {
                                                    var loop3;
                                                    var ender_trees;
                                                    var prev_trees;
                                                    var conf_set;
                                                    var sc_state_99;
                                                    var k;
                                                    var l2;
                                                    while (true) {
                                                        if ((k>=(0)))
                                                            if (((k>=i)&&(((sc_state_99 = (sc_states_96[k])), (conf_set = (sc_state_99[(prev+(1))])), ((conf_set!== false)?(conf_set[(i+(5))]):false))!== false)))
                                                                {
                                                                    (prev_trees = (deriv_trees(prev, i, k, sc_enders_92, sc_steps_93, sc_names_94, sc_toks_95, sc_states_96, BgL_sc_nbzd2nts_97zd2)));
                                                                    (ender_trees = (deriv_trees(ender, k, j, sc_enders_92, sc_steps_93, sc_names_94, sc_toks_95, sc_states_96, BgL_sc_nbzd2nts_97zd2)));
                                                                    loop3 = function(l3, l2) {
                                                                        var l4;
                                                                        var sc_l2_100;
                                                                        var ender_tree;
                                                                        if ((l3 instanceof sc_Pair))
                                                                            {
                                                                                (ender_tree = (sc_list((l3.car))));
                                                                                (l4 = prev_trees);
                                                                                (sc_l2_100 = l2);
                                                                                while ((l4 instanceof sc_Pair)) {
                                                                                    {
                                                                                        (sc_l2_100 = (new sc_Pair((sc_append((l4.car), ender_tree)), sc_l2_100)));
                                                                                        (l4 = (l4.cdr));
                                                                                    }
                                                                                }
                                                                                return (loop3((l3.cdr), sc_l2_100));
                                                                            }
                                                                        else
                                                                            return (loop2((ender_set[(k+(5))]), l2));
                                                                    };
                                                                    return (loop3(ender_trees, l2));
                                                                }
                                                            else
                                                                (k = (ender_set[(k+(5))]));
                                                        else
                                                            return (sc_loop1_98((l1.cdr), l2));
                                                    }
                                                };
                                                return (loop2((ender_set[(2)]), l2));
                                            }
                                        else
                                            (l1 = (l1.cdr));
                                    }
                                else
                                    return l2;
                            }
                        }), (sc_loop1_98((sc_enders_92[(sc_steps_93[prev])]), null)))));
                    };
                    BgL_sc_derivzd2treesza2_47z70 = function(nt, i, j, sc_nts_101, sc_enders_102, sc_steps_103, sc_names_104, sc_toks_105, sc_states_106) {
                        var conf_set;
                        var state;
                        var sc_conf_107;
                        var l;
                        var trees;
                        var BgL_sc_nbzd2nts_108zd2;
                        var BgL_sc_ntza2_109za2;
                        {
                            (BgL_sc_ntza2_109za2 = (sc_ind_43(nt, sc_nts_101)));
                            if ((BgL_sc_ntza2_109za2!== false))
                                {
                                    (BgL_sc_nbzd2nts_108zd2 = (sc_nts_101.length));
                                    (l = (sc_enders_102[BgL_sc_ntza2_109za2]));
                                    (trees = null);
                                    while ((l instanceof sc_Pair)) {
                                        {
                                            (sc_conf_107 = (l.car));
                                            if ((((state = (sc_states_106[j])), (conf_set = (state[(sc_conf_107+(1))])), ((conf_set!== false)?(conf_set[(i+(5))]):false))!== false))
                                                {
                                                    (l = (l.cdr));
                                                    (trees = (sc_append((deriv_trees(sc_conf_107, i, j, sc_enders_102, sc_steps_103, sc_names_104, sc_toks_105, sc_states_106, BgL_sc_nbzd2nts_108zd2)), trees)));
                                                }
                                            else
                                                (l = (l.cdr));
                                        }
                                    }
                                    return trees;
                                }
                            else
                                return false;
                        }
                    };
                    nb_deriv_trees = function(sc_conf_110, i, j, sc_enders_111, sc_steps_112, sc_toks_113, sc_states_114, BgL_sc_nbzd2nts_115zd2) {
                        var sc_loop1_116;
                        var tmp1124;
                        var prev;
                        return ((prev = (sc_conf_110-(1))), ((((tmp1124 = (sc_conf_110<BgL_sc_nbzd2nts_115zd2)), ((tmp1124!== false)?tmp1124:((sc_steps_112[prev])<(0))))!== false)?(1):((sc_loop1_116 = function(l, sc_n_118) {
                            var nb_ender_trees;
                            var nb_prev_trees;
                            var conf_set;
                            var state;
                            var k;
                            var n;
                            var ender_set;
                            var sc_state_117;
                            var ender;
                            var l;
                            var sc_n_118;
                            while (true) {
                                if ((l instanceof sc_Pair))
                                    {
                                        (ender = (l.car));
                                        (ender_set = ((sc_state_117 = (sc_states_114[j])), (sc_state_117[(ender+(1))])));
                                        if ((ender_set!== false))
                                            {
                                                (k = (ender_set[(2)]));
                                                (n = sc_n_118);
                                                while ((k>=(0))) {
                                                    if (((k>=i)&&(((state = (sc_states_114[k])), (conf_set = (state[(prev+(1))])), ((conf_set!== false)?(conf_set[(i+(5))]):false))!== false)))
                                                        {
                                                            (nb_prev_trees = (nb_deriv_trees(prev, i, k, sc_enders_111, sc_steps_112, sc_toks_113, sc_states_114, BgL_sc_nbzd2nts_115zd2)));
                                                            (nb_ender_trees = (nb_deriv_trees(ender, k, j, sc_enders_111, sc_steps_112, sc_toks_113, sc_states_114, BgL_sc_nbzd2nts_115zd2)));
                                                            (k = (ender_set[(k+(5))]));
                                                            (n +=(nb_prev_trees*nb_ender_trees));
                                                        }
                                                    else
                                                        (k = (ender_set[(k+(5))]));
                                                }
                                                return (sc_loop1_116((l.cdr), n));
                                            }
                                        else
                                            (l = (l.cdr));
                                    }
                                else
                                    return sc_n_118;
                            }
                        }), (sc_loop1_116((sc_enders_111[(sc_steps_112[prev])]), (0))))));
                    };
                    BgL_sc_nbzd2derivzd2treesza2_48za2 = function(nt, i, j, sc_nts_119, sc_enders_120, sc_steps_121, sc_toks_122, sc_states_123) {
                        var conf_set;
                        var state;
                        var sc_conf_124;
                        var l;
                        var nb_trees;
                        var BgL_sc_nbzd2nts_125zd2;
                        var BgL_sc_ntza2_126za2;
                        {
                            (BgL_sc_ntza2_126za2 = (sc_ind_43(nt, sc_nts_119)));
                            if ((BgL_sc_ntza2_126za2!== false))
                                {
                                    (BgL_sc_nbzd2nts_125zd2 = (sc_nts_119.length));
                                    (l = (sc_enders_120[BgL_sc_ntza2_126za2]));
                                    (nb_trees = (0));
                                    while ((l instanceof sc_Pair)) {
                                        {
                                            (sc_conf_124 = (l.car));
                                            if ((((state = (sc_states_123[j])), (conf_set = (state[(sc_conf_124+(1))])), ((conf_set!== false)?(conf_set[(i+(5))]):false))!== false))
                                                {
                                                    (l = (l.cdr));
                                                    (nb_trees = ((nb_deriv_trees(sc_conf_124, i, j, sc_enders_120, sc_steps_121, sc_toks_122, sc_states_123, BgL_sc_nbzd2nts_125zd2))+nb_trees));
                                                }
                                            else
                                                (l = (l.cdr));
                                        }
                                    }
                                    return nb_trees;
                                }
                            else
                                return false;
                        }
                    };
                    (lexer = (parser_descr[(0)]));
                    (sc_nts_42 = (parser_descr[(1)]));
                    (sc_starters_41 = (parser_descr[(2)]));
                    (sc_enders_40 = (parser_descr[(3)]));
                    (sc_predictors_39 = (parser_descr[(4)]));
                    (sc_steps_38 = (parser_descr[(5)]));
                    (sc_names_37 = (parser_descr[(6)]));
                    (falseHead1128 = (new sc_Pair(null, null)));
                    (L1125 = (lexer(input)));
                    (tail1129 = falseHead1128);
                    while (!(L1125 === null)) {
                        {
                            (tok = (L1125.car));
                            (l1 = (tok.cdr));
                            (l2 = null);
                            while ((l1 instanceof sc_Pair)) {
                                {
                                    (sc_i_29 = (sc_ind_43((l1.car), sc_nts_42)));
                                    if ((sc_i_29!== false))
                                        {
                                            (l1 = (l1.cdr));
                                            (l2 = (new sc_Pair(sc_i_29, l2)));
                                        }
                                    else
                                        (l1 = (l1.cdr));
                                }
                            }
                            (sc_optrOpnd_22 = (new sc_Pair((tok.car), (sc_reverse(l2)))));
                            (sc_optrOpnd_21 = (new sc_Pair(sc_optrOpnd_22, null)));
                            (tail1129.cdr = sc_optrOpnd_21);
                            (tail1129 = (tail1129.cdr));
                            (L1125 = (L1125.cdr));
                        }
                    }
                    (sc_optrOpnd_20 = (falseHead1128.cdr));
                    (sc_toks_36 = (sc_list2vector(sc_optrOpnd_20)));
                    (BgL_sc_nbzd2toks_35zd2 = (sc_toks_36.length));
                    (BgL_sc_nbzd2confs_34zd2 = (sc_steps_38.length));
                    (sc_states_33 = (make_states(BgL_sc_nbzd2toks_35zd2, BgL_sc_nbzd2confs_34zd2)));
                    (goal_starters = (sc_starters_41[(0)]));
                    (BgL_sc_confzd2setzd2adjoinza2_45za2(sc_states_33, (0), goal_starters, (0)));
                    (forw(sc_states_33, (0), sc_starters_41, sc_enders_40, sc_predictors_39, sc_steps_38, sc_nts_42));
                    (sc_i_28 = (0));
                    while ((sc_i_28<BgL_sc_nbzd2toks_35zd2)) {
                        {
                            (tok_nts = ((sc_toks_36[sc_i_28]).cdr));
                            (BgL_sc_confzd2setzd2adjoinza2_45za2(sc_states_33, (sc_i_28+(1)), tok_nts, sc_i_28));
                            (forw(sc_states_33, (sc_i_28+(1)), sc_starters_41, sc_enders_40, sc_predictors_39, sc_steps_38, sc_nts_42));
                            (++sc_i_28);
                        }
                    }
                    (nb_toks = (sc_toks_36.length));
                    (BgL_sc_nbzd2confs_32zd2 = (sc_steps_38.length));
                    (BgL_sc_nbzd2nts_31zd2 = (sc_nts_42.length));
                    (BgL_sc_statesza2_30za2 = (make_states(nb_toks, BgL_sc_nbzd2confs_32zd2)));
                    (goal_enders = (sc_enders_40[(0)]));
                    (l = goal_enders);
                    while ((l instanceof sc_Pair)) {
                        {
                            (conf = (l.car));
                            (BgL_sc_confzd2setzd2adjoinza2za2_46z00(sc_states_33, BgL_sc_statesza2_30za2, nb_toks, conf, (0)));
                            (l = (l.cdr));
                        }
                    }
                    (i = nb_toks);
                    while ((i>=(0))) {
                        {
                            (states = sc_states_33);
                            (BgL_sc_statesza2_27za2 = BgL_sc_statesza2_30za2);
                            (state_num = i);
                            (sc_enders_26 = sc_enders_40);
                            (sc_steps_25 = sc_steps_38);
                            (BgL_sc_nbzd2nts_24zd2 = BgL_sc_nbzd2nts_31zd2);
                            (toks = sc_toks_36);
                            (BgL_sc_stateza2_23za2 = (BgL_sc_statesza2_30za2[i]));
                            loop1 = function() {
                                var sc_loop1_127;
                                var prev;
                                var BgL_sc_statesza2_128za2;
                                var sc_states_129;
                                var j;
                                var i;
                                var sc_i_130;
                                var head;
                                var conf_set;
                                var sc_conf_131;
                                {
                                    (sc_conf_131 = (BgL_sc_stateza2_23za2[(0)]));
                                    if ((sc_conf_131>=(0)))
                                        {
                                            (conf_set = (BgL_sc_stateza2_23za2[(sc_conf_131+(1))]));
                                            (head = (conf_set[(4)]));
                                            (BgL_sc_stateza2_23za2[(0)] = (conf_set[(0)]));
                                            (conf_set_merge_new_bang(conf_set));
                                            (sc_i_130 = head);
                                            while ((sc_i_130>=(0))) {
                                                {
                                                    (i = sc_i_130);
                                                    (j = state_num);
                                                    (sc_states_129 = states);
                                                    (BgL_sc_statesza2_128za2 = BgL_sc_statesza2_27za2);
                                                    (prev = (sc_conf_131-(1)));
                                                    if (((sc_conf_131>=BgL_sc_nbzd2nts_24zd2)&&((sc_steps_25[prev])>=(0))))
                                                        {
                                                            sc_loop1_127 = function(l) {
                                                                var k;
                                                                var ender_set;
                                                                var state;
                                                                var ender;
                                                                var l;
                                                                while (true) {
                                                                    if ((l instanceof sc_Pair))
                                                                        {
                                                                            (ender = (l.car));
                                                                            (ender_set = ((state = (sc_states_129[j])), (state[(ender+(1))])));
                                                                            if ((ender_set!== false))
                                                                                {
                                                                                    (k = (ender_set[(2)]));
                                                                                    while ((k>=(0))) {
                                                                                        {
                                                                                            if ((k>=i))
                                                                                                if (((BgL_sc_confzd2setzd2adjoinza2za2_46z00(sc_states_129, BgL_sc_statesza2_128za2, k, prev, i))!== false))
                                                                                                    (BgL_sc_confzd2setzd2adjoinza2za2_46z00(sc_states_129, BgL_sc_statesza2_128za2, j, ender, k));
                                                                                            (k = (ender_set[(k+(5))]));
                                                                                        }
                                                                                    }
                                                                                    return (sc_loop1_127((l.cdr)));
                                                                                }
                                                                            else
                                                                                (l = (l.cdr));
                                                                        }
                                                                    else
                                                                        return undefined;
                                                                }
                                                            };
                                                            (sc_loop1_127((sc_enders_26[(sc_steps_25[prev])])));
                                                        }
                                                    (sc_i_130 = (conf_set[(sc_i_130+(5))]));
                                                }
                                            }
                                            return (loop1());
                                        }
                                    else
                                        return undefined;
                                }
                            };
                            (loop1());
                            (--i);
                        }
                    }
                    (optrOpnd = BgL_sc_statesza2_30za2);
                    return [sc_nts_42, sc_starters_41, sc_enders_40, sc_predictors_39, sc_steps_38, sc_names_37, sc_toks_36, optrOpnd, is_parsed, BgL_sc_derivzd2treesza2_47z70, BgL_sc_nbzd2derivzd2treesza2_48za2];
                }
            };
        }
    };
    BgL_parsezd2ze3parsedzf3zc2 = function(parse, nt, i, j) {
        var is_parsed;
        var states;
        var enders;
        var nts;
        return ((nts = (parse[(0)])), (enders = (parse[(2)])), (states = (parse[(7)])), (is_parsed = (parse[(8)])), (is_parsed(nt, i, j, nts, enders, states)));
    };
    BgL_parsezd2ze3treesz31 = function(parse, nt, i, j) {
        var BgL_sc_derivzd2treesza2_132z70;
        var states;
        var toks;
        var names;
        var steps;
        var enders;
        var nts;
        return ((nts = (parse[(0)])), (enders = (parse[(2)])), (steps = (parse[(4)])), (names = (parse[(5)])), (toks = (parse[(6)])), (states = (parse[(7)])), (BgL_sc_derivzd2treesza2_132z70 = (parse[(9)])), (BgL_sc_derivzd2treesza2_132z70(nt, i, j, nts, enders, steps, names, toks, states)));
    };
    BgL_parsezd2ze3nbzd2treesze3 = function(parse, nt, i, j) {
        var BgL_sc_nbzd2derivzd2treesza2_133za2;
        var states;
        var toks;
        var steps;
        var enders;
        var nts;
        return ((nts = (parse[(0)])), (enders = (parse[(2)])), (steps = (parse[(4)])), (toks = (parse[(6)])), (states = (parse[(7)])), (BgL_sc_nbzd2derivzd2treesza2_133za2 = (parse[(10)])), (BgL_sc_nbzd2derivzd2treesza2_133za2(nt, i, j, nts, enders, steps, toks, states)));
    };
    test = function(k) {
        var x;
        var p;
        return ((p = (BgL_makezd2parserzd2(const_earley, function(l) {
            var sc_x_134;
            var tail1134;
            var L1130;
            var falseHead1133;
            {
                (falseHead1133 = (new sc_Pair(null, null)));
                (tail1134 = falseHead1133);
                (L1130 = l);
                while (!(L1130 === null)) {
                    {
                        (tail1134.cdr = (new sc_Pair(((sc_x_134 = (L1130.car)), (sc_list(sc_x_134, sc_x_134))), null)));
                        (tail1134 = (tail1134.cdr));
                        (L1130 = (L1130.cdr));
                    }
                }
                return (falseHead1133.cdr);
            }
        }))), (x = (p((sc_vector2list((sc_makeVector(k, "\u1E9Ca"))))))), (sc_length((BgL_parsezd2ze3treesz31(x, "\u1E9Cs", (0), k)))));
    };
    BgL_earleyzd2benchmarkzd2 = function() {
        var args = null;
        for (var sc_tmp = arguments.length - 1; sc_tmp >= 0; sc_tmp--) {
            args = sc_cons(arguments[sc_tmp], args);
        }
        var k;
        return ((k = ((args === null)?(7):(args.car))), (BgL_runzd2benchmarkzd2("earley", (1), function() {
            return (test(k));
        }, function(result) {
            return ((sc_display(result)), (sc_newline()), result == 132);
        })));
    };
}


/************* END OF GENERATED CODE *************/
// Invoke this function to run a benchmark.
// The first argument is a string identifying the benchmark.
// The second argument is the number of times to run the benchmark.
// The third argument is a function that runs the benchmark.
// The fourth argument is a unary function that warns if the result
// returned by the benchmark is incorrect.
//
// Example:
// RunBenchmark("new Array()",
//              1,
//              function () { new Array(1000000); }
//              function (v) {
//                return (v instanceof Array) && (v.length == 1000000);
//              });

SC_DEFAULT_OUT = new sc_GenericOutputPort(function(s) {});
SC_ERROR_OUT = SC_DEFAULT_OUT;

function RunBenchmark(name, count, run, warn) {
  for (var n = 0; n < count; ++n) {
    var result = run();
    if (!warn(result)) {
      throw new Error("Earley or Boyer did incorrect number of rewrites");
    }
  }
}

var BgL_runzd2benchmarkzd2 = RunBenchmark;

var EarleyBoyer = new BenchmarkSuite('EarleyBoyer', 666463, [
  new Benchmark("Earley", function () { BgL_earleyzd2benchmarkzd2(); }),
  new Benchmark("Boyer", function () { BgL_nboyerzd2benchmarkzd2(); })
]);


// Copyright 2010 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Automatically generated on 2009-01-30. Manually updated on 2010-09-17.

// This benchmark is generated by loading 50 of the most popular pages
// on the web and logging all regexp operations performed.  Each
// operation is given a weight that is calculated from an estimate of
// the popularity of the pages where it occurs and the number of times
// it is executed while loading each page.  Furthermore the literal
// letters in the data are encoded using ROT13 in a way that does not
// affect how the regexps match their input.  Finally the strings are
// scrambled to exercise the regexp engine on different input strings.


var RegExp = new BenchmarkSuite('RegExp', 910985, [
  new Benchmark("RegExp", RegExpRun, RegExpSetup, RegExpTearDown)
]);

var regExpBenchmark = null;

function RegExpSetup() {
  regExpBenchmark = new RegExpBenchmark();
  RegExpRun(); // run once to get system initialized
}

function RegExpRun() {
  regExpBenchmark.run();
}

function RegExpTearDown() {
  regExpBenchmark = null;
}

// Returns an array of n different variants of the input string str.
// The variants are computed by randomly rotating one random
// character.
function computeInputVariants(str, n) {
  var variants = [ str ];
  for (var i = 1; i < n; i++) {
    var pos = Math.floor(Math.random() * str.length);
    var chr = String.fromCharCode((str.charCodeAt(pos) + Math.floor(Math.random() * 128)) % 128);
    variants[i] = str.substring(0, pos) + chr + str.substring(pos + 1, str.length);
  }
  return variants;
}

function RegExpBenchmark() {
  var re0 = /^ba/;
  var re1 = /(((\w+):\/\/)([^\/:]*)(:(\d+))?)?([^#?]*)(\?([^#]*))?(#(.*))?/;
  var re2 = /^\s*|\s*$/g;
  var re3 = /\bQBZPbageby_cynprubyqre\b/;
  var re4 = /,/;
  var re5 = /\bQBZPbageby_cynprubyqre\b/g;
  var re6 = /^[\s\xa0]+|[\s\xa0]+$/g;
  var re7 = /(\d*)(\D*)/g;
  var re8 = /=/;
  var re9 = /(^|\s)lhv\-h(\s|$)/;
  var str0 = 'Zbmvyyn/5.0 (Jvaqbjf; H; Jvaqbjf AG 5.1; ra-HF) NccyrJroXvg/528.9 (XUGZY, yvxr Trpxb) Puebzr/2.0.157.0 Fnsnev/528.9';
  var re10 = /\#/g;
  var re11 = /\./g;
  var re12 = /'/g;
  var re13 = /\?[\w\W]*(sevraqvq|punaaryvq|tebhcvq)=([^\&\?#]*)/i;
  var str1 = 'Fubpxjnir Synfu 9.0  e115';
  var re14 = /\s+/g;
  var re15 = /^\s*(\S*(\s+\S+)*)\s*$/;
  var re16 = /(-[a-z])/i;

  var s0 = computeInputVariants('pyvpx', 6511);
  var s1 = computeInputVariants('uggc://jjj.snprobbx.pbz/ybtva.cuc', 1844);
  var s2 = computeInputVariants('QBZPbageby_cynprubyqre', 739);
  var s3 = computeInputVariants('uggc://jjj.snprobbx.pbz/', 598);
  var s4 = computeInputVariants('uggc://jjj.snprobbx.pbz/fepu.cuc', 454);
  var s5 = computeInputVariants('qqqq, ZZZ q, llll', 352);
  var s6 = computeInputVariants('vachggrkg QBZPbageby_cynprubyqre', 312);
  var s7 = computeInputVariants('/ZlFcnprUbzrcntr/Vaqrk-FvgrUbzr,10000000', 282);
  var s8 = computeInputVariants('vachggrkg', 177);
  var s9 = computeInputVariants('528.9', 170);
  var s10 = computeInputVariants('528', 170);
  var s11 = computeInputVariants('VCPhygher=ra-HF', 156);
  var s12 = computeInputVariants('CersreerqPhygher=ra-HF', 156);
  var s13 = computeInputVariants('xrlcerff', 144);
  var s14 = computeInputVariants('521', 139);
  var s15 = computeInputVariants(str0, 139);
  var s16 = computeInputVariants('qvi .so_zrah', 137);
  var s17 = computeInputVariants('qvi.so_zrah', 137);
  var s18 = computeInputVariants('uvqqra_ryrz', 117);
  var s19 = computeInputVariants('sevraqfgre_naba=nvq%3Qn6ss9p85n868ro9s059pn854735956o3%26ers%3Q%26df%3Q%26vpgl%3QHF', 95);
  var s20 = computeInputVariants('uggc://ubzr.zlfcnpr.pbz/vaqrk.psz', 93);
  var s21 = computeInputVariants(str1, 92);
  var s22 = computeInputVariants('svefg', 85);
  var s23 = computeInputVariants('uggc://cebsvyr.zlfcnpr.pbz/vaqrk.psz', 85);
  var s24 = computeInputVariants('ynfg', 85);
  var s25 = computeInputVariants('qvfcynl', 85);

  function runBlock0() {
    for (var i = 0; i < 6511; i++) {
      re0.exec(s0[i]);
    }
    for (var i = 0; i < 1844; i++) {
      re1.exec(s1[i]);
    }
    for (var i = 0; i < 739; i++) {
      s2[i].replace(re2, '');
    }
    for (var i = 0; i < 598; i++) {
      re1.exec(s3[i]);
    }
    for (var i = 0; i < 454; i++) {
      re1.exec(s4[i]);
    }
    for (var i = 0; i < 352; i++) {
      /qqqq|qqq|qq|q|ZZZZ|ZZZ|ZZ|Z|llll|ll|l|uu|u|UU|U|zz|z|ff|f|gg|g|sss|ss|s|mmm|mm|m/g.exec(s5[i]);
    }
    for (var i = 0; i < 312; i++) {
      re3.exec(s6[i]);
    }
    for (var i = 0; i < 282; i++) {
      re4.exec(s7[i]);
    }
    for (var i = 0; i < 177; i++) {
      s8[i].replace(re5, '');
    }
    for (var i = 0; i < 170; i++) {
      s9[i].replace(re6, '');
      re7.exec(s10[i]);
    }
    for (var i = 0; i < 156; i++) {
      re8.exec(s11[i]);
      re8.exec(s12[i]);
    }
    for (var i = 0; i < 144; i++) {
      re0.exec(s13[i]);
    }
    for (var i = 0; i < 139; i++) {
      s14[i].replace(re6, '');
      re7.exec(s14[i]);
      re9.exec('');
      /JroXvg\/(\S+)/.exec(s15[i]);
    }
    for (var i = 0; i < 137; i++) {
      s16[i].replace(re10, '');
      s16[i].replace(/\[/g, '');
      s17[i].replace(re11, '');
    }
    for (var i = 0; i < 117; i++) {
      s18[i].replace(re2, '');
    }
    for (var i = 0; i < 95; i++) {
      /(?:^|;)\s*sevraqfgre_ynat=([^;]*)/.exec(s19[i]);
    }
    for (var i = 0; i < 93; i++) {
      s20[i].replace(re12, '');
      re13.exec(s20[i]);
    }
    for (var i = 0; i < 92; i++) {
      s21[i].replace(/([a-zA-Z]|\s)+/, '');
    }
    for (var i = 0; i < 85; i++) {
      s22[i].replace(re14, '');
      s22[i].replace(re15, '');
      s23[i].replace(re12, '');
      s24[i].replace(re14, '');
      s24[i].replace(re15, '');
      re16.exec(s25[i]);
      re13.exec(s23[i]);
    }
  }
  var re17 = /(^|[^\\])\"\\\/Qngr\((-?[0-9]+)\)\\\/\"/g;
  var str2 = '{"anzr":"","ahzoreSbezng":{"PheeraplQrpvznyQvtvgf":2,"PheeraplQrpvznyFrcnengbe":".","VfErnqBayl":gehr,"PheeraplTebhcFvmrf":[3],"AhzoreTebhcFvmrf":[3],"CrepragTebhcFvmrf":[3],"PheeraplTebhcFrcnengbe":",","PheeraplFlzoby":"\xa4","AnAFlzoby":"AnA","PheeraplArtngvirCnggrea":0,"AhzoreArtngvirCnggrea":1,"CrepragCbfvgvirCnggrea":0,"CrepragArtngvirCnggrea":0,"ArtngvirVasvavglFlzoby":"-Vasvavgl","ArtngvirFvta":"-","AhzoreQrpvznyQvtvgf":2,"AhzoreQrpvznyFrcnengbe":".","AhzoreTebhcFrcnengbe":",","PheeraplCbfvgvirCnggrea":0,"CbfvgvirVasvavglFlzoby":"Vasvavgl","CbfvgvirFvta":"+","CrepragQrpvznyQvtvgf":2,"CrepragQrpvznyFrcnengbe":".","CrepragTebhcFrcnengbe":",","CrepragFlzoby":"%","CreZvyyrFlzoby":"\u2030","AngvirQvtvgf":["0","1","2","3","4","5","6","7","8","9"],"QvtvgFhofgvghgvba":1},"qngrGvzrSbezng":{"NZQrfvtangbe":"NZ","Pnyraqne":{"ZvaFhccbegrqQngrGvzr":"@-62135568000000@","ZnkFhccbegrqQngrGvzr":"@253402300799999@","NytbevguzGlcr":1,"PnyraqneGlcr":1,"Renf":[1],"GjbQvtvgLrneZnk":2029,"VfErnqBayl":gehr},"QngrFrcnengbe":"/","SvefgQnlBsJrrx":0,"PnyraqneJrrxEhyr":0,"ShyyQngrGvzrCnggrea":"qqqq, qq ZZZZ llll UU:zz:ff","YbatQngrCnggrea":"qqqq, qq ZZZZ llll","YbatGvzrCnggrea":"UU:zz:ff","ZbaguQnlCnggrea":"ZZZZ qq","CZQrfvtangbe":"CZ","ESP1123Cnggrea":"qqq, qq ZZZ llll UU\':\'zz\':\'ff \'TZG\'","FubegQngrCnggrea":"ZZ/qq/llll","FubegGvzrCnggrea":"UU:zz","FbegnoyrQngrGvzrCnggrea":"llll\'-\'ZZ\'-\'qq\'G\'UU\':\'zz\':\'ff","GvzrFrcnengbe":":","HavirefnyFbegnoyrQngrGvzrCnggrea":"llll\'-\'ZZ\'-\'qq UU\':\'zz\':\'ff\'M\'","LrneZbaguCnggrea":"llll ZZZZ","NooerivngrqQnlAnzrf":["Fha","Zba","Ghr","Jrq","Guh","Sev","Fng"],"FubegrfgQnlAnzrf":["Fh","Zb","Gh","Jr","Gu","Se","Fn"],"QnlAnzrf":["Fhaqnl","Zbaqnl","Ghrfqnl","Jrqarfqnl","Guhefqnl","Sevqnl","Fngheqnl"],"NooerivngrqZbaguAnzrf":["Wna","Sro","Zne","Nce","Znl","Wha","Why","Nht","Frc","Bpg","Abi","Qrp",""],"ZbaguAnzrf":["Wnahnel","Sroehnel","Znepu","Ncevy","Znl","Whar","Whyl","Nhthfg","Frcgrzore","Bpgbore","Abirzore","Qrprzore",""],"VfErnqBayl":gehr,"AngvirPnyraqneAnzr":"Tertbevna Pnyraqne","NooerivngrqZbaguTravgvirAnzrf":["Wna","Sro","Zne","Nce","Znl","Wha","Why","Nht","Frc","Bpg","Abi","Qrp",""],"ZbaguTravgvirAnzrf":["Wnahnel","Sroehnel","Znepu","Ncevy","Znl","Whar","Whyl","Nhthfg","Frcgrzore","Bpgbore","Abirzore","Qrprzore",""]}}';
  var str3 = '{"anzr":"ra-HF","ahzoreSbezng":{"PheeraplQrpvznyQvtvgf":2,"PheeraplQrpvznyFrcnengbe":".","VfErnqBayl":snyfr,"PheeraplTebhcFvmrf":[3],"AhzoreTebhcFvmrf":[3],"CrepragTebhcFvmrf":[3],"PheeraplTebhcFrcnengbe":",","PheeraplFlzoby":"$","AnAFlzoby":"AnA","PheeraplArtngvirCnggrea":0,"AhzoreArtngvirCnggrea":1,"CrepragCbfvgvirCnggrea":0,"CrepragArtngvirCnggrea":0,"ArtngvirVasvavglFlzoby":"-Vasvavgl","ArtngvirFvta":"-","AhzoreQrpvznyQvtvgf":2,"AhzoreQrpvznyFrcnengbe":".","AhzoreTebhcFrcnengbe":",","PheeraplCbfvgvirCnggrea":0,"CbfvgvirVasvavglFlzoby":"Vasvavgl","CbfvgvirFvta":"+","CrepragQrpvznyQvtvgf":2,"CrepragQrpvznyFrcnengbe":".","CrepragTebhcFrcnengbe":",","CrepragFlzoby":"%","CreZvyyrFlzoby":"\u2030","AngvirQvtvgf":["0","1","2","3","4","5","6","7","8","9"],"QvtvgFhofgvghgvba":1},"qngrGvzrSbezng":{"NZQrfvtangbe":"NZ","Pnyraqne":{"ZvaFhccbegrqQngrGvzr":"@-62135568000000@","ZnkFhccbegrqQngrGvzr":"@253402300799999@","NytbevguzGlcr":1,"PnyraqneGlcr":1,"Renf":[1],"GjbQvtvgLrneZnk":2029,"VfErnqBayl":snyfr},"QngrFrcnengbe":"/","SvefgQnlBsJrrx":0,"PnyraqneJrrxEhyr":0,"ShyyQngrGvzrCnggrea":"qqqq, ZZZZ qq, llll u:zz:ff gg","YbatQngrCnggrea":"qqqq, ZZZZ qq, llll","YbatGvzrCnggrea":"u:zz:ff gg","ZbaguQnlCnggrea":"ZZZZ qq","CZQrfvtangbe":"CZ","ESP1123Cnggrea":"qqq, qq ZZZ llll UU\':\'zz\':\'ff \'TZG\'","FubegQngrCnggrea":"Z/q/llll","FubegGvzrCnggrea":"u:zz gg","FbegnoyrQngrGvzrCnggrea":"llll\'-\'ZZ\'-\'qq\'G\'UU\':\'zz\':\'ff","GvzrFrcnengbe":":","HavirefnyFbegnoyrQngrGvzrCnggrea":"llll\'-\'ZZ\'-\'qq UU\':\'zz\':\'ff\'M\'","LrneZbaguCnggrea":"ZZZZ, llll","NooerivngrqQnlAnzrf":["Fha","Zba","Ghr","Jrq","Guh","Sev","Fng"],"FubegrfgQnlAnzrf":["Fh","Zb","Gh","Jr","Gu","Se","Fn"],"QnlAnzrf":["Fhaqnl","Zbaqnl","Ghrfqnl","Jrqarfqnl","Guhefqnl","Sevqnl","Fngheqnl"],"NooerivngrqZbaguAnzrf":["Wna","Sro","Zne","Nce","Znl","Wha","Why","Nht","Frc","Bpg","Abi","Qrp",""],"ZbaguAnzrf":["Wnahnel","Sroehnel","Znepu","Ncevy","Znl","Whar","Whyl","Nhthfg","Frcgrzore","Bpgbore","Abirzore","Qrprzore",""],"VfErnqBayl":snyfr,"AngvirPnyraqneAnzr":"Tertbevna Pnyraqne","NooerivngrqZbaguTravgvirAnzrf":["Wna","Sro","Zne","Nce","Znl","Wha","Why","Nht","Frc","Bpg","Abi","Qrp",""],"ZbaguTravgvirAnzrf":["Wnahnel","Sroehnel","Znepu","Ncevy","Znl","Whar","Whyl","Nhthfg","Frcgrzore","Bpgbore","Abirzore","Qrprzore",""]}}';
  var str4 = 'HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
  var str5 = 'HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
  var re18 = /^\s+|\s+$/g;
  var str6 = 'uggc://jjj.snprobbx.pbz/vaqrk.cuc';
  var re19 = /(?:^|\s+)ba(?:\s+|$)/;
  var re20 = /[+, ]/;
  var re21 = /ybnqrq|pbzcyrgr/;
  var str7 = ';;jvaqbj.IjPurpxZbhfrCbfvgvbaNQ_VQ=shapgvba(r){vs(!r)ine r=jvaqbj.rirag;ine c=-1;vs(d1)c=d1.EbyybssCnary;ine bo=IjTrgBow("IjCnayNQ_VQ_"+c);vs(bo&&bo.fglyr.ivfvovyvgl=="ivfvoyr"){ine fns=IjFns?8:0;ine pheK=r.pyvragK+IjBOFpe("U")+fns,pheL=r.pyvragL+IjBOFpe("I")+fns;ine y=IjBOEC(NQ_VQ,bo,"Y"),g=IjBOEC(NQ_VQ,bo,"G");ine e=y+d1.Cnaryf[c].Jvqgu,o=g+d1.Cnaryf[c].Urvtug;vs((pheK<y)||(pheK>e)||(pheL<g)||(pheL>o)){vs(jvaqbj.IjBaEbyybssNQ_VQ)IjBaEbyybssNQ_VQ(c);ryfr IjPybfrNq(NQ_VQ,c,gehr,"");}ryfr erghea;}IjPnapryZbhfrYvfgrareNQ_VQ();};;jvaqbj.IjFrgEbyybssCnaryNQ_VQ=shapgvba(c){ine z="zbhfrzbir",q=qbphzrag,s=IjPurpxZbhfrCbfvgvbaNQ_VQ;c=IjTc(NQ_VQ,c);vs(d1&&d1.EbyybssCnary>-1)IjPnapryZbhfrYvfgrareNQ_VQ();vs(d1)d1.EbyybssCnary=c;gel{vs(q.nqqRiragYvfgrare)q.nqqRiragYvfgrare(z,s,snyfr);ryfr vs(q.nggnpuRirag)q.nggnpuRirag("ba"+z,s);}pngpu(r){}};;jvaqbj.IjPnapryZbhfrYvfgrareNQ_VQ=shapgvba(){ine z="zbhfrzbir",q=qbphzrag,s=IjPurpxZbhfrCbfvgvbaNQ_VQ;vs(d1)d1.EbyybssCnary=-1;gel{vs(q.erzbirRiragYvfgrare)q.erzbirRiragYvfgrare(z,s,snyfr);ryfr vs(q.qrgnpuRirag)q.qrgnpuRirag("ba"+z,s);}pngpu(r){}};;d1.IjTc=d2(n,c){ine nq=d1;vs(vfAnA(c)){sbe(ine v=0;v<nq.Cnaryf.yratgu;v++)vs(nq.Cnaryf[v].Anzr==c)erghea v;erghea 0;}erghea c;};;d1.IjTpy=d2(n,c,p){ine cn=d1.Cnaryf[IjTc(n,c)];vs(!cn)erghea 0;vs(vfAnA(p)){sbe(ine v=0;v<cn.Pyvpxguehf.yratgu;v++)vs(cn.Pyvpxguehf[v].Anzr==p)erghea v;erghea 0;}erghea p;};;d1.IjGenpr=d2(n,f){gel{vs(jvaqbj["Ij"+"QtQ"])jvaqbj["Ij"+"QtQ"](n,1,f);}pngpu(r){}};;d1.IjYvzvg1=d2(n,f){ine nq=d1,vh=f.fcyvg("/");sbe(ine v=0,p=0;v<vh.yratgu;v++){vs(vh[v].yratgu>0){vs(nq.FzV.yratgu>0)nq.FzV+="/";nq.FzV+=vh[v];nq.FtZ[nq.FtZ.yratgu]=snyfr;}}};;d1.IjYvzvg0=d2(n,f){ine nq=d1,vh=f.fcyvg("/");sbe(ine v=0;v<vh.yratgu;v++){vs(vh[v].yratgu>0){vs(nq.OvC.yratgu>0)nq.OvC+="/";nq.OvC+=vh[v];}}};;d1.IjRVST=d2(n,c){jvaqbj["IjCnayNQ_VQ_"+c+"_Bow"]=IjTrgBow("IjCnayNQ_VQ_"+c+"_Bow");vs(jvaqbj["IjCnayNQ_VQ_"+c+"_Bow"]==ahyy)frgGvzrbhg("IjRVST(NQ_VQ,"+c+")",d1.rvsg);};;d1.IjNavzSHC=d2(n,c){ine nq=d1;vs(c>nq.Cnaryf.yratgu)erghea;ine cna=nq.Cnaryf[c],nn=gehr,on=gehr,yn=gehr,en=gehr,cn=nq.Cnaryf[0],sf=nq.ShF,j=cn.Jvqgu,u=cn.Urvtug;vs(j=="100%"){j=sf;en=snyfr;yn=snyfr;}vs(u=="100%"){u=sf;nn=snyfr;on=snyfr;}vs(cn.YnY=="Y")yn=snyfr;vs(cn.YnY=="E")en=snyfr;vs(cn.GnY=="G")nn=snyfr;vs(cn.GnY=="O")on=snyfr;ine k=0,l=0;fjvgpu(nq.NshP%8){pnfr 0:oernx;pnfr 1:vs(nn)l=-sf;oernx;pnfr 2:k=j-sf;oernx;pnfr 3:vs(en)k=j;oernx;pnfr 4:k=j-sf;l=u-sf;oernx;pnfr 5:k=j-sf;vs(on)l=u;oernx;pnfr 6:l=u-sf;oernx;pnfr 7:vs(yn)k=-sf;l=u-sf;oernx;}vs(nq.NshP++ <nq.NshG)frgGvzrbhg(("IjNavzSHC(NQ_VQ,"+c+")"),nq.NshC);ryfr{k=-1000;l=k;}cna.YrsgBssfrg=k;cna.GbcBssfrg=l;IjNhErcb(n,c);};;d1.IjTrgErnyCbfvgvba=d2(n,b,j){erghea IjBOEC.nccyl(guvf,nethzragf);};;d1.IjPnapryGvzrbhg=d2(n,c){c=IjTc(n,c);ine cay=d1.Cnaryf[c];vs(cay&&cay.UgU!=""){pyrneGvzrbhg(cay.UgU);}};;d1.IjPnapryNyyGvzrbhgf=d2(n){vs(d1.YbpxGvzrbhgPunatrf)erghea;sbe(ine c=0;c<d1.bac;c++)IjPnapryGvzrbhg(n,c);};;d1.IjFgnegGvzrbhg=d2(n,c,bG){c=IjTc(n,c);ine cay=d1.Cnaryf[c];vs(cay&&((cay.UvqrGvzrbhgInyhr>0)||(nethzragf.yratgu==3&&bG>0))){pyrneGvzrbhg(cay.UgU);cay.UgU=frgGvzrbhg(cay.UvqrNpgvba,(nethzragf.yratgu==3?bG:cay.UvqrGvzrbhgInyhr));}};;d1.IjErfrgGvzrbhg=d2(n,c,bG){c=IjTc(n,c);IjPnapryGvzrbhg(n,c);riny("IjFgnegGvzrbhg(NQ_VQ,c"+(nethzragf.yratgu==3?",bG":"")+")");};;d1.IjErfrgNyyGvzrbhgf=d2(n){sbe(ine c=0;c<d1.bac;c++)IjErfrgGvzrbhg(n,c);};;d1.IjQrgnpure=d2(n,rig,sap){gel{vs(IjQVR5)riny("jvaqbj.qrgnpuRirag(\'ba"+rig+"\',"+sap+"NQ_VQ)");ryfr vs(!IjQVRZnp)riny("jvaqbj.erzbirRiragYvfgrare(\'"+rig+"\',"+sap+"NQ_VQ,snyfr)");}pngpu(r){}};;d1.IjPyrnaHc=d2(n){IjCvat(n,"G");ine nq=d1;sbe(ine v=0;v<nq.Cnaryf.yratgu;v++){IjUvqrCnary(n,v,gehr);}gel{IjTrgBow(nq.gya).vaareUGZY="";}pngpu(r){}vs(nq.gya!=nq.gya2)gel{IjTrgBow(nq.gya2).vaareUGZY="";}pngpu(r){}gel{d1=ahyy;}pngpu(r){}gel{IjQrgnpure(n,"haybnq","IjHayNQ_VQ");}pngpu(r){}gel{jvaqbj.IjHayNQ_VQ=ahyy;}pngpu(r){}gel{IjQrgnpure(n,"fpebyy","IjFeNQ_VQ");}pngpu(r){}gel{jvaqbj.IjFeNQ_VQ=ahyy;}pngpu(r){}gel{IjQrgnpure(n,"erfvmr","IjEmNQ_VQ");}pngpu(r){}gel{jvaqbj.IjEmNQ_VQ=ahyy;}pngpu(r){}gel{IjQrgnpure(n';
  var str8 = ';;jvaqbj.IjPurpxZbhfrCbfvgvbaNQ_VQ=shapgvba(r){vs(!r)ine r=jvaqbj.rirag;ine c=-1;vs(jvaqbj.IjNqNQ_VQ)c=jvaqbj.IjNqNQ_VQ.EbyybssCnary;ine bo=IjTrgBow("IjCnayNQ_VQ_"+c);vs(bo&&bo.fglyr.ivfvovyvgl=="ivfvoyr"){ine fns=IjFns?8:0;ine pheK=r.pyvragK+IjBOFpe("U")+fns,pheL=r.pyvragL+IjBOFpe("I")+fns;ine y=IjBOEC(NQ_VQ,bo,"Y"),g=IjBOEC(NQ_VQ,bo,"G");ine e=y+jvaqbj.IjNqNQ_VQ.Cnaryf[c].Jvqgu,o=g+jvaqbj.IjNqNQ_VQ.Cnaryf[c].Urvtug;vs((pheK<y)||(pheK>e)||(pheL<g)||(pheL>o)){vs(jvaqbj.IjBaEbyybssNQ_VQ)IjBaEbyybssNQ_VQ(c);ryfr IjPybfrNq(NQ_VQ,c,gehr,"");}ryfr erghea;}IjPnapryZbhfrYvfgrareNQ_VQ();};;jvaqbj.IjFrgEbyybssCnaryNQ_VQ=shapgvba(c){ine z="zbhfrzbir",q=qbphzrag,s=IjPurpxZbhfrCbfvgvbaNQ_VQ;c=IjTc(NQ_VQ,c);vs(jvaqbj.IjNqNQ_VQ&&jvaqbj.IjNqNQ_VQ.EbyybssCnary>-1)IjPnapryZbhfrYvfgrareNQ_VQ();vs(jvaqbj.IjNqNQ_VQ)jvaqbj.IjNqNQ_VQ.EbyybssCnary=c;gel{vs(q.nqqRiragYvfgrare)q.nqqRiragYvfgrare(z,s,snyfr);ryfr vs(q.nggnpuRirag)q.nggnpuRirag("ba"+z,s);}pngpu(r){}};;jvaqbj.IjPnapryZbhfrYvfgrareNQ_VQ=shapgvba(){ine z="zbhfrzbir",q=qbphzrag,s=IjPurpxZbhfrCbfvgvbaNQ_VQ;vs(jvaqbj.IjNqNQ_VQ)jvaqbj.IjNqNQ_VQ.EbyybssCnary=-1;gel{vs(q.erzbirRiragYvfgrare)q.erzbirRiragYvfgrare(z,s,snyfr);ryfr vs(q.qrgnpuRirag)q.qrgnpuRirag("ba"+z,s);}pngpu(r){}};;jvaqbj.IjNqNQ_VQ.IjTc=shapgvba(n,c){ine nq=jvaqbj.IjNqNQ_VQ;vs(vfAnA(c)){sbe(ine v=0;v<nq.Cnaryf.yratgu;v++)vs(nq.Cnaryf[v].Anzr==c)erghea v;erghea 0;}erghea c;};;jvaqbj.IjNqNQ_VQ.IjTpy=shapgvba(n,c,p){ine cn=jvaqbj.IjNqNQ_VQ.Cnaryf[IjTc(n,c)];vs(!cn)erghea 0;vs(vfAnA(p)){sbe(ine v=0;v<cn.Pyvpxguehf.yratgu;v++)vs(cn.Pyvpxguehf[v].Anzr==p)erghea v;erghea 0;}erghea p;};;jvaqbj.IjNqNQ_VQ.IjGenpr=shapgvba(n,f){gel{vs(jvaqbj["Ij"+"QtQ"])jvaqbj["Ij"+"QtQ"](n,1,f);}pngpu(r){}};;jvaqbj.IjNqNQ_VQ.IjYvzvg1=shapgvba(n,f){ine nq=jvaqbj.IjNqNQ_VQ,vh=f.fcyvg("/");sbe(ine v=0,p=0;v<vh.yratgu;v++){vs(vh[v].yratgu>0){vs(nq.FzV.yratgu>0)nq.FzV+="/";nq.FzV+=vh[v];nq.FtZ[nq.FtZ.yratgu]=snyfr;}}};;jvaqbj.IjNqNQ_VQ.IjYvzvg0=shapgvba(n,f){ine nq=jvaqbj.IjNqNQ_VQ,vh=f.fcyvg("/");sbe(ine v=0;v<vh.yratgu;v++){vs(vh[v].yratgu>0){vs(nq.OvC.yratgu>0)nq.OvC+="/";nq.OvC+=vh[v];}}};;jvaqbj.IjNqNQ_VQ.IjRVST=shapgvba(n,c){jvaqbj["IjCnayNQ_VQ_"+c+"_Bow"]=IjTrgBow("IjCnayNQ_VQ_"+c+"_Bow");vs(jvaqbj["IjCnayNQ_VQ_"+c+"_Bow"]==ahyy)frgGvzrbhg("IjRVST(NQ_VQ,"+c+")",jvaqbj.IjNqNQ_VQ.rvsg);};;jvaqbj.IjNqNQ_VQ.IjNavzSHC=shapgvba(n,c){ine nq=jvaqbj.IjNqNQ_VQ;vs(c>nq.Cnaryf.yratgu)erghea;ine cna=nq.Cnaryf[c],nn=gehr,on=gehr,yn=gehr,en=gehr,cn=nq.Cnaryf[0],sf=nq.ShF,j=cn.Jvqgu,u=cn.Urvtug;vs(j=="100%"){j=sf;en=snyfr;yn=snyfr;}vs(u=="100%"){u=sf;nn=snyfr;on=snyfr;}vs(cn.YnY=="Y")yn=snyfr;vs(cn.YnY=="E")en=snyfr;vs(cn.GnY=="G")nn=snyfr;vs(cn.GnY=="O")on=snyfr;ine k=0,l=0;fjvgpu(nq.NshP%8){pnfr 0:oernx;pnfr 1:vs(nn)l=-sf;oernx;pnfr 2:k=j-sf;oernx;pnfr 3:vs(en)k=j;oernx;pnfr 4:k=j-sf;l=u-sf;oernx;pnfr 5:k=j-sf;vs(on)l=u;oernx;pnfr 6:l=u-sf;oernx;pnfr 7:vs(yn)k=-sf;l=u-sf;oernx;}vs(nq.NshP++ <nq.NshG)frgGvzrbhg(("IjNavzSHC(NQ_VQ,"+c+")"),nq.NshC);ryfr{k=-1000;l=k;}cna.YrsgBssfrg=k;cna.GbcBssfrg=l;IjNhErcb(n,c);};;jvaqbj.IjNqNQ_VQ.IjTrgErnyCbfvgvba=shapgvba(n,b,j){erghea IjBOEC.nccyl(guvf,nethzragf);};;jvaqbj.IjNqNQ_VQ.IjPnapryGvzrbhg=shapgvba(n,c){c=IjTc(n,c);ine cay=jvaqbj.IjNqNQ_VQ.Cnaryf[c];vs(cay&&cay.UgU!=""){pyrneGvzrbhg(cay.UgU);}};;jvaqbj.IjNqNQ_VQ.IjPnapryNyyGvzrbhgf=shapgvba(n){vs(jvaqbj.IjNqNQ_VQ.YbpxGvzrbhgPunatrf)erghea;sbe(ine c=0;c<jvaqbj.IjNqNQ_VQ.bac;c++)IjPnapryGvzrbhg(n,c);};;jvaqbj.IjNqNQ_VQ.IjFgnegGvzrbhg=shapgvba(n,c,bG){c=IjTc(n,c);ine cay=jvaqbj.IjNqNQ_VQ.Cnaryf[c];vs(cay&&((cay.UvqrGvzrbhgInyhr>0)||(nethzragf.yratgu==3&&bG>0))){pyrneGvzrbhg(cay.UgU);cay.UgU=frgGvzrbhg(cay.UvqrNpgvba,(nethzragf.yratgu==3?bG:cay.UvqrGvzrbhgInyhr));}};;jvaqbj.IjNqNQ_VQ.IjErfrgGvzrbhg=shapgvba(n,c,bG){c=IjTc(n,c);IjPnapryGvzrbhg(n,c);riny("IjFgnegGvzrbhg(NQ_VQ,c"+(nethzragf.yratgu==3?",bG":"")+")");};;jvaqbj.IjNqNQ_VQ.IjErfrgNyyGvzrbhgf=shapgvba(n){sbe(ine c=0;c<jvaqbj.IjNqNQ_VQ.bac;c++)IjErfrgGvzrbhg(n,c);};;jvaqbj.IjNqNQ_VQ.IjQrgnpure=shapgvba(n,rig,sap){gel{vs(IjQVR5)riny("jvaqbj.qrgnpuRirag(\'ba"+rig+"\',"+sap+"NQ_VQ)");ryfr vs(!IjQVRZnp)riny("jvaqbj.erzbir';
  var str9 = ';;jvaqbj.IjPurpxZbhfrCbfvgvbaNQ_VQ=shapgvba(r){vs(!r)ine r=jvaqbj.rirag;ine c=-1;vs(jvaqbj.IjNqNQ_VQ)c=jvaqbj.IjNqNQ_VQ.EbyybssCnary;ine bo=IjTrgBow("IjCnayNQ_VQ_"+c);vs(bo&&bo.fglyr.ivfvovyvgl=="ivfvoyr"){ine fns=IjFns?8:0;ine pheK=r.pyvragK+IjBOFpe("U")+fns,pheL=r.pyvragL+IjBOFpe("I")+fns;ine y=IjBOEC(NQ_VQ,bo,"Y"),g=IjBOEC(NQ_VQ,bo,"G");ine e=y+jvaqbj.IjNqNQ_VQ.Cnaryf[c].Jvqgu,o=g+jvaqbj.IjNqNQ_VQ.Cnaryf[c].Urvtug;vs((pheK<y)||(pheK>e)||(pheL<g)||(pheL>o)){vs(jvaqbj.IjBaEbyybssNQ_VQ)IjBaEbyybssNQ_VQ(c);ryfr IjPybfrNq(NQ_VQ,c,gehr,"");}ryfr erghea;}IjPnapryZbhfrYvfgrareNQ_VQ();};;jvaqbj.IjFrgEbyybssCnaryNQ_VQ=shapgvba(c){ine z="zbhfrzbir",q=qbphzrag,s=IjPurpxZbhfrCbfvgvbaNQ_VQ;c=IjTc(NQ_VQ,c);vs(jvaqbj.IjNqNQ_VQ&&jvaqbj.IjNqNQ_VQ.EbyybssCnary>-1)IjPnapryZbhfrYvfgrareNQ_VQ();vs(jvaqbj.IjNqNQ_VQ)jvaqbj.IjNqNQ_VQ.EbyybssCnary=c;gel{vs(q.nqqRiragYvfgrare)q.nqqRiragYvfgrare(z,s,snyfr);ryfr vs(q.nggnpuRirag)q.nggnpuRirag("ba"+z,s);}pngpu(r){}};;jvaqbj.IjPnapryZbhfrYvfgrareNQ_VQ=shapgvba(){ine z="zbhfrzbir",q=qbphzrag,s=IjPurpxZbhfrCbfvgvbaNQ_VQ;vs(jvaqbj.IjNqNQ_VQ)jvaqbj.IjNqNQ_VQ.EbyybssCnary=-1;gel{vs(q.erzbirRiragYvfgrare)q.erzbirRiragYvfgrare(z,s,snyfr);ryfr vs(q.qrgnpuRirag)q.qrgnpuRirag("ba"+z,s);}pngpu(r){}};;jvaqbj.IjNqNQ_VQ.IjTc=d2(n,c){ine nq=jvaqbj.IjNqNQ_VQ;vs(vfAnA(c)){sbe(ine v=0;v<nq.Cnaryf.yratgu;v++)vs(nq.Cnaryf[v].Anzr==c)erghea v;erghea 0;}erghea c;};;jvaqbj.IjNqNQ_VQ.IjTpy=d2(n,c,p){ine cn=jvaqbj.IjNqNQ_VQ.Cnaryf[IjTc(n,c)];vs(!cn)erghea 0;vs(vfAnA(p)){sbe(ine v=0;v<cn.Pyvpxguehf.yratgu;v++)vs(cn.Pyvpxguehf[v].Anzr==p)erghea v;erghea 0;}erghea p;};;jvaqbj.IjNqNQ_VQ.IjGenpr=d2(n,f){gel{vs(jvaqbj["Ij"+"QtQ"])jvaqbj["Ij"+"QtQ"](n,1,f);}pngpu(r){}};;jvaqbj.IjNqNQ_VQ.IjYvzvg1=d2(n,f){ine nq=jvaqbj.IjNqNQ_VQ,vh=f.fcyvg("/");sbe(ine v=0,p=0;v<vh.yratgu;v++){vs(vh[v].yratgu>0){vs(nq.FzV.yratgu>0)nq.FzV+="/";nq.FzV+=vh[v];nq.FtZ[nq.FtZ.yratgu]=snyfr;}}};;jvaqbj.IjNqNQ_VQ.IjYvzvg0=d2(n,f){ine nq=jvaqbj.IjNqNQ_VQ,vh=f.fcyvg("/");sbe(ine v=0;v<vh.yratgu;v++){vs(vh[v].yratgu>0){vs(nq.OvC.yratgu>0)nq.OvC+="/";nq.OvC+=vh[v];}}};;jvaqbj.IjNqNQ_VQ.IjRVST=d2(n,c){jvaqbj["IjCnayNQ_VQ_"+c+"_Bow"]=IjTrgBow("IjCnayNQ_VQ_"+c+"_Bow");vs(jvaqbj["IjCnayNQ_VQ_"+c+"_Bow"]==ahyy)frgGvzrbhg("IjRVST(NQ_VQ,"+c+")",jvaqbj.IjNqNQ_VQ.rvsg);};;jvaqbj.IjNqNQ_VQ.IjNavzSHC=d2(n,c){ine nq=jvaqbj.IjNqNQ_VQ;vs(c>nq.Cnaryf.yratgu)erghea;ine cna=nq.Cnaryf[c],nn=gehr,on=gehr,yn=gehr,en=gehr,cn=nq.Cnaryf[0],sf=nq.ShF,j=cn.Jvqgu,u=cn.Urvtug;vs(j=="100%"){j=sf;en=snyfr;yn=snyfr;}vs(u=="100%"){u=sf;nn=snyfr;on=snyfr;}vs(cn.YnY=="Y")yn=snyfr;vs(cn.YnY=="E")en=snyfr;vs(cn.GnY=="G")nn=snyfr;vs(cn.GnY=="O")on=snyfr;ine k=0,l=0;fjvgpu(nq.NshP%8){pnfr 0:oernx;pnfr 1:vs(nn)l=-sf;oernx;pnfr 2:k=j-sf;oernx;pnfr 3:vs(en)k=j;oernx;pnfr 4:k=j-sf;l=u-sf;oernx;pnfr 5:k=j-sf;vs(on)l=u;oernx;pnfr 6:l=u-sf;oernx;pnfr 7:vs(yn)k=-sf;l=u-sf;oernx;}vs(nq.NshP++ <nq.NshG)frgGvzrbhg(("IjNavzSHC(NQ_VQ,"+c+")"),nq.NshC);ryfr{k=-1000;l=k;}cna.YrsgBssfrg=k;cna.GbcBssfrg=l;IjNhErcb(n,c);};;jvaqbj.IjNqNQ_VQ.IjTrgErnyCbfvgvba=d2(n,b,j){erghea IjBOEC.nccyl(guvf,nethzragf);};;jvaqbj.IjNqNQ_VQ.IjPnapryGvzrbhg=d2(n,c){c=IjTc(n,c);ine cay=jvaqbj.IjNqNQ_VQ.Cnaryf[c];vs(cay&&cay.UgU!=""){pyrneGvzrbhg(cay.UgU);}};;jvaqbj.IjNqNQ_VQ.IjPnapryNyyGvzrbhgf=d2(n){vs(jvaqbj.IjNqNQ_VQ.YbpxGvzrbhgPunatrf)erghea;sbe(ine c=0;c<jvaqbj.IjNqNQ_VQ.bac;c++)IjPnapryGvzrbhg(n,c);};;jvaqbj.IjNqNQ_VQ.IjFgnegGvzrbhg=d2(n,c,bG){c=IjTc(n,c);ine cay=jvaqbj.IjNqNQ_VQ.Cnaryf[c];vs(cay&&((cay.UvqrGvzrbhgInyhr>0)||(nethzragf.yratgu==3&&bG>0))){pyrneGvzrbhg(cay.UgU);cay.UgU=frgGvzrbhg(cay.UvqrNpgvba,(nethzragf.yratgu==3?bG:cay.UvqrGvzrbhgInyhr));}};;jvaqbj.IjNqNQ_VQ.IjErfrgGvzrbhg=d2(n,c,bG){c=IjTc(n,c);IjPnapryGvzrbhg(n,c);riny("IjFgnegGvzrbhg(NQ_VQ,c"+(nethzragf.yratgu==3?",bG":"")+")");};;jvaqbj.IjNqNQ_VQ.IjErfrgNyyGvzrbhgf=d2(n){sbe(ine c=0;c<jvaqbj.IjNqNQ_VQ.bac;c++)IjErfrgGvzrbhg(n,c);};;jvaqbj.IjNqNQ_VQ.IjQrgnpure=d2(n,rig,sap){gel{vs(IjQVR5)riny("jvaqbj.qrgnpuRirag(\'ba"+rig+"\',"+sap+"NQ_VQ)");ryfr vs(!IjQVRZnp)riny("jvaqbj.erzbirRiragYvfgrare(\'"+rig+"\',"+sap+"NQ_VQ,snyfr)");}pngpu(r){}};;jvaqbj.IjNqNQ_VQ.IjPyrna';

  var s26 = computeInputVariants('VC=74.125.75.1', 81);
  var s27 = computeInputVariants('9.0  e115', 78);
  var s28 = computeInputVariants('k',78);
  var s29 = computeInputVariants(str2, 81);
  var s30 = computeInputVariants(str3, 81);
  var s31 = computeInputVariants('144631658', 78);
  var s32 = computeInputVariants('Pbhagel=IIZ%3Q', 78);
  var s33 = computeInputVariants('Pbhagel=IIZ=', 78);
  var s34 = computeInputVariants('CersreerqPhygherCraqvat=', 78);
  var s35 = computeInputVariants(str4, 78);
  var s36 = computeInputVariants(str5, 78);
  var s37 = computeInputVariants('__hgzp=144631658', 78);
  var s38 = computeInputVariants('gvzrMbar=-8', 78);
  var s39 = computeInputVariants('gvzrMbar=0', 78);
  // var s40 = computeInputVariants(s15[i], 78);
  var s41 = computeInputVariants('vachggrkg  QBZPbageby_cynprubyqre', 78);
  var s42 = computeInputVariants('xrlqbja', 78);
  var s43 = computeInputVariants('xrlhc', 78);
  var s44 = computeInputVariants('uggc://zrffntvat.zlfcnpr.pbz/vaqrk.psz', 77);
  var s45 = computeInputVariants('FrffvbaFgbentr=%7O%22GnoThvq%22%3N%7O%22thvq%22%3N1231367125017%7Q%7Q', 73);
  var s46 = computeInputVariants(str6, 72);
  var s47 = computeInputVariants('3.5.0.0', 70);
  var s48 = computeInputVariants(str7, 70);
  var s49 = computeInputVariants(str8, 70);
  var s50 = computeInputVariants(str9, 70);
  var s51 = computeInputVariants('NI%3Q1_CI%3Q1_PI%3Q1_EI%3Q1_HI%3Q1_HP%3Q1_IC%3Q0.0.0.0_IH%3Q0', 70);
  var s52 = computeInputVariants('svz_zlfcnpr_ubzrcntr_abgybttrqva,svz_zlfcnpr_aba_HTP,svz_zlfcnpr_havgrq-fgngrf', 70);
  var s53 = computeInputVariants('ybnqvat', 70);
  var s54 = computeInputVariants('#', 68);
  var s55 = computeInputVariants('ybnqrq', 68);
  var s56 = computeInputVariants('pbybe', 49);
  var s57 = computeInputVariants('uggc://sevraqf.zlfcnpr.pbz/vaqrk.psz', 44);

  function runBlock1() {
    for (var i = 0; i < 81; i++) {
      re8.exec(s26[i]);
    }
    for (var i = 0; i < 78; i++) {
      s27[i].replace(/(\s)+e/, '');
      s28[i].replace(/./, '');
      s29[i].replace(re17, '');
      s30[i].replace(re17, '');
      re8.exec(s31[i]);
      re8.exec(s32[i]);
      re8.exec(s33[i]);
      re8.exec(s34[i]);
      re8.exec(s35[i]);
      re8.exec(s36[i]);
      re8.exec(s37[i]);
      re8.exec(s38[i]);
      re8.exec(s39[i]);
      /Fnsnev\/(\d+\.\d+)/.exec(s15[i]);
      re3.exec(s41[i]);
      re0.exec(s42[i]);
      re0.exec(s43[i]);
    }
    for (var i = 0; i < 77; i++) {
      s44[i].replace(re12, '');
      re13.exec(s44[i]);
    }
    for (var i = 0; i < 73; i++) {
      s45[i].replace(re18, '');
    }
    for (var i = 0; i < 72; i++) {
      re1.exec(s46[i]);
    }
    for (var i = 0; i < 71; i++) {
      re19.exec('');
    }
    for (var i = 0; i < 70; i++) {
      s47[i].replace(re11, '');
      s48[i].replace(/d1/g, '');
      s49[i].replace(/NQ_VQ/g, '');
      s50[i].replace(/d2/g, '');
      s51[i].replace(/_/g, '');
      s52[i].split(re20);
      re21.exec(s53[i]);
    }
    for (var i = 0; i < 68; i++) {
      re1.exec(s54[i]);
      /(?:ZFVR.(\d+\.\d+))|(?:(?:Sversbk|TenaCnenqvfb|Vprjrnfry).(\d+\.\d+))|(?:Bcren.(\d+\.\d+))|(?:NccyrJroXvg.(\d+(?:\.\d+)?))/.exec(s15[i]);
      /(Znp BF K)|(Jvaqbjf;)/.exec(s15[i]);
      /Trpxb\/([0-9]+)/.exec(s15[i]);
      re21.exec(s55[i]);
    }
    for (var i = 0; i < 49; i++) {
      re16.exec(s56[i]);
    }
    for (var i = 0; i < 44; i++) {
      s57[i].replace(re12, '');
      re13.exec(s57[i]);
    }
  }
  var re22 = /\bso_zrah\b/;
  var re23 = /^(?:(?:[^:\/?#]+):)?(?:\/\/(?:[^\/?#]*))?([^?#]*)(?:\?([^#]*))?(?:#(.*))?/;
  var re24 = /uggcf?:\/\/([^\/]+\.)?snprobbx\.pbz\//;
  var re25 = /"/g;
  var re26 = /^([^?#]+)(?:\?([^#]*))?(#.*)?/;
  var s57a = computeInputVariants('fryrpgrq', 40);
  var s58 = computeInputVariants('vachggrkg uvqqra_ryrz', 40);
  var s59 = computeInputVariants('vachggrkg ', 40);
  var s60 = computeInputVariants('vachggrkg', 40);
  var s61 = computeInputVariants('uggc://jjj.snprobbx.pbz/', 40);
  var s62 = computeInputVariants('uggc://jjj.snprobbx.pbz/ybtva.cuc', 40);
  var s63 = computeInputVariants('Funer guvf tnqtrg', 40);
  var s64 = computeInputVariants('uggc://jjj.tbbtyr.pbz/vt/qverpgbel', 40);
  var s65 = computeInputVariants('419', 40);
  var s66 = computeInputVariants('gvzrfgnzc', 40);

  function runBlock2() {
    for (var i = 0; i < 40; i++) {
      s57a[i].replace(re14, '');
      s57a[i].replace(re15, '');
    }
    for (var i = 0; i < 39; i++) {
      s58[i].replace(/\buvqqra_ryrz\b/g, '');
      re3.exec(s59[i]);
      re3.exec(s60[i]);
      re22.exec('HVYvaxOhggba');
      re22.exec('HVYvaxOhggba_E');
      re22.exec('HVYvaxOhggba_EJ');
      re22.exec('zrah_ybtva_pbagnvare');
      /\buvqqra_ryrz\b/.exec('vachgcnffjbeq');
    }
    for (var i = 0; i < 37; i++) {
      re8.exec('111soqs57qo8o8480qo18sor2011r3n591q7s6s37r120904');
      re8.exec('SbeprqRkcvengvba=633669315660164980');
      re8.exec('FrffvbaQQS2=111soqs57qo8o8480qo18sor2011r3n591q7s6s37r120904');
    }
    for (var i = 0; i < 35; i++) {
      'puvyq p1 svefg'.replace(re14, '');
      'puvyq p1 svefg'.replace(re15, '');
      'sylbhg pybfrq'.replace(re14, '');
      'sylbhg pybfrq'.replace(re15, '');
    }
    for (var i = 0; i < 34; i++) {
      re19.exec('gno2');
      re19.exec('gno3');
      re8.exec('44132r503660');
      re8.exec('SbeprqRkcvengvba=633669316860113296');
      re8.exec('AFP_zp_dfctwzs-aowb_80=44132r503660');
      re8.exec('FrffvbaQQS2=s6r4579npn4rn2135s904r0s75pp1o5334p6s6pospo12696');
      re8.exec('s6r4579npn4rn2135s904r0s75pp1o5334p6s6pospo12696');
    }
    for (var i = 0; i < 32; i++) {
      /puebzr/i.exec(s15[i]);
    }
    for (var i = 0; i < 31; i++) {
      s61[i].replace(re23, '');
      re8.exec('SbeprqRkcvengvba=633669358527244818');
      re8.exec('VC=66.249.85.130');
      re8.exec('FrffvbaQQS2=s15q53p9n372sn76npr13o271n4s3p5r29p235746p908p58');
      re8.exec('s15q53p9n372sn76npr13o271n4s3p5r29p235746p908p58');
      re24.exec(s61[i]);
    }
    for (var i = 0; i < 30; i++) {
      s65[i].replace(re6, '');
      /(?:^|\s+)gvzrfgnzc(?:\s+|$)/.exec(s66[i]);
      re7.exec(s65[i]);
    }
    for (var i = 0; i < 29; i++) {
      s62[i].replace(re23, '');
    }
    for (var i = 0; i < 28; i++) {
      s63[i].replace(re25, '');
      s63[i].replace(re12, '');
      re26.exec(s64[i]);
    }
  }
  var re27 = /-\D/g;
  var re28 = /\bnpgvingr\b/;
  var re29 = /%2R/gi;
  var re30 = /%2S/gi;
  var re31 = /^(mu-(PA|GJ)|wn|xb)$/;
  var re32 = /\s?;\s?/;
  var re33 = /%\w?$/;
  var re34 = /TNQP=([^;]*)/i;
  var str10 = 'FrffvbaQQS2=111soqs57qo8o8480qo18sor2011r3n591q7s6s37r120904; ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669315660164980&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
  var str11 = 'FrffvbaQQS2=111soqs57qo8o8480qo18sor2011r3n591q7s6s37r120904; __hgzm=144631658.1231363570.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar); __hgzn=144631658.3426875219718084000.1231363570.1231363570.1231363570.1; __hgzo=144631658.0.10.1231363570; __hgzp=144631658; ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669315660164980&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
  var str12 = 'uggc://tbbtyrnqf.t.qbhoyrpyvpx.arg/cntrnq/nqf?pyvrag=pn-svz_zlfcnpr_zlfcnpr-ubzrcntr_wf&qg=1231363514065&uy=ra&nqfnsr=uvtu&br=hgs8&ahz_nqf=4&bhgchg=wf&nqgrfg=bss&pbeeryngbe=1231363514065&punaary=svz_zlfcnpr_ubzrcntr_abgybttrqva%2Psvz_zlfcnpr_aba_HTP%2Psvz_zlfcnpr_havgrq-fgngrf&hey=uggc%3N%2S%2Subzr.zlfcnpr.pbz%2Svaqrk.psz&nq_glcr=grkg&rvq=6083027&rn=0&sez=0&tn_ivq=1326469221.1231363557&tn_fvq=1231363557&tn_uvq=1114636509&synfu=9.0.115&h_u=768&h_j=1024&h_nu=738&h_nj=1024&h_pq=24&h_gm=-480&h_uvf=2&h_wnin=gehr&h_acyht=7&h_azvzr=22';
  var str13 = 'ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669315660164980&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
  var str14 = 'ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669315660164980&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
  var re35 = /[<>]/g;
  var str15 = 'FrffvbaQQS2=s6r4579npn4rn2135s904r0s75pp1o5334p6s6pospo12696; ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669316860113296&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=; AFP_zp_dfctwzs-aowb_80=44132r503660';
  var str16 = 'FrffvbaQQS2=s6r4579npn4rn2135s904r0s75pp1o5334p6s6pospo12696; AFP_zp_dfctwzs-aowb_80=44132r503660; __hgzm=144631658.1231363638.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar); __hgzn=144631658.965867047679498800.1231363638.1231363638.1231363638.1; __hgzo=144631658.0.10.1231363638; __hgzp=144631658; ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669316860113296&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
  var str17 = 'uggc://tbbtyrnqf.t.qbhoyrpyvpx.arg/cntrnq/nqf?pyvrag=pn-svz_zlfcnpr_zlfcnpr-ubzrcntr_wf&qg=1231363621014&uy=ra&nqfnsr=uvtu&br=hgs8&ahz_nqf=4&bhgchg=wf&nqgrfg=bss&pbeeryngbe=1231363621014&punaary=svz_zlfcnpr_ubzrcntr_abgybttrqva%2Psvz_zlfcnpr_aba_HTP%2Psvz_zlfcnpr_havgrq-fgngrf&hey=uggc%3N%2S%2Scebsvyr.zlfcnpr.pbz%2Svaqrk.psz&nq_glcr=grkg&rvq=6083027&rn=0&sez=0&tn_ivq=348699119.1231363624&tn_fvq=1231363624&tn_uvq=895511034&synfu=9.0.115&h_u=768&h_j=1024&h_nu=738&h_nj=1024&h_pq=24&h_gm=-480&h_uvf=2&h_wnin=gehr&h_acyht=7&h_azvzr=22';
  var str18 = 'uggc://jjj.yrobapbva.se/yv';
  var str19 = 'ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669316860113296&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
  var str20 = 'ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669316860113296&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';

  var s67 = computeInputVariants('e115', 27);
  var s68 = computeInputVariants('qvfcynl', 27);
  var s69 = computeInputVariants('cbfvgvba', 27);
  var s70 = computeInputVariants('uggc://jjj.zlfcnpr.pbz/', 27);
  var s71 = computeInputVariants('cntrivrj', 27);
  var s72 = computeInputVariants('VC=74.125.75.3', 27);
  var s73 = computeInputVariants('ra', 27);
  var s74 = computeInputVariants(str10, 27);
  var s75 = computeInputVariants(str11, 27);
  var s76 = computeInputVariants(str12, 27);
  var s77 = computeInputVariants(str17, 27);
  var s78 = computeInputVariants(str18, 27);

  function runBlock3() {
    for (var i = 0; i < 27; i++) {
      s67[i].replace(/[A-Za-z]/g, '');
    }
    for (var i = 0; i < 23; i++) {
      s68[i].replace(re27, '');
      s69[i].replace(re27, '');
    }
    for (var i = 0; i < 22; i++) {
      'unaqyr'.replace(re14, '');
      'unaqyr'.replace(re15, '');
      'yvar'.replace(re14, '');
      'yvar'.replace(re15, '');
      'cnerag puebzr6 fvatyr1 gno'.replace(re14, '');
      'cnerag puebzr6 fvatyr1 gno'.replace(re15, '');
      'fyvqre'.replace(re14, '');
      'fyvqre'.replace(re15, '');
      re28.exec('');
    }
    for (var i = 0; i < 21; i++) {
      s70[i].replace(re12, '');
      re13.exec(s70[i]);
    }
    for (var i = 0; i < 20; i++) {
      s71[i].replace(re29, '');
      s71[i].replace(re30, '');
      re19.exec('ynfg');
      re19.exec('ba svefg');
      re8.exec(s72[i]);
    }
    for (var i = 0; i < 19; i++) {
      re31.exec(s73[i]);
    }
    for (var i = 0; i < 18; i++) {
      s74[i].split(re32);
      s75[i].split(re32);
      s76[i].replace(re33, '');
      re8.exec('144631658.0.10.1231363570');
      re8.exec('144631658.1231363570.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
      re8.exec('144631658.3426875219718084000.1231363570.1231363570.1231363570.1');
      re8.exec(str13);
      re8.exec(str14);
      re8.exec('__hgzn=144631658.3426875219718084000.1231363570.1231363570.1231363570.1');
      re8.exec('__hgzo=144631658.0.10.1231363570');
      re8.exec('__hgzm=144631658.1231363570.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
      re34.exec(s74[i]);
      re34.exec(s75[i]);
    }
    for (var i = 0; i < 17; i++) {
      s15[i].match(/zfvr/gi);
      s15[i].match(/bcren/gi);
      str15.split(re32);
      str16.split(re32);
      'ohggba'.replace(re14, '');
      'ohggba'.replace(re15, '');
      'puvyq p1 svefg sylbhg pybfrq'.replace(re14, '');
      'puvyq p1 svefg sylbhg pybfrq'.replace(re15, '');
      'pvgvrf'.replace(re14, '');
      'pvgvrf'.replace(re15, '');
      'pybfrq'.replace(re14, '');
      'pybfrq'.replace(re15, '');
      'qry'.replace(re14, '');
      'qry'.replace(re15, '');
      'uqy_zba'.replace(re14, '');
      'uqy_zba'.replace(re15, '');
      s77[i].replace(re33, '');
      s78[i].replace(/%3P/g, '');
      s78[i].replace(/%3R/g, '');
      s78[i].replace(/%3q/g, '');
      s78[i].replace(re35, '');
      'yvaxyvfg16'.replace(re14, '');
      'yvaxyvfg16'.replace(re15, '');
      'zvahf'.replace(re14, '');
      'zvahf'.replace(re15, '');
      'bcra'.replace(re14, '');
      'bcra'.replace(re15, '');
      'cnerag puebzr5 fvatyr1 ps NU'.replace(re14, '');
      'cnerag puebzr5 fvatyr1 ps NU'.replace(re15, '');
      'cynlre'.replace(re14, '');
      'cynlre'.replace(re15, '');
      'cyhf'.replace(re14, '');
      'cyhf'.replace(re15, '');
      'cb_uqy'.replace(re14, '');
      'cb_uqy'.replace(re15, '');
      'hyJVzt'.replace(re14, '');
      'hyJVzt'.replace(re15, '');
      re8.exec('144631658.0.10.1231363638');
      re8.exec('144631658.1231363638.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
      re8.exec('144631658.965867047679498800.1231363638.1231363638.1231363638.1');
      re8.exec('4413268q3660');
      re8.exec('4ss747o77904333q374or84qrr1s9r0nprp8r5q81534o94n');
      re8.exec('SbeprqRkcvengvba=633669321699093060');
      re8.exec('VC=74.125.75.20');
      re8.exec(str19);
      re8.exec(str20);
      re8.exec('AFP_zp_tfwsbrg-aowb_80=4413268q3660');
      re8.exec('FrffvbaQQS2=4ss747o77904333q374or84qrr1s9r0nprp8r5q81534o94n');
      re8.exec('__hgzn=144631658.965867047679498800.1231363638.1231363638.1231363638.1');
      re8.exec('__hgzo=144631658.0.10.1231363638');
      re8.exec('__hgzm=144631658.1231363638.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
      re34.exec(str15);
      re34.exec(str16);
    }
  }
  var re36 = /uers|fep|fryrpgrq/;
  var re37 = /\s*([+>~\s])\s*([a-zA-Z#.*:\[])/g;
  var re38 = /^(\w+|\*)$/;
  var str21 = 'FrffvbaQQS2=s15q53p9n372sn76npr13o271n4s3p5r29p235746p908p58; ZFPhygher=VC=66.249.85.130&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669358527244818&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
  var str22 = 'FrffvbaQQS2=s15q53p9n372sn76npr13o271n4s3p5r29p235746p908p58; __hgzm=144631658.1231367822.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar); __hgzn=144631658.4127520630321984500.1231367822.1231367822.1231367822.1; __hgzo=144631658.0.10.1231367822; __hgzp=144631658; ZFPhygher=VC=66.249.85.130&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669358527244818&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
  var str23 = 'uggc://tbbtyrnqf.t.qbhoyrpyvpx.arg/cntrnq/nqf?pyvrag=pn-svz_zlfcnpr_zlfcnpr-ubzrcntr_wf&qg=1231367803797&uy=ra&nqfnsr=uvtu&br=hgs8&ahz_nqf=4&bhgchg=wf&nqgrfg=bss&pbeeryngbe=1231367803797&punaary=svz_zlfcnpr_ubzrcntr_abgybttrqva%2Psvz_zlfcnpr_aba_HTP%2Psvz_zlfcnpr_havgrq-fgngrf&hey=uggc%3N%2S%2Szrffntvat.zlfcnpr.pbz%2Svaqrk.psz&nq_glcr=grkg&rvq=6083027&rn=0&sez=0&tn_ivq=1192552091.1231367807&tn_fvq=1231367807&tn_uvq=1155446857&synfu=9.0.115&h_u=768&h_j=1024&h_nu=738&h_nj=1024&h_pq=24&h_gm=-480&h_uvf=2&h_wnin=gehr&h_acyht=7&h_azvzr=22';
  var str24 = 'ZFPhygher=VC=66.249.85.130&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669358527244818&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
  var str25 = 'ZFPhygher=VC=66.249.85.130&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669358527244818&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
  var str26 = 'hy.ynat-fryrpgbe';
  var re39 = /\\/g;
  var re40 = / /g;
  var re41 = /\/\xc4\/t/;
  var re42 = /\/\xd6\/t/;
  var re43 = /\/\xdc\/t/;
  var re44 = /\/\xdf\/t/;
  var re45 = /\/\xe4\/t/;
  var re46 = /\/\xf6\/t/;
  var re47 = /\/\xfc\/t/;
  var re48 = /\W/g;
  var re49 = /uers|fep|fglyr/;
  var s79 = computeInputVariants(str21, 16);
  var s80 = computeInputVariants(str22, 16);
  var s81 = computeInputVariants(str23, 16);
  var s82 = computeInputVariants(str26, 16);

  function runBlock4() {
    for (var i = 0; i < 16; i++) {
      ''.replace(/\*/g, '');
      /\bnpgvir\b/.exec('npgvir');
      /sversbk/i.exec(s15[i]);
      re36.exec('glcr');
      /zfvr/i.exec(s15[i]);
      /bcren/i.exec(s15[i]);
    }
    for (var i = 0; i < 15; i++) {
      s79[i].split(re32);
      s80[i].split(re32);
      'uggc://ohyyrgvaf.zlfcnpr.pbz/vaqrk.psz'.replace(re12, '');
      s81[i].replace(re33, '');
      'yv'.replace(re37, '');
      'yv'.replace(re18, '');
      re8.exec('144631658.0.10.1231367822');
      re8.exec('144631658.1231367822.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
      re8.exec('144631658.4127520630321984500.1231367822.1231367822.1231367822.1');
      re8.exec(str24);
      re8.exec(str25);
      re8.exec('__hgzn=144631658.4127520630321984500.1231367822.1231367822.1231367822.1');
      re8.exec('__hgzo=144631658.0.10.1231367822');
      re8.exec('__hgzm=144631658.1231367822.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
      re34.exec(s79[i]);
      re34.exec(s80[i]);
      /\.([\w-]+)|\[(\w+)(?:([!*^$~|]?=)["']?(.*?)["']?)?\]|:([\w-]+)(?:\(["']?(.*?)?["']?\)|$)/g.exec(s82[i]);
      re13.exec('uggc://ohyyrgvaf.zlfcnpr.pbz/vaqrk.psz');
      re38.exec('yv');
    }
    for (var i = 0; i < 14; i++) {
      ''.replace(re18, '');
      '9.0  e115'.replace(/(\s+e|\s+o[0-9]+)/, '');
      'Funer guvf tnqtrg'.replace(/</g, '');
      'Funer guvf tnqtrg'.replace(/>/g, '');
      'Funer guvf tnqtrg'.replace(re39, '');
      'uggc://cebsvyrrqvg.zlfcnpr.pbz/vaqrk.psz'.replace(re12, '');
      'grnfre'.replace(re40, '');
      'grnfre'.replace(re41, '');
      'grnfre'.replace(re42, '');
      'grnfre'.replace(re43, '');
      'grnfre'.replace(re44, '');
      'grnfre'.replace(re45, '');
      'grnfre'.replace(re46, '');
      'grnfre'.replace(re47, '');
      'grnfre'.replace(re48, '');
      re16.exec('znetva-gbc');
      re16.exec('cbfvgvba');
      re19.exec('gno1');
      re9.exec('qz');
      re9.exec('qg');
      re9.exec('zbqobk');
      re9.exec('zbqobkva');
      re9.exec('zbqgvgyr');
      re13.exec('uggc://cebsvyrrqvg.zlfcnpr.pbz/vaqrk.psz');
      re26.exec('/vt/znvytnqtrg');
      re49.exec('glcr');
    }
  }
  var re50 = /(?:^|\s+)fryrpgrq(?:\s+|$)/;
  var re51 = /\&/g;
  var re52 = /\+/g;
  var re53 = /\?/g;
  var re54 = /\t/g;
  var re55 = /(\$\{nqiHey\})|(\$nqiHey\b)/g;
  var re56 = /(\$\{cngu\})|(\$cngu\b)/g;
  function runBlock5() {
    for (var i = 0; i < 13; i++) {
      'purpx'.replace(re14, '');
      'purpx'.replace(re15, '');
      'pvgl'.replace(re14, '');
      'pvgl'.replace(re15, '');
      'qrpe fyvqrgrkg'.replace(re14, '');
      'qrpe fyvqrgrkg'.replace(re15, '');
      'svefg fryrpgrq'.replace(re14, '');
      'svefg fryrpgrq'.replace(re15, '');
      'uqy_rag'.replace(re14, '');
      'uqy_rag'.replace(re15, '');
      'vape fyvqrgrkg'.replace(re14, '');
      'vape fyvqrgrkg'.replace(re15, '');
      'vachggrkg QBZPbageby_cynprubyqre'.replace(re5, '');
      'cnerag puebzr6 fvatyr1 gno fryrpgrq'.replace(re14, '');
      'cnerag puebzr6 fvatyr1 gno fryrpgrq'.replace(re15, '');
      'cb_guz'.replace(re14, '');
      'cb_guz'.replace(re15, '');
      'fhozvg'.replace(re14, '');
      'fhozvg'.replace(re15, '');
      re50.exec('');
      /NccyrJroXvg\/([^\s]*)/.exec(s15[i]);
      /XUGZY/.exec(s15[i]);
    }
    for (var i = 0; i < 12; i++) {
      '${cebg}://${ubfg}${cngu}/${dz}'.replace(/(\$\{cebg\})|(\$cebg\b)/g, '');
      '1'.replace(re40, '');
      '1'.replace(re10, '');
      '1'.replace(re51, '');
      '1'.replace(re52, '');
      '1'.replace(re53, '');
      '1'.replace(re39, '');
      '1'.replace(re54, '');
      '9.0  e115'.replace(/^(.*)\..*$/, '');
      '9.0  e115'.replace(/^.*e(.*)$/, '');
      '<!-- ${nqiHey} -->'.replace(re55, '');
      '<fpevcg glcr="grkg/wninfpevcg" fep="${nqiHey}"></fpevcg>'.replace(re55, '');
      s21[i].replace(/^.*\s+(\S+\s+\S+$)/, '');
      'tzk%2Subzrcntr%2Sfgneg%2Sqr%2S'.replace(re30, '');
      'tzk'.replace(re30, '');
      'uggc://${ubfg}${cngu}/${dz}'.replace(/(\$\{ubfg\})|(\$ubfg\b)/g, '');
      'uggc://nqpyvrag.hvzfrei.arg${cngu}/${dz}'.replace(re56, '');
      'uggc://nqpyvrag.hvzfrei.arg/wf.at/${dz}'.replace(/(\$\{dz\})|(\$dz\b)/g, '');
      'frpgvba'.replace(re29, '');
      'frpgvba'.replace(re30, '');
      'fvgr'.replace(re29, '');
      'fvgr'.replace(re30, '');
      'fcrpvny'.replace(re29, '');
      'fcrpvny'.replace(re30, '');
      re36.exec('anzr');
      /e/.exec('9.0  e115');
    }
  }
  var re57 = /##yv4##/gi;
  var re58 = /##yv16##/gi;
  var re59 = /##yv19##/gi;
  var str27 = '<hy pynff="nqi">##yv4##Cbjreshy Zvpebfbsg grpuabybtl urycf svtug fcnz naq vzcebir frphevgl.##yv19##Trg zber qbar gunaxf gb terngre rnfr naq fcrrq.##yv16##Ybgf bs fgbentr &#40;5 TO&#41; - zber pbby fghss ba gur jnl.##OE## ##OE## ##N##Yrnea zber##/N##</hy>';
  var str28 = '<hy pynff="nqi"><yv vq="YvOYG4" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg4.cat)">Cbjreshy Zvpebfbsg grpuabybtl urycf svtug fcnz naq vzcebir frphevgl.##yv19##Trg zber qbar gunaxf gb terngre rnfr naq fcrrq.##yv16##Ybgf bs fgbentr &#40;5 TO&#41; - zber pbby fghss ba gur jnl.##OE## ##OE## ##N##Yrnea zber##/N##</hy>';
  var str29 = '<hy pynff="nqi"><yv vq="YvOYG4" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg4.cat)">Cbjreshy Zvpebfbsg grpuabybtl urycf svtug fcnz naq vzcebir frphevgl.##yv19##Trg zber qbar gunaxf gb terngre rnfr naq fcrrq.<yv vq="YvOYG16" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg16.cat)">Ybgf bs fgbentr &#40;5 TO&#41; - zber pbby fghss ba gur jnl.##OE## ##OE## ##N##Yrnea zber##/N##</hy>';
  var str30 = '<hy pynff="nqi"><yv vq="YvOYG4" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg4.cat)">Cbjreshy Zvpebfbsg grpuabybtl urycf svtug fcnz naq vzcebir frphevgl.<yv vq="YvOYG19" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg19.cat)">Trg zber qbar gunaxf gb terngre rnfr naq fcrrq.<yv vq="YvOYG16" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg16.cat)">Ybgf bs fgbentr &#40;5 TO&#41; - zber pbby fghss ba gur jnl.##OE## ##OE## ##N##Yrnea zber##/N##</hy>';
  var str31 = '<hy pynff="nqi"><yv vq="YvOYG4" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg4.cat)">Cbjreshy Zvpebfbsg grpuabybtl urycf svtug fcnz naq vzcebir frphevgl.<yv vq="YvOYG19" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg19.cat)">Trg zber qbar gunaxf gb terngre rnfr naq fcrrq.<yv vq="YvOYG16" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg16.cat)">Ybgf bs fgbentr &#40;5 TO&#41; - zber pbby fghss ba gur jnl.<oe> <oe> ##N##Yrnea zber##/N##</hy>';
  var str32 = '<hy pynff="nqi"><yv vq="YvOYG4" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg4.cat)">Cbjreshy Zvpebfbsg grpuabybtl urycf svtug fcnz naq vzcebir frphevgl.<yv vq="YvOYG19" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg19.cat)">Trg zber qbar gunaxf gb terngre rnfr naq fcrrq.<yv vq="YvOYG16" fglyr="onpxtebhaq-vzntr:hey(uggc://vzt.jykef.pbz/~Yvir.FvgrPbagrag.VQ/~14.2.1230/~/~/~/oyg16.cat)">Ybgf bs fgbentr &#40;5 TO&#41; - zber pbby fghss ba gur jnl.<oe> <oe> <n uers="uggc://znvy.yvir.pbz/znvy/nobhg.nfck" gnetrg="_oynax">Yrnea zber##/N##</hy>';
  var str33 = 'Bar Jvaqbjf Yvir VQ trgf lbh vagb <o>Ubgznvy</o>, <o>Zrffratre</o>, <o>Kobk YVIR</o> \u2014 naq bgure cynprf lbh frr #~#argjbexybtb#~#';
  var re60 = /(?:^|\s+)bss(?:\s+|$)/;
  var re61 = /^(([^:\/?#]+):)?(\/\/([^\/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?$/;
  var re62 = /^[^<]*(<(.|\s)+>)[^>]*$|^#(\w+)$/;
  var str34 = '${1}://${2}${3}${4}${5}';
  var str35 = ' O=6gnyg0g4znrrn&o=3&f=gc; Q=_lyu=K3bQZGSxnT4lZzD3OS9GNmV3ZGLkAQxRpTyxNmRlZmRmAmNkAQLRqTImqNZjOUEgpTjQnJ5xMKtgoN--; SCF=qy';
  var s83 = computeInputVariants(str27, 11);
  var s84 = computeInputVariants(str28, 11);
  var s85 = computeInputVariants(str29, 11);
  var s86 = computeInputVariants(str30, 11);
  var s87 = computeInputVariants(str31, 11);
  var s88 = computeInputVariants(str32, 11);
  var s89 = computeInputVariants(str33, 11);
  var s90 = computeInputVariants(str34, 11);

  function runBlock6() {
    for (var i = 0; i < 11; i++) {
      s83[i].replace(/##yv0##/gi, '');
      s83[i].replace(re57, '');
      s84[i].replace(re58, '');
      s85[i].replace(re59, '');
      s86[i].replace(/##\/o##/gi, '');
      s86[i].replace(/##\/v##/gi, '');
      s86[i].replace(/##\/h##/gi, '');
      s86[i].replace(/##o##/gi, '');
      s86[i].replace(/##oe##/gi, '');
      s86[i].replace(/##v##/gi, '');
      s86[i].replace(/##h##/gi, '');
      s87[i].replace(/##n##/gi, '');
      s88[i].replace(/##\/n##/gi, '');
      s89[i].replace(/#~#argjbexybtb#~#/g, '');
      / Zbovyr\//.exec(s15[i]);
      /##yv1##/gi.exec(s83[i]);
      /##yv10##/gi.exec(s84[i]);
      /##yv11##/gi.exec(s84[i]);
      /##yv12##/gi.exec(s84[i]);
      /##yv13##/gi.exec(s84[i]);
      /##yv14##/gi.exec(s84[i]);
      /##yv15##/gi.exec(s84[i]);
      re58.exec(s84[i]);
      /##yv17##/gi.exec(s85[i]);
      /##yv18##/gi.exec(s85[i]);
      re59.exec(s85[i]);
      /##yv2##/gi.exec(s83[i]);
      /##yv20##/gi.exec(s86[i]);
      /##yv21##/gi.exec(s86[i]);
      /##yv22##/gi.exec(s86[i]);
      /##yv23##/gi.exec(s86[i]);
      /##yv3##/gi.exec(s83[i]);
      re57.exec(s83[i]);
      /##yv5##/gi.exec(s84[i]);
      /##yv6##/gi.exec(s84[i]);
      /##yv7##/gi.exec(s84[i]);
      /##yv8##/gi.exec(s84[i]);
      /##yv9##/gi.exec(s84[i]);
      re8.exec('473qq1rs0n2r70q9qo1pq48n021s9468ron90nps048p4p29');
      re8.exec('SbeprqRkcvengvba=633669325184628362');
      re8.exec('FrffvbaQQS2=473qq1rs0n2r70q9qo1pq48n021s9468ron90nps048p4p29');
      /AbxvnA[^\/]*/.exec(s15[i]);
    }
    for (var i = 0; i < 10; i++) {
      ' bss'.replace(/(?:^|\s+)bss(?:\s+|$)/g, '');
      s90[i].replace(/(\$\{0\})|(\$0\b)/g, '');
      s90[i].replace(/(\$\{1\})|(\$1\b)/g, '');
      s90[i].replace(/(\$\{pbzcyrgr\})|(\$pbzcyrgr\b)/g, '');
      s90[i].replace(/(\$\{sentzrag\})|(\$sentzrag\b)/g, '');
      s90[i].replace(/(\$\{ubfgcbeg\})|(\$ubfgcbeg\b)/g, '');
      s90[i].replace(re56, '');
      s90[i].replace(/(\$\{cebgbpby\})|(\$cebgbpby\b)/g, '');
      s90[i].replace(/(\$\{dhrel\})|(\$dhrel\b)/g, '');
      'nqfvmr'.replace(re29, '');
      'nqfvmr'.replace(re30, '');
      'uggc://${2}${3}${4}${5}'.replace(/(\$\{2\})|(\$2\b)/g, '');
      'uggc://wf.hv-cbegny.qr${3}${4}${5}'.replace(/(\$\{3\})|(\$3\b)/g, '');
      'arjf'.replace(re40, '');
      'arjf'.replace(re41, '');
      'arjf'.replace(re42, '');
      'arjf'.replace(re43, '');
      'arjf'.replace(re44, '');
      'arjf'.replace(re45, '');
      'arjf'.replace(re46, '');
      'arjf'.replace(re47, '');
      'arjf'.replace(re48, '');
      / PC=i=(\d+)&oe=(.)/.exec(str35);
      re60.exec(' ');
      re60.exec(' bss');
      re60.exec('');
      re19.exec(' ');
      re19.exec('svefg ba');
      re19.exec('ynfg vtaber');
      re19.exec('ba');
      re9.exec('scnq so ');
      re9.exec('zrqvgobk');
      re9.exec('hsgy');
      re9.exec('lhv-h');
      /Fnsnev|Xbadhrebe|XUGZY/gi.exec(s15[i]);
      re61.exec('uggc://wf.hv-cbegny.qr/tzk/ubzr/wf/20080602/onfr.wf');
      re62.exec('#Ybtva_rznvy');
    }
  }
  var re63 = /\{0\}/g;
  var str36 = 'FrffvbaQQS2=4ss747o77904333q374or84qrr1s9r0nprp8r5q81534o94n; ZFPhygher=VC=74.125.75.20&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669321699093060&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=; AFP_zp_tfwsbrg-aowb_80=4413268q3660';
  var str37 = 'FrffvbaQQS2=4ss747o77904333q374or84qrr1s9r0nprp8r5q81534o94n; AFP_zp_tfwsbrg-aowb_80=4413268q3660; __hgzm=144631658.1231364074.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar); __hgzn=144631658.2294274870215848400.1231364074.1231364074.1231364074.1; __hgzo=144631658.0.10.1231364074; __hgzp=144631658; ZFPhygher=VC=74.125.75.20&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669321699093060&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
  var str38 = 'uggc://tbbtyrnqf.t.qbhoyrpyvpx.arg/cntrnq/nqf?pyvrag=pn-svz_zlfcnpr_zlfcnpr-ubzrcntr_wf&qg=1231364057761&uy=ra&nqfnsr=uvtu&br=hgs8&ahz_nqf=4&bhgchg=wf&nqgrfg=bss&pbeeryngbe=1231364057761&punaary=svz_zlfcnpr_ubzrcntr_abgybttrqva%2Psvz_zlfcnpr_aba_HTP%2Psvz_zlfcnpr_havgrq-fgngrf&hey=uggc%3N%2S%2Ssevraqf.zlfcnpr.pbz%2Svaqrk.psz&nq_glcr=grkg&rvq=6083027&rn=0&sez=0&tn_ivq=1667363813.1231364061&tn_fvq=1231364061&tn_uvq=1917563877&synfu=9.0.115&h_u=768&h_j=1024&h_nu=738&h_nj=1024&h_pq=24&h_gm=-480&h_uvf=2&h_wnin=gehr&h_acyht=7&h_azvzr=22';
  var str39 = 'ZFPhygher=VC=74.125.75.20&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669321699093060&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
  var str40 = 'ZFPhygher=VC=74.125.75.20&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669321699093060&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
  var s91 = computeInputVariants(str36, 9);
  var s92 = computeInputVariants(str37, 9);
  var s93 = computeInputVariants(str38, 9);
  function runBlock7() {
    for (var i = 0; i < 9; i++) {
      '0'.replace(re40, '');
      '0'.replace(re10, '');
      '0'.replace(re51, '');
      '0'.replace(re52, '');
      '0'.replace(re53, '');
      '0'.replace(re39, '');
      '0'.replace(re54, '');
      'Lrf'.replace(re40, '');
      'Lrf'.replace(re10, '');
      'Lrf'.replace(re51, '');
      'Lrf'.replace(re52, '');
      'Lrf'.replace(re53, '');
      'Lrf'.replace(re39, '');
      'Lrf'.replace(re54, '');
    }
    for (var i = 0; i < 8; i++) {
      'Pybfr {0}'.replace(re63, '');
      'Bcra {0}'.replace(re63, '');
      s91[i].split(re32);
      s92[i].split(re32);
      'puvyq p1 svefg gnournqref'.replace(re14, '');
      'puvyq p1 svefg gnournqref'.replace(re15, '');
      'uqy_fcb'.replace(re14, '');
      'uqy_fcb'.replace(re15, '');
      'uvag'.replace(re14, '');
      'uvag'.replace(re15, '');
      s93[i].replace(re33, '');
      'yvfg'.replace(re14, '');
      'yvfg'.replace(re15, '');
      'at_bhgre'.replace(re30, '');
      'cnerag puebzr5 qbhoyr2 NU'.replace(re14, '');
      'cnerag puebzr5 qbhoyr2 NU'.replace(re15, '');
      'cnerag puebzr5 dhnq5 ps NU osyvax zbarl'.replace(re14, '');
      'cnerag puebzr5 dhnq5 ps NU osyvax zbarl'.replace(re15, '');
      'cnerag puebzr6 fvatyr1'.replace(re14, '');
      'cnerag puebzr6 fvatyr1'.replace(re15, '');
      'cb_qrs'.replace(re14, '');
      'cb_qrs'.replace(re15, '');
      'gnopbagrag'.replace(re14, '');
      'gnopbagrag'.replace(re15, '');
      'iv_svefg_gvzr'.replace(re30, '');
      /(^|.)(ronl|qri-ehf3.wbg)(|fgberf|zbgbef|yvirnhpgvbaf|jvxv|rkcerff|punggre).(pbz(|.nh|.pa|.ux|.zl|.ft|.oe|.zk)|pb(.hx|.xe|.am)|pn|qr|se|vg|ay|or|ng|pu|vr|va|rf|cy|cu|fr)$/i.exec('cntrf.ronl.pbz');
      re8.exec('144631658.0.10.1231364074');
      re8.exec('144631658.1231364074.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
      re8.exec('144631658.2294274870215848400.1231364074.1231364074.1231364074.1');
      re8.exec('4413241q3660');
      re8.exec('SbeprqRkcvengvba=633669357391353591');
      re8.exec(str39);
      re8.exec(str40);
      re8.exec('AFP_zp_kkk-gdzogv_80=4413241q3660');
      re8.exec('FrffvbaQQS2=p98s8o9q42nr21or1r61pqorn1n002nsss569635984s6qp7');
      re8.exec('__hgzn=144631658.2294274870215848400.1231364074.1231364074.1231364074.1');
      re8.exec('__hgzo=144631658.0.10.1231364074');
      re8.exec('__hgzm=144631658.1231364074.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
      re8.exec('p98s8o9q42nr21or1r61pqorn1n002nsss569635984s6qp7');
      re34.exec(s91[i]);
      re34.exec(s92[i]);
    }
  }
  var re64 = /\b[a-z]/g;
  var re65 = /^uggc:\/\//;
  var re66 = /(?:^|\s+)qvfnoyrq(?:\s+|$)/;
  var str41 = 'uggc://cebsvyr.zlfcnpr.pbz/Zbqhyrf/Nccyvpngvbaf/Cntrf/Pnainf.nfck';
  function runBlock8() {
    for (var i = 0; i < 7; i++) {
      s21[i].match(/\d+/g);
      'nsgre'.replace(re64, '');
      'orsber'.replace(re64, '');
      'obggbz'.replace(re64, '');
      'ohvygva_jrngure.kzy'.replace(re65, '');
      'ohggba'.replace(re37, '');
      'ohggba'.replace(re18, '');
      'qngrgvzr.kzy'.replace(re65, '');
      'uggc://eff.paa.pbz/eff/paa_gbcfgbevrf.eff'.replace(re65, '');
      'vachg'.replace(re37, '');
      'vachg'.replace(re18, '');
      'vafvqr'.replace(re64, '');
      'cbvagre'.replace(re27, '');
      'cbfvgvba'.replace(/[A-Z]/g, '');
      'gbc'.replace(re27, '');
      'gbc'.replace(re64, '');
      'hy'.replace(re37, '');
      'hy'.replace(re18, '');
      str26.replace(re37, '');
      str26.replace(re18, '');
      'lbhghor_vtbbtyr/i2/lbhghor.kzy'.replace(re65, '');
      'm-vaqrk'.replace(re27, '');
      /#([\w-]+)/.exec(str26);
      re16.exec('urvtug');
      re16.exec('znetvaGbc');
      re16.exec('jvqgu');
      re19.exec('gno0 svefg ba');
      re19.exec('gno0 ba');
      re19.exec('gno4 ynfg');
      re19.exec('gno4');
      re19.exec('gno5');
      re19.exec('gno6');
      re19.exec('gno7');
      re19.exec('gno8');
      /NqborNVE\/([^\s]*)/.exec(s15[i]);
      /NccyrJroXvg\/([^ ]*)/.exec(s15[i]);
      /XUGZY/gi.exec(s15[i]);
      /^(?:obql|ugzy)$/i.exec('YV');
      re38.exec('ohggba');
      re38.exec('vachg');
      re38.exec('hy');
      re38.exec(str26);
      /^(\w+|\*)/.exec(str26);
      /znp|jva|yvahk/i.exec('Jva32');
      /eton?\([\d\s,]+\)/.exec('fgngvp');
    }
    for (var i = 0; i < 6; i++) {
      ''.replace(/\r/g, '');
      '/'.replace(re40, '');
      '/'.replace(re10, '');
      '/'.replace(re51, '');
      '/'.replace(re52, '');
      '/'.replace(re53, '');
      '/'.replace(re39, '');
      '/'.replace(re54, '');
      'uggc://zfacbegny.112.2b7.arg/o/ff/zfacbegnyubzr/1/U.7-cqi-2/{0}?[NDO]&{1}&{2}&[NDR]'.replace(re63, '');
      str41.replace(re12, '');
      'uggc://jjj.snprobbx.pbz/fepu.cuc'.replace(re23, '');
      'freivpr'.replace(re40, '');
      'freivpr'.replace(re41, '');
      'freivpr'.replace(re42, '');
      'freivpr'.replace(re43, '');
      'freivpr'.replace(re44, '');
      'freivpr'.replace(re45, '');
      'freivpr'.replace(re46, '');
      'freivpr'.replace(re47, '');
      'freivpr'.replace(re48, '');
      /((ZFVR\s+([6-9]|\d\d)\.))/.exec(s15[i]);
      re66.exec('');
      re50.exec('fryrpgrq');
      re8.exec('8sqq78r9n442851q565599o401385sp3s04r92rnn7o19ssn');
      re8.exec('SbeprqRkcvengvba=633669340386893867');
      re8.exec('VC=74.125.75.17');
      re8.exec('FrffvbaQQS2=8sqq78r9n442851q565599o401385sp3s04r92rnn7o19ssn');
      /Xbadhrebe|Fnsnev|XUGZY/.exec(s15[i]);
      re13.exec(str41);
      re49.exec('unfsbphf');
    }
  }
  var re67 = /zrah_byq/g;
  var str42 = 'FrffvbaQQS2=473qq1rs0n2r70q9qo1pq48n021s9468ron90nps048p4p29; ZFPhygher=VC=74.125.75.3&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669325184628362&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
  var str43 = 'FrffvbaQQS2=473qq1rs0n2r70q9qo1pq48n021s9468ron90nps048p4p29; __hgzm=144631658.1231364380.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar); __hgzn=144631658.3931862196947939300.1231364380.1231364380.1231364380.1; __hgzo=144631658.0.10.1231364380; __hgzp=144631658; ZFPhygher=VC=74.125.75.3&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669325184628362&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
  var str44 = 'uggc://tbbtyrnqf.t.qbhoyrpyvpx.arg/cntrnq/nqf?pyvrag=pn-svz_zlfcnpr_vzntrf_wf&qg=1231364373088&uy=ra&nqfnsr=uvtu&br=hgs8&ahz_nqf=4&bhgchg=wf&nqgrfg=bss&pbeeryngbe=1231364373088&punaary=svz_zlfcnpr_hfre-ivrj-pbzzragf%2Psvz_zlfcnpr_havgrq-fgngrf&hey=uggc%3N%2S%2Spbzzrag.zlfcnpr.pbz%2Svaqrk.psz&nq_glcr=grkg&rvq=6083027&rn=0&sez=0&tn_ivq=1158737789.1231364375&tn_fvq=1231364375&tn_uvq=415520832&synfu=9.0.115&h_u=768&h_j=1024&h_nu=738&h_nj=1024&h_pq=24&h_gm=-480&h_uvf=2&h_wnin=gehr&h_acyht=7&h_azvzr=22';
  var str45 = 'ZFPhygher=VC=74.125.75.3&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669325184628362&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
  var str46 = 'ZFPhygher=VC=74.125.75.3&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669325184628362&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
  var re68 = /^([#.]?)((?:[\w\u0128-\uffff*_-]|\\.)*)/;
  var re69 = /\{1\}/g;
  var re70 = /\s+/;
  var re71 = /(\$\{4\})|(\$4\b)/g;
  var re72 = /(\$\{5\})|(\$5\b)/g;
  var re73 = /\{2\}/g;
  var re74 = /[^+>] [^+>]/;
  var re75 = /\bucpyv\s*=\s*([^;]*)/i;
  var re76 = /\bucuvqr\s*=\s*([^;]*)/i;
  var re77 = /\bucfie\s*=\s*([^;]*)/i;
  var re78 = /\bhfucjrn\s*=\s*([^;]*)/i;
  var re79 = /\bmvc\s*=\s*([^;]*)/i;
  var re80 = /^((?:[\w\u0128-\uffff*_-]|\\.)+)(#)((?:[\w\u0128-\uffff*_-]|\\.)+)/;
  var re81 = /^([>+~])\s*(\w*)/i;
  var re82 = /^>\s*((?:[\w\u0128-\uffff*_-]|\\.)+)/;
  var re83 = /^[\s[]?shapgvba/;
  var re84 = /v\/g.tvs#(.*)/i;
  var str47 = '#Zbq-Vasb-Vasb-WninFpevcgUvag';
  var str48 = ',n.svryqOgaPnapry';
  var str49 = 'FrffvbaQQS2=p98s8o9q42nr21or1r61pqorn1n002nsss569635984s6qp7; ZFPhygher=VC=74.125.75.3&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669357391353591&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=; AFP_zp_kkk-gdzogv_80=4413241q3660';
  var str50 = 'FrffvbaQQS2=p98s8o9q42nr21or1r61pqorn1n002nsss569635984s6qp7; AFP_zp_kkk-gdzogv_80=4413241q3660; AFP_zp_kkk-aowb_80=4413235p3660; __hgzm=144631658.1231367708.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar); __hgzn=144631658.2770915348920628700.1231367708.1231367708.1231367708.1; __hgzo=144631658.0.10.1231367708; __hgzp=144631658; ZFPhygher=VC=74.125.75.3&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669357391353591&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
  var str51 = 'uggc://tbbtyrnqf.t.qbhoyrpyvpx.arg/cntrnq/nqf?pyvrag=pn-svz_zlfcnpr_zlfcnpr-ubzrcntr_wf&qg=1231367691141&uy=ra&nqfnsr=uvtu&br=hgs8&ahz_nqf=4&bhgchg=wf&nqgrfg=bss&pbeeryngbe=1231367691141&punaary=svz_zlfcnpr_ubzrcntr_abgybttrqva%2Psvz_zlfcnpr_aba_HTP%2Psvz_zlfcnpr_havgrq-fgngrf&hey=uggc%3N%2S%2Sjjj.zlfcnpr.pbz%2S&nq_glcr=grkg&rvq=6083027&rn=0&sez=0&tn_ivq=320757904.1231367694&tn_fvq=1231367694&tn_uvq=1758792003&synfu=9.0.115&h_u=768&h_j=1024&h_nu=738&h_nj=1024&h_pq=24&h_gm=-480&h_uvf=2&h_wnin=gehr&h_acyht=7&h_azvzr=22';
  var str52 = 'uggc://zfacbegny.112.2b7.arg/o/ff/zfacbegnyubzr/1/U.7-cqi-2/f55332979829981?[NDO]&aqu=1&g=7%2S0%2S2009%2014%3N38%3N42%203%20480&af=zfacbegny&cntrAnzr=HF%20UCZFSGJ&t=uggc%3N%2S%2Sjjj.zfa.pbz%2S&f=1024k768&p=24&x=L&oj=994&ou=634&uc=A&{2}&[NDR]';
  var str53 = 'cnerag puebzr6 fvatyr1 gno fryrpgrq ovaq qbhoyr2 ps';
  var str54 = 'ZFPhygher=VC=74.125.75.3&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669357391353591&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
  var str55 = 'ZFPhygher=VC=74.125.75.3&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669357391353591&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
  var str56 = 'ne;ng;nh;or;oe;pn;pu;py;pa;qr;qx;rf;sv;se;to;ux;vq;vr;va;vg;wc;xe;zk;zl;ay;ab;am;cu;cy;cg;eh;fr;ft;gu;ge;gj;mn;';
  var str57 = 'ZP1=I=3&THVQ=6nnpr9q661804s33nnop45nosqp17q85; zu=ZFSG; PHYGHER=RA-HF; SyvtugTebhcVq=97; SyvtugVq=OnfrCntr; ucfie=Z:5|S:5|G:5|R:5|Q:oyh|J:S; ucpyv=J.U|Y.|F.|E.|H.Y|P.|U.; hfucjrn=jp:HFPN0746; ZHVQ=Q783SN9O14054831N4869R51P0SO8886&GHVQ=1';
  var str58 = 'ZP1=I=3&THVQ=6nnpr9q661804s33nnop45nosqp17q85; zu=ZFSG; PHYGHER=RA-HF; SyvtugTebhcVq=97; SyvtugVq=OnfrCntr; ucfie=Z:5|S:5|G:5|R:5|Q:oyh|J:S; ucpyv=J.U|Y.|F.|E.|H.Y|P.|U.; hfucjrn=jp:HFPN0746; ZHVQ=Q783SN9O14054831N4869R51P0SO8886';
  var str59 = 'ZP1=I=3&THVQ=6nnpr9q661804s33nnop45nosqp17q85; zu=ZFSG; PHYGHER=RA-HF; SyvtugTebhcVq=97; SyvtugVq=OnfrCntr; ucfie=Z:5|S:5|G:5|R:5|Q:oyh|J:S; ucpyv=J.U|Y.|F.|E.|H.Y|P.|U.; hfucjrn=jp:HFPN0746; ZHVQ=Q783SN9O14054831N4869R51P0SO8886; mvc=m:94043|yn:37.4154|yb:-122.0585|p:HF|ue:1';
  var str60 = 'ZP1=I=3&THVQ=6nnpr9q661804s33nnop45nosqp17q85; zu=ZFSG; PHYGHER=RA-HF; SyvtugTebhcVq=97; SyvtugVq=OnfrCntr; ucfie=Z:5|S:5|G:5|R:5|Q:oyh|J:S; ucpyv=J.U|Y.|F.|E.|H.Y|P.|U.; hfucjrn=jp:HFPN0746; ZHVQ=Q783SN9O14054831N4869R51P0SO8886; mvc=m:94043|yn:37.4154|yb:-122.0585|p:HF';
  var str61 = 'uggc://gx2.fgp.f-zfa.pbz/oe/uc/11/ra-hf/pff/v/g.tvs#uggc://gx2.fgo.f-zfa.pbz/v/29/4RQP4969777N048NPS4RRR3PO2S7S.wct';
  var str62 = 'uggc://gx2.fgp.f-zfa.pbz/oe/uc/11/ra-hf/pff/v/g.tvs#uggc://gx2.fgo.f-zfa.pbz/v/OQ/63NP9O94NS5OQP1249Q9S1ROP7NS3.wct';
  var str63 = 'zbmvyyn/5.0 (jvaqbjf; h; jvaqbjf ag 5.1; ra-hf) nccyrjroxvg/528.9 (xugzy, yvxr trpxb) puebzr/2.0.157.0 fnsnev/528.9';
  var s94 = computeInputVariants(str42, 5);
  var s95 = computeInputVariants(str43, 5);
  var s96 = computeInputVariants(str44, 5);
  var s97 = computeInputVariants(str47, 5);
  var s98 = computeInputVariants(str48, 5);
  var s99 = computeInputVariants(str49, 5);
  var s100 = computeInputVariants(str50, 5);
  var s101 = computeInputVariants(str51, 5);
  var s102 = computeInputVariants(str52, 5);
  var s103 = computeInputVariants(str53, 5);

  function runBlock9() {
    for (var i = 0; i < 5; i++) {
      s94[i].split(re32);
      s95[i].split(re32);
      'svz_zlfcnpr_hfre-ivrj-pbzzragf,svz_zlfcnpr_havgrq-fgngrf'.split(re20);
      s96[i].replace(re33, '');
      'zrah_arj zrah_arj_gbttyr zrah_gbttyr'.replace(re67, '');
      'zrah_byq zrah_byq_gbttyr zrah_gbttyr'.replace(re67, '');
      re8.exec('102n9o0o9pq60132qn0337rr867p75953502q2s27s2s5r98');
      re8.exec('144631658.0.10.1231364380');
      re8.exec('144631658.1231364380.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
      re8.exec('144631658.3931862196947939300.1231364380.1231364380.1231364380.1');
      re8.exec('441326q33660');
      re8.exec('SbeprqRkcvengvba=633669341278771470');
      re8.exec(str45);
      re8.exec(str46);
      re8.exec('AFP_zp_dfctwzssrwh-aowb_80=441326q33660');
      re8.exec('FrffvbaQQS2=102n9o0o9pq60132qn0337rr867p75953502q2s27s2s5r98');
      re8.exec('__hgzn=144631658.3931862196947939300.1231364380.1231364380.1231364380.1');
      re8.exec('__hgzo=144631658.0.10.1231364380');
      re8.exec('__hgzm=144631658.1231364380.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
    }
    for (var i = 0; i < 4; i++) {
      ' yvfg1'.replace(re14, '');
      ' yvfg1'.replace(re15, '');
      ' yvfg2'.replace(re14, '');
      ' yvfg2'.replace(re15, '');
      ' frneputebhc1'.replace(re14, '');
      ' frneputebhc1'.replace(re15, '');
      s97[i].replace(re68, '');
      s97[i].replace(re18, '');
      ''.replace(/&/g, '');
      ''.replace(re35, '');
      '(..-{0})(\|(\d+)|)'.replace(re63, '');
      s98[i].replace(re18, '');
      '//vzt.jro.qr/vij/FC/${cngu}/${anzr}/${inyhr}?gf=${abj}'.replace(re56, '');
      '//vzt.jro.qr/vij/FC/tzk_uc/${anzr}/${inyhr}?gf=${abj}'.replace(/(\$\{anzr\})|(\$anzr\b)/g, '');
      '<fcna pynff="urnq"><o>Jvaqbjf Yvir Ubgznvy</o></fcna><fcna pynff="zft">{1}</fcna>'.replace(re69, '');
      '<fcna pynff="urnq"><o>{0}</o></fcna><fcna pynff="zft">{1}</fcna>'.replace(re63, '');
      '<fcna pynff="fvtahc"><n uers=uggc://jjj.ubgznvy.pbz><o>{1}</o></n></fcna>'.replace(re69, '');
      '<fcna pynff="fvtahc"><n uers={0}><o>{1}</o></n></fcna>'.replace(re63, '');
      'Vzntrf'.replace(re15, '');
      'ZFA'.replace(re15, '');
      'Zncf'.replace(re15, '');
      'Zbq-Vasb-Vasb-WninFpevcgUvag'.replace(re39, '');
      'Arjf'.replace(re15, '');
      s99[i].split(re32);
      s100[i].split(re32);
      'Ivqrb'.replace(re15, '');
      'Jro'.replace(re15, '');
      'n'.replace(re39, '');
      'nwnkFgneg'.split(re70);
      'nwnkFgbc'.split(re70);
      'ovaq'.replace(re14, '');
      'ovaq'.replace(re15, '');
      'oevatf lbh zber. Zber fcnpr (5TO), zber frphevgl, fgvyy serr.'.replace(re63, '');
      'puvyq p1 svefg qrpx'.replace(re14, '');
      'puvyq p1 svefg qrpx'.replace(re15, '');
      'puvyq p1 svefg qbhoyr2'.replace(re14, '');
      'puvyq p1 svefg qbhoyr2'.replace(re15, '');
      'puvyq p2 ynfg'.replace(re14, '');
      'puvyq p2 ynfg'.replace(re15, '');
      'puvyq p2'.replace(re14, '');
      'puvyq p2'.replace(re15, '');
      'puvyq p3'.replace(re14, '');
      'puvyq p3'.replace(re15, '');
      'puvyq p4 ynfg'.replace(re14, '');
      'puvyq p4 ynfg'.replace(re15, '');
      'pbclevtug'.replace(re14, '');
      'pbclevtug'.replace(re15, '');
      'qZFAZR_1'.replace(re14, '');
      'qZFAZR_1'.replace(re15, '');
      'qbhoyr2 ps'.replace(re14, '');
      'qbhoyr2 ps'.replace(re15, '');
      'qbhoyr2'.replace(re14, '');
      'qbhoyr2'.replace(re15, '');
      'uqy_arj'.replace(re14, '');
      'uqy_arj'.replace(re15, '');
      'uc_fubccvatobk'.replace(re30, '');
      'ugzy%2Rvq'.replace(re29, '');
      'ugzy%2Rvq'.replace(re30, '');
      s101[i].replace(re33, '');
      'uggc://wf.hv-cbegny.qr/tzk/ubzr/wf/20080602/cebgbglcr.wf${4}${5}'.replace(re71, '');
      'uggc://wf.hv-cbegny.qr/tzk/ubzr/wf/20080602/cebgbglcr.wf${5}'.replace(re72, '');
      s102[i].replace(re73, '');
      'uggc://zfacbegny.112.2b7.arg/o/ff/zfacbegnyubzr/1/U.7-cqi-2/f55332979829981?[NDO]&{1}&{2}&[NDR]'.replace(re69, '');
      'vztZFSG'.replace(re14, '');
      'vztZFSG'.replace(re15, '');
      'zfasbbg1 ps'.replace(re14, '');
      'zfasbbg1 ps'.replace(re15, '');
      s103[i].replace(re14, '');
      s103[i].replace(re15, '');
      'cnerag puebzr6 fvatyr1 gno fryrpgrq ovaq'.replace(re14, '');
      'cnerag puebzr6 fvatyr1 gno fryrpgrq ovaq'.replace(re15, '');
      'cevznel'.replace(re14, '');
      'cevznel'.replace(re15, '');
      'erpgnatyr'.replace(re30, '');
      'frpbaqnel'.replace(re14, '');
      'frpbaqnel'.replace(re15, '');
      'haybnq'.split(re70);
      '{0}{1}1'.replace(re63, '');
      '|{1}1'.replace(re69, '');
      /(..-HF)(\|(\d+)|)/i.exec('xb-xe,ra-va,gu-gu');
      re4.exec('/ZlFcnprNccf/NccPnainf,45000012');
      re8.exec('144631658.0.10.1231367708');
      re8.exec('144631658.1231367708.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
      re8.exec('144631658.2770915348920628700.1231367708.1231367708.1231367708.1');
      re8.exec('4413235p3660');
      re8.exec('441327q73660');
      re8.exec('9995p6rp12rrnr893334ro7nq70o7p64p69rqn844prs1473');
      re8.exec('SbeprqRkcvengvba=633669350559478880');
      re8.exec(str54);
      re8.exec(str55);
      re8.exec('AFP_zp_dfctwzs-aowb_80=441327q73660');
      re8.exec('AFP_zp_kkk-aowb_80=4413235p3660');
      re8.exec('FrffvbaQQS2=9995p6rp12rrnr893334ro7nq70o7p64p69rqn844prs1473');
      re8.exec('__hgzn=144631658.2770915348920628700.1231367708.1231367708.1231367708.1');
      re8.exec('__hgzo=144631658.0.10.1231367708');
      re8.exec('__hgzm=144631658.1231367708.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
      re34.exec(s99[i]);
      re34.exec(s100[i]);
      /ZFVR\s+5[.]01/.exec(s15[i]);
      /HF(?=;)/i.exec(str56);
      re74.exec(s97[i]);
      re28.exec('svefg npgvir svefgNpgvir');
      re28.exec('ynfg');
      /\bp:(..)/i.exec('m:94043|yn:37.4154|yb:-122.0585|p:HF');
      re75.exec(str57);
      re75.exec(str58);
      re76.exec(str57);
      re76.exec(str58);
      re77.exec(str57);
      re77.exec(str58);
      /\bhfucce\s*=\s*([^;]*)/i.exec(str59);
      re78.exec(str57);
      re78.exec(str58);
      /\bjci\s*=\s*([^;]*)/i.exec(str59);
      re79.exec(str58);
      re79.exec(str60);
      re79.exec(str59);
      /\|p:([a-z]{2})/i.exec('m:94043|yn:37.4154|yb:-122.0585|p:HF|ue:1');
      re80.exec(s97[i]);
      re61.exec('cebgbglcr.wf');
      re68.exec(s97[i]);
      re81.exec(s97[i]);
      re82.exec(s97[i]);
      /^Fubpxjnir Synfu (\d)/.exec(s21[i]);
      /^Fubpxjnir Synfu (\d+)/.exec(s21[i]);
      re83.exec('[bowrpg tybony]');
      re62.exec(s97[i]);
      re84.exec(str61);
      re84.exec(str62);
      /jroxvg/.exec(str63);
    }
  }
  var re85 = /eaq_zbqobkva/;
  var str64 = '1231365729213';
  var str65 = '74.125.75.3-1057165600.29978900';
  var str66 = '74.125.75.3-1057165600.29978900.1231365730214';
  var str67 = 'Frnepu%20Zvpebfbsg.pbz';
  var str68 = 'FrffvbaQQS2=8sqq78r9n442851q565599o401385sp3s04r92rnn7o19ssn; ZFPhygher=VC=74.125.75.17&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669340386893867&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
  var str69 = 'FrffvbaQQS2=8sqq78r9n442851q565599o401385sp3s04r92rnn7o19ssn; __hgzm=144631658.1231365779.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar); __hgzn=144631658.1877536177953918500.1231365779.1231365779.1231365779.1; __hgzo=144631658.0.10.1231365779; __hgzp=144631658; ZFPhygher=VC=74.125.75.17&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669340386893867&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
  var str70 = 'I=3%26THVQ=757q3ss871q44o7o805n8113n5p72q52';
  var str71 = 'I=3&THVQ=757q3ss871q44o7o805n8113n5p72q52';
  var str72 = 'uggc://tbbtyrnqf.t.qbhoyrpyvpx.arg/cntrnq/nqf?pyvrag=pn-svz_zlfcnpr_zlfcnpr-ubzrcntr_wf&qg=1231365765292&uy=ra&nqfnsr=uvtu&br=hgs8&ahz_nqf=4&bhgchg=wf&nqgrfg=bss&pbeeryngbe=1231365765292&punaary=svz_zlfcnpr_ubzrcntr_abgybttrqva%2Psvz_zlfcnpr_aba_HTP%2Psvz_zlfcnpr_havgrq-fgngrf&hey=uggc%3N%2S%2Sohyyrgvaf.zlfcnpr.pbz%2Svaqrk.psz&nq_glcr=grkg&rvq=6083027&rn=0&sez=0&tn_ivq=1579793869.1231365768&tn_fvq=1231365768&tn_uvq=2056210897&synfu=9.0.115&h_u=768&h_j=1024&h_nu=738&h_nj=1024&h_pq=24&h_gm=-480&h_uvf=2&h_wnin=gehr&h_acyht=7&h_azvzr=22';
  var str73 = 'frnepu.zvpebfbsg.pbz';
  var str74 = 'frnepu.zvpebfbsg.pbz/';
  var str75 = 'ZFPhygher=VC=74.125.75.17&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669340386893867&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
  var str76 = 'ZFPhygher=VC=74.125.75.17&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669340386893867&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
  function runBlock10() {
    for (var i = 0; i < 3; i++) {
      '%3Szxg=ra-HF'.replace(re39, '');
      '-8'.replace(re40, '');
      '-8'.replace(re10, '');
      '-8'.replace(re51, '');
      '-8'.replace(re52, '');
      '-8'.replace(re53, '');
      '-8'.replace(re39, '');
      '-8'.replace(re54, '');
      '1.5'.replace(re40, '');
      '1.5'.replace(re10, '');
      '1.5'.replace(re51, '');
      '1.5'.replace(re52, '');
      '1.5'.replace(re53, '');
      '1.5'.replace(re39, '');
      '1.5'.replace(re54, '');
      '1024k768'.replace(re40, '');
      '1024k768'.replace(re10, '');
      '1024k768'.replace(re51, '');
      '1024k768'.replace(re52, '');
      '1024k768'.replace(re53, '');
      '1024k768'.replace(re39, '');
      '1024k768'.replace(re54, '');
      str64.replace(re40, '');
      str64.replace(re10, '');
      str64.replace(re51, '');
      str64.replace(re52, '');
      str64.replace(re53, '');
      str64.replace(re39, '');
      str64.replace(re54, '');
      '14'.replace(re40, '');
      '14'.replace(re10, '');
      '14'.replace(re51, '');
      '14'.replace(re52, '');
      '14'.replace(re53, '');
      '14'.replace(re39, '');
      '14'.replace(re54, '');
      '24'.replace(re40, '');
      '24'.replace(re10, '');
      '24'.replace(re51, '');
      '24'.replace(re52, '');
      '24'.replace(re53, '');
      '24'.replace(re39, '');
      '24'.replace(re54, '');
      str65.replace(re40, '');
      str65.replace(re10, '');
      str65.replace(re51, '');
      str65.replace(re52, '');
      str65.replace(re53, '');
      str65.replace(re39, '');
      str65.replace(re54, '');
      str66.replace(re40, '');
      str66.replace(re10, '');
      str66.replace(re51, '');
      str66.replace(re52, '');
      str66.replace(re53, '');
      str66.replace(re39, '');
      str66.replace(re54, '');
      '9.0'.replace(re40, '');
      '9.0'.replace(re10, '');
      '9.0'.replace(re51, '');
      '9.0'.replace(re52, '');
      '9.0'.replace(re53, '');
      '9.0'.replace(re39, '');
      '9.0'.replace(re54, '');
      '994k634'.replace(re40, '');
      '994k634'.replace(re10, '');
      '994k634'.replace(re51, '');
      '994k634'.replace(re52, '');
      '994k634'.replace(re53, '');
      '994k634'.replace(re39, '');
      '994k634'.replace(re54, '');
      '?zxg=ra-HF'.replace(re40, '');
      '?zxg=ra-HF'.replace(re10, '');
      '?zxg=ra-HF'.replace(re51, '');
      '?zxg=ra-HF'.replace(re52, '');
      '?zxg=ra-HF'.replace(re53, '');
      '?zxg=ra-HF'.replace(re54, '');
      'PAA.pbz'.replace(re25, '');
      'PAA.pbz'.replace(re12, '');
      'PAA.pbz'.replace(re39, '');
      'Qngr & Gvzr'.replace(re25, '');
      'Qngr & Gvzr'.replace(re12, '');
      'Qngr & Gvzr'.replace(re39, '');
      'Frnepu Zvpebfbsg.pbz'.replace(re40, '');
      'Frnepu Zvpebfbsg.pbz'.replace(re54, '');
      str67.replace(re10, '');
      str67.replace(re51, '');
      str67.replace(re52, '');
      str67.replace(re53, '');
      str67.replace(re39, '');
      str68.split(re32);
      str69.split(re32);
      str70.replace(re52, '');
      str70.replace(re53, '');
      str70.replace(re39, '');
      str71.replace(re40, '');
      str71.replace(re10, '');
      str71.replace(re51, '');
      str71.replace(re54, '');
      'Jrngure'.replace(re25, '');
      'Jrngure'.replace(re12, '');
      'Jrngure'.replace(re39, '');
      'LbhGhor'.replace(re25, '');
      'LbhGhor'.replace(re12, '');
      'LbhGhor'.replace(re39, '');
      str72.replace(re33, '');
      'erzbgr_vsenzr_1'.replace(/^erzbgr_vsenzr_/, '');
      str73.replace(re40, '');
      str73.replace(re10, '');
      str73.replace(re51, '');
      str73.replace(re52, '');
      str73.replace(re53, '');
      str73.replace(re39, '');
      str73.replace(re54, '');
      str74.replace(re40, '');
      str74.replace(re10, '');
      str74.replace(re51, '');
      str74.replace(re52, '');
      str74.replace(re53, '');
      str74.replace(re39, '');
      str74.replace(re54, '');
      'lhv-h'.replace(/\-/g, '');
      re9.exec('p');
      re9.exec('qz p');
      re9.exec('zbqynory');
      re9.exec('lhv-h svefg');
      re8.exec('144631658.0.10.1231365779');
      re8.exec('144631658.1231365779.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
      re8.exec('144631658.1877536177953918500.1231365779.1231365779.1231365779.1');
      re8.exec(str75);
      re8.exec(str76);
      re8.exec('__hgzn=144631658.1877536177953918500.1231365779.1231365779.1231365779.1');
      re8.exec('__hgzo=144631658.0.10.1231365779');
      re8.exec('__hgzm=144631658.1231365779.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
      re34.exec(str68);
      re34.exec(str69);
      /^$/.exec('');
      re31.exec('qr');
      /^znk\d+$/.exec('');
      /^zva\d+$/.exec('');
      /^erfgber$/.exec('');
      re85.exec('zbqobkva zbqobk_abcnqqvat ');
      re85.exec('zbqgvgyr');
      re85.exec('eaq_zbqobkva ');
      re85.exec('eaq_zbqgvgyr ');
      /frpgvba\d+_pbagragf/.exec('obggbz_ani');
    }
  }
  var re86 = /;\s*/;
  var re87 = /(\$\{inyhr\})|(\$inyhr\b)/g;
  var re88 = /(\$\{abj\})|(\$abj\b)/g;
  var re89 = /\s+$/;
  var re90 = /^\s+/;
  var re91 = /(\\\"|\x00-|\x1f|\x7f-|\x9f|\u00ad|\u0600-|\u0604|\u070f|\u17b4|\u17b5|\u200c-|\u200f|\u2028-|\u202f|\u2060-|\u206f|\ufeff|\ufff0-|\uffff)/g;
  var re92 = /^(:)([\w-]+)\("?'?(.*?(\(.*?\))?[^(]*?)"?'?\)/;
  var re93 = /^([:.#]*)((?:[\w\u0128-\uffff*_-]|\\.)+)/;
  var re94 = /^(\[) *@?([\w-]+) *([!*$^~=]*) *('?"?)(.*?)\4 *\]/;
  var str77 = '#fubhgobk .pybfr';
  var str78 = 'FrffvbaQQS2=102n9o0o9pq60132qn0337rr867p75953502q2s27s2s5r98; ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669341278771470&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=; AFP_zp_dfctwzssrwh-aowb_80=441326q33660';
  var str79 = 'FrffvbaQQS2=102n9o0o9pq60132qn0337rr867p75953502q2s27s2s5r98; AFP_zp_dfctwzssrwh-aowb_80=441326q33660; __hgzm=144631658.1231365869.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar); __hgzn=144631658.1670816052019209000.1231365869.1231365869.1231365869.1; __hgzo=144631658.0.10.1231365869; __hgzp=144631658; ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669341278771470&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
  var str80 = 'FrffvbaQQS2=9995p6rp12rrnr893334ro7nq70o7p64p69rqn844prs1473; ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669350559478880&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=; AFP_zp_dfctwzs-aowb_80=441327q73660';
  var str81 = 'FrffvbaQQS2=9995p6rp12rrnr893334ro7nq70o7p64p69rqn844prs1473; AFP_zp_dfctwzs-aowb_80=441327q73660; __hgzm=144631658.1231367054.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar); __hgzn=144631658.1796080716621419500.1231367054.1231367054.1231367054.1; __hgzo=144631658.0.10.1231367054; __hgzp=144631658; ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669350559478880&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
  var str82 = '[glcr=fhozvg]';
  var str83 = 'n.svryqOga,n.svryqOgaPnapry';
  var str84 = 'n.svryqOgaPnapry';
  var str85 = 'oyvpxchaxg';
  var str86 = 'qvi.bow-nppbeqvba qg';
  var str87 = 'uggc://tbbtyrnqf.t.qbhoyrpyvpx.arg/cntrnq/nqf?pyvrag=pn-svz_zlfcnpr_nccf_wf&qg=1231367052227&uy=ra&nqfnsr=uvtu&br=hgs8&ahz_nqf=4&bhgchg=wf&nqgrfg=bss&pbeeryngbe=1231367052227&punaary=svz_zlfcnpr_nccf-pnainf%2Psvz_zlfcnpr_havgrq-fgngrf&hey=uggc%3N%2S%2Scebsvyr.zlfcnpr.pbz%2SZbqhyrf%2SNccyvpngvbaf%2SCntrf%2SPnainf.nfck&nq_glcr=grkg&rvq=6083027&rn=0&sez=1&tn_ivq=716357910.1231367056&tn_fvq=1231367056&tn_uvq=1387206491&synfu=9.0.115&h_u=768&h_j=1024&h_nu=738&h_nj=1024&h_pq=24&h_gm=-480&h_uvf=2&h_wnin=gehr&h_acyht=7&h_azvzr=22';
  var str88 = 'uggc://tbbtyrnqf.t.qbhoyrpyvpx.arg/cntrnq/nqf?pyvrag=pn-svz_zlfcnpr_zlfcnpr-ubzrcntr_wf&qg=1231365851658&uy=ra&nqfnsr=uvtu&br=hgs8&ahz_nqf=4&bhgchg=wf&nqgrfg=bss&pbeeryngbe=1231365851658&punaary=svz_zlfcnpr_ubzrcntr_abgybttrqva%2Psvz_zlfcnpr_aba_HTP%2Psvz_zlfcnpr_havgrq-fgngrf&hey=uggc%3N%2S%2Scebsvyrrqvg.zlfcnpr.pbz%2Svaqrk.psz&nq_glcr=grkg&rvq=6083027&rn=0&sez=0&tn_ivq=1979828129.1231365855&tn_fvq=1231365855&tn_uvq=2085229649&synfu=9.0.115&h_u=768&h_j=1024&h_nu=738&h_nj=1024&h_pq=24&h_gm=-480&h_uvf=2&h_wnin=gehr&h_acyht=7&h_azvzr=22';
  var str89 = 'uggc://zfacbegny.112.2b7.arg/o/ff/zfacbegnyubzr/1/U.7-cqi-2/f55023338617756?[NDO]&aqu=1&g=7%2S0%2S2009%2014%3N12%3N47%203%20480&af=zfacbegny&cntrAnzr=HF%20UCZFSGJ&t=uggc%3N%2S%2Sjjj.zfa.pbz%2S&f=0k0&p=43835816&x=A&oj=994&ou=634&uc=A&{2}&[NDR]';
  var str90 = 'zrgn[anzr=nwnkHey]';
  var str91 = 'anpuevpugra';
  var str92 = 'b oS={\'oT\':1.1};x $8n(B){z(B!=o9)};x $S(B){O(!$8n(B))z A;O(B.4L)z\'T\';b S=7t B;O(S==\'2P\'&&B.p4){23(B.7f){12 1:z\'T\';12 3:z/\S/.2g(B.8M)?\'ox\':\'oh\'}}O(S==\'2P\'||S==\'x\'){23(B.nE){12 2V:z\'1O\';12 7I:z\'5a\';12 18:z\'4B\'}O(7t B.I==\'4F\'){O(B.3u)z\'pG\';O(B.8e)z\'1p\'}}z S};x $2p(){b 4E={};Z(b v=0;v<1p.I;v++){Z(b X 1o 1p[v]){b nc=1p[v][X];b 6E=4E[X];O(6E&&$S(nc)==\'2P\'&&$S(6E)==\'2P\')4E[X]=$2p(6E,nc);17 4E[X]=nc}}z 4E};b $E=7p.E=x(){b 1d=1p;O(!1d[1])1d=[p,1d[0]];Z(b X 1o 1d[1])1d[0][X]=1d[1][X];z 1d[0]};b $4D=7p.pJ=x(){Z(b v=0,y=1p.I;v<y;v++){1p[v].E=x(1J){Z(b 1I 1o 1J){O(!p.1Y[1I])p.1Y[1I]=1J[1I];O(!p[1I])p[1I]=$4D.6C(1I)}}}};$4D.6C=x(1I){z x(L){z p.1Y[1I].3H(L,2V.1Y.nV.1F(1p,1))}};$4D(7F,2V,6J,nb);b 3l=x(B){B=B||{};B.E=$E;z B};b pK=Y 3l(H);b pZ=Y 3l(C);C.6f=C.35(\'6f\')[0];x $2O(B){z!!(B||B===0)};x $5S(B,n8){z $8n(B)?B:n8};x $7K(3c,1m){z 1q.na(1q.7K()*(1m-3c+1)+3c)};x $3N(){z Y 97().os()};x $4M(1U){pv(1U);pa(1U);z 1S};H.43=!!(C.5Z);O(H.nB)H.31=H[H.7q?\'py\':\'nL\']=1r;17 O(C.9N&&!C.om&&!oy.oZ)H.pF=H.4Z=H[H.43?\'pt\':\'65\']=1r;17 O(C.po!=1S)H.7J=1r;O(7t 5B==\'o9\'){b 5B=x(){};O(H.4Z)C.nd("pW");5B.1Y=(H.4Z)?H["[[oN.1Y]]"]:{}}5B.1Y.4L=1r;O(H.nL)5s{C.oX("pp",A,1r)}4K(r){};b 18=x(1X){b 63=x(){z(1p[0]!==1S&&p.1w&&$S(p.1w)==\'x\')?p.1w.3H(p,1p):p};$E(63,p);63.1Y=1X;63.nE=18;z 63};18.1z=x(){};18.1Y={E:x(1X){b 7x=Y p(1S);Z(b X 1o 1X){b nC=7x[X];7x[X]=18.nY(nC,1X[X])}z Y 18(7x)},3d:x(){Z(b v=0,y=1p.I;v<y;v++)$E(p.1Y,1p[v])}};18.nY=x(2b,2n){O(2b&&2b!=2n){b S=$S(2n);O(S!=$S(2b))z 2n;23(S){12\'x\':b 7R=x(){p.1e=1p.8e.1e;z 2n.3H(p,1p)};7R.1e=2b;z 7R;12\'2P\':z $2p(2b,2n)}}z 2n};b 8o=Y 18({oQ:x(J){p.4w=p.4w||[];p.4w.1x(J);z p},7g:x(){O(p.4w&&p.4w.I)p.4w.9J().2x(10,p)},oP:x(){p.4w=[]}});b 2d=Y 18({1V:x(S,J){O(J!=18.1z){p.$19=p.$19||{};p.$19[S]=p.$19[S]||[];p.$19[S].5j(J)}z p},1v:x(S,1d,2x){O(p.$19&&p.$19[S]){p.$19[S].1b(x(J){J.3n({\'L\':p,\'2x\':2x,\'1p\':1d})()},p)}z p},3M:x(S,J){O(p.$19&&p.$19[S])p.$19[S].2U(J);z p}});b 4v=Y 18({2H:x(){p.P=$2p.3H(1S,[p.P].E(1p));O(!p.1V)z p;Z(b 3O 1o p.P){O($S(p.P[3O]==\'x\')&&3O.2g(/^5P[N-M]/))p.1V(3O,p.P[3O])}z p}});2V.E({7y:x(J,L){Z(b v=0,w=p.I;v<w;v++)J.1F(L,p[v],v,p)},3s:x(J,L){b 54=[];Z(b v=0,w=p.I;v<w;v++){O(J.1F(L,p[v],v,p))54.1x(p[v])}z 54},2X:x(J,L){b 54=[];Z(b v=0,w=p.I;v<w;v++)54[v]=J.1F(L,p[v],v,p);z 54},4i:x(J,L){Z(b v=0,w=p.I;v<w;v++){O(!J.1F(L,p[v],v,p))z A}z 1r},ob:x(J,L){Z(b v=0,w=p.I;v<w;v++){O(J.1F(L,p[v],v,p))z 1r}z A},3F:x(3u,15){b 3A=p.I;Z(b v=(15<0)?1q.1m(0,3A+15):15||0;v<3A;v++){O(p[v]===3u)z v}z-1},8z:x(1u,I){1u=1u||0;O(1u<0)1u=p.I+1u;I=I||(p.I-1u);b 89=[];Z(b v=0;v<I;v++)89[v]=p[1u++];z 89},2U:x(3u){b v=0;b 3A=p.I;6L(v<3A){O(p[v]===3u){p.6l(v,1);3A--}17{v++}}z p},1y:x(3u,15){z p.3F(3u,15)!=-1},oz:x(1C){b B={},I=1q.3c(p.I,1C.I);Z(b v=0;v<I;v++)B[1C[v]]=p[v];z B},E:x(1O){Z(b v=0,w=1O.I;v<w;v++)p.1x(1O[v]);z p},2p:x(1O){Z(b v=0,y=1O.I;v<y;v++)p.5j(1O[v]);z p},5j:x(3u){O(!p.1y(3u))p.1x(3u);z p},oc:x(){z p[$7K(0,p.I-1)]||A},7L:x(){z p[p.I-1]||A}});2V.1Y.1b=2V.1Y.7y;2V.1Y.2g=2V.1Y.1y;x $N(1O){z 2V.8z(1O)};x $1b(3J,J,L){O(3J&&7t 3J.I==\'4F\'&&$S(3J)!=\'2P\')2V.7y(3J,J,L);17 Z(b 1j 1o 3J)J.1F(L||3J,3J[1j],1j)};6J.E({2g:x(6b,2F){z(($S(6b)==\'2R\')?Y 7I(6b,2F):6b).2g(p)},3p:x(){z 5K(p,10)},o4:x(){z 69(p)},7A:x(){z p.3y(/-\D/t,x(2G){z 2G.7G(1).nW()})},9b:x(){z p.3y(/\w[N-M]/t,x(2G){z(2G.7G(0)+\'-\'+2G.7G(1).5O())})},8V:x(){z p.3y(/\b[n-m]/t,x(2G){z 2G.nW()})},5L:x(){z p.3y(/^\s+|\s+$/t,\'\')},7j:x(){z p.3y(/\s{2,}/t,\' \').5L()},5V:x(1O){b 1i=p.2G(/\d{1,3}/t);z(1i)?1i.5V(1O):A},5U:x(1O){b 3P=p.2G(/^#?(\w{1,2})(\w{1,2})(\w{1,2})$/);z(3P)?3P.nV(1).5U(1O):A},1y:x(2R,f){z(f)?(f+p+f).3F(f+2R+f)>-1:p.3F(2R)>-1},nX:x(){z p.3y(/([.*+?^${}()|[\]\/\\])/t,\'\\$1\')}});2V.E({5V:x(1O){O(p.I<3)z A;O(p.I==4&&p[3]==0&&!1O)z\'p5\';b 3P=[];Z(b v=0;v<3;v++){b 52=(p[v]-0).4h(16);3P.1x((52.I==1)?\'0\'+52:52)}z 1O?3P:\'#\'+3P.2u(\'\')},5U:x(1O){O(p.I!=3)z A;b 1i=[];Z(b v=0;v<3;v++){1i.1x(5K((p[v].I==1)?p[v]+p[v]:p[v],16))}z 1O?1i:\'1i(\'+1i.2u(\',\')+\')\'}});7F.E({3n:x(P){b J=p;P=$2p({\'L\':J,\'V\':A,\'1p\':1S,\'2x\':A,\'4s\':A,\'6W\':A},P);O($2O(P.1p)&&$S(P.1p)!=\'1O\')P.1p=[P.1p];z x(V){b 1d;O(P.V){V=V||H.V;1d=[(P.V===1r)?V:Y P.V(V)];O(P.1p)1d.E(P.1p)}17 1d=P.1p||1p;b 3C=x(){z J.3H($5S(P';
  var str93 = 'hagreunyghat';
  var str94 = 'ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669341278771470&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
  var str95 = 'ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&Pbhagel=IIZ%3Q&SbeprqRkcvengvba=633669350559478880&gvzrMbar=-8&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R%3Q';
  var str96 = 'ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669341278771470&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
  var str97 = 'ZFPhygher=VC=74.125.75.1&VCPhygher=ra-HF&CersreerqPhygher=ra-HF&CersreerqPhygherCraqvat=&Pbhagel=IIZ=&SbeprqRkcvengvba=633669350559478880&gvzrMbar=0&HFEYBP=DKWyLHAiMTH9AwHjWxAcqUx9GJ91oaEunJ4tIzyyqlMQo3IhqUW5D29xMG1IHlMQo3IhqUW5GzSgMG1Iozy0MJDtH3EuqTImWxEgLHAiMTH9BQN3WxkuqTy0qJEyCGZ3YwDkBGVzGT9hM2y0qJEyCF0kZwVhZQH3APMDo3A0LJkQo2EyCGx0ZQDmWyWyM2yiox5uoJH9D0R=';
  var str98 = 'shapgvba (){Cuk.Nccyvpngvba.Frghc.Pber();Cuk.Nccyvpngvba.Frghc.Nwnk();Cuk.Nccyvpngvba.Frghc.Synfu();Cuk.Nccyvpngvba.Frghc.Zbqhyrf()}';
  function runBlock11() {
    for (var i = 0; i < 2; i++) {
      ' .pybfr'.replace(re18, '');
      ' n.svryqOgaPnapry'.replace(re18, '');
      ' qg'.replace(re18, '');
      str77.replace(re68, '');
      str77.replace(re18, '');
      ''.replace(re39, '');
      ''.replace(/^/, '');
      ''.split(re86);
      '*'.replace(re39, '');
      '*'.replace(re68, '');
      '*'.replace(re18, '');
      '.pybfr'.replace(re68, '');
      '.pybfr'.replace(re18, '');
      '//vzt.jro.qr/vij/FC/tzk_uc/fperra/${inyhr}?gf=${abj}'.replace(re87, '');
      '//vzt.jro.qr/vij/FC/tzk_uc/fperra/1024?gf=${abj}'.replace(re88, '');
      '//vzt.jro.qr/vij/FC/tzk_uc/jvafvmr/${inyhr}?gf=${abj}'.replace(re87, '');
      '//vzt.jro.qr/vij/FC/tzk_uc/jvafvmr/992/608?gf=${abj}'.replace(re88, '');
      '300k120'.replace(re30, '');
      '300k250'.replace(re30, '');
      '310k120'.replace(re30, '');
      '310k170'.replace(re30, '');
      '310k250'.replace(re30, '');
      '9.0  e115'.replace(/^.*\.(.*)\s.*$/, '');
      'Nppbeqvba'.replace(re2, '');
      'Nxghryy\x0a'.replace(re89, '');
      'Nxghryy\x0a'.replace(re90, '');
      'Nccyvpngvba'.replace(re2, '');
      'Oyvpxchaxg\x0a'.replace(re89, '');
      'Oyvpxchaxg\x0a'.replace(re90, '');
      'Svanamra\x0a'.replace(re89, '');
      'Svanamra\x0a'.replace(re90, '');
      'Tnzrf\x0a'.replace(re89, '');
      'Tnzrf\x0a'.replace(re90, '');
      'Ubebfxbc\x0a'.replace(re89, '');
      'Ubebfxbc\x0a'.replace(re90, '');
      'Xvab\x0a'.replace(re89, '');
      'Xvab\x0a'.replace(re90, '');
      'Zbqhyrf'.replace(re2, '');
      'Zhfvx\x0a'.replace(re89, '');
      'Zhfvx\x0a'.replace(re90, '');
      'Anpuevpugra\x0a'.replace(re89, '');
      'Anpuevpugra\x0a'.replace(re90, '');
      'Cuk'.replace(re2, '');
      'ErdhrfgSvavfu'.split(re70);
      'ErdhrfgSvavfu.NWNK.Cuk'.split(re70);
      'Ebhgr\x0a'.replace(re89, '');
      'Ebhgr\x0a'.replace(re90, '');
      str78.split(re32);
      str79.split(re32);
      str80.split(re32);
      str81.split(re32);
      'Fcbeg\x0a'.replace(re89, '');
      'Fcbeg\x0a'.replace(re90, '');
      'GI-Fcbg\x0a'.replace(re89, '');
      'GI-Fcbg\x0a'.replace(re90, '');
      'Gbhe\x0a'.replace(re89, '');
      'Gbhe\x0a'.replace(re90, '');
      'Hagreunyghat\x0a'.replace(re89, '');
      'Hagreunyghat\x0a'.replace(re90, '');
      'Ivqrb\x0a'.replace(re89, '');
      'Ivqrb\x0a'.replace(re90, '');
      'Jrggre\x0a'.replace(re89, '');
      'Jrggre\x0a'.replace(re90, '');
      str82.replace(re68, '');
      str82.replace(re18, '');
      str83.replace(re68, '');
      str83.replace(re18, '');
      str84.replace(re68, '');
      str84.replace(re18, '');
      'nqiFreivprObk'.replace(re30, '');
      'nqiFubccvatObk'.replace(re30, '');
      'nwnk'.replace(re39, '');
      'nxghryy'.replace(re40, '');
      'nxghryy'.replace(re41, '');
      'nxghryy'.replace(re42, '');
      'nxghryy'.replace(re43, '');
      'nxghryy'.replace(re44, '');
      'nxghryy'.replace(re45, '');
      'nxghryy'.replace(re46, '');
      'nxghryy'.replace(re47, '');
      'nxghryy'.replace(re48, '');
      str85.replace(re40, '');
      str85.replace(re41, '');
      str85.replace(re42, '');
      str85.replace(re43, '');
      str85.replace(re44, '');
      str85.replace(re45, '');
      str85.replace(re46, '');
      str85.replace(re47, '');
      str85.replace(re48, '');
      'pngrtbel'.replace(re29, '');
      'pngrtbel'.replace(re30, '');
      'pybfr'.replace(re39, '');
      'qvi'.replace(re39, '');
      str86.replace(re68, '');
      str86.replace(re18, '');
      'qg'.replace(re39, '');
      'qg'.replace(re68, '');
      'qg'.replace(re18, '');
      'rzorq'.replace(re39, '');
      'rzorq'.replace(re68, '');
      'rzorq'.replace(re18, '');
      'svryqOga'.replace(re39, '');
      'svryqOgaPnapry'.replace(re39, '');
      'svz_zlfcnpr_nccf-pnainf,svz_zlfcnpr_havgrq-fgngrf'.split(re20);
      'svanamra'.replace(re40, '');
      'svanamra'.replace(re41, '');
      'svanamra'.replace(re42, '');
      'svanamra'.replace(re43, '');
      'svanamra'.replace(re44, '');
      'svanamra'.replace(re45, '');
      'svanamra'.replace(re46, '');
      'svanamra'.replace(re47, '');
      'svanamra'.replace(re48, '');
      'sbphf'.split(re70);
      'sbphf.gno sbphfva.gno'.split(re70);
      'sbphfva'.split(re70);
      'sbez'.replace(re39, '');
      'sbez.nwnk'.replace(re68, '');
      'sbez.nwnk'.replace(re18, '');
      'tnzrf'.replace(re40, '');
      'tnzrf'.replace(re41, '');
      'tnzrf'.replace(re42, '');
      'tnzrf'.replace(re43, '');
      'tnzrf'.replace(re44, '');
      'tnzrf'.replace(re45, '');
      'tnzrf'.replace(re46, '');
      'tnzrf'.replace(re47, '');
      'tnzrf'.replace(re48, '');
      'ubzrcntr'.replace(re30, '');
      'ubebfxbc'.replace(re40, '');
      'ubebfxbc'.replace(re41, '');
      'ubebfxbc'.replace(re42, '');
      'ubebfxbc'.replace(re43, '');
      'ubebfxbc'.replace(re44, '');
      'ubebfxbc'.replace(re45, '');
      'ubebfxbc'.replace(re46, '');
      'ubebfxbc'.replace(re47, '');
      'ubebfxbc'.replace(re48, '');
      'uc_cebzbobk_ugzy%2Puc_cebzbobk_vzt'.replace(re30, '');
      'uc_erpgnatyr'.replace(re30, '');
      str87.replace(re33, '');
      str88.replace(re33, '');
      'uggc://wf.hv-cbegny.qr/tzk/ubzr/wf/20080602/onfr.wf${4}${5}'.replace(re71, '');
      'uggc://wf.hv-cbegny.qr/tzk/ubzr/wf/20080602/onfr.wf${5}'.replace(re72, '');
      'uggc://wf.hv-cbegny.qr/tzk/ubzr/wf/20080602/qlaYvo.wf${4}${5}'.replace(re71, '');
      'uggc://wf.hv-cbegny.qr/tzk/ubzr/wf/20080602/qlaYvo.wf${5}'.replace(re72, '');
      'uggc://wf.hv-cbegny.qr/tzk/ubzr/wf/20080602/rssrpgYvo.wf${4}${5}'.replace(re71, '');
      'uggc://wf.hv-cbegny.qr/tzk/ubzr/wf/20080602/rssrpgYvo.wf${5}'.replace(re72, '');
      str89.replace(re73, '');
      'uggc://zfacbegny.112.2b7.arg/o/ff/zfacbegnyubzr/1/U.7-cqi-2/f55023338617756?[NDO]&{1}&{2}&[NDR]'.replace(re69, '');
      str6.replace(re23, '');
      'xvab'.replace(re40, '');
      'xvab'.replace(re41, '');
      'xvab'.replace(re42, '');
      'xvab'.replace(re43, '');
      'xvab'.replace(re44, '');
      'xvab'.replace(re45, '');
      'xvab'.replace(re46, '');
      'xvab'.replace(re47, '');
      'xvab'.replace(re48, '');
      'ybnq'.split(re70);
      'zrqvnzbqgno lhv-anifrg lhv-anifrg-gbc'.replace(re18, '');
      'zrgn'.replace(re39, '');
      str90.replace(re68, '');
      str90.replace(re18, '');
      'zbhfrzbir'.split(re70);
      'zbhfrzbir.gno'.split(re70);
      str63.replace(/^.*jroxvg\/(\d+(\.\d+)?).*$/, '');
      'zhfvx'.replace(re40, '');
      'zhfvx'.replace(re41, '');
      'zhfvx'.replace(re42, '');
      'zhfvx'.replace(re43, '');
      'zhfvx'.replace(re44, '');
      'zhfvx'.replace(re45, '');
      'zhfvx'.replace(re46, '');
      'zhfvx'.replace(re47, '');
      'zhfvx'.replace(re48, '');
      'zlfcnpr_nccf_pnainf'.replace(re52, '');
      str91.replace(re40, '');
      str91.replace(re41, '');
      str91.replace(re42, '');
      str91.replace(re43, '');
      str91.replace(re44, '');
      str91.replace(re45, '');
      str91.replace(re46, '');
      str91.replace(re47, '');
      str91.replace(re48, '');
      'anzr'.replace(re39, '');
      str92.replace(/\b\w+\b/g, '');
      'bow-nppbeqvba'.replace(re39, '');
      'bowrpg'.replace(re39, '');
      'bowrpg'.replace(re68, '');
      'bowrpg'.replace(re18, '');
      'cnenzf%2Rfglyrf'.replace(re29, '');
      'cnenzf%2Rfglyrf'.replace(re30, '');
      'cbchc'.replace(re30, '');
      'ebhgr'.replace(re40, '');
      'ebhgr'.replace(re41, '');
      'ebhgr'.replace(re42, '');
      'ebhgr'.replace(re43, '');
      'ebhgr'.replace(re44, '');
      'ebhgr'.replace(re45, '');
      'ebhgr'.replace(re46, '');
      'ebhgr'.replace(re47, '');
      'ebhgr'.replace(re48, '');
      'freivprobk_uc'.replace(re30, '');
      'fubccvatobk_uc'.replace(re30, '');
      'fubhgobk'.replace(re39, '');
      'fcbeg'.replace(re40, '');
      'fcbeg'.replace(re41, '');
      'fcbeg'.replace(re42, '');
      'fcbeg'.replace(re43, '');
      'fcbeg'.replace(re44, '');
      'fcbeg'.replace(re45, '');
      'fcbeg'.replace(re46, '');
      'fcbeg'.replace(re47, '');
      'fcbeg'.replace(re48, '');
      'gbhe'.replace(re40, '');
      'gbhe'.replace(re41, '');
      'gbhe'.replace(re42, '');
      'gbhe'.replace(re43, '');
      'gbhe'.replace(re44, '');
      'gbhe'.replace(re45, '');
      'gbhe'.replace(re46, '');
      'gbhe'.replace(re47, '');
      'gbhe'.replace(re48, '');
      'gi-fcbg'.replace(re40, '');
      'gi-fcbg'.replace(re41, '');
      'gi-fcbg'.replace(re42, '');
      'gi-fcbg'.replace(re43, '');
      'gi-fcbg'.replace(re44, '');
      'gi-fcbg'.replace(re45, '');
      'gi-fcbg'.replace(re46, '');
      'gi-fcbg'.replace(re47, '');
      'gi-fcbg'.replace(re48, '');
      'glcr'.replace(re39, '');
      'haqrsvarq'.replace(/\//g, '');
      str93.replace(re40, '');
      str93.replace(re41, '');
      str93.replace(re42, '');
      str93.replace(re43, '');
      str93.replace(re44, '');
      str93.replace(re45, '');
      str93.replace(re46, '');
      str93.replace(re47, '');
      str93.replace(re48, '');
      'ivqrb'.replace(re40, '');
      'ivqrb'.replace(re41, '');
      'ivqrb'.replace(re42, '');
      'ivqrb'.replace(re43, '');
      'ivqrb'.replace(re44, '');
      'ivqrb'.replace(re45, '');
      'ivqrb'.replace(re46, '');
      'ivqrb'.replace(re47, '');
      'ivqrb'.replace(re48, '');
      'ivfvgf=1'.split(re86);
      'jrggre'.replace(re40, '');
      'jrggre'.replace(re41, '');
      'jrggre'.replace(re42, '');
      'jrggre'.replace(re43, '');
      'jrggre'.replace(re44, '');
      'jrggre'.replace(re45, '');
      'jrggre'.replace(re46, '');
      'jrggre'.replace(re47, '');
      'jrggre'.replace(re48, '');
      /#[a-z0-9]+$/i.exec('uggc://jjj.fpuhryreim.arg/Qrsnhyg');
      re66.exec('fryrpgrq');
      /(?:^|\s+)lhv-ani(?:\s+|$)/.exec('sff lhv-ani');
      /(?:^|\s+)lhv-anifrg(?:\s+|$)/.exec('zrqvnzbqgno lhv-anifrg');
      /(?:^|\s+)lhv-anifrg-gbc(?:\s+|$)/.exec('zrqvnzbqgno lhv-anifrg');
      re91.exec('GnoThvq');
      re91.exec('thvq');
      /(pbzcngvoyr|jroxvg)/.exec(str63);
      /.+(?:ei|vg|en|vr)[\/: ]([\d.]+)/.exec(str63);
      re8.exec('144631658.0.10.1231365869');
      re8.exec('144631658.0.10.1231367054');
      re8.exec('144631658.1231365869.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
      re8.exec('144631658.1231367054.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
      re8.exec('144631658.1670816052019209000.1231365869.1231365869.1231365869.1');
      re8.exec('144631658.1796080716621419500.1231367054.1231367054.1231367054.1');
      re8.exec(str94);
      re8.exec(str95);
      re8.exec(str96);
      re8.exec(str97);
      re8.exec('__hgzn=144631658.1670816052019209000.1231365869.1231365869.1231365869.1');
      re8.exec('__hgzn=144631658.1796080716621419500.1231367054.1231367054.1231367054.1');
      re8.exec('__hgzo=144631658.0.10.1231365869');
      re8.exec('__hgzo=144631658.0.10.1231367054');
      re8.exec('__hgzm=144631658.1231365869.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
      re8.exec('__hgzm=144631658.1231367054.1.1.hgzpfe=(qverpg)|hgzppa=(qverpg)|hgzpzq=(abar)');
      re34.exec(str78);
      re34.exec(str79);
      re34.exec(str81);
      re74.exec(str77);
      re74.exec('*');
      re74.exec(str82);
      re74.exec(str83);
      re74.exec(str86);
      re74.exec('rzorq');
      re74.exec('sbez.nwnk');
      re74.exec(str90);
      re74.exec('bowrpg');
      /\/onfr.wf(\?.+)?$/.exec('/uggc://wf.hv-cbegny.qr/tzk/ubzr/wf/20080602/onfr.wf');
      re28.exec('uvag ynfgUvag ynfg');
      re75.exec('');
      re76.exec('');
      re77.exec('');
      re78.exec('');
      re80.exec(str77);
      re80.exec('*');
      re80.exec('.pybfr');
      re80.exec(str82);
      re80.exec(str83);
      re80.exec(str84);
      re80.exec(str86);
      re80.exec('qg');
      re80.exec('rzorq');
      re80.exec('sbez.nwnk');
      re80.exec(str90);
      re80.exec('bowrpg');
      re61.exec('qlaYvo.wf');
      re61.exec('rssrpgYvo.wf');
      re61.exec('uggc://jjj.tzk.arg/qr/?fgnghf=uvajrvf');
      re92.exec(' .pybfr');
      re92.exec(' n.svryqOgaPnapry');
      re92.exec(' qg');
      re92.exec(str48);
      re92.exec('.nwnk');
      re92.exec('.svryqOga,n.svryqOgaPnapry');
      re92.exec('.svryqOgaPnapry');
      re92.exec('.bow-nppbeqvba qg');
      re68.exec(str77);
      re68.exec('*');
      re68.exec('.pybfr');
      re68.exec(str82);
      re68.exec(str83);
      re68.exec(str84);
      re68.exec(str86);
      re68.exec('qg');
      re68.exec('rzorq');
      re68.exec('sbez.nwnk');
      re68.exec(str90);
      re68.exec('bowrpg');
      re93.exec(' .pybfr');
      re93.exec(' n.svryqOgaPnapry');
      re93.exec(' qg');
      re93.exec(str48);
      re93.exec('.nwnk');
      re93.exec('.svryqOga,n.svryqOgaPnapry');
      re93.exec('.svryqOgaPnapry');
      re93.exec('.bow-nppbeqvba qg');
      re81.exec(str77);
      re81.exec('*');
      re81.exec(str48);
      re81.exec('.pybfr');
      re81.exec(str82);
      re81.exec(str83);
      re81.exec(str84);
      re81.exec(str86);
      re81.exec('qg');
      re81.exec('rzorq');
      re81.exec('sbez.nwnk');
      re81.exec(str90);
      re81.exec('bowrpg');
      re94.exec(' .pybfr');
      re94.exec(' n.svryqOgaPnapry');
      re94.exec(' qg');
      re94.exec(str48);
      re94.exec('.nwnk');
      re94.exec('.svryqOga,n.svryqOgaPnapry');
      re94.exec('.svryqOgaPnapry');
      re94.exec('.bow-nppbeqvba qg');
      re94.exec('[anzr=nwnkHey]');
      re94.exec(str82);
      re31.exec('rf');
      re31.exec('wn');
      re82.exec(str77);
      re82.exec('*');
      re82.exec(str48);
      re82.exec('.pybfr');
      re82.exec(str82);
      re82.exec(str83);
      re82.exec(str84);
      re82.exec(str86);
      re82.exec('qg');
      re82.exec('rzorq');
      re82.exec('sbez.nwnk');
      re82.exec(str90);
      re82.exec('bowrpg');
      re83.exec(str98);
      re83.exec('shapgvba sbphf() { [angvir pbqr] }');
      re62.exec('#Ybtva');
      re62.exec('#Ybtva_cnffjbeq');
      re62.exec(str77);
      re62.exec('#fubhgobkWf');
      re62.exec('#fubhgobkWfReebe');
      re62.exec('#fubhgobkWfFhpprff');
      re62.exec('*');
      re62.exec(str82);
      re62.exec(str83);
      re62.exec(str86);
      re62.exec('rzorq');
      re62.exec('sbez.nwnk');
      re62.exec(str90);
      re62.exec('bowrpg');
      re49.exec('pbagrag');
      re24.exec(str6);
      /xbadhrebe/.exec(str63);
      /znp/.exec('jva32');
      /zbmvyyn/.exec(str63);
      /zfvr/.exec(str63);
      /ag\s5\.1/.exec(str63);
      /bcren/.exec(str63);
      /fnsnev/.exec(str63);
      /jva/.exec('jva32');
      /jvaqbjf/.exec(str63);
    }
  }

  function run() {
    for (var i = 0; i < 5; i++) {
      runBlock0();
      runBlock1();
      runBlock2();
      runBlock3();
      runBlock4();
      runBlock5();
      runBlock6();
      runBlock7();
      runBlock8();
      runBlock9();
      runBlock10();
      runBlock11();
    }
  }

  this.run = run;
}
// Copyright 2009 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This benchmark is based on a JavaScript log processing module used
// by the V8 profiler to generate execution time profiles for runs of
// JavaScript applications, and it effectively measures how fast the
// JavaScript engine is at allocating nodes and reclaiming the memory
// used for old nodes. Because of the way splay trees work, the engine
// also has to deal with a lot of changes to the large tree object
// graph.

// Configuration.
var kSplayTreeSize = 8000;
var kSplayTreeModifications = 80;
var kSplayTreePayloadDepth = 5;

var splayTree = null;


function GeneratePayloadTree(depth, tag) {
  if (depth == 0) {
    return {
      array  : [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 ],
      string : 'String for key ' + tag + ' in leaf node'
    };
  } else {
    return {
      left:  GeneratePayloadTree(depth - 1, tag),
      right: GeneratePayloadTree(depth - 1, tag)
    };
  }
}


function GenerateKey() {
  // The benchmark framework guarantees that Math.random is
  // deterministic; see base.js.
  return Math.random();
}


function InsertNewNode() {
  // Insert new node with a unique key.
  var key;
  do {
    key = GenerateKey();
  } while (splayTree.find(key) != null);
  var payload = GeneratePayloadTree(kSplayTreePayloadDepth, String(key));
  splayTree.insert(key, payload);
  return key;
}



function SplaySetup() {
  splayTree = new SplayTree();
  for (var i = 0; i < kSplayTreeSize; i++) InsertNewNode();
}


function SplayTearDown() {
  // Allow the garbage collector to reclaim the memory
  // used by the splay tree no matter how we exit the
  // tear down function.
  var keys = splayTree.exportKeys();
  splayTree = null;

  // Verify that the splay tree has the right size.
  var length = keys.length;
  if (length != kSplayTreeSize) {
    throw new Error("Splay tree has wrong size");
  }

  // Verify that the splay tree has sorted, unique keys.
  for (var i = 0; i < length - 1; i++) {
    if (keys[i] >= keys[i + 1]) {
      throw new Error("Splay tree not sorted");
    }
  }
}


function SplayRun() {
  // Replace a few nodes in the splay tree.
  for (var i = 0; i < kSplayTreeModifications; i++) {
    var key = InsertNewNode();
    var greatest = splayTree.findGreatestLessThan(key);
    if (greatest == null) splayTree.remove(key);
    else splayTree.remove(greatest.key);
  }
}


/**
 * Constructs a Splay tree.  A splay tree is a self-balancing binary
 * search tree with the additional property that recently accessed
 * elements are quick to access again. It performs basic operations
 * such as insertion, look-up and removal in O(log(n)) amortized time.
 *
 * @constructor
 */
function SplayTree() {
};


/**
 * Pointer to the root node of the tree.
 *
 * @type {SplayTree.Node}
 * @private
 */
SplayTree.prototype.root_ = null;


/**
 * @return {boolean} Whether the tree is empty.
 */
SplayTree.prototype.isEmpty = function() {
  return !this.root_;
};


/**
 * Inserts a node into the tree with the specified key and value if
 * the tree does not already contain a node with the specified key. If
 * the value is inserted, it becomes the root of the tree.
 *
 * @param {number} key Key to insert into the tree.
 * @param {*} value Value to insert into the tree.
 */
SplayTree.prototype.insert = function(key, value) {
  if (this.isEmpty()) {
    this.root_ = new SplayTree.Node(key, value);
    return;
  }
  // Splay on the key to move the last node on the search path for
  // the key to the root of the tree.
  this.splay_(key);
  if (this.root_.key == key) {
    return;
  }
  var node = new SplayTree.Node(key, value);
  if (key > this.root_.key) {
    node.left = this.root_;
    node.right = this.root_.right;
    this.root_.right = null;
  } else {
    node.right = this.root_;
    node.left = this.root_.left;
    this.root_.left = null;
  }
  this.root_ = node;
};


/**
 * Removes a node with the specified key from the tree if the tree
 * contains a node with this key. The removed node is returned. If the
 * key is not found, an exception is thrown.
 *
 * @param {number} key Key to find and remove from the tree.
 * @return {SplayTree.Node} The removed node.
 */
SplayTree.prototype.remove = function(key) {
  if (this.isEmpty()) {
    throw Error('Key not found: ' + key);
  }
  this.splay_(key);
  if (this.root_.key != key) {
    throw Error('Key not found: ' + key);
  }
  var removed = this.root_;
  if (!this.root_.left) {
    this.root_ = this.root_.right;
  } else {
    var right = this.root_.right;
    this.root_ = this.root_.left;
    // Splay to make sure that the new root has an empty right child.
    this.splay_(key);
    // Insert the original right child as the right child of the new
    // root.
    this.root_.right = right;
  }
  return removed;
};


/**
 * Returns the node having the specified key or null if the tree doesn't contain
 * a node with the specified key.
 *
 * @param {number} key Key to find in the tree.
 * @return {SplayTree.Node} Node having the specified key.
 */
SplayTree.prototype.find = function(key) {
  if (this.isEmpty()) {
    return null;
  }
  this.splay_(key);
  return this.root_.key == key ? this.root_ : null;
};


/**
 * @return {SplayTree.Node} Node having the maximum key value.
 */
SplayTree.prototype.findMax = function(opt_startNode) {
  if (this.isEmpty()) {
    return null;
  }
  var current = opt_startNode || this.root_;
  while (current.right) {
    current = current.right;
  }
  return current;
};


/**
 * @return {SplayTree.Node} Node having the maximum key value that
 *     is less than the specified key value.
 */
SplayTree.prototype.findGreatestLessThan = function(key) {
  if (this.isEmpty()) {
    return null;
  }
  // Splay on the key to move the node with the given key or the last
  // node on the search path to the top of the tree.
  this.splay_(key);
  // Now the result is either the root node or the greatest node in
  // the left subtree.
  if (this.root_.key < key) {
    return this.root_;
  } else if (this.root_.left) {
    return this.findMax(this.root_.left);
  } else {
    return null;
  }
};


/**
 * @return {Array<*>} An array containing all the keys of tree's nodes.
 */
SplayTree.prototype.exportKeys = function() {
  var result = [];
  if (!this.isEmpty()) {
    this.root_.traverse_(function(node) { result.push(node.key); });
  }
  return result;
};


/**
 * Perform the splay operation for the given key. Moves the node with
 * the given key to the top of the tree.  If no node has the given
 * key, the last node on the search path is moved to the top of the
 * tree. This is the simplified top-down splaying algorithm from:
 * "Self-adjusting Binary Search Trees" by Sleator and Tarjan
 *
 * @param {number} key Key to splay the tree on.
 * @private
 */
SplayTree.prototype.splay_ = function(key) {
  if (this.isEmpty()) {
    return;
  }
  // Create a dummy node.  The use of the dummy node is a bit
  // counter-intuitive: The right child of the dummy node will hold
  // the L tree of the algorithm.  The left child of the dummy node
  // will hold the R tree of the algorithm.  Using a dummy node, left
  // and right will always be nodes and we avoid special cases.
  var dummy, left, right;
  dummy = left = right = new SplayTree.Node(null, null);
  var current = this.root_;
  while (true) {
    if (key < current.key) {
      if (!current.left) {
        break;
      }
      if (key < current.left.key) {
        // Rotate right.
        var tmp = current.left;
        current.left = tmp.right;
        tmp.right = current;
        current = tmp;
        if (!current.left) {
          break;
        }
      }
      // Link right.
      right.left = current;
      right = current;
      current = current.left;
    } else if (key > current.key) {
      if (!current.right) {
        break;
      }
      if (key > current.right.key) {
        // Rotate left.
        var tmp = current.right;
        current.right = tmp.left;
        tmp.left = current;
        current = tmp;
        if (!current.right) {
          break;
        }
      }
      // Link left.
      left.right = current;
      left = current;
      current = current.right;
    } else {
      break;
    }
  }
  // Assemble.
  left.right = current.left;
  right.left = current.right;
  current.left = dummy.right;
  current.right = dummy.left;
  this.root_ = current;
};


/**
 * Constructs a Splay tree node.
 *
 * @param {number} key Key.
 * @param {*} value Value.
 */
SplayTree.Node = function(key, value) {
  this.key = key;
  this.value = value;
};


/**
 * @type {SplayTree.Node}
 */
SplayTree.Node.prototype.left = null;


/**
 * @type {SplayTree.Node}
 */
SplayTree.Node.prototype.right = null;


/**
 * Performs an ordered traversal of the subtree starting at
 * this SplayTree.Node.
 *
 * @param {function(SplayTree.Node)} f Visitor function.
 * @private
 */
SplayTree.Node.prototype.traverse_ = function(f) {
  var current = this;
  while (current) {
    var left = current.left;
    if (left) left.traverse_(f);
    f(current);
    current = current.right;
  }
};

var Splay = new BenchmarkSuite('Splay', 81491, [
  new Benchmark("Splay", SplayRun, SplaySetup, SplayTearDown)
]);
/**
 * Copyright 2012 the V8 project authors. All rights reserved.
 * Copyright 2009 Oliver Hunt <http://nerget.com>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

var solver = null;

function runNavierStokes()
{
    solver.update();
}

function setupNavierStokes()
{
    solver = new FluidField(null);
    solver.setResolution(128, 128);
    solver.setIterations(20);
    solver.setDisplayFunction(function(){});
    solver.setUICallback(prepareFrame);
    solver.reset();
}

function tearDownNavierStokes()
{
    solver = null;
}

function addPoints(field) {
    var n = 64;
    for (var i = 1; i <= n; i++) {
        field.setVelocity(i, i, n, n);
        field.setDensity(i, i, 5);
        field.setVelocity(i, n - i, -n, -n);
        field.setDensity(i, n - i, 20);
        field.setVelocity(128 - i, n + i, -n, -n);
        field.setDensity(128 - i, n + i, 30);
    }
}

var framesTillAddingPoints = 0;
var framesBetweenAddingPoints = 5;

function prepareFrame(field)
{
    if (framesTillAddingPoints == 0) {
        addPoints(field);
        framesTillAddingPoints = framesBetweenAddingPoints;
        framesBetweenAddingPoints++;
    } else {
        framesTillAddingPoints--;
    }
}

// Code from Oliver Hunt (http://nerget.com/fluidSim/pressure.js) starts here.
function FluidField(canvas) {
    function addFields(x, s, dt)
    {
        for (var i=0; i<size ; i++ ) x[i] += dt*s[i];
    }

    function set_bnd(b, x)
    {
        if (b===1) {
            for (var i = 1; i <= width; i++) {
                x[i] =  x[i + rowSize];
                x[i + (height+1) *rowSize] = x[i + height * rowSize];
            }

            for (var j = 1; i <= height; i++) {
                x[j * rowSize] = -x[1 + j * rowSize];
                x[(width + 1) + j * rowSize] = -x[width + j * rowSize];
            }
        } else if (b === 2) {
            for (var i = 1; i <= width; i++) {
                x[i] = -x[i + rowSize];
                x[i + (height + 1) * rowSize] = -x[i + height * rowSize];
            }

            for (var j = 1; j <= height; j++) {
                x[j * rowSize] =  x[1 + j * rowSize];
                x[(width + 1) + j * rowSize] =  x[width + j * rowSize];
            }
        } else {
            for (var i = 1; i <= width; i++) {
                x[i] =  x[i + rowSize];
                x[i + (height + 1) * rowSize] = x[i + height * rowSize];
            }

            for (var j = 1; j <= height; j++) {
                x[j * rowSize] =  x[1 + j * rowSize];
                x[(width + 1) + j * rowSize] =  x[width + j * rowSize];
            }
        }
        var maxEdge = (height + 1) * rowSize;
        x[0]                 = 0.5 * (x[1] + x[rowSize]);
        x[maxEdge]           = 0.5 * (x[1 + maxEdge] + x[height * rowSize]);
        x[(width+1)]         = 0.5 * (x[width] + x[(width + 1) + rowSize]);
        x[(width+1)+maxEdge] = 0.5 * (x[width + maxEdge] + x[(width + 1) + height * rowSize]);
    }

    function lin_solve(b, x, x0, a, c)
    {
        if (a === 0 && c === 1) {
            for (var j=1 ; j<=height; j++) {
                var currentRow = j * rowSize;
                ++currentRow;
                for (var i = 0; i < width; i++) {
                    x[currentRow] = x0[currentRow];
                    ++currentRow;
                }
            }
            set_bnd(b, x);
        } else {
            var invC = 1 / c;
            for (var k=0 ; k<iterations; k++) {
                for (var j=1 ; j<=height; j++) {
                    var lastRow = (j - 1) * rowSize;
                    var currentRow = j * rowSize;
                    var nextRow = (j + 1) * rowSize;
                    var lastX = x[currentRow];
                    ++currentRow;
                    for (var i=1; i<=width; i++)
                        lastX = x[currentRow] = (x0[currentRow] + a*(lastX+x[++currentRow]+x[++lastRow]+x[++nextRow])) * invC;
                }
                set_bnd(b, x);
            }
        }
    }

    function diffuse(b, x, x0, dt)
    {
        var a = 0;
        lin_solve(b, x, x0, a, 1 + 4*a);
    }

    function lin_solve2(x, x0, y, y0, a, c)
    {
        if (a === 0 && c === 1) {
            for (var j=1 ; j <= height; j++) {
                var currentRow = j * rowSize;
                ++currentRow;
                for (var i = 0; i < width; i++) {
                    x[currentRow] = x0[currentRow];
                    y[currentRow] = y0[currentRow];
                    ++currentRow;
                }
            }
            set_bnd(1, x);
            set_bnd(2, y);
        } else {
            var invC = 1/c;
            for (var k=0 ; k<iterations; k++) {
                for (var j=1 ; j <= height; j++) {
                    var lastRow = (j - 1) * rowSize;
                    var currentRow = j * rowSize;
                    var nextRow = (j + 1) * rowSize;
                    var lastX = x[currentRow];
                    var lastY = y[currentRow];
                    ++currentRow;
                    for (var i = 1; i <= width; i++) {
                        lastX = x[currentRow] = (x0[currentRow] + a * (lastX + x[currentRow] + x[lastRow] + x[nextRow])) * invC;
                        lastY = y[currentRow] = (y0[currentRow] + a * (lastY + y[++currentRow] + y[++lastRow] + y[++nextRow])) * invC;
                    }
                }
                set_bnd(1, x);
                set_bnd(2, y);
            }
        }
    }

    function diffuse2(x, x0, y, y0, dt)
    {
        var a = 0;
        lin_solve2(x, x0, y, y0, a, 1 + 4 * a);
    }

    function advect(b, d, d0, u, v, dt)
    {
        var Wdt0 = dt * width;
        var Hdt0 = dt * height;
        var Wp5 = width + 0.5;
        var Hp5 = height + 0.5;
        for (var j = 1; j<= height; j++) {
            var pos = j * rowSize;
            for (var i = 1; i <= width; i++) {
                var x = i - Wdt0 * u[++pos];
                var y = j - Hdt0 * v[pos];
                if (x < 0.5)
                    x = 0.5;
                else if (x > Wp5)
                    x = Wp5;
                var i0 = x | 0;
                var i1 = i0 + 1;
                if (y < 0.5)
                    y = 0.5;
                else if (y > Hp5)
                    y = Hp5;
                var j0 = y | 0;
                var j1 = j0 + 1;
                var s1 = x - i0;
                var s0 = 1 - s1;
                var t1 = y - j0;
                var t0 = 1 - t1;
                var row1 = j0 * rowSize;
                var row2 = j1 * rowSize;
                d[pos] = s0 * (t0 * d0[i0 + row1] + t1 * d0[i0 + row2]) + s1 * (t0 * d0[i1 + row1] + t1 * d0[i1 + row2]);
            }
        }
        set_bnd(b, d);
    }

    function project(u, v, p, div)
    {
        var h = -0.5 / Math.sqrt(width * height);
        for (var j = 1 ; j <= height; j++ ) {
            var row = j * rowSize;
            var previousRow = (j - 1) * rowSize;
            var prevValue = row - 1;
            var currentRow = row;
            var nextValue = row + 1;
            var nextRow = (j + 1) * rowSize;
            for (var i = 1; i <= width; i++ ) {
                div[++currentRow] = h * (u[++nextValue] - u[++prevValue] + v[++nextRow] - v[++previousRow]);
                p[currentRow] = 0;
            }
        }
        set_bnd(0, div);
        set_bnd(0, p);

        lin_solve(0, p, div, 1, 4 );
        var wScale = 0.5 * width;
        var hScale = 0.5 * height;
        for (var j = 1; j<= height; j++ ) {
            var prevPos = j * rowSize - 1;
            var currentPos = j * rowSize;
            var nextPos = j * rowSize + 1;
            var prevRow = (j - 1) * rowSize;
            var currentRow = j * rowSize;
            var nextRow = (j + 1) * rowSize;

            for (var i = 1; i<= width; i++) {
                u[++currentPos] -= wScale * (p[++nextPos] - p[++prevPos]);
                v[currentPos]   -= hScale * (p[++nextRow] - p[++prevRow]);
            }
        }
        set_bnd(1, u);
        set_bnd(2, v);
    }

    function dens_step(x, x0, u, v, dt)
    {
        addFields(x, x0, dt);
        diffuse(0, x0, x, dt );
        advect(0, x, x0, u, v, dt );
    }

    function vel_step(u, v, u0, v0, dt)
    {
        addFields(u, u0, dt );
        addFields(v, v0, dt );
        var temp = u0; u0 = u; u = temp;
        var temp = v0; v0 = v; v = temp;
        diffuse2(u,u0,v,v0, dt);
        project(u, v, u0, v0);
        var temp = u0; u0 = u; u = temp;
        var temp = v0; v0 = v; v = temp;
        advect(1, u, u0, u0, v0, dt);
        advect(2, v, v0, u0, v0, dt);
        project(u, v, u0, v0 );
    }
    var uiCallback = function(d,u,v) {};

    function Field(dens, u, v) {
        // Just exposing the fields here rather than using accessors is a measurable win during display (maybe 5%)
        // but makes the code ugly.
        this.setDensity = function(x, y, d) {
             dens[(x + 1) + (y + 1) * rowSize] = d;
        }
        this.getDensity = function(x, y) {
             return dens[(x + 1) + (y + 1) * rowSize];
        }
        this.setVelocity = function(x, y, xv, yv) {
             u[(x + 1) + (y + 1) * rowSize] = xv;
             v[(x + 1) + (y + 1) * rowSize] = yv;
        }
        this.getXVelocity = function(x, y) {
             return u[(x + 1) + (y + 1) * rowSize];
        }
        this.getYVelocity = function(x, y) {
             return v[(x + 1) + (y + 1) * rowSize];
        }
        this.width = function() { return width; }
        this.height = function() { return height; }
    }
    function queryUI(d, u, v)
    {
        for (var i = 0; i < size; i++)
            u[i] = v[i] = d[i] = 0.0;
        uiCallback(new Field(d, u, v));
    }

    this.update = function () {
        queryUI(dens_prev, u_prev, v_prev);
        vel_step(u, v, u_prev, v_prev, dt);
        dens_step(dens, dens_prev, u, v, dt);
        displayFunc(new Field(dens, u, v));
    }
    this.setDisplayFunction = function(func) {
        displayFunc = func;
    }

    this.iterations = function() { return iterations; }
    this.setIterations = function(iters) {
        if (iters > 0 && iters <= 100)
           iterations = iters;
    }
    this.setUICallback = function(callback) {
        uiCallback = callback;
    }
    var iterations = 10;
    var visc = 0.5;
    var dt = 0.1;
    var dens;
    var dens_prev;
    var u;
    var u_prev;
    var v;
    var v_prev;
    var width;
    var height;
    var rowSize;
    var size;
    var displayFunc;
    function reset()
    {
        rowSize = width + 2;
        size = (width+2)*(height+2);
        dens = new Array(size);
        dens_prev = new Array(size);
        u = new Array(size);
        u_prev = new Array(size);
        v = new Array(size);
        v_prev = new Array(size);
        for (var i = 0; i < size; i++)
            dens_prev[i] = u_prev[i] = v_prev[i] = dens[i] = u[i] = v[i] = 0;
    }
    this.reset = reset;
    this.setResolution = function (hRes, wRes)
    {
        var res = wRes * hRes;
        if (res > 0 && res < 1000000 && (wRes != width || hRes != height)) {
            width = wRes;
            height = hRes;
            reset();
            return true;
        }
        return false;
    }
    this.setResolution(64, 64);
}

var NavierStokes = new BenchmarkSuite('NavierStokes', 1484000,
                                      [new Benchmark('NavierStokes',
                                                     runNavierStokes,
                                                     setupNavierStokes,
                                                     tearDownNavierStokes)]);

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
