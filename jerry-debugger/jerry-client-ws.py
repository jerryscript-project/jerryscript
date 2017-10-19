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
from pprint import pprint  # For the readable stack printing.
import argparse
import logging
import re
import select
import socket
import struct
import sys

# Messages sent by the server to client.
JERRY_DEBUGGER_CONFIGURATION = 1
JERRY_DEBUGGER_PARSE_ERROR = 2
JERRY_DEBUGGER_BYTE_CODE_CP = 3
JERRY_DEBUGGER_PARSE_FUNCTION = 4
JERRY_DEBUGGER_BREAKPOINT_LIST = 5
JERRY_DEBUGGER_BREAKPOINT_OFFSET_LIST = 6
JERRY_DEBUGGER_SOURCE_CODE = 7
JERRY_DEBUGGER_SOURCE_CODE_END = 8
JERRY_DEBUGGER_SOURCE_CODE_NAME = 9
JERRY_DEBUGGER_SOURCE_CODE_NAME_END = 10
JERRY_DEBUGGER_FUNCTION_NAME = 11
JERRY_DEBUGGER_FUNCTION_NAME_END = 12
JERRY_DEBUGGER_RELEASE_BYTE_CODE_CP = 13
JERRY_DEBUGGER_MEMSTATS_RECEIVE = 14
JERRY_DEBUGGER_BREAKPOINT_HIT = 15
JERRY_DEBUGGER_EXCEPTION_HIT = 16
JERRY_DEBUGGER_EXCEPTION_STR = 17
JERRY_DEBUGGER_EXCEPTION_STR_END = 18
JERRY_DEBUGGER_BACKTRACE = 19
JERRY_DEBUGGER_BACKTRACE_END = 20
JERRY_DEBUGGER_EVAL_RESULT = 21
JERRY_DEBUGGER_EVAL_RESULT_END = 22
JERRY_DEBUGGER_WAIT_FOR_SOURCE = 23
JERRY_DEBUGGER_OUTPUT_RESULT = 24
JERRY_DEBUGGER_OUTPUT_RESULT_END = 25

# Subtypes of eval
JERRY_DEBUGGER_EVAL_OK = 1
JERRY_DEBUGGER_EVAL_ERROR = 2

# Subtypes of output
JERRY_DEBUGGER_OUTPUT_OK = 1
JERRY_DEBUGGER_OUTPUT_ERROR = 2
JERRY_DEBUGGER_OUTPUT_WARNING = 3
JERRY_DEBUGGER_OUTPUT_DEBUG = 4
JERRY_DEBUGGER_OUTPUT_TRACE = 5


# Messages sent by the client to server.
JERRY_DEBUGGER_FREE_BYTE_CODE_CP = 1
JERRY_DEBUGGER_UPDATE_BREAKPOINT = 2
JERRY_DEBUGGER_EXCEPTION_CONFIG = 3
JERRY_DEBUGGER_MEMSTATS = 4
JERRY_DEBUGGER_STOP = 5
JERRY_DEBUGGER_CLIENT_SOURCE = 6
JERRY_DEBUGGER_CLIENT_SOURCE_PART = 7
JERRY_DEBUGGER_NO_MORE_SOURCES = 8
JERRY_DEBUGGER_CONTEXT_RESET = 9
JERRY_DEBUGGER_CONTINUE = 10
JERRY_DEBUGGER_STEP = 11
JERRY_DEBUGGER_NEXT = 12
JERRY_DEBUGGER_GET_BACKTRACE = 13
JERRY_DEBUGGER_EVAL = 14
JERRY_DEBUGGER_EVAL_PART = 15

MAX_BUFFER_SIZE = 128
WEBSOCKET_BINARY_FRAME = 2
WEBSOCKET_FIN_BIT = 0x80


def arguments_parse():
    parser = argparse.ArgumentParser(description="JerryScript debugger client")

    parser.add_argument("address", action="store", nargs="?", default="localhost:5001",
                        help="specify a unique network address for connection (default: %(default)s)")
    parser.add_argument("-v", "--verbose", action="store_true", default=False,
                        help="increase verbosity (default: %(default)s)")
    parser.add_argument("--non-interactive", action="store_true", default=False,
                        help="disable stop when newline is pressed (default: %(default)s)")
    parser.add_argument("--color", action="store_true", default=False,
                        help="enable color highlighting on source commands (default: %(default)s)")
    parser.add_argument("--display", action="store", default=None, type=int,
                        help="set display range")
    parser.add_argument("--exception", action="store", default=None, type=int, choices=[0, 1],
                        help="set exception config, usage 1: [Enable] or 0: [Disable]")
    parser.add_argument("--client-source", action="store", default=[], type=str, nargs="+",
                        help="specify a javascript source file to execute")

    args = parser.parse_args()

    if args.verbose:
        logging.basicConfig(format="%(levelname)s: %(message)s", level=logging.DEBUG)
        logging.debug("Debug logging mode: ON")

    return args


