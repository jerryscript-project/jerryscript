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

import select
import serial

class Serial(object):
    """ Create a new socket using the given address family, socket type and protocol number. """
    def __init__(self, serial_config):
        config = serial_config.split(',')
        config_size = len(config)

        port = config[0] if config_size > 0 else "/dev/ttyUSB0"
        baudrate = config[1] if config_size > 1 else 115200
        bytesize = int(config[2]) if config_size > 2 else 8
        parity = config[3] if config_size > 3 else 'N'
        stopbits = int(config[4]) if config_size > 4 else 1

        self.ser = serial.Serial(port=port, baudrate=baudrate, parity=parity,
                                 stopbits=stopbits, bytesize=bytesize, timeout=1)

    def connect(self):
        """ Connect to the server, write a 'c' to the serial port """
        self.send_data('c')

    def close(self):
        """"  close the serial port. """
        self.ser.close()

    def receive_data(self, max_size=1024):
        """ The maximum amount of data to be received at once is specified by max_size. """
        return self.ser.read(max_size)

    def send_data(self, data):
        """ Write data to the serial port. """
        return self.ser.write(data)

    def ready(self):
        """ Monitor the file descriptor. """
        result = select.select([self.ser.fileno()], [], [], 0)[0]

        return self.ser.fileno() in result
