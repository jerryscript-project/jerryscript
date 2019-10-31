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
import bisect
import csv
import itertools
import os
import warnings

from gen_c_source import LICENSE, format_code
from settings import PROJECT_DIR


RANGES_C_SOURCE = os.path.join(PROJECT_DIR, 'jerry-core/lit/lit-unicode-ranges.inc.h')
CONVERSIONS_C_SOURCE = os.path.join(PROJECT_DIR, 'jerry-core/lit/lit-unicode-conversions.inc.h')


# common code generation


class UniCodeSource(object):
    def __init__(self, filepath):
        self.__filepath = filepath
        self.__header = [LICENSE, ""]
        self.__data = []

    def complete_header(self, completion):
        self.__header.append(completion)
        self.__header.append("")  # for an extra empty line

    def add_table(self, table, table_name, table_type, table_descr):
        self.__data.append(table_descr)
        self.__data.append("static const %s lit_%s[] JERRY_ATTR_CONST_DATA =" % (table_type, table_name))
        self.__data.append("{")
        self.__data.append(format_code(table, 1))
        self.__data.append("};")
        self.__data.append("")  # for an extra empty line

    def generate(self):
        with open(self.__filepath, 'w') as generated_source:
            generated_source.write("\n".join(self.__header))
            generated_source.write("\n".join(self.__data))

class UnicodeCategorizer(object):
    def __init__(self):
        # unicode categories:      Lu Ll Lt Mn Mc Me Nd Nl No Zs Zl Zp Cc Cf Cs
        #                          Co Lm Lo Pc Pd Ps Pe Pi Pf Po Sm Sc Sk So
        # letter:                  Lu Ll Lt Lm Lo Nl
        # non-letter-indent-part:
        #   digit:                 Nd
        #   punctuation mark:      Mn Mc
        #   connector punctuation: Pc
        # separators:              Zs
        self._unicode_categories = {
            'letters_category' : ["Lu", "Ll", "Lt", "Lm", "Lo", "Nl"],
            'non_letters_category' : ["Nd", "Mn", "Mc", "Pc"],
            'separators_category' : ["Zs"]
        }

        self._categories = {
            'letters' : [],
            'non_letters' : [],
            'separators' : []
        }

    def _store_by_category(self, unicode_id, category):
        """
        Store the given unicode_id by its category
        """
        for target_category in self._categories:
            if category in self._unicode_categories[target_category + '_category']:
                self._categories[target_category].append(unicode_id)

    def read_categories(self, unicode_data_file):
        """
        Read the corresponding unicode values and store them in category lists.

        :return: List of letters, non_letter and separators.
        """

        range_start_id = 0

        with open(unicode_data_file) as unicode_data:
            for line in csv.reader(unicode_data, delimiter=';'):
                unicode_id = int(line[0], 16)

                # Skip supplementary planes and ascii chars
                if unicode_id >= 0x10000 or unicode_id < 128:
                    continue

                category = line[2]

                if range_start_id != 0:
                    while range_start_id <= unicode_id:
                        self._store_by_category(range_start_id, category)
                        range_start_id += 1
                    range_start_id = 0
                    continue

                if line[1].startswith('<'):
                    # Save the start position of the range
                    range_start_id = unicode_id

                self._store_by_category(unicode_id, category)

        # This separator char is handled separatly
        separators = self._categories['separators']
        non_breaking_space = 0x00A0
        if non_breaking_space in separators:
            separators.remove(int(non_breaking_space))

        # These separator chars are not in the unicode data file or not in Zs category
        mongolian_vowel_separator = 0x180E
        medium_mathematical_space = 0x205F
        zero_width_space = 0x200B

        if mongolian_vowel_separator not in separators:
            bisect.insort(separators, int(mongolian_vowel_separator))
        if medium_mathematical_space not in separators:
            bisect.insort(separators, int(medium_mathematical_space))
        if zero_width_space not in separators:
            bisect.insort(separators, int(zero_width_space))

        # https://www.ecma-international.org/ecma-262/5.1/#sec-7.1 format-control characters
        non_letters = self._categories['non_letters']
        zero_width_non_joiner = 0x200C
        zero_width_joiner = 0x200D

        bisect.insort(non_letters, int(zero_width_non_joiner))
        bisect.insort(non_letters, int(zero_width_joiner))

        return self._categories['letters'], self._categories['non_letters'], self._categories['separators']