class JerryBreakpoint(object):

    def __init__(self, line, offset, function):
        self.line = line
        self.offset = offset
        self.function = function
        self.active_index = -1

    def __str__(self):
        result = self.function.source_name or "<unknown>"
        result += ":%d" % (self.line)

        if self.function.is_func:
            result += " (in "
            result += self.function.name or "function"
            result += "() at line:%d, col:%d)" % (self.function.line, self.function.column)
        return result

    def __repr__(self):
        return ("Breakpoint(line:%d, offset:%d, active_index:%d)"
                % (self.line, self.offset, self.active_index))

class JerryPendingBreakpoint(object):
    def __init__(self, line=None, source_name=None, function=None):
        self.function = function
        self.line = line
        self.source_name = source_name

        self.index = -1

    def __str__(self):
        result = self.source_name or ""
        if self.line:
            result += ":%d" % (self.line)
        else:
            result += "%s()" % (self.function)
        return result



class JerryFunction(object):

    def __init__(self, is_func, byte_code_cp, source, source_name, line, column, name, lines, offsets):
        self.is_func = is_func
        self.byte_code_cp = byte_code_cp
        self.source = re.split("\r\n|[\r\n]", source)
        self.source_name = source_name
        self.name = name
        self.lines = {}
        self.offsets = {}
        self.line = line
        self.column = column
        self.first_breakpoint_line = lines[0]
        self.first_breakpoint_offset = offsets[0]

        if len(self.source) > 1 and not self.source[-1]:
          self.source.pop()

        for i in range(len(lines)):
            line = lines[i]
            offset = offsets[i]
            breakpoint = JerryBreakpoint(line, offset, self)
            self.lines[line] = breakpoint
            self.offsets[offset] = breakpoint

    def __repr__(self):
        result = ("Function(byte_code_cp:0x%x, source_name:%r, name:%r, line:%d, column:%d { "
                  % (self.byte_code_cp, self.source_name, self.name, self.line, self.column))

        result += ','.join([str(breakpoint) for breakpoint in self.lines.values()])

        return result + " })"


