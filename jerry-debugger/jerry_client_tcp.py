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

import socket
import select

# pylint: disable=too-many-arguments,superfluous-parens
class Socket:
    """ Create a new socket using the given address family, socket type and protocol number. """
    def __init__(self, address, socket_family=socket.AF_INET, socket_type=socket.SOCK_STREAM, proto=0, fileno=None):
        self.address = address
        self.socket = socket.socket(socket_family, socket_type, proto, fileno)

    def connect(self):
        """
        Connect to a remote socket at address (host, port).
        The format of address depends on the address family.
        """
        print(f"Connecting to: {self.address[0]}:{self.address[1]}")
        self.socket.connect(self.address)

    def close(self):
        """" Mark the socket closed. """
        self.socket.close()

    def receive_data(self, max_size=1024):
        """ The maximum amount of data to be received at once is specified by max_size. """
        return self.socket.recv(max_size)

    def send_data(self, data):
        """ Send data to the socket. The socket must be connected to a remote socket. """
        return self.socket.send(data)

    def ready(self):
        """ Monitor the file descriptor. """
        result = select.select([self.socket], [], [], 0)[0]

        return self.socket in result