def group_ranges(i):
    """
    Convert an increasing list of integers into a range list

    :return: List of ranges.
    """
    for _, group in itertools.groupby(enumerate(i), lambda q: (q[1] - q[0])):
        group = list(group)
        yield group[0][1], group[-1][1]


def split_list(category_list):
    """
    Split list of ranges into intervals and single char lists.

    :return: List of interval starting points, interval lengths and single chars
    """

    interval_sps = []
    interval_lengths = []
    chars = []

    for element in category_list:
        interval_length = element[1] - element[0]
        if interval_length == 0:
            chars.append(element[0])
        elif interval_length > 255:
            for i in range(element[0], element[1], 256):
                length = 255 if (element[1] - i > 255) else (element[1] - i)
                interval_sps.append(i)
                interval_lengths.append(length)
        else:
            interval_sps.append(element[0])
            interval_lengths.append(element[1] - element[0])

    return interval_sps, interval_lengths, chars


def generate_ranges(script_args):
    categorizer = UnicodeCategorizer()
    letters, non_letters, separators = categorizer.read_categories(script_args.unicode_data)

    letter_tables = split_list(list(group_ranges(letters)))
    non_letter_tables = split_list(list(group_ranges(non_letters)))
    separator_tables = split_list(list(group_ranges(separators)))

    c_source = UniCodeSource(RANGES_C_SOURCE)

    header_completion = ["/* This file is automatically generated by the %s script" % os.path.basename(__file__),
                         " * from %s. Do not edit! */" % os.path.basename(script_args.unicode_data),
                         ""]

    c_source.complete_header("\n".join(header_completion))

    c_source.add_table(letter_tables[0],
                       "unicode_letter_interval_sps",
                       "uint16_t",
                       ("/**\n"
                        " * Character interval starting points for the unicode letters.\n"
                        " *\n"
                        " * The characters covered by these intervals are from\n"
                        " * the following Unicode categories: Lu, Ll, Lt, Lm, Lo, Nl\n"
                        " */"))

    c_source.add_table(letter_tables[1],
                       "unicode_letter_interval_lengths",
                       "uint8_t",
                       ("/**\n"
                        " * Character lengths for the unicode letters.\n"
                        " *\n"
                        " * The characters covered by these intervals are from\n"
                        " * the following Unicode categories: Lu, Ll, Lt, Lm, Lo, Nl\n"
                        " */"))

    c_source.add_table(letter_tables[2],
                       "unicode_letter_chars",
                       "uint16_t",
                       ("/**\n"
                        " * Those unicode letter characters that are not inside any of\n"
                        " * the intervals specified in lit_unicode_letter_interval_sps array.\n"
                        " *\n"
                        " * The characters are from the following Unicode categories:\n"
                        " * Lu, Ll, Lt, Lm, Lo, Nl\n"
                        " */"))

    c_source.add_table(non_letter_tables[0],
                       "unicode_non_letter_ident_part_interval_sps",
                       "uint16_t",
                       ("/**\n"
                        " * Character interval starting points for non-letter character\n"
                        " * that can be used as a non-first character of an identifier.\n"
                        " *\n"
                        " * The characters covered by these intervals are from\n"
                        " * the following Unicode categories: Nd, Mn, Mc, Pc\n"
                        " */"))

    c_source.add_table(non_letter_tables[1],
                       "unicode_non_letter_ident_part_interval_lengths",
                       "uint8_t",
                       ("/**\n"
                        " * Character interval lengths for non-letter character\n"
                        " * that can be used as a non-first character of an identifier.\n"
                        " *\n"
                        " * The characters covered by these intervals are from\n"
                        " * the following Unicode categories: Nd, Mn, Mc, Pc\n"
                        " */"))

    c_source.add_table(non_letter_tables[2],
                       "unicode_non_letter_ident_part_chars",
                       "uint16_t",
                       ("/**\n"
                        " * Those non-letter characters that can be used as a non-first\n"
                        " * character of an identifier and not included in any of the intervals\n"
                        " * specified in lit_unicode_non_letter_ident_part_interval_sps array.\n"
                        " *\n"
                        " * The characters are from the following Unicode categories:\n"
                        " * Nd, Mn, Mc, Pc\n"
                        " */"))

    c_source.add_table(separator_tables[0],
                       "unicode_separator_char_interval_sps",
                       "uint16_t",
                       ("/**\n"
                        " * Unicode separator character interval starting points from Unicode category: Zs\n"
                        " */"))

    c_source.add_table(separator_tables[1],
                       "unicode_separator_char_interval_lengths",
                       "uint8_t",
                       ("/**\n"
                        " * Unicode separator character interval lengths from Unicode category: Zs\n"
                        " */"))

    c_source.add_table(separator_tables[2],
                       "unicode_separator_chars",
                       "uint16_t",
                       ("/**\n"
                        " * Unicode separator characters that are not in the\n"
                        " * lit_unicode_separator_char_intervals array.\n"
                        " *\n"
                        " * Unicode category: Zs\n"
                        " */"))

    c_source.generate()