class DebuggerPrompt(Cmd):

    def __init__(self, debugger):
        Cmd.__init__(self)
        self.debugger = debugger
        self.stop = False
        self.quit = False
        self.cont = True
        self.non_interactive = False
        self.client_sources = []

    def precmd(self, line):
        self.stop = False
        self.cont = False
        if self.non_interactive:
            print("%s" % line)
        return line

    def postcmd(self, stop, line):
        return self.stop

    def disable_args(self, args):
        if args:
            print("Error: No argument expected")
            return True
        return False

    def do_quit(self, args):
        """ Exit JerryScript debugger """
        self.do_delete("all")
        self.do_exception("0")  # disable the exception handler
        self.exec_command(args, JERRY_DEBUGGER_CONTINUE)
        self.stop = True
        self.quit = True

    def do_break(self, args):
        """ Insert breakpoints on the given lines or functions """
        if args == "":
            print("Error: Breakpoint index expected")
        elif ':' in args:
            try:
                args_second = int(args.split(':', 1)[1])
                if args_second < 0:
                    print("Error: Positive breakpoint index expected")
                else:
                    set_breakpoint(self.debugger, args, False)
            except ValueError as val_errno:
                print("Error: Positive breakpoint index expected: %s" % val_errno)
        else:
            set_breakpoint(self.debugger, args, False)

    do_b = do_break

    def exec_command(self, args, command_id):
        self.stop = True
        if args != "":
            print("Error: No argument expected")
        else:
            self.debugger.send_command(command_id)

    def do_continue(self, args):
        """ Continue execution """
        self.exec_command(args, JERRY_DEBUGGER_CONTINUE)
        self.stop = True
        self.cont = True
        if not self.non_interactive:
            print("Press enter to stop JavaScript execution.")

    do_c = do_continue

    def do_step(self, args):
        """ Next breakpoint, step into functions """
        self.exec_command(args, JERRY_DEBUGGER_STEP)
        self.cont = True

    do_s = do_step

    def do_next(self, args):
        """ Next breakpoint in the same context """
        self.exec_command(args, JERRY_DEBUGGER_NEXT)
        self.cont = True

    do_n = do_next

    def do_list(self, args):
        """ Lists the available breakpoints """
        if self.debugger.active_breakpoint_list:
            print("=== %sActive breakpoints %s ===" % (self.debugger.green_bg, self.debugger.nocolor))
            for breakpoint in self.debugger.active_breakpoint_list.values():
                print(" %d: %s" % (breakpoint.active_index, breakpoint))

        if self.debugger.pending_breakpoint_list:
            print("=== %sPending breakpoints%s ===" % (self.debugger.yellow_bg, self.debugger.nocolor))
            for breakpoint in self.debugger.pending_breakpoint_list.values():
                print(" %d: %s (pending)" % (breakpoint.index, breakpoint))

        if not self.debugger.active_breakpoint_list and not self.debugger.pending_breakpoint_list:
            print("No breakpoints")

    def do_delete(self, args):
        """ Delete the given breakpoint, use 'delete all|active|pending' to clear all the given breakpoints """
        if not args:
            print("Error: Breakpoint index expected")
            return
        elif args == "all":
            self.debugger.delete_active()
            self.debugger.delete_pending()
        elif args == "pending":
            self.debugger.delete_pending()
        elif args == "active":
            self.debugger.delete_active()
        else:
            try:
                breakpoint_index = int(args)
            except ValueError as val_errno:
                print("Error: Integer number expected, %s" % (val_errno))
                return

            if breakpoint_index in self.debugger.active_breakpoint_list:
                breakpoint = self.debugger.active_breakpoint_list[breakpoint_index]
                del self.debugger.active_breakpoint_list[breakpoint_index]
                breakpoint.active_index = -1
                self.debugger.send_breakpoint(breakpoint)
            elif breakpoint_index in self.debugger.pending_breakpoint_list:
                del self.debugger.pending_breakpoint_list[breakpoint_index]
            else:
                print("Error: Breakpoint %d not found" % (breakpoint_index))

    def exec_backtrace(self, args):
        max_depth = 0

        if args:
            try:
                max_depth = int(args)
                if max_depth <= 0:
                    print("Error: Positive integer number expected")
                    return
            except ValueError as val_errno:
                print("Error: Positive integer number expected, %s" % (val_errno))
                return

        message = struct.pack(self.debugger.byte_order + "BBIB" + self.debugger.idx_format,
                              WEBSOCKET_BINARY_FRAME | WEBSOCKET_FIN_BIT,
                              WEBSOCKET_FIN_BIT + 1 + 4,
                              0,
                              JERRY_DEBUGGER_GET_BACKTRACE,
                              max_depth)
        self.debugger.send_message(message)
        self.stop = True

    def do_backtrace(self, args):
        """ Get backtrace data from debugger """
        self.exec_backtrace(args)

    do_bt = do_backtrace

    def do_src(self, args):
        """ Get current source code """
        if args:
            line_num = src_check_args(args)
            if line_num >= 0:
                print_source(self.debugger, line_num)
            elif line_num == 0:
                print_source(self.debugger, self.debugger.default_viewrange)
            else:
                return

    do_source = do_src

    def do_display(self, args):
        """ Toggle source code display after breakpoints """
        if args:
            line_num = src_check_args(args)
            if line_num >= 0:
                self.debugger.display = line_num
            else:
                return

        else:
            print("Non-negative integer number expected, 0 turns off this function")
            return

    def do_dump(self, args):
        """ Dump all of the debugger data """
        if self.disable_args(args):
            return

        pprint(self.debugger.function_list)

    def send_string(self, args, message_type):
        size = len(args)
        if size == 0:
            return

        # 1: length of type byte
        # 4: length of an uint32 value
        message_header = 1 + 4
        max_fragment = min(self.debugger.max_message_size - message_header, size)

        message = struct.pack(self.debugger.byte_order + "BBIBI",
                              WEBSOCKET_BINARY_FRAME | WEBSOCKET_FIN_BIT,
                              WEBSOCKET_FIN_BIT + max_fragment + message_header,
                              0,
                              message_type,
                              size)

        if size == max_fragment:
            self.debugger.send_message(message + args)
            self.stop = True
            return

        self.debugger.send_message(message + args[0:max_fragment])
        offset = max_fragment

        if message_type == JERRY_DEBUGGER_EVAL:
            message_type = JERRY_DEBUGGER_EVAL_PART
        else:
            message_type = JERRY_DEBUGGER_CLIENT_SOURCE_PART

        # 1: length of type byte
        message_header = 1

        max_fragment = self.debugger.max_message_size - message_header
        while offset < size:
            next_fragment = min(max_fragment, size - offset)

            message = struct.pack(self.debugger.byte_order + "BBIB",
                                  WEBSOCKET_BINARY_FRAME | WEBSOCKET_FIN_BIT,
                                  WEBSOCKET_FIN_BIT + next_fragment + message_header,
                                  0,
                                  message_type)

            prev_offset = offset
            offset += next_fragment
            self.debugger.send_message(message + args[prev_offset:offset])

        self.stop = True

    def do_eval(self, args):
        """ Evaluate JavaScript source code """
        self.send_string(args, JERRY_DEBUGGER_EVAL)

    do_e = do_eval

    def do_exception(self, args):
        """ Config the exception handler module """
        if not args:
            print("Error: Status expected!")
            return

        enable = int(args)

        if enable == 1:
            logging.debug("Stop at exception enabled")
        elif enable == 0:
            logging.debug("Stop at exception disabled")
        else:
            print("Error: Invalid input! Usage 1: [Enable] or 0: [Disable].")
            return

        self.debugger.send_exception_config(enable)

    def do_memstats(self, args):
        """ Memory statistics """
        self.exec_command(args, JERRY_DEBUGGER_MEMSTATS)
        return

    do_ms = do_memstats

    def store_client_sources(self, args):
        self.client_sources = args

    def send_client_source(self):
        # Send no more source message if there is no source
        if not self.client_sources:
            self.send_no_more_source()
            return

        path = self.client_sources.pop(0)
        if not path.lower().endswith('.js'):
            sys.exit("Error: Javascript file expected!")
            return

        with open(path, 'r') as f:
            content = path + "\0" + f.read()
            self.send_string(content, JERRY_DEBUGGER_CLIENT_SOURCE)

    def send_no_more_source(self):
        self.exec_command("", JERRY_DEBUGGER_NO_MORE_SOURCES)
        self.cont = True

