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
import socket
import sys
import argparse
import logging
import re
from cmd import Cmd
from struct import *
from pprint import pprint  # For the readable stack printing.

# Messages sent by the server to client.
JERRY_DEBUGGER_CONFIGURATION = 1
JERRY_DEBUGGER_PARSE_ERROR = 2
JERRY_DEBUGGER_BYTE_CODE_CP = 3
JERRY_DEBUGGER_PARSE_FUNCTION = 4
JERRY_DEBUGGER_BREAKPOINT_LIST = 5
JERRY_DEBUGGER_BREAKPOINT_OFFSET_LIST = 6
JERRY_DEBUGGER_RESOURCE_NAME = 7
JERRY_DEBUGGER_RESOURCE_NAME_END = 8
JERRY_DEBUGGER_FUNCTION_NAME = 9
JERRY_DEBUGGER_FUNCTION_NAME_END = 10
JERRY_DEBUGGER_RELEASE_BYTE_CODE_CP = 11
JERRY_DEBUGGER_BREAKPOINT_HIT = 12
JERRY_DEBUGGER_BACKTRACE = 13
JERRY_DEBUGGER_BACKTRACE_END = 14
JERRY_DEBUGGER_EVAL_RESULT = 15
JERRY_DEBUGGER_EVAL_RESULT_END = 16
JERRY_DEBUGGER_EVAL_ERROR = 17
JERRY_DEBUGGER_EVAL_ERROR_END = 18

# Messages sent by the client to server.
JERRY_DEBUGGER_FREE_BYTE_CODE_CP = 1
JERRY_DEBUGGER_UPDATE_BREAKPOINT = 2
JERRY_DEBUGGER_STOP = 3
JERRY_DEBUGGER_CONTINUE = 4
JERRY_DEBUGGER_STEP = 5
JERRY_DEBUGGER_NEXT = 6
JERRY_DEBUGGER_GET_BACKTRACE = 7
JERRY_DEBUGGER_EVAL = 8
JERRY_DEBUGGER_EVAL_PART = 9

MAX_BUFFER_SIZE = 128
WEBSOCKET_BINARY_FRAME = 2
WEBSOCKET_FIN_BIT = 0x80


def arguments_parse():
    parser = argparse.ArgumentParser(description="JerryScript debugger client")

    parser.add_argument("address", action="store", nargs="?", default="localhost:5001", help="specify a unique network address for connection (default: %(default)s)")
    parser.add_argument("-v", "--verbose", action="store_true", default=False, help="increase verbosity (default: %(default)s)")

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

    def to_string(self):
        result = self.function.source

        if result == "":
            result = "<unknown>"

        result += ":%d" % (self.line)

        if self.function.name:
            result += " (in %s)" % (self.function.name)
        return result

    def __repr__(self):
        return ("Breakpoint(line:%d, offset:%d, active_index:%d)"
                % (self.line, self.offset, self.active_index))


class JerryFunction(object):

    def __init__(self, byte_code_cp, source, name, lines, offsets):
        self.byte_code_cp = byte_code_cp
        self.source = source
        self.name = name
        self.lines = {}
        self.offsets = {}
        self.first_line = -1

        if len > 0:
            self.first_line = lines[0]

        for i in range(len(lines)):
            line = lines[i]
            offset = offsets[i]
            breakpoint = JerryBreakpoint(line, offset, self)
            self.lines[line] = breakpoint
            self.offsets[offset] = breakpoint

    def __repr__(self):
        result = ("Function(byte_code_cp:0x%x, source:\"%s\", name:\"%s\", { "
                  % (self.byte_code_cp, self.source, self.name))

        result += ','.join([str(breakpoint) for breakpoint in self.lines.values()])

        return result + " })"


