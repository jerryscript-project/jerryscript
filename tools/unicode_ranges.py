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

#
# http://www.unicode.org/Public/3.0-Update/UnicodeData-3.0.0.txt
#

# unicode categories:      Lu Ll Lt Mn Mc Me Nd Nl No Zs Zl Zp Cc Cf Cs Co Lm Lo Pc Pd Ps Pe Pi Pf Po Sm Sc Sk So
# letter:                  Lu Ll Lt Lm Lo Nl
# non-letter-indent-part:
#   digit:                 Nd
#   punctuation mark:      Mn Mc
#   connector punctuation: Pc
# separators:              Zs

import argparse
import bisect
import csv
import itertools
import os

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.normpath(os.path.join(TOOLS_DIR, '..'))
C_SOURCE_FILE = os.path.join(PROJECT_DIR, 'jerry-core/lit/lit-unicode-ranges.inc.h')

parser = argparse.ArgumentParser()

parser.add_argument('unicode_data',
                    metavar='FILE',
                    action='store',
                    help='specify the unicode data file')

parser.add_argument('--c-source',
                    metavar='FILE',
                    action='store',
                    default=C_SOURCE_FILE,
                    help='specify the output c source (default: %(default)s)')

script_args = parser.parse_args()


def main():
    if not os.path.isfile(script_args.unicode_data) or not os.access(script_args.unicode_data, os.R_OK):
        print('The %s file is missing or not readable!' % script_args.unicode_data)
        sys.exit(1)

    letters, non_letters, separators = read_categories()

    letters_list = list(ranges(letters))
    letter_interval_sps, letter_interval_lengths, letter_chars = split_list(letters_list)

    non_letters_list = list(ranges(non_letters))
    non_letter_interval_sps, non_letter_interval_lengths, non_letter_chars = split_list(non_letters_list)

    separator_list = list(ranges(separators))
    separator_interval_sps, separator_interval_lengths, separator_chars = split_list(separator_list)

    source = GenSource()

    letter_interval_sps_desc = """/**
 * Character interval starting points for the unicode letters.
 *
 * The characters covered by these intervals are from
 * the following Unicode categories: Lu, Ll, Lt, Lm, Lo, Nl
 */"""
    source.add_table("uint16_t",
                     "unicode_letter_interval_sps",
                     letter_interval_sps,
                     letter_interval_sps_desc)

    letter_interval_lengths_desc = """/**
 * Character lengths for the unicode letters.
 *
 * The characters covered by these intervals are from
 * the following Unicode categories: Lu, Ll, Lt, Lm, Lo, Nl
 */"""
    source.add_table("uint8_t",
                     "unicode_letter_interval_lengths",
                     letter_interval_lengths,
                     letter_interval_lengths_desc)

    letter_chars_desc = """/**
 * Those unicode letter characters that are not inside any of
 * the intervals specified in jerry_unicode_letter_interval_sps array.
 *
 * The characters are from the following Unicode categories:
 * Lu, Ll, Lt, Lm, Lo, Nl
 */"""
    source.add_table("uint16_t",
                     "unicode_letter_chars",
                     letter_chars,
                     letter_chars_desc)

    non_letter_interval_sps_desc = """/**
 * Character interval starting points for non-letter character
 * that can be used as a non-first character of an identifier.
 *
 * The characters covered by these intervals are from
 * the following Unicode categories: Nd, Mn, Mc, Pc
 */"""
    source.add_table("uint16_t",
                     "unicode_non_letter_ident_part_interval_sps",
                     non_letter_interval_sps,
                     non_letter_interval_sps_desc)

    non_letter_interval_lengths_desc = """/**
 * Character interval lengths for non-letter character
 * that can be used as a non-first character of an identifier.
 *
 * The characters covered by these intervals are from
 * the following Unicode categories: Nd, Mn, Mc, Pc
 */"""
    source.add_table("uint8_t",
                     "unicode_non_letter_ident_part_interval_lengths",
                     non_letter_interval_lengths,
                     non_letter_interval_lengths_desc)

    non_letter_chars_desc = """/**
 * Those non-letter characters that can be used as a non-first
 * character of an identifier and not included in any of the intervals
 * specified in jerry_unicode_non_letter_ident_part_interval_sps array.
 *
 * The characters are from the following Unicode categories:
 * Nd, Mn, Mc, Pc
 */"""
    source.add_table("uint16_t",
                     "unicode_non_letter_ident_part_chars",
                     non_letter_chars,
                     non_letter_chars_desc)

    separator_interval_sps_desc = """/**
 * Unicode separator character interval starting points from Unicode category: Zs
 */"""
    source.add_table("uint16_t",
                     "unicode_separator_char_interval_sps",
                     separator_interval_sps,
                     separator_interval_sps_desc)

    separator_interval_lengths_desc = """/**
 * Unicode separator character interval lengths from Unicode category: Zs
 */"""
    source.add_table("uint8_t",
                     "unicode_separator_char_interval_lengths",
                     separator_interval_lengths,
                     separator_interval_lengths_desc)

    separator_chars_desc = """/**
 * Unicode separator characters that are not in the
 * jerry_unicode_separator_char_intervals array.
 *
 * Unicode category: Zs
 */"""
    source.add_table("uint16_t",
                     "unicode_separator_chars",
                     separator_chars,
                     separator_chars_desc)

    source.write_source()