class Multimap(object):

    def __init__(self):
        self.map = {}

    def get(self, key):
        if key in self.map:
            return self.map[key]
        return []

    def insert(self, key, value):
        if key in self.map:
            self.map[key].append(value)
        else:
            self.map[key] = [value]

    def delete(self, key, value):
        items = self.map[key]

        if len(items) == 1:
            del self.map[key]
        else:
            del items[items.index(value)]

    def __repr__(self):
        return "Multimap(%r)" % (self.map)


class JerryDebugger(object):

    def __init__(self, address):

        if ":" not in address:
            self.host = address
            self.port = 5001  # use default port
        else:
            self.host, self.port = address.split(":")
            self.port = int(self.port)

        print("Connecting to: %s:%s" % (self.host, self.port))

        self.message_data = b""
        self.function_list = {}
        self.last_breakpoint_hit = None
        self.next_breakpoint_index = 0
        self.active_breakpoint_list = {}
        self.pending_breakpoint_list = {}
        self.line_list = Multimap()
        self.display = 0
        self.default_viewrange = 3
        self.green = ''
        self.red = ''
        self.yellow = ''
        self.green_bg = ''
        self.yellow_bg = ''
        self.blue = ''
        self.nocolor = ''
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client_socket.connect((self.host, self.port))

        self.send_message(b"GET /jerry-debugger HTTP/1.1\r\n" +
                          b"Upgrade: websocket\r\n" +
                          b"Connection: Upgrade\r\n" +
                          b"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n")
        result = b""
        expected = (b"HTTP/1.1 101 Switching Protocols\r\n" +
                    b"Upgrade: websocket\r\n" +
                    b"Connection: Upgrade\r\n" +
                    b"Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n\r\n")

        len_expected = len(expected)

        while len(result) < len_expected:
            result += self.client_socket.recv(1024)

        len_result = len(result)

        if result[0:len_expected] != expected:
            raise Exception("Unexpected handshake")

        if len_result > len_expected:
            result = result[len_expected:]
        else:
            result = b""

        len_expected = 6
        # Network configurations, which has the following struct:
        # header [2] - opcode[1], size[1]
        # type [1]
        # max_message_size [1]
        # cpointer_size [1]
        # little_endian [1]

        while len(result) < len_expected:
            result += self.client_socket.recv(1024)

        len_result = len(result)

        expected = struct.pack("BBB",
                               WEBSOCKET_BINARY_FRAME | WEBSOCKET_FIN_BIT,
                               4,
                               JERRY_DEBUGGER_CONFIGURATION)

        if result[0:3] != expected:
            raise Exception("Unexpected configuration")

        self.max_message_size = ord(result[3])
        self.cp_size = ord(result[4])
        self.little_endian = ord(result[5])

        if self.little_endian:
            self.byte_order = "<"
            logging.debug("Little-endian machine")
        else:
            self.byte_order = ">"
            logging.debug("Big-endian machine")

        if self.cp_size == 2:
            self.cp_format = "H"
        else:
            self.cp_format = "I"

        self.idx_format = "I"

        logging.debug("Compressed pointer size: %d", self.cp_size)

        if len_result > len_expected:
            self.message_data = result[len_expected:]

    def __del__(self):
        self.client_socket.close()

    def send_message(self, message):
        size = len(message)
        while size > 0:
            bytes_send = self.client_socket.send(message)
            if bytes_send < size:
                message = message[bytes_send:]
            size -= bytes_send

    def delete_active(self):
        for i in self.active_breakpoint_list.values():
            breakpoint = self.active_breakpoint_list[i.active_index]
            del self.active_breakpoint_list[i.active_index]
            breakpoint.active_index = -1
            self.send_breakpoint(breakpoint)

    def delete_pending(self):
        self.pending_breakpoint_list.clear()

    def breakpoint_pending_exists(self, breakpoint):
        for existing_bp in self.pending_breakpoint_list.values():
            if (breakpoint.line and existing_bp.source_name == breakpoint.source_name and \
                  existing_bp.line == breakpoint.line) \
               or (not breakpoint.line and existing_bp.function == breakpoint.function):
                return True

        return False

    def send_breakpoint(self, breakpoint):
        message = struct.pack(self.byte_order + "BBIBB" + self.cp_format + self.idx_format,
                              WEBSOCKET_BINARY_FRAME | WEBSOCKET_FIN_BIT,
                              WEBSOCKET_FIN_BIT + 1 + 1 + self.cp_size + 4,
                              0,
                              JERRY_DEBUGGER_UPDATE_BREAKPOINT,
                              int(breakpoint.active_index >= 0),
                              breakpoint.function.byte_code_cp,
                              breakpoint.offset)
        self.send_message(message)

    def send_bytecode_cp(self, byte_code_cp):
        message = struct.pack(self.byte_order + "BBIB" + self.cp_format,
                              WEBSOCKET_BINARY_FRAME | WEBSOCKET_FIN_BIT,
                              WEBSOCKET_FIN_BIT + 1 + self.cp_size,
                              0,
                              JERRY_DEBUGGER_FREE_BYTE_CODE_CP,
                              byte_code_cp)
        self.send_message(message)

    def send_command(self, command):
        message = struct.pack(self.byte_order + "BBIB",
                              WEBSOCKET_BINARY_FRAME | WEBSOCKET_FIN_BIT,
                              WEBSOCKET_FIN_BIT + 1,
                              0,
                              command)
        self.send_message(message)

    def send_exception_config(self, enable):
        message = struct.pack(self.byte_order + "BBIBB",
                              WEBSOCKET_BINARY_FRAME | WEBSOCKET_FIN_BIT,
                              WEBSOCKET_FIN_BIT + 1 + 1,
                              0,
                              JERRY_DEBUGGER_EXCEPTION_CONFIG,
                              enable)
        self.send_message(message)

    def set_colors(self):
        self.green = '\033[92m'
        self.nocolor = '\033[0m'
        self.red = '\033[31m'
        self.yellow = '\033[93m'
        self.green_bg = '\033[42m'
        self.yellow_bg = '\033[43m'
        self.blue = '\033[94m'

    def get_message(self, blocking):
        # Connection was closed
        if self.message_data is None:
            return None

        while True:
            if len(self.message_data) >= 2:
                if ord(self.message_data[0]) != WEBSOCKET_BINARY_FRAME | WEBSOCKET_FIN_BIT:
                    raise Exception("Unexpected data frame")

                size = ord(self.message_data[1])
                if size == 0 or size >= 126:
                    raise Exception("Unexpected data frame")

                if len(self.message_data) >= size + 2:
                    result = self.message_data[0:size + 2]
                    self.message_data = self.message_data[size + 2:]
                    return result

            if not blocking:
                select_result = select.select([self.client_socket], [], [], 0)[0]
                if self.client_socket not in select_result:
                    return b''

            data = self.client_socket.recv(MAX_BUFFER_SIZE)

            if not data:
                self.message_data = None
                return None

            self.message_data += data