class DebuggerPrompt(Cmd):

    def __init__(self, debugger):
        Cmd.__init__(self)
        self.debugger = debugger
        self.stop = False

    def precmd(self, line):
        self.stop = False
        return line

    def postcmd(self, stop, line):
        return self.stop

    def insert_breakpoint(self, args):
        if args == "":
            print("Error: Breakpoint index expected")
        else:
            set_breakpoint(self.debugger, args)

    def do_break(self, args):
        """ Insert breakpoints on the given lines """
        self.insert_breakpoint(args)

    def do_b(self, args):
        """ Insert breakpoints on the given lines """
        self.insert_breakpoint(args)

    def exec_command(self, args, command_id):
        self.stop = True
        if args != "":
            print("Error: No argument expected")
        else:
            self.debugger.send_command(command_id)

    def do_continue(self, args):
        """ Continue execution """
        self.exec_command(args, JERRY_DEBUGGER_CONTINUE)

    def do_c(self, args):
        """ Continue execution """
        self.exec_command(args, JERRY_DEBUGGER_CONTINUE)

    def do_step(self, args):
        """ Next breakpoint, step into functions """
        self.exec_command(args, JERRY_DEBUGGER_STEP)

    def do_s(self, args):
        """ Next breakpoint, step into functions """
        self.exec_command(args, JERRY_DEBUGGER_STEP)

    def do_next(self, args):
        """ Next breakpoint in the same context """
        self.exec_command(args, JERRY_DEBUGGER_NEXT)

    def do_n(self, args):
        """ Next breakpoint in the same context """
        self.exec_command(args, JERRY_DEBUGGER_NEXT)

    def do_list(self, args):
        """ Lists the available breakpoints """
        if args != "":
            print("Error: No argument expected")
            return

        for breakpoint in self.debugger.active_breakpoint_list.values():
            source = breakpoint.function.source
            print("%d: %s" % (breakpoint.active_index, breakpoint.to_string()))

    def do_delete(self, args):
        """ Delete the given breakpoint """
        if not args:
            print("Error: Breakpoint index expected")
            return

        try:
            breakpoint_index = int(args)
        except:
            print("Error: Integer number expected")
            return

        if breakpoint_index in self.debugger.active_breakpoint_list:
            breakpoint = self.debugger.active_breakpoint_list[breakpoint_index]
            del self.debugger.active_breakpoint_list[breakpoint_index]
            breakpoint.active_index = -1
            self.debugger.send_breakpoint(breakpoint)
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
            except:
                print("Error: Positive integer number expected")
                return

        message = pack(self.debugger.byte_order + "BBIB" + self.debugger.idx_format,
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

    def do_bt(self, args):
        """ Get backtrace data from debugger """
        self.exec_backtrace(args)

    def do_dump(self, args):
        """ Dump all of the debugger data """
        pprint(self.debugger.function_list)

    def eval_string(self, args):
        size = len(args)
        if size == 0:
            return

        # 1: length of type byte
        # 4: length of an uint32 value
        message_header = 1 + 4
        max_fragment = min(self.debugger.max_message_size - message_header, size)

        message = pack(self.debugger.byte_order + "BBIBI",
                       WEBSOCKET_BINARY_FRAME | WEBSOCKET_FIN_BIT,
                       WEBSOCKET_FIN_BIT + max_fragment + message_header,
                       0,
                       JERRY_DEBUGGER_EVAL,
                       size)

        if size == max_fragment:
            self.debugger.send_message(message + args)
            self.stop = True
            return

        self.debugger.send_message(message + args[0:max_fragment])
        offset = max_fragment

        # 1: length of type byte
        message_header = 1

        max_fragment = self.debugger.max_message_size - message_header
        while offset < size:
            next_fragment = min(max_fragment, size - offset)

            message = pack(self.debugger.byte_order + "BBIB",
                           WEBSOCKET_BINARY_FRAME | WEBSOCKET_FIN_BIT,
                           WEBSOCKET_FIN_BIT + next_fragment + message_header,
                           0,
                           JERRY_DEBUGGER_EVAL_PART)

            prev_offset = offset
            offset += next_fragment
            self.debugger.send_message(message + args[prev_offset:offset])

        self.stop = True

    def do_eval(self, args):
        """ Evaluate JavaScript source code """
        self.eval_string(args)

    def do_e(self, args):
        """ Evaluate JavaScript source code """
        self.eval_string(args)


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
        return "Multimap(%s)" % (self.map)


class JerryDebugger(object):

    def __init__(self, address):

        if ":" not in address:
            print("Wrong address settings: Use the 'IP:PORT' format.")
        else:
            self.host, self.port = address.split(":")
            self.port = int(self.port)
            print("Address setup: %s:%s" % (self.host, self.port))

        self.message_data = b""
        self.function_list = {}
        self.next_breakpoint_index = 0
        self.active_breakpoint_list = {}
        self.line_list = Multimap()
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

        expected = pack("BBB",
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

        logging.debug("Compressed pointer size: %d" % (self.cp_size))

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

    def send_breakpoint(self, breakpoint):
        message = pack(self.byte_order + "BBIBB" + self.cp_format + self.idx_format,
                       WEBSOCKET_BINARY_FRAME | WEBSOCKET_FIN_BIT,
                       WEBSOCKET_FIN_BIT + 1 + 1 + self.cp_size + 4,
                       0,
                       JERRY_DEBUGGER_UPDATE_BREAKPOINT,
                       int(breakpoint.active_index >= 0),
                       breakpoint.function.byte_code_cp,
                       breakpoint.offset)
        self.send_message(message)

    def send_command(self, command):
        message = pack(self.byte_order + "BBIB",
                       WEBSOCKET_BINARY_FRAME | WEBSOCKET_FIN_BIT,
                       WEBSOCKET_FIN_BIT + 1,
                       0,
                       command)
        self.send_message(message)

    def get_message(self):
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

            data = self.client_socket.recv(MAX_BUFFER_SIZE)
            if not data:
                self.message_data = None
                return None

            self.message_data += data


def parse_source(debugger, data):
    source_name = ""
    function_name = ""
    stack = [{"lines": [], "offsets": [], "name": ""}]
    new_function_list = {}

    while True:
        if data is None:
            return

        buffer_type = ord(data[2])
        buffer_size = ord(data[1]) - 1

        logging.debug("Parser buffer type: %d, message size: %d" % (buffer_type, buffer_size))

        if buffer_type == JERRY_DEBUGGER_PARSE_ERROR:
            logging.error("Parser error!")
            return

        if buffer_type in [JERRY_DEBUGGER_RESOURCE_NAME, JERRY_DEBUGGER_RESOURCE_NAME_END]:
            source_name += data[3:]

        elif buffer_type in [JERRY_DEBUGGER_FUNCTION_NAME, JERRY_DEBUGGER_FUNCTION_NAME_END]:
            function_name += data[3:]

        elif buffer_type == JERRY_DEBUGGER_PARSE_FUNCTION:
            logging.debug("Source name: %s, function name: %s" % (source_name, function_name))
            stack.append({"name": function_name, "source": source_name, "lines": [], "offsets": []})
            function_name = ""

        elif buffer_type in [JERRY_DEBUGGER_BREAKPOINT_LIST, JERRY_DEBUGGER_BREAKPOINT_OFFSET_LIST]:
            name = "lines"
            if buffer_type == JERRY_DEBUGGER_BREAKPOINT_OFFSET_LIST:
                name = "offsets"

            logging.debug("Breakpoint %s received" % (name))

            buffer_pos = 3
            while buffer_size > 0:
                line = unpack(debugger.byte_order + debugger.idx_format,
                              data[buffer_pos: buffer_pos + 4])
                stack[-1][name].append(line[0])
                buffer_pos += 4
                buffer_size -= 4

        elif buffer_type == JERRY_DEBUGGER_BYTE_CODE_CP:
            byte_code_cp = unpack(debugger.byte_order + debugger.cp_format,
                                  data[3: 3 + debugger.cp_size])[0]

            logging.debug("Byte code cptr received: {0x%x}" % (byte_code_cp))

            func_desc = stack.pop()

            # We know the last item in the list is the general byte code.
            if len(stack) == 0:
                func_desc["source"] = source_name

            function = JerryFunction(byte_code_cp,
                                     func_desc["source"],
                                     func_desc["name"],
                                     func_desc["lines"],
                                     func_desc["offsets"])

            new_function_list[byte_code_cp] = function

            if len(stack) == 0:
                logging.debug("Parse completed.")
                break

        else:
            logging.error("Parser error!")
            return

        data = debugger.get_message()

    # Copy the ready list to the global storage.
    debugger.function_list.update(new_function_list)

    for function in new_function_list.values():
        for line, breakpoint in function.lines.items():
            debugger.line_list.insert(line, breakpoint)


def release_function(debugger, data):
    byte_code_cp = unpack(debugger.byte_order + debugger.cp_format,
                          data[3: 3 + debugger.cp_size])[0]

    function = debugger.function_list[byte_code_cp]

    for line, breakpoint in function.lines.items():
        debugger.line_list.delete(line, breakpoint)
        if breakpoint.active_index >= 0:
            del debugger.active_breakpoint_list[breakpoint.active_index]

    del debugger.function_list[byte_code_cp]

    message = pack(debugger.byte_order + "BBIB" + debugger.cp_format,
                   WEBSOCKET_BINARY_FRAME | WEBSOCKET_FIN_BIT,
                   WEBSOCKET_FIN_BIT + 1 + debugger.cp_size,
                   0,
                   JERRY_DEBUGGER_FREE_BYTE_CODE_CP,
                   byte_code_cp)

    debugger.send_message(message)

    logging.debug("Function {0x%x} byte-code released" % byte_code_cp)


def enable_breakpoint(debugger, breakpoint):
    if breakpoint.active_index < 0:
        debugger.next_breakpoint_index += 1

        debugger.active_breakpoint_list[debugger.next_breakpoint_index] = breakpoint
        breakpoint.active_index = debugger.next_breakpoint_index
        debugger.send_breakpoint(breakpoint)

    print ("Breakpoint %d at %s"
           % (breakpoint.active_index, breakpoint.to_string()))


def set_breakpoint(debugger, string):
    line = re.match("(.*):(\\d+)$", string)
    found = False

    if line:
        source = line.group(1)
        line = int(line.group(2))

        for breakpoint in debugger.line_list.get(line):
            func_source = breakpoint.function.source
            if (source == func_source or
                    func_source.endswith("/" + source) or
                    func_source.endswith("\\" + source)):

                enable_breakpoint(debugger, breakpoint)
                found = True

    else:
        for function in debugger.function_list.values():
            if function.name == string:
                if function.first_line >= 0:
                    enable_breakpoint(debugger, function.lines[function.first_line])
                else:
                    print("Function %s has no breakpoints." % (string))
                found = True

    if not found:
        print("Breakpoint not found")
        return


def main():
    args = arguments_parse()

    debugger = JerryDebugger(args.address)

    logging.debug("Connected to JerryScript on %d port" % (debugger.port))

    prompt = DebuggerPrompt(debugger)
    prompt.prompt = "(jerry-debugger) "

    while True:

        data = debugger.get_message()

        if not data:  # Break the while loop if there is no more data.
            break

        buffer_type = ord(data[2])
        buffer_size = ord(data[1]) - 1

        logging.debug("Main buffer type: %d, message size: %d" % (buffer_type, buffer_size))

        if buffer_type in [JERRY_DEBUGGER_PARSE_ERROR,
                           JERRY_DEBUGGER_BYTE_CODE_CP,
                           JERRY_DEBUGGER_PARSE_FUNCTION,
                           JERRY_DEBUGGER_BREAKPOINT_LIST,
                           JERRY_DEBUGGER_RESOURCE_NAME,
                           JERRY_DEBUGGER_RESOURCE_NAME_END,
                           JERRY_DEBUGGER_FUNCTION_NAME,
                           JERRY_DEBUGGER_FUNCTION_NAME_END]:
            parse_source(debugger, data)

        elif buffer_type == JERRY_DEBUGGER_RELEASE_BYTE_CODE_CP:
            release_function(debugger, data)

        elif buffer_type == JERRY_DEBUGGER_BREAKPOINT_HIT:
            breakpoint_data = unpack(debugger.byte_order + debugger.cp_format + debugger.idx_format, data[3:])

            function = debugger.function_list[breakpoint_data[0]]
            breakpoint = function.offsets[breakpoint_data[1]]

            breakpoint_index = ""
            if breakpoint.active_index >= 0:
                breakpoint_index = " breakpoint:%d" % (breakpoint.active_index)

            print("Stopped at%s %s" % (breakpoint_index, breakpoint.to_string()))

            prompt.cmdloop()

        elif buffer_type in [JERRY_DEBUGGER_BACKTRACE, JERRY_DEBUGGER_BACKTRACE_END]:
            frame_index = 0

            while True:

                buffer_pos = 3
                while buffer_size > 0:
                    breakpoint_data = unpack(debugger.byte_order + debugger.cp_format + debugger.idx_format,
                                             data[buffer_pos: buffer_pos + debugger.cp_size + 4])

                    function = debugger.function_list[breakpoint_data[0]]
                    best_offset = -1

                    for offset in function.offsets:
                        if offset <= breakpoint_data[1] and offset > best_offset:
                            best_offset = offset

                    if best_offset >= 0:
                        breakpoint = function.offsets[best_offset]
                        print("Frame %d: %s" % (frame_index, breakpoint.to_string()))
                    elif function.name:
                        print("Frame %d: %s()" % (frame_index, function.name))
                    else:
                        print("Frame %d: <unknown>()" % (frame_index))

                    frame_index += 1
                    buffer_pos += 6
                    buffer_size -= 6

                if buffer_type == JERRY_DEBUGGER_BACKTRACE_END:
                    break

                data = debugger.get_message()
                buffer_type = ord(data[2])
                buffer_size = ord(data[1]) - 1

                if buffer_type not in [JERRY_DEBUGGER_BACKTRACE,
                                       JERRY_DEBUGGER_BACKTRACE_END]:
                    raise Exception("Backtrace data expected")

            prompt.cmdloop()

        elif buffer_type in [JERRY_DEBUGGER_EVAL_RESULT,
                             JERRY_DEBUGGER_EVAL_RESULT_END,
                             JERRY_DEBUGGER_EVAL_ERROR,
                             JERRY_DEBUGGER_EVAL_ERROR_END]:

            message = b""
            eval_type = buffer_type
            while True:
                message += data[3:]

                if buffer_type in [JERRY_DEBUGGER_EVAL_RESULT_END,
                                   JERRY_DEBUGGER_EVAL_ERROR_END]:
                    break

                data = debugger.get_message()
                buffer_type = ord(data[2])
                buffer_size = ord(data[1]) - 1

                if buffer_type not in [eval_type,
                                       eval_type + 1]:
                    raise Exception("Eval result expected")

            if buffer_type == JERRY_DEBUGGER_EVAL_ERROR_END:
                print("Uncaught exception: %s" % (message))
            else:
                print(message)

            prompt.cmdloop()

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
        elif errno == 32:
            sys.exit("Connection closed.")
        else:
            sys.exit("Failed to connect to the JerryScript debugger.\nError: %s" % (msg))