def read_categories():
    """
    Read the corresponding unicode values and store them in category lists.

    :return: List of letters, non_letter and separators.
    """

    letter_category = ["Lu", "Ll", "Lt", "Lm", "Lo", "Nl"]
    non_letter_category = ["Nd", "Mn", "Mc", "Pc"]
    separator_category = ["Zs"]

    letters = []
    non_letters = []
    separators = []

    with open(script_args.unicode_data) as unicode_data:
        unicode_data_reader = csv.reader(unicode_data, delimiter=';')

        for line in unicode_data_reader:
            unicode_id = int(line[0], 16)

            # Skip supplementary planes and ascii chars
            if unicode_id >= 0x10000 or unicode_id < 128:
                continue

            category = line[2]

            if category in letter_category:
                letters.append(unicode_id)
            elif category in non_letter_category:
                non_letters.append(unicode_id)
            elif category in separator_category:
                separators.append(unicode_id)

    # This separator char is handled separatly
    non_breaking_space = 0x00A0
    if non_breaking_space in separators:
        separators.remove(int(non_breaking_space))

    # These separator chars are not in UnicodeData-3.0.0.txt or not in Zs category
    mongolian_vowel_separator = 0x180E
    medium_mathematical_space = 0x205F

    if mongolian_vowel_separator not in separators:
        bisect.insort(separators, int(mongolian_vowel_separator))
    if medium_mathematical_space not in separators:
        bisect.insort(separators, int(medium_mathematical_space))

    return letters, non_letters, separators


def ranges(i):
    """
    Convert an increasing list of integers into a range list

    :return: List of ranges.
    """

    for a, b in itertools.groupby(enumerate(i), lambda (x, y): y - x):
        b = list(b)
        yield b[0][1], b[-1][1]


def split_list(category_list):
    """
    Split list of ranges into intervals and single char lists.

    :return: List of interval starting points, interval lengths and single chars
    """

    unicode_category_interval_sps = []
    unicode_category_interval_lengths = []
    unicode_category_chars = []

    for element in category_list:
        interval_length = element[1] - element[0]
        if interval_length == 0:
            unicode_category_chars.append(element[0])

        elif (interval_length > 255):
            for i in range(element[0], element[1], 256):
                length = 255 if (element[1] - i > 255) else (element[1] - i)
                unicode_category_interval_sps.append(i)
                unicode_category_interval_lengths.append(length)
        else:
            unicode_category_interval_sps.append(element[0])
            unicode_category_interval_lengths.append(element[1] - element[0])

    return unicode_category_interval_sps, unicode_category_interval_lengths, unicode_category_chars


class GenSource(object):
    """Class defines a default generated c source."""

    def __init__(self):
        self._data = []

        header = """/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file is automatically generated by the {SCRIPT} script
 * from {UNICODES}. Do not edit!
 */

""".format(SCRIPT=os.path.basename(__file__), UNICODES=os.path.basename(script_args.unicode_data))

        self._data.append(header)

    def _regroup(self, l, n):
        return [l[i:i+n] for i in range(0, len(l), n)]

    def _hex_format(self, ch):
        if isinstance(ch, str):
            ch = ord(ch)

        return "0x{:04x}".format(ch)

    def _format_code(self, code, indent):
        lines = []
        # convert all characters to hex format
        converted_code = map(self._hex_format, code)
        # 10 hex number per line
        for line in self._regroup(", ".join(converted_code), 10 * 8):
            lines.append(('  ' * indent) + line.strip())
        return "\n".join(lines)

    def add_table(self, type_name, array_name, table, description=""):
        table_str = """{DESC}
static const {TYPE} jerry_{NAME}[] JERRY_CONST_DATA =
{{
{TABLE}
}};

""".format(DESC=description, TYPE=type_name, NAME=array_name, TABLE=self._format_code(table, 1))

        self._data.append(table_str)

    def write_source(self):
        with open(script_args.c_source, 'w') as genereted_source:
            genereted_source.write(''.join(self._data))


if __name__ == "__main__":
    main()