def parse_source(debugger, data):
    source_code = ""
    source_code_name = ""
    function_name = ""
    stack = [{"line": 1,
              "column": 1,
              "name": "",
              "lines": [],
              "offsets": []}]
    new_function_list = {}

    while True:
        if data is None:
            return

        buffer_type = ord(data[2])
        buffer_size = ord(data[1]) - 1

        logging.debug("Parser buffer type: %d, message size: %d", buffer_type, buffer_size)

        if buffer_type == JERRY_DEBUGGER_PARSE_ERROR:
            logging.error("Parser error!")
            return

        elif buffer_type in [JERRY_DEBUGGER_SOURCE_CODE, JERRY_DEBUGGER_SOURCE_CODE_END]:
            source_code += data[3:]

        elif buffer_type in [JERRY_DEBUGGER_SOURCE_CODE_NAME, JERRY_DEBUGGER_SOURCE_CODE_NAME_END]:
            source_code_name += data[3:]

        elif buffer_type in [JERRY_DEBUGGER_FUNCTION_NAME, JERRY_DEBUGGER_FUNCTION_NAME_END]:
            function_name += data[3:]

        elif buffer_type == JERRY_DEBUGGER_PARSE_FUNCTION:
            logging.debug("Source name: %s, function name: %s", source_code_name, function_name)

            position = struct.unpack(debugger.byte_order + debugger.idx_format + debugger.idx_format,
                                     data[3: 3 + 4 + 4])

            stack.append({"source": source_code,
                          "source_name": source_code_name,
                          "line": position[0],
                          "column": position[1],
                          "name": function_name,
                          "lines": [],
                          "offsets": []})
            function_name = ""

        elif buffer_type in [JERRY_DEBUGGER_BREAKPOINT_LIST, JERRY_DEBUGGER_BREAKPOINT_OFFSET_LIST]:
            name = "lines"
            if buffer_type == JERRY_DEBUGGER_BREAKPOINT_OFFSET_LIST:
                name = "offsets"

            logging.debug("Breakpoint %s received", name)

            buffer_pos = 3
            while buffer_size > 0:
                line = struct.unpack(debugger.byte_order + debugger.idx_format,
                                     data[buffer_pos: buffer_pos + 4])
                stack[-1][name].append(line[0])
                buffer_pos += 4
                buffer_size -= 4

        elif buffer_type == JERRY_DEBUGGER_BYTE_CODE_CP:
            byte_code_cp = struct.unpack(debugger.byte_order + debugger.cp_format,
                                         data[3: 3 + debugger.cp_size])[0]

            logging.debug("Byte code cptr received: {0x%x}", byte_code_cp)

            func_desc = stack.pop()

            # We know the last item in the list is the general byte code.
            if len(stack) == 0:
                func_desc["source"] = source_code
                func_desc["source_name"] = source_code_name

            function = JerryFunction(len(stack) != 0,
                                     byte_code_cp,
                                     func_desc["source"],
                                     func_desc["source_name"],
                                     func_desc["line"],
                                     func_desc["column"],
                                     func_desc["name"],
                                     func_desc["lines"],
                                     func_desc["offsets"])

            new_function_list[byte_code_cp] = function

            if len(stack) == 0:
                logging.debug("Parse completed.")
                break

        elif buffer_type == JERRY_DEBUGGER_RELEASE_BYTE_CODE_CP:
            # Redefined functions are dropped during parsing.
            byte_code_cp = struct.unpack(debugger.byte_order + debugger.cp_format,
                                         data[3: 3 + debugger.cp_size])[0]

            if byte_code_cp in new_function_list:
                del new_function_list[byte_code_cp]
                debugger.send_bytecode_cp(byte_code_cp)
            else:
                release_function(debugger, data)

        else:
            logging.error("Parser error!")
            return

        data = debugger.get_message(True)

    # Copy the ready list to the global storage.
    debugger.function_list.update(new_function_list)

    for function in new_function_list.values():
        for line, breakpoint in function.lines.items():
            debugger.line_list.insert(line, breakpoint)

    # Try to set the pending breakpoints
    if len(debugger.pending_breakpoint_list) != 0:
        logging.debug("Pending breakpoints list: %s", debugger.pending_breakpoint_list)
        bp_list = debugger.pending_breakpoint_list

        for breakpoint in bp_list:
            for src in debugger.function_list.values():
                if src.source_name == bp_list[breakpoint].source_name:
                    source_lines = len(src.source)
                else:
                    source_lines = 0

            if bp_list[breakpoint].line:
                if bp_list[breakpoint].line <= source_lines:
                    tmp_bp = breakpoint
                    breakpoint = bp_list[breakpoint].source_name + ":" + str(bp_list[breakpoint].line)
                    if set_breakpoint(debugger, breakpoint, True):
                        del tmp_bp
            elif bp_list[breakpoint].function:
                tmp_bp = breakpoint
                breakpoint = bp_list[breakpoint].function
                if set_breakpoint(debugger, breakpoint, True):
                    del tmp_bp


    else:
        logging.debug("No pending breakpoints")


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


