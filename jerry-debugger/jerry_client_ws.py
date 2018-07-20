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
import argparse
import logging
import re
import select
import socket
import struct
import sys

# Expected debugger protocol version.
JERRY_DEBUGGER_VERSION = 5
JERRY_DEBUGGER_DATA_END = '\3'

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
JERRY_DEBUGGER_WAITING_AFTER_PARSE = 13
JERRY_DEBUGGER_RELEASE_BYTE_CODE_CP = 14
JERRY_DEBUGGER_MEMSTATS_RECEIVE = 15
JERRY_DEBUGGER_BREAKPOINT_HIT = 16
JERRY_DEBUGGER_EXCEPTION_HIT = 17
JERRY_DEBUGGER_EXCEPTION_STR = 18
JERRY_DEBUGGER_EXCEPTION_STR_END = 19
JERRY_DEBUGGER_BACKTRACE_TOTAL = 20
JERRY_DEBUGGER_BACKTRACE = 21
JERRY_DEBUGGER_BACKTRACE_END = 22
JERRY_DEBUGGER_EVAL_RESULT = 23
JERRY_DEBUGGER_EVAL_RESULT_END = 24
JERRY_DEBUGGER_WAIT_FOR_SOURCE = 25
JERRY_DEBUGGER_OUTPUT_RESULT = 26
JERRY_DEBUGGER_OUTPUT_RESULT_END = 27

# Subtypes of eval
JERRY_DEBUGGER_EVAL_EVAL = "\0"
JERRY_DEBUGGER_EVAL_THROW = "\1"
JERRY_DEBUGGER_EVAL_ABORT = "\2"

# Subtypes of eval result
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
JERRY_DEBUGGER_PARSER_CONFIG = 4
JERRY_DEBUGGER_MEMSTATS = 5
JERRY_DEBUGGER_STOP = 6
JERRY_DEBUGGER_PARSER_RESUME = 7
JERRY_DEBUGGER_CLIENT_SOURCE = 8
JERRY_DEBUGGER_CLIENT_SOURCE_PART = 9
JERRY_DEBUGGER_NO_MORE_SOURCES = 10
JERRY_DEBUGGER_CONTEXT_RESET = 11
JERRY_DEBUGGER_CONTINUE = 12
JERRY_DEBUGGER_STEP = 13
JERRY_DEBUGGER_NEXT = 14
JERRY_DEBUGGER_FINISH = 15
JERRY_DEBUGGER_GET_BACKTRACE = 16
JERRY_DEBUGGER_EVAL = 17
JERRY_DEBUGGER_EVAL_PART = 18

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
    # pylint: disable=too-many-instance-attributes,too-many-arguments
    def __init__(self, is_func, byte_code_cp, source, source_name, line, column, name, lines, offsets):
        self.is_func = bool(is_func)
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

        for i, _line in enumerate(lines):
            offset = offsets[i]
            breakpoint = JerryBreakpoint(_line, offset, self)
            self.lines[_line] = breakpoint
            self.offsets[offset] = breakpoint

    def __repr__(self):
        result = ("Function(byte_code_cp:0x%x, source_name:%r, name:%r, line:%d, column:%d { "
                  % (self.byte_code_cp, self.source_name, self.name, self.line, self.column))

        result += ','.join([str(breakpoint) for breakpoint in self.lines.values()])

        return result + " })"


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


class DisplayData(object):

    def __init__(self, stype, sdata):
        self.stype = stype
        self.sdata = sdata

    def get_type(self):
        return self.stype

    def get_data(self):
        return self.sdata


