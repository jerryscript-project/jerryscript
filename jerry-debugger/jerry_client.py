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
import jerry_client_main

from jerry_client_websocket import WebSocket
from jerry_client_rawpacket import RawPacket
from jerry_client_tcp import Socket

def write(string):
    print(string, end='')

class DebuggerPrompt(Cmd):
    # pylint: disable=too-many-instance-attributes,too-many-arguments
    def __init__(self, debugger):
        Cmd.__init__(self)
        self.debugger = debugger
        self.stop = False
        self.quit = False

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
        write(self.debugger.set_break(args))
    do_b = do_break

    def do_list(self, _):
        """ Lists the available breakpoints """
        write(self.debugger.breakpoint_list())

    def do_delete(self, args):
        """ Delete the given breakpoint, use 'delete all|active|pending' to clear all the given breakpoints """
        write(self.debugger.delete(args))

    def do_next(self, args):
        """ Next breakpoint in the same context """
        self.stop = True
        if not args:
            args = 0
            self.debugger.next()
            return

        try:
            args = int(args)
            if args <= 0:
                raise ValueError(args)

            while args > 0:
                self.debugger.next()
                time.sleep(0.1)

                while True:
                    result = self.debugger.process_messages()
                    res_type = result.get_type()

                    if res_type == result.END:
                        self.quit = True
                        return
                    elif res_type == result.TEXT:
                        write(result.get_text())
                    elif res_type == result.PROMPT:
                        break

                args -= 1
        except ValueError as val_errno:
            print("Error: expected a positive integer: %s" % val_errno)
    do_n = do_next

    def do_step(self, _):
        """ Next breakpoint, step into functions """
        self.debugger.step()
        self.stop = True
    do_s = do_step

    def do_backtrace(self, args):
        """ Get backtrace data from debugger """
        write(self.debugger.backtrace(args))
        self.stop = True
    do_bt = do_backtrace

    def do_variables(self, args):
        """ Get scope variables from debugger """
        write(self.debugger.scope_variables(args))
        self.stop = True

    def do_src(self, args):
        """ Get current source code """
        if args:
            line_num = src_check_args(args)
            if line_num >= 0:
                write(self.debugger.print_source(line_num, 0))
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
        if not self.debugger.non_interactive:
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

    def do_eval_at(self, args):
        """ Evaluate JavaScript source code at a scope chain level """

        code = ''
        index = 0
        try:
            args = args.split(" ", 1)

            index = int(args[0])

            if len(args) == 2:
                code = args[1]

            if index < 0 or index > 65535:
                raise ValueError("Invalid scope chain index: %d (must be between 0 and 65535)" % index)

        except ValueError as val_errno:
            print("Error: %s" % (val_errno))
            return

        self.debugger.eval_at(code, index)
        self.stop = True

    def do_memstats(self, _):
        """ Memory statistics """
        self.debugger.memstats()
        self.stop = True
    do_ms = do_memstats

    def do_scopes(self, _):
        """ Memory statistics """
        self.debugger.scope_chain()
        self.stop = True

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
        write(self.debugger.exception(args))

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

# pylint: disable=too-many-branches,too-many-locals,too-many-statements,redefined-variable-type
def main():
    args = jerry_client_main.arguments_parse()

    channel = None
    protocol = None

    if args.protocol == "tcp":
        address = None
        if ":" not in args.address:
            address = (args.address, 5001) # use default port
        else:
            host, port = args.address.split(":")
            address = (host, int(port))

        protocol = Socket(address)
    elif args.protocol == "serial":
        from jerry_client_serial import Serial
        protocol = Serial(args.serial_config)
    else:
        print("Unsupported transmission protocol")
        return -1

    if args.channel == "websocket":
        channel = WebSocket(protocol=protocol)
    elif args.channel == "rawpacket":
        channel = RawPacket(protocol=protocol)
    else:
        print("Unsupported communication channel")
        return -1

    debugger = jerry_client_main.JerryDebugger(channel)
    debugger.non_interactive = args.non_interactive

    logging.debug("Connected to JerryScript")

    prompt = DebuggerPrompt(debugger)
    prompt.prompt = "(jerry-debugger) "

    if args.color:
        debugger.set_colors()

    if args.display:
        debugger.display = args.display
        prompt.do_display(args.display)
    else:
        prompt.stop = False

    if args.exception is not None:
        prompt.do_exception(str(args.exception))

    if args.client_source:
        debugger.store_client_sources(args.client_source)

    while True:
        if prompt.quit:
            break

        result = debugger.process_messages()
        res_type = result.get_type()

        if res_type == result.END:
            break
        elif res_type == result.PROMPT:
            prompt.cmdloop()
        elif res_type == result.TEXT:
            write(result.get_text())
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