def print_source(debugger, line_num):
    last_bp = debugger.last_breakpoint_hit
    if not last_bp:
        return

    lines = last_bp.function.source
    if last_bp.function.source_name:
        print("Source: %s" % (last_bp.function.source_name))

    if line_num == 0:
        start = 0
        end = len(last_bp.function.source)
    else:
        start = max(last_bp.line - line_num, 0)
        end = min(last_bp.line + line_num - 1, len(last_bp.function.source))

    for i in range(start, end):
        if i == last_bp.line - 1:
            print("%s%4d%s %s>%s %s" % (debugger.green, i + 1, debugger.nocolor, debugger.red, \
                                        debugger.nocolor, lines[i]))
        else:
            print("%s%4d%s   %s" % (debugger.green, i + 1, debugger.nocolor, lines[i]))


def release_function(debugger, data):
    byte_code_cp = struct.unpack(debugger.byte_order + debugger.cp_format,
                                 data[3: 3 + debugger.cp_size])[0]

    function = debugger.function_list[byte_code_cp]

    for line, breakpoint in function.lines.items():
        debugger.line_list.delete(line, breakpoint)
        if breakpoint.active_index >= 0:
            del debugger.active_breakpoint_list[breakpoint.active_index]

    del debugger.function_list[byte_code_cp]

    debugger.send_bytecode_cp(byte_code_cp)

    logging.debug("Function {0x%x} byte-code released", byte_code_cp)


def enable_breakpoint(debugger, breakpoint):
    if isinstance(breakpoint, JerryPendingBreakpoint):
        if not debugger.breakpoint_pending_exists(breakpoint):
            debugger.next_breakpoint_index += 1
            breakpoint.index = debugger.next_breakpoint_index
            debugger.pending_breakpoint_list[debugger.next_breakpoint_index] = breakpoint
            print("%sPending breakpoint%s at %s" % (debugger.yellow, debugger.nocolor, breakpoint))
        else:
            print("%sPending breakpoint%s already exists" % (debugger.yellow, debugger.nocolor))

    else:
        if breakpoint.active_index < 0:
            debugger.next_breakpoint_index += 1
            debugger.active_breakpoint_list[debugger.next_breakpoint_index] = breakpoint
            breakpoint.active_index = debugger.next_breakpoint_index
            debugger.send_breakpoint(breakpoint)

        print("%sBreakpoint %d %sat %s" % (debugger.green, breakpoint.active_index, debugger.nocolor, breakpoint))


