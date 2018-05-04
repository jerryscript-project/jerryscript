#!/usr/bin/env python

# Copyright JS Foundation and other contributors, http://js.foundation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import print_function
from cmd import Cmd
from pprint import pprint
import math
import socket
import sys
import logging
import time
import jerry_client_ws

class DebuggerPrompt(Cmd):
    # pylint: disable=too-many-instance-attributes,too-many-arguments
    def __init__(self, debugger):
        Cmd.__init__(self)
        self.debugger = debugger
        self.stop = False
        self.quit = False
        self.backtrace = True
        self.debugger.non_interactive = False

    def precmd(self, line):
        self.stop = False
        if self.debugger.non_interactive:
            print("%s" % line)
        return line

    def postcmd(self, stop, line):
        return self.stop

    def do_quit(self, _):
        """ Exit JerryScript debugger """
        self.debugger.quit()
        self.quit = True
        self.stop = True

    def do_display(self, args):
        """ Toggle source code display after breakpoints """
        if args:
            line_num = src_check_args(args)
            if line_num >= 0:
                self.debugger.display = line_num
        else:
            print("Non-negative integer number expected, 0 turns off this function")

    def do_break(self, args):
        """ Insert breakpoints on the given lines or functions """
        result = ""
        result = self.debugger.set_break(args)
        if self.debugger.not_empty(result):
            print(result.get_data())
    do_b = do_break

    def do_list(self, _):
        """ Lists the available breakpoints """
        result = self.debugger.show_breakpoint_list()
        print(result.get_data())

    def do_delete(self, args):
        """ Delete the given breakpoint, use 'delete all|active|pending' to clear all the given breakpoints """
        result = self.debugger.delete(args)
        if self.debugger.not_empty(result):
            print(result.get_data())

    def do_next(self, args):
        """ Next breakpoint in the same context """
        self.stop = True
        if self.debugger.check_empty_data(args):
            args = 0
            self.debugger.next()
        else:
            try:
                args = int(args)
                if args <= 0:
                    raise ValueError(args)
                else:
                    while int(args) != 0:
                        self.debugger.next()
                        time.sleep(0.5)
                        result = self.debugger.mainloop().get_data()
                        if result.endswith('\n'):
                            result = result.rstrip()
                        if jerry_client_ws.JERRY_DEBUGGER_DATA_END in result:
                            result = result.replace(jerry_client_ws.JERRY_DEBUGGER_DATA_END, '')
                        if result:
                            print(result)
                            self.debugger.smessage = ''
                        if self.debugger.display > 0:
                            print(self.debugger.print_source(self.debugger.display,
                                                             self.debugger.src_offset).get_data())
                        args = int(args) - 1
                    self.cmdloop()
            except ValueError as val_errno:
                print("Error: expected a positive integer: %s" % val_errno)
                self.cmdloop()
    do_n = do_next

    def do_step(self, _):
        """ Next breakpoint, step into functions """
        self.debugger.step()
        self.stop = True
    do_s = do_step

    def do_backtrace(self, args):
        """ Get backtrace data from debugger """
        result = self.debugger.backtrace(args)
        if self.debugger.not_empty(result):
            print(result.get_data())
            self.stop = True
            self.cmdloop()
        else:
            self.stop = True
            self.backtrace = True
    do_bt = do_backtrace

    def do_src(self, args):
        """ Get current source code """
        if args:
            line_num = src_check_args(args)
            if line_num >= 0:
                print(self.debugger.print_source(line_num, 0).get_data())
    do_source = do_src

    def do_scroll(self, _):
        """ Scroll the source up or down """
        while True:
            key = sys.stdin.readline()
            if key == 'w\n':
                _scroll_direction(self.debugger, "up")
            elif key == 's\n':
                _scroll_direction(self.debugger, "down")
            elif key == 'q\n':
                break
            else:
                print("Invalid key")

    def do_continue(self, _):
        """ Continue execution """
        self.debugger.get_continue()
        self.stop = True
        if self.debugger.check_empty_data(self.debugger.non_interactive):
            print("Press enter to stop JavaScript execution.")
    do_c = do_continue

    def do_finish(self, _):
        """ Continue running until the current function returns """
        self.debugger.finish()
        self.stop = True
    do_f = do_finish

    def do_dump(self, args):
        """ Dump all of the debugger data """
        if args:
            print("Error: No argument expected")
        else:
            pprint(self.debugger.function_list)

    def do_eval(self, args):
        """ Evaluate JavaScript source code """
        self.debugger.eval(args)
        self.stop = True
    do_e = do_eval

    def do_memstats(self, _):
        """ Memory statistics """
        self.debugger.memstats()
        self.stop = True
    do_ms = do_memstats

    def do_abort(self, args):
        """ Throw an exception """
        self.debugger.abort(args)
        self.stop = True

    def do_restart(self, _):
        """ Restart the engine's debug session """
        self.debugger.restart()
        self.stop = True
    do_res = do_restart

    def do_throw(self, args):
        """ Throw an exception """
        self.debugger.throw(args)
        self.stop = True

    def do_exception(self, args):
        """ Config the exception handler module """
        result = self.debugger.exception(args)
        print(result.get_data())