class JerryDebugger(object):
    # pylint: disable=too-many-instance-attributes,too-many-statements,too-many-public-methods,no-self-use
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
        self.source = ''
        self.source_name = ''
        self.client_sources = []
        self.last_breakpoint_hit = None
        self.next_breakpoint_index = 0
        self.active_breakpoint_list = {}
        self.pending_breakpoint_list = {}
        self.line_list = Multimap()
        self.display = 0
        self.green = ''
        self.red = ''
        self.yellow = ''
        self.green_bg = ''
        self.yellow_bg = ''
        self.blue = ''
        self.nocolor = ''
        self.src_offset = 0
        self.src_offset_diff = 0
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client_socket.connect((self.host, self.port))
        self.non_interactive = False
        self.breakpoint_info = ''
        self.smessage = ''
        self.min_depth = 0
        self.max_depth = 0
        self.get_total = 0

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

        len_expected = 7
        # Network configurations, which has the following struct:
        # header [2] - opcode[1], size[1]
        # type [1]
        # max_message_size [1]
        # cpointer_size [1]
        # little_endian [1]
        # version [1]

        while len(result) < len_expected:
            result += self.client_socket.recv(1024)

        len_result = len(result)

        expected = struct.pack("BBB",
                               WEBSOCKET_BINARY_FRAME | WEBSOCKET_FIN_BIT,
                               5,
                               JERRY_DEBUGGER_CONFIGURATION)

        if result[0:3] != expected:
            raise Exception("Unexpected configuration")

        self.max_message_size = ord(result[3])
        self.cp_size = ord(result[4])
        self.little_endian = ord(result[5])
        self.version = ord(result[6])

        if self.version != JERRY_DEBUGGER_VERSION:
            raise Exception("Incorrect debugger version from target: %d expected: %d" %
                            (self.version, JERRY_DEBUGGER_VERSION))


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

    def _exec_command(self, command_id):
        self.send_command(command_id)

    def quit(self):
        self._exec_command(JERRY_DEBUGGER_CONTINUE)

    def get_continue(self):
        self._exec_command(JERRY_DEBUGGER_CONTINUE)

    def stop(self):
        self.send_command(JERRY_DEBUGGER_STOP)

    def finish(self):
        self._exec_command(JERRY_DEBUGGER_FINISH)

    def next(self):
        self._exec_command(JERRY_DEBUGGER_NEXT)

    def step(self):
        self._exec_command(JERRY_DEBUGGER_STEP)

    def memstats(self):
        self._exec_command(JERRY_DEBUGGER_MEMSTATS)

    def set_break(self, args):
        if args == "":
            return DisplayData("break", "Error: Breakpoint index expected")
        elif ':' in args:
            try:
                args_second = int(args.split(':', 1)[1])
                if args_second < 0:
                    return DisplayData("break", "Error: Positive breakpoint index expected")
                else:
                    _set_breakpoint(self, args, False)
            except ValueError as val_errno:
                return DisplayData("break", "Positive breakpoint index expected: %s" % val_errno)
        else:
            _set_breakpoint(self, args, False)
        if self.breakpoint_info == '':
            return None

        sbp = self.breakpoint_info
        self.breakpoint_info = ''
        return  DisplayData("break", sbp)

    def delete(self, args):
        if not args:
            result = "Error: Breakpoint index expected\n" \
                       "Delete the given breakpoint, use 'delete all|active|pending' \
                       to clear all the given breakpoints "
            return DisplayData("delete", result)
        elif args in ['all', 'pending', 'active']:
            if args == "all":
                self.delete_active()
                self.delete_pending()
            elif args == "pending":
                self.delete_pending()
            elif args == "active":
                self.delete_active()
        else:
            try:
                breakpoint_index = int(args)
            except ValueError as val_errno:
                return DisplayData("delete", "Error: Integer number expected, %s" % (val_errno))

            if breakpoint_index in self.active_breakpoint_list:
                breakpoint = self.active_breakpoint_list[breakpoint_index]
                del self.active_breakpoint_list[breakpoint_index]
                breakpoint.active_index = -1
                self.send_breakpoint(breakpoint)
            elif breakpoint_index in self.pending_breakpoint_list:
                del self.pending_breakpoint_list[breakpoint_index]
                if not self.pending_breakpoint_list:
                    self.send_parser_config(0)
            else:
                return DisplayData("delete", "Error: Breakpoint %d not found" % (breakpoint_index))

    def show_breakpoint_list(self):
        result = ''
        if self.active_breakpoint_list:
            result += "=== %sActive breakpoints %s ===\n" % (self.green_bg, self.nocolor)
            for breakpoint in self.active_breakpoint_list.values():
                result += " %d: %s\n" % (breakpoint.active_index, breakpoint)
        if self.pending_breakpoint_list:
            result += "=== %sPending breakpoints%s ===\n" % (self.yellow_bg, self.nocolor)
            for breakpoint in self.pending_breakpoint_list.values():
                result += " %d: %s (pending)\n" % (breakpoint.index, breakpoint)

        if not self.active_breakpoint_list and not self.pending_breakpoint_list:
            result += "No breakpoints"
        if result[-1:] == '\n':
            result = result[:-1]
        return DisplayData("delete", result)

    def backtrace(self, args):
        self.min_depth = 0
        self.max_depth = 0
        self.get_total = 0

        if args:
            args = args.split(" ")
            try:
                if "t" in args:
                    self.get_total = 1
                    args = args[:-1]
                if len(args) == 2:
                    self.min_depth = int(args[0])
                    self.max_depth = int(args[1])
                    if self.max_depth <= 0 or self.min_depth < 0:
                        return DisplayData("backtrace", "Error: Positive integer number expected")
                    if self.min_depth > self.max_depth:
                        return DisplayData("backtrace", "Error: Start depth needs to be lower than or equal to max" \
                                            " depth")

                elif len(args) == 1:
                    self.max_depth = int(args[0])
                    if self.max_depth <= 0:
                        return DisplayData("backtrace", "Error: Positive integer number expected")

            except ValueError as val_errno:
                return DisplayData("backtrace", "Error: Positive integer number expected, %s" % (val_errno))

        message = struct.pack(self.byte_order + "BBIB" + self.idx_format + self.idx_format  + "B",
                              WEBSOCKET_BINARY_FRAME | WEBSOCKET_FIN_BIT,
                              WEBSOCKET_FIN_BIT + 1 + 4 + 4 + 1,
                              0,
                              JERRY_DEBUGGER_GET_BACKTRACE,
                              self.min_depth,
                              self.max_depth,
                              self.get_total)
        self.send_message(message)

    def eval(self, code):
        self._send_string(JERRY_DEBUGGER_EVAL_EVAL + code, JERRY_DEBUGGER_EVAL)

    def throw(self, code):
        self._send_string(JERRY_DEBUGGER_EVAL_THROW + code, JERRY_DEBUGGER_EVAL)

    def abort(self, args):
        self.delete("all")
        self.exception("0")  # disable the exception handler
        self._send_string(JERRY_DEBUGGER_EVAL_ABORT + args, JERRY_DEBUGGER_EVAL)

    def restart(self):
        self._send_string(JERRY_DEBUGGER_EVAL_ABORT + "\"r353t\"", JERRY_DEBUGGER_EVAL)

    def exception(self, args):
        try:
            enabled = int(args)
        except (ValueError, TypeError) as val_errno:
            return DisplayData("exception", "Error: Positive integer number expected, %s" % (val_errno))

        if enabled not in [0, 1]:
            return DisplayData("delete", "Error: Invalid input! Usage 1: [Enable] or 0: [Disable].")

        if enabled:
            logging.debug("Stop at exception enabled")
            self.send_exception_config(enabled)

            return DisplayData("exception", "Stop at exception enabled")

        logging.debug("Stop at exception disabled")
        self.send_exception_config(enabled)

        return DisplayData("exception", "Stop at exception disabled")

    def _send_string(self, args, message_type):
        size = len(args)

        # 1: length of type byte
        # 4: length of an uint32 value
        message_header = 1 + 4
        max_fragment = min(self.max_message_size - message_header, size)

        message = struct.pack(self.byte_order + "BBIBI",
                              WEBSOCKET_BINARY_FRAME | WEBSOCKET_FIN_BIT,
                              WEBSOCKET_FIN_BIT + max_fragment + message_header,
                              0,
                              message_type,
                              size)
        if size == max_fragment:
            self.send_message(message + args)
            return

        self.send_message(message + args[0:max_fragment])
        offset = max_fragment

        if message_type == JERRY_DEBUGGER_EVAL:
            message_type = JERRY_DEBUGGER_EVAL_PART
        else:
            message_type = JERRY_DEBUGGER_CLIENT_SOURCE_PART

        # 1: length of type byte
        message_header = 1

        max_fragment = self.max_message_size - message_header
        while offset < size:
            next_fragment = min(max_fragment, size - offset)

            message = struct.pack(self.byte_order + "BBIB",
                                  WEBSOCKET_BINARY_FRAME | WEBSOCKET_FIN_BIT,
                                  WEBSOCKET_FIN_BIT + next_fragment + message_header,
                                  0,
                                  message_type)

            prev_offset = offset
            offset += next_fragment
            self.send_message(message + args[prev_offset:offset])

    def delete_active(self):
        for i in self.active_breakpoint_list.values():
            breakpoint = self.active_breakpoint_list[i.active_index]
            del self.active_breakpoint_list[i.active_index]
            breakpoint.active_index = -1
            self.send_breakpoint(breakpoint)

    def delete_pending(self):
        if self.pending_breakpoint_list:
            self.pending_breakpoint_list.clear()
            self.send_parser_config(0)

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

    def send_parser_config(self, enable):
        message = struct.pack(self.byte_order + "BBIBB",
                              WEBSOCKET_BINARY_FRAME | WEBSOCKET_FIN_BIT,
                              WEBSOCKET_FIN_BIT + 1 + 1,
                              0,
                              JERRY_DEBUGGER_PARSER_CONFIG,
                              enable)
        self.send_message(message)

    def set_colors(self):
        self.nocolor = '\033[0m'
        self.green = '\033[92m'
        self.red = '\033[31m'
        self.yellow = '\033[93m'
        self.green_bg = '\033[42m\033[30m'
        self.yellow_bg = '\033[43m\033[30m'
        self.blue = '\033[94m'

    def send_message(self, message):
        size = len(message)
        while size > 0:
            bytes_send = self.client_socket.send(message)
            if bytes_send < size:
                message = message[bytes_send:]
            size -= bytes_send


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

        with open(path, 'r') as src_file:
            content = path + "\0" + src_file.read()
            self._send_string(content, JERRY_DEBUGGER_CLIENT_SOURCE)

    def send_no_more_source(self):
        self._exec_command(JERRY_DEBUGGER_NO_MORE_SOURCES)

    # pylint: disable=too-many-branches,too-many-locals,too-many-statements
    def mainloop(self):
        result = ''
        exception_string = ""

        while True:
            data = self.get_message(False)

            if not self.non_interactive:
                if sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
                    sys.stdin.readline()
                    self.stop()
                    continue

            if data == b'':
                continue

            if not data:  # Break the while loop if there is no more data.
                if result[-1:] == '\n':
                    result = result[:-1]
                result += JERRY_DEBUGGER_DATA_END
                self.smessage = result
                return DisplayData("EOF", self.smessage)

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
                _parse_source(self, data)

            elif buffer_type == JERRY_DEBUGGER_WAITING_AFTER_PARSE:
                self.send_command(JERRY_DEBUGGER_PARSER_RESUME)

            elif buffer_type == JERRY_DEBUGGER_RELEASE_BYTE_CODE_CP:
                _release_function(self, data)

            elif buffer_type in [JERRY_DEBUGGER_BREAKPOINT_HIT, JERRY_DEBUGGER_EXCEPTION_HIT]:
                breakpoint_data = struct.unpack(self.byte_order + self.cp_format + self.idx_format, data[3:])

                breakpoint = _get_breakpoint(self, breakpoint_data)
                self.last_breakpoint_hit = breakpoint[0]

                if buffer_type == JERRY_DEBUGGER_EXCEPTION_HIT:
                    result += "Exception throw detected (to disable automatic stop type exception 0)\n"
                    if exception_string:
                        result += "Exception hint: %s\n" % (exception_string)
                        exception_string = ""

                if breakpoint[1]:
                    breakpoint_info = "at"
                else:
                    breakpoint_info = "around"

                if breakpoint[0].active_index >= 0:
                    breakpoint_info += " breakpoint:%s%d%s" % (self.red, breakpoint[0].active_index, self.nocolor)
                if self.breakpoint_info != '':
                    result += self.breakpoint_info + '\n'
                    self.breakpoint_info = ''
                result += "Stopped %s %s" % (breakpoint_info, breakpoint[0])
                self.smessage = result
                return DisplayData("break/exception", self.smessage)

            elif buffer_type == JERRY_DEBUGGER_EXCEPTION_STR:
                exception_string += data[3:]

            elif buffer_type == JERRY_DEBUGGER_EXCEPTION_STR_END:
                exception_string += data[3:]

            elif buffer_type in [JERRY_DEBUGGER_BACKTRACE,
                                 JERRY_DEBUGGER_BACKTRACE_END,
                                 JERRY_DEBUGGER_BACKTRACE_TOTAL]:
                if self.min_depth != 0:
                    frame_index = self.min_depth
                else:
                    frame_index = 0

                total_frames = 0
                while True:
                    if buffer_type == JERRY_DEBUGGER_BACKTRACE_TOTAL:
                        total_frames = struct.unpack(self.byte_order + self.idx_format, data[3:7])[0]

                    buffer_pos = 3
                    while buffer_size > 4:
                        breakpoint_data = struct.unpack(self.byte_order + self.cp_format + self.idx_format,
                                                        data[buffer_pos: buffer_pos + self.cp_size + 4])

                        breakpoint = _get_breakpoint(self, breakpoint_data)

                        result += "Frame %d: %s" % (frame_index, breakpoint[0])

                        frame_index += 1
                        buffer_pos += 6
                        buffer_size -= 6
                        if buffer_size > 0:
                            result += '\n'

                    if buffer_type == JERRY_DEBUGGER_BACKTRACE_END:
                        if self.get_total == 1:
                            if self.max_depth > total_frames:
                                self.max_depth = total_frames
                            if self.max_depth == 0:
                                result += ("\nTotal Frames: %d" % total_frames)
                            elif self.min_depth != 0 and self.max_depth != 0 and \
                                    (self.max_depth - self.min_depth) > 0:
                                result += ("\nGetting %d frames. There are %d in total." %
                                           ((self.max_depth - self.min_depth), total_frames))
                            elif self.min_depth == 0 and self.max_depth != 0:
                                result += ("\nGetting the first %d frames out of %d." % (self.max_depth, total_frames))
                        break

                    data = self.get_message(True)
                    buffer_type = ord(data[2])
                    buffer_size = ord(data[1]) - 1

                    if buffer_type not in [JERRY_DEBUGGER_BACKTRACE,
                                           JERRY_DEBUGGER_BACKTRACE_END]:
                        raise Exception("Backtrace data expected")
                self.smessage = result
                return DisplayData("backtrace", self.smessage)

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

                    data = self.get_message(True)
                    buffer_type = ord(data[2])
                    buffer_size = ord(data[1]) - 1
                    # Checks if the next frame would be an invalid data frame.
                    # If it is not the message type, or the end type of it, an exception is thrown.
                    if buffer_type not in [msg_type, msg_type + 1]:
                        raise Exception("Invalid data caught")

                # Subtypes of output
                if buffer_type == JERRY_DEBUGGER_OUTPUT_RESULT_END:
                    message = message.rstrip('\n')
                    if subtype in [JERRY_DEBUGGER_OUTPUT_OK,
                                   JERRY_DEBUGGER_OUTPUT_DEBUG]:
                        result += "%sout: %s%s\n" % (self.blue, self.nocolor, message)
                    elif subtype == JERRY_DEBUGGER_OUTPUT_WARNING:
                        result += "%swarning: %s%s\n" % (self.yellow, self.nocolor, message)
                    elif subtype == JERRY_DEBUGGER_OUTPUT_ERROR:
                        result += "%serr: %s%s\n" % (self.red, self.nocolor, message)
                    elif subtype == JERRY_DEBUGGER_OUTPUT_TRACE:
                        result += "%strace: %s%s\n" % (self.blue, self.nocolor, message)
                # Subtypes of eval
                elif buffer_type == JERRY_DEBUGGER_EVAL_RESULT_END:
                    if subtype == JERRY_DEBUGGER_EVAL_ERROR:
                        result += "Uncaught exception: %s" % (message)
                    else:
                        result += message
                    self.smessage = result
                    return DisplayData("eval_result", self.smessage)

            elif buffer_type == JERRY_DEBUGGER_MEMSTATS_RECEIVE:

                memory_stats = struct.unpack(self.byte_order + self.idx_format *5,
                                             data[3: 3 + 4 *5])

                result += "Allocated bytes: %s\n" % memory_stats[0]
                result += "Byte code bytes: %s\n" % memory_stats[1]
                result += "String bytes: %s\n" % memory_stats[2]
                result += "Object bytes: %s\n" % memory_stats[3]
                result += "Property bytes: %s\n" % memory_stats[4]

                self.smessage = str(result)
                return DisplayData("memstats", self.smessage)

            elif buffer_type == JERRY_DEBUGGER_WAIT_FOR_SOURCE:
                self.send_client_source()
            else:
                raise Exception("Unknown message")

    def print_source(self, line_num, offset):
        msg = ''
        last_bp = self.last_breakpoint_hit

        if not last_bp:
            return None

        lines = last_bp.function.source
        if last_bp.function.source_name:
            msg += "Source: %s\n" % (last_bp.function.source_name)

        if line_num == 0:
            start = 0
            end = len(last_bp.function.source)
        else:
            start = max(last_bp.line - line_num, 0)
            end = min(last_bp.line + line_num - 1, len(last_bp.function.source))
            if offset:
                if start + offset < 0:
                    self.src_offset += self.src_offset_diff
                    offset += self.src_offset_diff
                elif end + offset > len(last_bp.function.source):
                    self.src_offset -= self.src_offset_diff
                    offset -= self.src_offset_diff

                start = max(start + offset, 0)
                end = min(end + offset, len(last_bp.function.source))

        for i in range(start, end):
            if i == last_bp.line - 1:
                msg += "%s%4d%s %s>%s %s\n" % (self.green, i + 1, self.nocolor, self.red, \
                                                         self.nocolor, lines[i])
            else:
                msg += "%s%4d%s   %s\n" % (self.green, i + 1, self.nocolor, lines[i])
        msg = msg[:-1]
        return DisplayData("print_source", msg)

    def not_empty(self, args):
        if args is not None:
            return True

    def wait_data(self, args):
        if args is None:
            return True

    def check_empty_data(self, args):
        if not args:
            return True