def set_breakpoint(debugger, string, pending):
    line = re.match("(.*):(\\d+)$", string)
    found = False

    if line:
        source_name = line.group(1)
        new_line = int(line.group(2))

        for breakpoint in debugger.line_list.get(new_line):
            func_source = breakpoint.function.source_name
            if (source_name == func_source or
                    func_source.endswith("/" + source_name) or
                    func_source.endswith("\\" + source_name)):

                enable_breakpoint(debugger, breakpoint)
                found = True

    else:
        for function in debugger.function_list.values():
            if function.name == string:
                enable_breakpoint(debugger, function.lines[function.first_breakpoint_line])
                found = True

    if not found and not pending:
        print("No breakpoint found, do you want to add a %spending breakpoint%s? (y or [n])" % \
             (debugger.yellow, debugger.nocolor))
        ans = sys.stdin.readline()
        if ans in ['yes\n', 'y\n']:
            if line:
                breakpoint = JerryPendingBreakpoint(int(line.group(2)), line.group(1))
            else:
                breakpoint = JerryPendingBreakpoint(function=string)
            enable_breakpoint(debugger, breakpoint)
    elif not found and pending:
        return False

    return True


def get_breakpoint(debugger, breakpoint_data):
    function = debugger.function_list[breakpoint_data[0]]
    offset = breakpoint_data[1]

    if offset in function.offsets:
        return (function.offsets[offset], True)

    if offset < function.first_breakpoint_offset:
        return (function.offsets[function.first_breakpoint_offset], False)

    nearest_offset = -1

    for current_offset in function.offsets:
        if current_offset <= offset and current_offset > nearest_offset:
            nearest_offset = current_offset

    return (function.offsets[nearest_offset], False)


