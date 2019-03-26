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

import struct

MAX_BUFFER_SIZE = 256

class RawPacket(object):
    """ Simplified transmission layer. """
    def __init__(self, protocol):
        self.protocol = protocol
        self.data_buffer = b""

    def connect(self, config_size):
        """  Create connection. """
        self.protocol.connect()
        self.data_buffer = b""

        # It will return with the Network configurations, which has the following struct:
        # header [1] - size[1]
        # configuration [config_size]
        len_expected = config_size + 1

        while len(self.data_buffer) < len_expected:
            self.data_buffer += self.protocol.receive_data()

        expected = struct.pack("B", config_size)

        if self.data_buffer[0:1] != expected:
            raise Exception("Unexpected configuration")

        result = self.data_buffer[1:len_expected]
        self.data_buffer = self.data_buffer[len_expected:]

        return result

    def close(self):
        """ Close connection. """
        self.protocol.close()

    def send_message(self, _, data):
        """ Send message. """
        msg_size = len(data)

        while msg_size > 0:
            bytes_send = self.protocol.send_data(data)
            if bytes_send < msg_size:
                data = data[bytes_send:]
            msg_size -= bytes_send

    def get_message(self, blocking):
        """ Receive message. """

        # Connection was closed
        if self.data_buffer is None:
            return None

        while True:
            if len(self.data_buffer) >= 1:
                size = ord(self.data_buffer[0])
                if size == 0:
                    raise Exception("Unexpected data frame")

                if len(self.data_buffer) >= size + 1:
                    result = self.data_buffer[1:size + 1]
                    self.data_buffer = self.data_buffer[size + 1:]
                    return result

            if not blocking and not self.protocol.ready():
                return b''

            received_data = self.protocol.receive_data(MAX_BUFFER_SIZE)

            if not received_data:
                return None

            self.data_buffer += received_data