# pylint: disable=too-many-branches,too-many-locals,too-many-statements
def _parse_source(debugger, data):
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
            if not stack:
                func_desc["source"] = source_code
                func_desc["source_name"] = source_code_name

            function = JerryFunction(stack,
                                     byte_code_cp,
                                     func_desc["source"],
                                     func_desc["source_name"],
                                     func_desc["line"],
                                     func_desc["column"],
                                     func_desc["name"],
                                     func_desc["lines"],
                                     func_desc["offsets"])

            new_function_list[byte_code_cp] = function

            if not stack:
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
                _release_function(debugger, data)

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
    if debugger.pending_breakpoint_list:
        logging.debug("Pending breakpoints list: %s", debugger.pending_breakpoint_list)
        bp_list = debugger.pending_breakpoint_list

        for breakpoint_index, breakpoint in bp_list.items():
            for src in debugger.function_list.values():
                if src.source_name == breakpoint.source_name:
                    source_lines = len(src.source)
                else:
                    source_lines = 0

            if breakpoint.line:
                if breakpoint.line <= source_lines:
                    command = breakpoint.source_name + ":" + str(breakpoint.line)
                    if _set_breakpoint(debugger, command, True):
                        del bp_list[breakpoint_index]
            elif breakpoint.function:
                command = breakpoint.function
                if _set_breakpoint(debugger, command, True):
                    del bp_list[breakpoint_index]

        if not bp_list:
            debugger.send_parser_config(0)

    else:
        logging.debug("No pending breakpoints")


