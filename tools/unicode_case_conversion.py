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

import argparse
import csv
import itertools
import os
import sys
import warnings

try:
    unichr
except NameError:
    unichr = chr

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.normpath(os.path.join(TOOLS_DIR, '..'))
C_SOURCE_FILE = os.path.join(PROJECT_DIR, 'jerry-core/lit/lit-unicode-conversions.inc.h')


def parse_unicode_sequence(raw_data):
    """
    Parse unicode sequence from raw data.

    :param raw_data: Contains the unicode sequence which needs to parse.
    :return: The parsed unicode sequence.
    """

    result = ''

    for unicode_char in raw_data.split(' '):
        if unicode_char == '':
            continue

        # Convert it to unicode code point (from hex value without 0x prefix)
        result += unichr(int(unicode_char, 16))
    return result


def read_case_mappings(unicode_data_file, special_casing_file):
    """
    Read the corresponding unicode values of lower and upper case letters and store these in tables.

    :param unicode_data_file: Contains the default case mappings (one-to-one mappings).
    :param special_casing_file: Contains additional informative case mappings that are either not one-to-one
                                or which are context-sensitive.
    :return: Upper and lower case mappings.
    """

    lower_case_mapping = CaseMapping()
    upper_case_mapping = CaseMapping()

    # Add one-to-one mappings
    with open(unicode_data_file) as unicode_data:
        unicode_data_reader = csv.reader(unicode_data, delimiter=';')

        for line in unicode_data_reader:
            letter_id = int(line[0], 16)

            # Skip supplementary planes and ascii chars
            if letter_id >= 0x10000 or letter_id < 128:
                continue

            capital_letter = line[12]
            small_letter = line[13]

            if capital_letter:
                upper_case_mapping.add(letter_id, parse_unicode_sequence(capital_letter))

            if small_letter:
                lower_case_mapping.add(letter_id, parse_unicode_sequence(small_letter))

    # Update the conversion tables with the special cases
    with open(special_casing_file) as special_casing:
        special_casing_reader = csv.reader(special_casing, delimiter=';')

        for line in special_casing_reader:
            # Skip comment sections and empty lines
            if not line or line[0].startswith('#'):
                continue

            # Replace '#' character with empty string
            for idx, i in enumerate(line):
                if i.find('#') >= 0:
                    line[idx] = ''

            letter_id = int(line[0], 16)
            condition_list = line[4]

            # Skip supplementary planes, ascii chars, and condition_list
            if letter_id >= 0x10000 or letter_id < 128 or condition_list:
                continue

            small_letter = parse_unicode_sequence(line[1])
            capital_letter = parse_unicode_sequence(line[3])

            lower_case_mapping.add(letter_id, small_letter)
            upper_case_mapping.add(letter_id, capital_letter)

    return lower_case_mapping, upper_case_mapping


