---
layout: default
---

JerryScript is a lightweight JavaScript engine intended to run on a very constrained devices such as microcontrollers:

- Only few kilobytes of RAM available to the engine (<64 KB RAM)
- Constrained ROM space for the code of the engine (<200 KB ROM)
- On-device compilation and execution
- The engine provides access to peripherals from JavaScript

{% highlight js %}
print_hello("Tom")

function print_hello (name) {
  print ("Hi, " + name)
}
{% endhighlight %}

Check out the [JerryScript sources][jerryscript] and start using JerryScript in you projects. Please, report all found bugs and request new features at [JerryScript issue tracker][jerryscript-issue]. If you have questions, feel free to ask them on [issue tracker][jerryscript-issue-questions] and don't forget to label it with `question` or `discussion`.

[jerryscript]:                  http://github.com/Samsung/jerryscript
[jerryscript-issue]:            http://github.com/Samsung/jerryscript/issues
[jerryscript-pull]:             http://github.com/Samsung/jerryscript/pulls
[jerryscript-issue-questions]:  http://github.com/Samsung/jerryscript/labels/question
[jerryscript-wiki]:             http://github.com/Samsung/jerryscript/wiki