def _release_function(debugger, data):
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


def _enable_breakpoint(debugger, breakpoint):
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
        if debugger.breakpoint_info:
            debugger.breakpoint_info += '\n'
        debugger.breakpoint_info += "%sBreakpoint %d %sat %s" % (debugger.green,
                                                                 breakpoint.active_index,
                                                                 debugger.nocolor,
                                                                 breakpoint)


def _set_breakpoint(debugger, string, pending):
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

                _enable_breakpoint(debugger, breakpoint)
                found = True

    else:
        for function in debugger.function_list.values():
            if function.name == string:
                _enable_breakpoint(debugger, function.lines[function.first_breakpoint_line])
                found = True

    if not found and not pending:
        print("No breakpoint found, do you want to add a %spending breakpoint%s? (y or [n])" % \
              (debugger.yellow, debugger.nocolor))

        ans = sys.stdin.readline()
        if ans in ['yes\n', 'y\n']:
            if not debugger.pending_breakpoint_list:
                debugger.send_parser_config(1)

            if line:
                breakpoint = JerryPendingBreakpoint(int(line.group(2)), line.group(1))
            else:
                breakpoint = JerryPendingBreakpoint(function=string)
            _enable_breakpoint(debugger, breakpoint)

    elif not found and pending:
        return False

    return True


def _get_breakpoint(debugger, breakpoint_data):
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