class CaseMapping(dict):
    """Class defines an informative, default mapping."""

    def __init__(self):
        """Initialize the case mapping table."""
        self._conversion_table = {}

    def add(self, letter_id, mapped_value):
        """
        Add mapped value of the unicode letter.

        :param letter_id: An integer, representing the unicode code point of the character.
        :param mapped_value: Corresponding character of the case type.
        """
        self._conversion_table[letter_id] = mapped_value

    def remove(self, letter_id):
        """
        Remove mapping from the conversion table.

        :param letter_id: An integer, representing the unicode code point of the character.
        """
        del self._conversion_table[letter_id]

    def get_value(self, letter_id):
        """
        Get the mapped value of the given unicode character.

        :param letter_id: An integer, representing the unicode code point of the character.
        :return: The mapped value of the character.
        """

        if self.contains(letter_id):
            return self._conversion_table[letter_id]

        return None

    def get_conversion_distance(self, letter_id):
        """
        Calculate the distance between the unicode character and its mapped value
        (only needs and works with one-to-one mappings).

        :param letter_id: An integer, representing the unicode code point of the character.
        :return: The conversion distance.
        """

        mapped_value = self.get_value(letter_id)

        if mapped_value and len(mapped_value) == 1:
            return ord(mapped_value) - letter_id

        return None

    def is_bidirectional_conversion(self, letter_id, other_case_mapping):
        """
        Check that two unicode value are also a mapping value of each other.

        :param letter_id: An integer, representing the unicode code point of the character.
        :param other_case_mapping: Comparable case mapping table which possible contains
                                   the return direction of the conversion.
        :return: True, if it's a reverible conversion, false otherwise.
        """

        if not self.contains(letter_id):
            return False

        # Check one-to-one mapping
        mapped_value = self.get_value(letter_id)
        if len(mapped_value) > 1:
            return False

        # Check two way conversions
        mapped_value_id = ord(mapped_value)
        if other_case_mapping.get_value(mapped_value_id) != unichr(letter_id):
            return False

        return True

    def contains(self, letter_id):
        """
        Check that a unicode character is in the conversion table.

        :param letter_id: An integer, representing the unicode code point of the character.
        :return: True, if it contains the character, false otherwise.
        """
        if letter_id in self._conversion_table:
            return True

        return False

    def get_table(self):
        return self._conversion_table

    def extract_ranges(self, other_case_mapping=None):
        """
        Extract ranges from case mappings
        (the second param is optional, if it's not empty, a range will contains bidirectional conversions only).

        :param letter_id: An integer, representing the unicode code point of the character.
        :param other_case_mapping: Comparable case mapping table which contains the return direction of the conversion.
        :return: A table with the start points and their mapped value, and another table with the lengths of the ranges.
        """

        in_range = False
        range_position = -1
        ranges = []
        range_lengths = []

        for letter_id in sorted(self._conversion_table.keys()):
            prev_letter_id = letter_id - 1

            # One-way conversions
            if other_case_mapping is None:
                if len(self.get_value(letter_id)) > 1:
                    in_range = False
                    continue

                if not self.contains(prev_letter_id) or len(self.get_value(prev_letter_id)) > 1:
                    in_range = False
                    continue

            # Two way conversions
            else:
                if not self.is_bidirectional_conversion(letter_id, other_case_mapping):
                    in_range = False
                    continue

                if not self.is_bidirectional_conversion(prev_letter_id, other_case_mapping):
                    in_range = False
                    continue

            conv_distance = self.get_conversion_distance(letter_id)
            prev_conv_distance = self.get_conversion_distance(prev_letter_id)

            if (conv_distance != prev_conv_distance):
                in_range = False
                continue

            if in_range:
                range_lengths[range_position] += 1
            else:
                in_range = True
                range_position += 1

                # Add the start point of the range and its mapped value
                ranges.extend([prev_letter_id, ord(self.get_value(prev_letter_id))])
                range_lengths.append(2)

        # Remove all ranges from the case mapping table.
        index = 0
        while index != len(ranges):
            range_length = range_lengths[index // 2]

            for incr in range(range_length):
                self.remove(ranges[index] + incr)
                if other_case_mapping is not None:
                    other_case_mapping.remove(ranges[index + 1] + incr)

            index += 2

        return ranges, range_lengths

    def extract_character_pair_ranges(self, other_case_mapping):
        """
        Extract two or more character pairs from the case mapping tables.

        :param other_case_mapping: Comparable case mapping table which contains the return direction of the conversion.
        :return: A table with the start points, and another table with the lengths of the ranges.
        """

        start_points = []
        lengths = []
        in_range = False
        element_counter = -1

        for letter_id in sorted(self._conversion_table.keys()):
            # Only extract character pairs
            if not self.is_bidirectional_conversion(letter_id, other_case_mapping):
                in_range = False
                continue

            if self.get_value(letter_id) == unichr(letter_id + 1):
                prev_letter_id = letter_id - 2

                if not self.is_bidirectional_conversion(prev_letter_id, other_case_mapping):
                    in_range = False

                if in_range:
                    lengths[element_counter] += 2
                else:
                    element_counter += 1
                    start_points.append(letter_id)
                    lengths.append(2)
                    in_range = True

            else:
                in_range = False

        # Remove all founded case mapping from the conversion tables after the scanning method
        idx = 0
        while idx != len(start_points):
            letter_id = start_points[idx]
            conv_length = lengths[idx]

            for incr in range(0, conv_length, 2):
                self.remove(letter_id + incr)
                other_case_mapping.remove(letter_id + 1 + incr)

            idx += 1

        return start_points, lengths

    def extract_character_pairs(self, other_case_mapping):
        """
        Extract character pairs. Check that two unicode value are also a mapping value of each other.

        :param other_case_mapping: Comparable case mapping table which contains the return direction of the conversion.
        :return: A table with character pairs.
        """

        character_pairs = []

        for letter_id in sorted(self._conversion_table.keys()):
            if self.is_bidirectional_conversion(letter_id, other_case_mapping):
                mapped_value = self.get_value(letter_id)
                character_pairs.extend([letter_id, ord(mapped_value)])

                # Remove character pairs from case mapping tables
                self.remove(letter_id)
                other_case_mapping.remove(ord(mapped_value))

        return character_pairs

    def extract_special_ranges(self):
        """
        Extract special ranges. It contains that ranges of one-to-two mappings where the second character
        of the mapped values are equals and the other characters are following each other.
        eg.: \u1f80 and \u1f81 will be in one range becase their upper-case values are \u1f08\u0399 and \u1f09\u0399

        :return: A table with the start points and their mapped values, and a table with the lengths of the ranges.
        """

        special_ranges = []
        special_range_lengths = []

        range_position = -1

        for letter_id in sorted(self._conversion_table.keys()):
            mapped_value = self.get_value(letter_id)

            if len(mapped_value) != 2:
                continue

            prev_letter_id = letter_id - 1

            if not self.contains(prev_letter_id):
                in_range = False
                continue

            prev_mapped_value = self.get_value(prev_letter_id)

            if len(prev_mapped_value) != 2:
                continue

            if prev_mapped_value[1] != mapped_value[1]:
                continue

            if (ord(prev_mapped_value[0]) - prev_letter_id) != (ord(mapped_value[0]) - letter_id):
                in_range = False
                continue

            if in_range:
                special_range_lengths[range_position] += 1
            else:
                range_position += 1
                in_range = True

                special_ranges.extend([prev_letter_id, ord(prev_mapped_value[0]), ord(prev_mapped_value[1])])
                special_range_lengths.append(1)

        # Remove special ranges from the conversion table
        idx = 0
        while idx != len(special_ranges):
            range_length = special_range_lengths[idx // 3]
            letter_id = special_ranges[idx]

            for incr in range(range_length):
                self.remove(special_ranges[idx] + incr)

            idx += 3

        return special_ranges, special_range_lengths

    def extract_conversions(self):
        """
        Extract conversions. It provide the full (or remained) case mappings from the table.
        The counter table contains the information of how much one-to-one, one-to-two or one-to-three mappings
        exists successively in the conversion table.

        :return: A table with conversions, and a table with counters.
        """

        unicodes = [[], [], []]
        unicode_lengths = [0, 0, 0]

        # 1 to 1 byte
        for letter_id in sorted(self._conversion_table.keys()):
            mapped_value = self.get_value(letter_id)

            if len(mapped_value) != 1:
                continue

            unicodes[0].extend([letter_id, ord(mapped_value)])
            self.remove(letter_id)

        # 1 to 2 bytes
        for letter_id in sorted(self._conversion_table.keys()):
            mapped_value = self.get_value(letter_id)

            if len(mapped_value) != 2:
                continue

            unicodes[1].extend([letter_id, ord(mapped_value[0]), ord(mapped_value[1])])
            self.remove(letter_id)

        # 1 to 3 bytes
        for letter_id in sorted(self._conversion_table.keys()):
            mapped_value = self.get_value(letter_id)

            if len(mapped_value) != 3:
                continue

            unicodes[2].extend([letter_id, ord(mapped_value[0]), ord(mapped_value[1]), ord(mapped_value[2])])
            self.remove(letter_id)

        unicode_lengths = [int(len(unicodes[0]) / 2), int(len(unicodes[1]) / 3), int(len(unicodes[2]) / 4)]

        return list(itertools.chain.from_iterable(unicodes)), unicode_lengths


def regroup(l, n):
    return [l[i:i+n] for i in range(0, len(l), n)]


def hex_format(ch):
    if isinstance(ch, str):
        ch = ord(ch)

    return "0x{:04x}".format(ch)


def format_code(code, indent):
    lines = []
    # convert all characters to hex format
    converted_code = map(hex_format, code)
    # 10 hex number per line
    for line in regroup(", ".join(converted_code), 10 * 8):
        lines.append(('  ' * indent) + line.strip())
    return "\n".join(lines)


def create_c_format_table(type_name, array_name, table, description=""):
    return """{DESC}
static const {TYPE} jerry_{NAME}[] JERRY_CONST_DATA =
{{
{TABLE}
}};

""".format(DESC=description, TYPE=type_name, NAME=array_name, TABLE=format_code(table, 1))


def copy_tables_to_c_source(gen_tables, c_source):
    data = []

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
 * This file is automatically generated by the {SCRIPT} script. Do not edit!
 */

""".format(SCRIPT=os.path.basename(__file__))

    data.append(header)

    character_case_ranges = gen_tables.get_character_case_ranges()
    character_pair_ranges = gen_tables.get_character_pair_ranges()
    character_pairs = gen_tables.get_character_pairs()
    upper_case_special_ranges = gen_tables.get_upper_case_special_ranges()
    lower_case_ranges = gen_tables.get_lower_case_ranges()
    lower_case_conversions = gen_tables.get_lower_case_conversions()
    upper_case_conversions = gen_tables.get_upper_case_conversions()

    description = "/* Contains start points of character case ranges (these are bidirectional conversions). */"
    data.append(create_c_format_table('uint16_t', 'character_case_ranges',
                                      character_case_ranges[0],
                                      description))

    description = "/* Interval lengths of start points in `character_case_ranges` table. */"
    data.append(create_c_format_table('uint8_t',
                                      'character_case_range_lengths',
                                      character_case_ranges[1],
                                      description))

    description = "/* Contains the start points of bidirectional conversion ranges. */"
    data.append(create_c_format_table('uint16_t',
                                      'character_pair_ranges',
                                      character_pair_ranges[0],
                                      description))

    description = "/* Interval lengths of start points in `character_pair_ranges` table. */"
    data.append(create_c_format_table('uint8_t',
                                      'character_pair_range_lengths',
                                      character_pair_ranges[1],
                                      description))

    description = "/* Contains lower/upper case bidirectional conversion pairs. */"
    data.append(create_c_format_table('uint16_t',
                                      'character_pairs',
                                      character_pairs,
                                      description))

    description = """/* Contains start points of one-to-two uppercase ranges where the second character
 * is always the same.
 */"""
    data.append(create_c_format_table('uint16_t',
                                      'upper_case_special_ranges',
                                      upper_case_special_ranges[0],
                                      description))

    description = "/* Interval lengths for start points in `upper_case_special_ranges` table. */"
    data.append(create_c_format_table('uint8_t',
                                      'upper_case_special_range_lengths',
                                      upper_case_special_ranges[1],
                                      description))

    description = "/* Contains start points of lowercase ranges. */"
    data.append(create_c_format_table('uint16_t', 'lower_case_ranges', lower_case_ranges[0], description))

    description = "/* Interval lengths for start points in `lower_case_ranges` table. */"
    data.append(create_c_format_table('uint8_t', 'lower_case_range_lengths', lower_case_ranges[1], description))

    description = "/* The remaining lowercase conversions. The lowercase variant can be one-to-three character long. */"
    data.append(create_c_format_table('uint16_t',
                                      'lower_case_conversions',
                                      lower_case_conversions[0],
                                      description))

    description = "/* Number of one-to-one, one-to-two, and one-to-three lowercase conversions. */"

    data.append(create_c_format_table('uint8_t',
                                      'lower_case_conversion_counters',
                                      lower_case_conversions[1],
                                      description))

    description = "/* The remaining uppercase conversions. The uppercase variant can be one-to-three character long. */"
    data.append(create_c_format_table('uint16_t',
                                      'upper_case_conversions',
                                      upper_case_conversions[0],
                                      description))

    description = "/* Number of one-to-one, one-to-two, and one-to-three lowercase conversions. */"
    data.append(create_c_format_table('uint8_t',
                                      'upper_case_conversion_counters',
                                      upper_case_conversions[1],
                                      description))

    with open(c_source, 'w') as genereted_source:
        genereted_source.write(''.join(data))


class GenTables(object):
    """Class defines an informative, default generated tables."""

    def __init__(self, lower_case_table, upper_case_table):
        """
        Generate the extracted tables from the given case mapping tables.

        :param lower_case_table: Lower-case mappings.
        :param upper_case_table: Upper-case mappings.
        """

        self._character_case_ranges = lower_case_table.extract_ranges(upper_case_table)
        self._character_pair_ranges = lower_case_table.extract_character_pair_ranges(upper_case_table)
        self._character_pairs = lower_case_table.extract_character_pairs(upper_case_table)
        self._upper_case_special_ranges = upper_case_table.extract_special_ranges()
        self._lower_case_ranges = lower_case_table.extract_ranges()
        self._lower_case_conversions = lower_case_table.extract_conversions()
        self._upper_case_conversions = upper_case_table.extract_conversions()

        if lower_case_table.get_table():
            warnings.warn('Not all elements extracted from the lowercase conversion table!')
        if upper_case_table.get_table():
            warnings.warn('Not all elements extracted from the uppercase conversion table!')

    def get_character_case_ranges(self):
        return self._character_case_ranges

    def get_character_pair_ranges(self):
        return self._character_pair_ranges

    def get_character_pairs(self):
        return self._character_pairs

    def get_upper_case_special_ranges(self):
        return self._upper_case_special_ranges

    def get_lower_case_ranges(self):
        return self._lower_case_ranges

    def get_lower_case_conversions(self):
        return self._lower_case_conversions

    def get_upper_case_conversions(self):
        return self._upper_case_conversions


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument('--unicode-data',
                        metavar='FILE',
                        action='store',
                        required=True,
                        help='specify the unicode data file')

    parser.add_argument('--special-casing',
                        metavar='FILE',
                        action='store',
                        required=True,
                        help='specify the special casing file')

    parser.add_argument('--c-source',
                        metavar='FILE',
                        action='store',
                        default=C_SOURCE_FILE,
                        help='specify the output c source (default: %(default)s)')

    script_args = parser.parse_args()

    if not os.path.isfile(script_args.unicode_data) or not os.access(script_args.unicode_data, os.R_OK):
        print('The %s file is missing or not readable!' % script_args.unicode_data)
        sys.exit(1)

    if not os.path.isfile(script_args.special_casing) or not os.access(script_args.special_casing, os.R_OK):
        print('The %s file is missing or not readable!' % script_args.special_casing)
        sys.exit(1)

    lower_case_table, upper_case_table = read_case_mappings(script_args.unicode_data, script_args.special_casing)

    gen_tables = GenTables(lower_case_table, upper_case_table)

    copy_tables_to_c_source(gen_tables, script_args.c_source)

if __name__ == "__main__":
    main()