# functions for unicode conversions


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
        hex_val = int(unicode_char, 16)
        try:
            result += unichr(hex_val)
        except NameError:
            result += chr(hex_val)

    return result


def read_case_mappings(unicode_data_file, special_casing_file):
    """
    Read the corresponding unicode values of lower and upper case letters and store these in tables.

    :param unicode_data_file: Contains the default case mappings (one-to-one mappings).
    :param special_casing_file: Contains additional informative case mappings that are either not one-to-one
                                or which are context-sensitive.
    :return: Upper and lower case mappings.
    """

    lower_case_mapping = {}
    upper_case_mapping = {}

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
                upper_case_mapping[letter_id] = parse_unicode_sequence(capital_letter)

            if small_letter:
                lower_case_mapping[letter_id] = parse_unicode_sequence(small_letter)

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

            lower_case_mapping[letter_id] = small_letter
            upper_case_mapping[letter_id] = capital_letter

    return lower_case_mapping, upper_case_mapping


def extract_ranges(letter_case, reverse_letter_case=None):
    """
    Extract ranges from case mappings
    (the second param is optional, if it's not empty, a range will contains bidirectional conversions only).

    :param letter_id: An integer, representing the unicode code point of the character.
    :param letter_case: case mappings dictionary which contains the conversions.
    :param reverse_letter_case: Comparable case mapping table which contains the return direction of the conversion.
    :return: A table with the start points and their mapped value, and another table with the lengths of the ranges.
    """

    in_range = False
    range_position = -1
    ranges = []
    range_lengths = []

    for letter_id in sorted(letter_case.keys()):
        prev_letter_id = letter_id - 1

        # One-way conversions
        if reverse_letter_case is None:
            if len(letter_case[letter_id]) > 1:
                in_range = False
                continue

            if prev_letter_id not in letter_case or len(letter_case[prev_letter_id]) > 1:
                in_range = False
                continue

        # Two way conversions
        else:
            if not is_bidirectional_conversion(letter_id, letter_case, reverse_letter_case):
                in_range = False
                continue

            if not is_bidirectional_conversion(prev_letter_id, letter_case, reverse_letter_case):
                in_range = False
                continue

        conv_distance = calculate_conversion_distance(letter_case, letter_id)
        prev_conv_distance = calculate_conversion_distance(letter_case, prev_letter_id)

        if conv_distance != prev_conv_distance:
            in_range = False
            continue

        if in_range:
            range_lengths[range_position] += 1
        else:
            in_range = True
            range_position += 1

            # Add the start point of the range and its mapped value
            ranges.extend([prev_letter_id, ord(letter_case[prev_letter_id])])
            range_lengths.append(2)

    # Remove all ranges from the case mapping table.
    for idx in range(0, len(ranges), 2):
        range_length = range_lengths[idx // 2]

        for incr in range(range_length):
            del letter_case[ranges[idx] + incr]
            if reverse_letter_case is not None:
                del reverse_letter_case[ranges[idx + 1] + incr]

    return ranges, range_lengths


def extract_character_pair_ranges(letter_case, reverse_letter_case):
    """
    Extract two or more character pairs from the case mapping tables.

    :param letter_case: case mappings dictionary which contains the conversions.
    :param reverse_letter_case: Comparable case mapping table which contains the return direction of the conversion.
    :return: A table with the start points, and another table with the lengths of the ranges.
    """

    start_points = []
    lengths = []
    in_range = False
    element_counter = -1

    for letter_id in sorted(letter_case.keys()):
        # Only extract character pairs
        if not is_bidirectional_conversion(letter_id, letter_case, reverse_letter_case):
            in_range = False
            continue

        if ord(letter_case[letter_id]) == letter_id + 1:
            prev_letter_id = letter_id - 2

            if not is_bidirectional_conversion(prev_letter_id, letter_case, reverse_letter_case):
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

    # Remove all found case mapping from the conversion tables after the scanning method
    for idx, letter_id in enumerate(start_points):
        conv_length = lengths[idx]

        for incr in range(0, conv_length, 2):
            del letter_case[letter_id + incr]
            del reverse_letter_case[letter_id + 1 + incr]

    return start_points, lengths


def extract_character_pairs(letter_case, reverse_letter_case):
    """
    Extract character pairs. Check that two unicode value are also a mapping value of each other.

    :param letter_case: case mappings dictionary which contains the conversions.
    :param reverse_letter_case: Comparable case mapping table which contains the return direction of the conversion.
    :return: A table with character pairs.
    """

    character_pairs = []

    for letter_id in sorted(letter_case.keys()):
        if is_bidirectional_conversion(letter_id, letter_case, reverse_letter_case):
            mapped_value = letter_case[letter_id]
            character_pairs.extend([letter_id, ord(mapped_value)])

            # Remove character pairs from case mapping tables
            del letter_case[letter_id]
            del reverse_letter_case[ord(mapped_value)]

    return character_pairs


def extract_special_ranges(letter_case):
    """
    Extract special ranges. It contains start points of one-to-two letter case ranges
    where the second character is always the same.

    :param letter_case: case mappings dictionary which contains the conversions.

    :return: A table with the start points and their mapped values, and a table with the lengths of the ranges.
    """

    special_ranges = []
    special_range_lengths = []

    range_position = -1

    for letter_id in sorted(letter_case.keys()):
        mapped_value = letter_case[letter_id]

        if len(mapped_value) != 2:
            continue

        prev_letter_id = letter_id - 1

        if prev_letter_id not in letter_case:
            in_range = False
            continue

        prev_mapped_value = letter_case[prev_letter_id]

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
    for idx in range(0, len(special_ranges), 3):
        range_length = special_range_lengths[idx // 3]
        letter_id = special_ranges[idx]

        for incr in range(range_length):
            del letter_case[special_ranges[idx] + incr]

    return special_ranges, special_range_lengths


def extract_conversions(letter_case):
    """
    Extract conversions. It provide the full (or remained) case mappings from the table.
    The counter table contains the information of how much one-to-one, one-to-two or one-to-three mappings
    exists successively in the conversion table.

    :return: A table with conversions, and a table with counters.
    """

    unicodes = [[], [], []]
    unicode_lengths = [0, 0, 0]

    # 1 to 1 byte
    for letter_id in sorted(letter_case.keys()):
        mapped_value = letter_case[letter_id]

        if len(mapped_value) != 1:
            continue

        unicodes[0].extend([letter_id, ord(mapped_value)])
        del letter_case[letter_id]

    # 1 to 2 bytes
    for letter_id in sorted(letter_case.keys()):
        mapped_value = letter_case[letter_id]

        if len(mapped_value) != 2:
            continue

        unicodes[1].extend([letter_id, ord(mapped_value[0]), ord(mapped_value[1])])
        del letter_case[letter_id]

    # 1 to 3 bytes
    for letter_id in sorted(letter_case.keys()):
        mapped_value = letter_case[letter_id]

        if len(mapped_value) != 3:
            continue

        unicodes[2].extend([letter_id, ord(mapped_value[0]), ord(mapped_value[1]), ord(mapped_value[2])])
        del letter_case[letter_id]

    unicode_lengths = [int(len(unicodes[0]) / 2), int(len(unicodes[1]) / 3), int(len(unicodes[2]) / 4)]

    return list(itertools.chain.from_iterable(unicodes)), unicode_lengths


def is_bidirectional_conversion(letter_id, letter_case, reverse_letter_case):
    """
    Check that two unicode value are also a mapping value of each other.

    :param letter_id: An integer, representing the unicode code point of the character.
    :param other_case_mapping: Comparable case mapping table which possible contains
                               the return direction of the conversion.
    :return: True, if it's a reverible conversion, false otherwise.
    """

    if letter_id not in letter_case:
        return False

    # Check one-to-one mapping
    mapped_value = letter_case[letter_id]
    if len(mapped_value) > 1:
        return False

    # Check two way conversions
    mapped_value_id = ord(mapped_value)

    if mapped_value_id not in reverse_letter_case or len(reverse_letter_case[mapped_value_id]) > 1:
        return False

    if ord(reverse_letter_case[mapped_value_id]) != letter_id:
        return False

    return True


def calculate_conversion_distance(letter_case, letter_id):
    """
    Calculate the distance between the unicode character and its mapped value
    (only needs and works with one-to-one mappings).

    :param letter_case: case mappings dictionary which contains the conversions.
    :param letter_id: An integer, representing the unicode code point of the character.
    :return: The conversion distance.
    """

    if letter_id not in letter_case or len(letter_case[letter_id]) > 1:
        return None

    return ord(letter_case[letter_id]) - letter_id


def generate_conversions(script_args):
    # Read the corresponding unicode values of lower and upper case letters and store these in tables
    case_mappings = read_case_mappings(script_args.unicode_data, script_args.special_casing)
    lower_case = case_mappings[0]
    upper_case = case_mappings[1]

    character_case_ranges = extract_ranges(lower_case, upper_case)
    character_pair_ranges = extract_character_pair_ranges(lower_case, upper_case)
    character_pairs = extract_character_pairs(lower_case, upper_case)
    upper_case_special_ranges = extract_special_ranges(upper_case)
    lower_case_ranges = extract_ranges(lower_case)
    lower_case_conversions = extract_conversions(lower_case)
    upper_case_conversions = extract_conversions(upper_case)

    if lower_case:
        warnings.warn('Not all elements extracted from the lowercase table!')
    if upper_case:
        warnings.warn('Not all elements extracted from the uppercase table!')

    # Generate conversions output
    c_source = UniCodeSource(CONVERSIONS_C_SOURCE)

    unicode_file = os.path.basename(script_args.unicode_data)
    spec_casing_file = os.path.basename(script_args.special_casing)

    header_completion = ["/* This file is automatically generated by the %s script" % os.path.basename(__file__),
                         " * from %s and %s files. Do not edit! */" % (unicode_file, spec_casing_file),
                         ""]

    c_source.complete_header("\n".join(header_completion))

    c_source.add_table(character_case_ranges[0],
                       "character_case_ranges",
                       "uint16_t",
                       ("/* Contains start points of character case ranges "
                        "(these are bidirectional conversions). */"))

    c_source.add_table(character_case_ranges[1],
                       "character_case_range_lengths",
                       "uint8_t",
                       "/* Interval lengths of start points in `character_case_ranges` table. */")

    c_source.add_table(character_pair_ranges[0],
                       "character_pair_ranges",
                       "uint16_t",
                       "/* Contains the start points of bidirectional conversion ranges. */")

    c_source.add_table(character_pair_ranges[1],
                       "character_pair_range_lengths",
                       "uint8_t",
                       "/* Interval lengths of start points in `character_pair_ranges` table. */")

    c_source.add_table(character_pairs,
                       "character_pairs",
                       "uint16_t",
                       "/* Contains lower/upper case bidirectional conversion pairs. */")

    c_source.add_table(upper_case_special_ranges[0],
                       "upper_case_special_ranges",
                       "uint16_t",
                       ("/* Contains start points of one-to-two uppercase ranges where the second character\n"
                        " * is always the same.\n"
                        " */"))

    c_source.add_table(upper_case_special_ranges[1],
                       "upper_case_special_range_lengths",
                       "uint8_t",
                       "/* Interval lengths for start points in `upper_case_special_ranges` table. */")

    c_source.add_table(lower_case_ranges[0],
                       "lower_case_ranges",
                       "uint16_t",
                       "/* Contains start points of lowercase ranges. */")

    c_source.add_table(lower_case_ranges[1],
                       "lower_case_range_lengths",
                       "uint8_t",
                       "/* Interval lengths for start points in `lower_case_ranges` table. */")

    c_source.add_table(lower_case_conversions[0],
                       "lower_case_conversions",
                       "uint16_t",
                       ("/* The remaining lowercase conversions. The lowercase variant can "
                        "be one-to-three character long. */"))

    c_source.add_table(lower_case_conversions[1],
                       "lower_case_conversion_counters",
                       "uint8_t",
                       "/* Number of one-to-one, one-to-two, and one-to-three lowercase conversions. */")

    c_source.add_table(upper_case_conversions[0],
                       "upper_case_conversions",
                       "uint16_t",
                       ("/* The remaining uppercase conversions. The uppercase variant can "
                        "be one-to-three character long. */"))

    c_source.add_table(upper_case_conversions[1],
                       "upper_case_conversion_counters",
                       "uint8_t",
                       "/* Number of one-to-one, one-to-two, and one-to-three uppercase conversions. */")

    c_source.generate()


# entry point


def main():
    parser = argparse.ArgumentParser(description='lit-unicode-{conversions,ranges}.inc.h generator',
                                     epilog='''
                                        The input files (UnicodeData.txt, SpecialCasing.txt)
                                        must be retrieved from
                                        http://www.unicode.org/Public/<VERSION>/ucd/.
                                        The last known good version is 9.0.0.
                                        ''')

    parser.add_argument('--unicode-data', metavar='FILE', action='store', required=True,
                        help='specify the unicode data file')
    parser.add_argument('--special-casing', metavar='FILE', action='store', required=True,
                        help='specify the special casing file')

    script_args = parser.parse_args()

    if not os.path.isfile(script_args.unicode_data) or not os.access(script_args.unicode_data, os.R_OK):
        parser.error('The %s file is missing or not readable!' % script_args.unicode_data)

    if not os.path.isfile(script_args.special_casing) or not os.access(script_args.special_casing, os.R_OK):
        parser.error('The %s file is missing or not readable!' % script_args.special_casing)

    generate_ranges(script_args)
    generate_conversions(script_args)


if __name__ == "__main__":
    main()
