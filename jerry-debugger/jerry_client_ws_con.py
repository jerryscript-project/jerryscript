import logging
import select
import socket
import struct

# Messages sent by the server to client.
JERRY_DEBUGGER_CONFIGURATION = 1

# Expected debugger protocol version.
JERRY_DEBUGGER_VERSION = 3

MAX_BUFFER_SIZE = 128
WEBSOCKET_BINARY_FRAME = 2
WEBSOCKET_FIN_BIT = 0x80

class Connect(object):
    def __init__(self, address):

        if ":" not in address:
            self.host = address
            self.port = 5001  # use default port
        else:
            self.host, self.port = address.split(":")
            self.port = int(self.port)

        print("Connecting to: %s:%s" % (self.host, self.port))

        self.message_data = b""
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