def _scroll_direction(debugger, direction):
    """ Helper function for do_scroll """
    debugger.src_offset_diff = int(max(math.floor(debugger.display / 3), 1))
    if direction == "up":
        debugger.src_offset -= debugger.src_offset_diff
    else:
        debugger.src_offset += debugger.src_offset_diff
    print(debugger.print_source(debugger.display, debugger.src_offset)['value'])

def src_check_args(args):
    try:
        line_num = int(args)
        if line_num < 0:
            print("Error: Non-negative integer number expected")
            return -1

        return line_num
    except ValueError as val_errno:
        print("Error: Non-negative integer number expected: %s" % (val_errno))
        return -1

# pylint: disable=too-many-branches,too-many-locals,too-many-statements
def main():
    args = jerry_client_ws.arguments_parse()

    debugger = jerry_client_ws.JerryDebugger(args.address)

    logging.debug("Connected to JerryScript on %d port", debugger.port)

    prompt = DebuggerPrompt(debugger)
    prompt.prompt = "(jerry-debugger) "
    prompt.debugger.non_interactive = args.non_interactive

    if args.color:
        debugger.set_colors()

    if args.display:
        prompt.debugger.display = args.display
        prompt.do_display(args.display)
    else:
        prompt.stop = False
        if prompt.debugger.check_empty_data(args.client_source):
            prompt.debugger.mainloop()
            result = prompt.debugger.smessage
            print(result)
            prompt.debugger.smessage = ''
            prompt.cmdloop()

    if  prompt.debugger.not_empty(args.exception):
        prompt.do_exception(str(args.exception))

    if args.client_source:
        prompt.debugger.store_client_sources(args.client_source)

    while True:
        if prompt.quit:
            break

        result = prompt.debugger.mainloop().get_data()

        if prompt.debugger.check_empty_data(result) and prompt.backtrace is False:
            break

        if prompt.debugger.wait_data(result):
            continue

        elif jerry_client_ws.JERRY_DEBUGGER_DATA_END in result:
            result = result.replace(jerry_client_ws.JERRY_DEBUGGER_DATA_END, '')
            if result.endswith('\n'):
                result = result.rstrip()
            if result:
                print(result)
                prompt.debugger.smessage = ''
            if prompt.debugger.display > 0:
                print(prompt.debugger.print_source(prompt.debugger.display, prompt.debugger.src_offset).get_data())
            prompt.backtrace = False
            break
        else:
            if result.endswith('\n'):
                result = result.rstrip()
            if result:
                print(result)
                prompt.debugger.smessage = ''
            if prompt.debugger.display > 0:
                print(prompt.debugger.print_source(prompt.debugger.display, prompt.debugger.src_offset).get_data())
        prompt.cmdloop()
        continue

if __name__ == "__main__":
    try:
        main()
    except socket.error as error_msg:
        ERRNO = error_msg.errno
        MSG = str(error_msg)
        if ERRNO == 111:
            sys.exit("Failed to connect to the JerryScript debugger.")
        elif ERRNO == 32 or ERRNO == 104:
            sys.exit("Connection closed.")
        else:
            sys.exit("Failed to connect to the JerryScript debugger.\nError: %s" % (MSG))
