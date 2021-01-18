from __future__ import print_function
from pprint import pformat
import sys
import codecs

def jerry_ord(single_char):
    if isinstance(single_char, str):
        return ord(single_char)
    return single_char


def jerry_encode(string):
    return string.encode('utf-8')


def jerry_decode(byte_list):
    return byte_list.decode('utf-8')


def write(utf8_bytes):
    print(codecs.decode(utf8_bytes, 'utf-8', 'ignore'), end='')
    sys.stdout.flush()


def writeln(utf8_bytes):
    write(utf8_bytes + b'\n')


def jerry_pprint(function_list):
    writeln(jerry_encode(pformat(function_list)))