def main():
    args = arguments_parse()

    debugger = JerryDebugger(args.address)
    exception_string = ""

    if args.color:
        debugger.set_colors()

    non_interactive = args.non_interactive

    logging.debug("Connected to JerryScript on %d port", debugger.port)

    prompt = DebuggerPrompt(debugger)
    prompt.prompt = "(jerry-debugger) "
    prompt.non_interactive = non_interactive

    if args.display:
        prompt.do_display(args.display)

    if args.exception is not None:
        prompt.do_exception(str(args.exception))

    if args.client_source is not None:
        prompt.store_client_sources(args.client_source)

    while True:
        if not non_interactive and prompt.cont:
            if sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
                sys.stdin.readline()
                prompt.cont = False
                debugger.send_command(JERRY_DEBUGGER_STOP)

        data = debugger.get_message(False)

        if data == b'':
            continue

        if not data:  # Break the while loop if there is no more data.
            break

        buffer_type = ord(data[2])
        buffer_size = ord(data[1]) - 1

        logging.debug("Main buffer type: %d, message size: %d", buffer_type, buffer_size)

        if buffer_type in [JERRY_DEBUGGER_PARSE_ERROR,
                           JERRY_DEBUGGER_BYTE_CODE_CP,
                           JERRY_DEBUGGER_PARSE_FUNCTION,
                           JERRY_DEBUGGER_BREAKPOINT_LIST,
                           JERRY_DEBUGGER_SOURCE_CODE,
                           JERRY_DEBUGGER_SOURCE_CODE_END,
                           JERRY_DEBUGGER_SOURCE_CODE_NAME,
                           JERRY_DEBUGGER_SOURCE_CODE_NAME_END,
                           JERRY_DEBUGGER_FUNCTION_NAME,
                           JERRY_DEBUGGER_FUNCTION_NAME_END]:
            parse_source(debugger, data)

        elif buffer_type == JERRY_DEBUGGER_RELEASE_BYTE_CODE_CP:
            release_function(debugger, data)

        elif buffer_type in [JERRY_DEBUGGER_BREAKPOINT_HIT, JERRY_DEBUGGER_EXCEPTION_HIT]:
            breakpoint_data = struct.unpack(debugger.byte_order + debugger.cp_format + debugger.idx_format, data[3:])

            breakpoint = get_breakpoint(debugger, breakpoint_data)
            debugger.last_breakpoint_hit = breakpoint[0]

            if buffer_type == JERRY_DEBUGGER_EXCEPTION_HIT:
                print("Exception throw detected (to disable automatic stop type exception 0)")
                if exception_string:
                    print("Exception hint: %s" % (exception_string))
                    exception_string = ""

            if breakpoint[1]:
                breakpoint_info = "at"
            else:
                breakpoint_info = "around"

            if breakpoint[0].active_index >= 0:
                breakpoint_info += " breakpoint:%s%d%s" % (debugger.red, breakpoint[0].active_index, debugger.nocolor)

            print("Stopped %s %s" % (breakpoint_info, breakpoint[0]))
            if debugger.display:
                print_source(prompt.debugger, debugger.display)

            prompt.cmdloop()
            if prompt.quit:
                break

        elif buffer_type == JERRY_DEBUGGER_EXCEPTION_STR:
            exception_string += data[3:]

        elif buffer_type == JERRY_DEBUGGER_EXCEPTION_STR_END:
            exception_string += data[3:]

        elif buffer_type in [JERRY_DEBUGGER_BACKTRACE, JERRY_DEBUGGER_BACKTRACE_END]:
            frame_index = 0

            while True:

                buffer_pos = 3
                while buffer_size > 0:
                    breakpoint_data = struct.unpack(debugger.byte_order + debugger.cp_format + debugger.idx_format,
                                                    data[buffer_pos: buffer_pos + debugger.cp_size + 4])

                    breakpoint = get_breakpoint(debugger, breakpoint_data)

                    print("Frame %d: %s" % (frame_index, breakpoint[0]))

                    frame_index += 1
                    buffer_pos += 6
                    buffer_size -= 6

                if buffer_type == JERRY_DEBUGGER_BACKTRACE_END:
                    break

                data = debugger.get_message(True)
                buffer_type = ord(data[2])
                buffer_size = ord(data[1]) - 1

                if buffer_type not in [JERRY_DEBUGGER_BACKTRACE,
                                       JERRY_DEBUGGER_BACKTRACE_END]:
                    raise Exception("Backtrace data expected")

            prompt.cmdloop()


        elif buffer_type in [JERRY_DEBUGGER_EVAL_RESULT,
                             JERRY_DEBUGGER_EVAL_RESULT_END,
                             JERRY_DEBUGGER_OUTPUT_RESULT,
                             JERRY_DEBUGGER_OUTPUT_RESULT_END]:
            message = b""
            msg_type = buffer_type
            while True:
                if buffer_type in [JERRY_DEBUGGER_EVAL_RESULT_END,
                                   JERRY_DEBUGGER_OUTPUT_RESULT_END]:
                    subtype = ord(data[-1])
                    message += data[3:-1]
                    break
                else:
                    message += data[3:]

                data = debugger.get_message(True)
                buffer_type = ord(data[2])
                buffer_size = ord(data[1]) - 1
                # Checks if the next frame would be an invalid data frame.
                # If it is not the message type, or the end type of it, an exception is thrown.
                if buffer_type not in [msg_type, msg_type + 1]:
                    raise Exception("Invalid data caught")

            # Subtypes of output
            if buffer_type == JERRY_DEBUGGER_OUTPUT_RESULT_END:
                if subtype in [JERRY_DEBUGGER_OUTPUT_OK,
                               JERRY_DEBUGGER_OUTPUT_DEBUG]:
                    print("%sout: %s%s" % (debugger.blue, debugger.nocolor, message))
                elif subtype == JERRY_DEBUGGER_OUTPUT_WARNING:
                    print("%swarning: %s%s" % (debugger.yellow, debugger.nocolor, message))
                elif subtype == JERRY_DEBUGGER_OUTPUT_ERROR:
                    print("%serr: %s%s" % (debugger.red, debugger.nocolor, message))
                elif subtype == JERRY_DEBUGGER_OUTPUT_TRACE:
                    print("%strace: %s%s" % (debugger.blue, debugger.nocolor, message))

            # Subtypes of eval
            elif buffer_type == JERRY_DEBUGGER_EVAL_RESULT_END:
                if subtype == JERRY_DEBUGGER_EVAL_ERROR:
                    print("Uncaught exception: %s" % (message))
                else:
                    print(message)

                prompt.cmdloop()

        elif buffer_type == JERRY_DEBUGGER_MEMSTATS_RECEIVE:

            memory_stats = struct.unpack(debugger.byte_order + debugger.idx_format *5,
                                         data[3: 3 + 4 *5])

            print("Allocated bytes: %d" % (memory_stats[0]))
            print("Byte code bytes: %d" % (memory_stats[1]))
            print("String bytes: %d" % (memory_stats[2]))
            print("Object bytes: %d" % (memory_stats[3]))
            print("Property bytes: %d" % (memory_stats[4]))

            prompt.cmdloop()

        elif buffer_type == JERRY_DEBUGGER_WAIT_FOR_SOURCE:
            prompt.send_client_source()


        else:
            raise Exception("Unknown message")


if __name__ == "__main__":
    try:
        main()
    except socket.error as error_msg:
        try:
            errno = error_msg.errno
            msg = str(error_msg)
        except:
            errno = error_msg[0]
            msg = error_msg[1]

        if errno == 111:
            sys.exit("Failed to connect to the JerryScript debugger.")
        elif errno == 32 or errno == 104:
            sys.exit("Connection closed.")
        else:
            sys.exit("Failed to connect to the JerryScript debugger.\nError: %s" % (msg))
