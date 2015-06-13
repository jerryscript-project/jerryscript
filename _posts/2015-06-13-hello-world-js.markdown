---
layout: post
title:  "Hello World.js!"
date:   2015-06-13 13:30:58
categories:
---
The concept of the Internet of Things (IoT) is becoming more and more popular. According to the reports of global corporations, there will be from 25 to 75 billion Internet of Things devices by 2020. Existence of a unified platform for developing applications of the IoT is essential domination in this field. Popularity of the JavaScript language in modern web, both on client and server sides, implies that providing a JavaScript-based platform for the IoT can attract developers to this platform.

{% highlight js %}
print_hello("Tom")

function print_hello (name) {
  print ("Hi, " + name)
}
{% endhighlight %}

JerryScript engine satisfies the following requirements:

- Capable to run on MCU (ARM Cortex M)
- Only few kilobytes of RAM available to the engine (<64 KB RAM)
- Constrained ROM space for the code of the engine (<200 KB ROM)
- On-device compilation and execution
- The engine provides access to peripherals from JavaScript.

Check out the [JerryScript sources][jerryscript] and start using JerryScript in you projects. Please, report all found bugs and request new features at [JerryScript issue tracker][jerryscript-issue]. If you have questions, feel free to ask them on [issue tracker][jerryscript-issue] and don't forget to label it with `question` or `discussion`.

[jerryscript]:          http://github.com/Samsung/jerryscript
[jerryscript-issue]:    http://github.com/Samsung/jerryscript/issues
[jerryscript-pull]:     http://github.com/Samsung/jerryscript/pulls
[jerryscript-wiki]:     http://github.com/Samsung/jerryscript/wiki
