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
import re
import warnings

from gen_c_source import LICENSE, format_code
from settings import PROJECT_DIR


UNICODE_DATA_FILE = 'UnicodeData.txt'
SPECIAL_CASING_FILE = 'SpecialCasing.txt'
DERIVED_PROPS_FILE = 'DerivedCoreProperties.txt'
PROP_LIST_FILE = 'PropList.txt'
CASE_FOLDING_FILE = 'CaseFolding.txt'

RANGES_C_SOURCE = os.path.join(PROJECT_DIR, 'jerry-core/lit/lit-unicode-ranges.inc.h')
RANGES_SUP_C_SOURCE = os.path.join(PROJECT_DIR, 'jerry-core/lit/lit-unicode-ranges-sup.inc.h')
CONVERSIONS_C_SOURCE = os.path.join(PROJECT_DIR, 'jerry-core/lit/lit-unicode-conversions.inc.h')
CONVERSIONS_SUP_C_SOURCE = os.path.join(PROJECT_DIR, 'jerry-core/lit/lit-unicode-conversions-sup.inc.h')
FOLDING_C_SOURCE = os.path.join(PROJECT_DIR, 'jerry-core/lit/lit-unicode-folding.inc.h')
FOLDING_SUP_C_SOURCE = os.path.join(PROJECT_DIR, 'jerry-core/lit/lit-unicode-folding-sup.inc.h')

UNICODE_PLANE_TYPE_BASIC = 0
UNICODE_PLANE_TYPE_SUPPLEMENTARY = 1

# common code generation

class UnicodeBasicSource:
    # pylint: disable=too-many-instance-attributes
    def __init__(self, filepath, character_type="uint16_t", length_type="uint8_t"):
        self._filepath = filepath
        self._header = [LICENSE, ""]
        self._data = []
        self._table_name_suffix = ""
        self.character_type = character_type
        self.length_type = length_type

        self._range_table_types = [self.character_type,
                                   self.length_type,
                                   self.character_type]
        self._range_table_names = ["interval_starts",
                                   "interval_lengths",
                                   "chars"]
        self._range_table_descriptions = ["Character interval starting points for",
                                          "Character interval lengths for",
                                          "Non-interval characters for"]

        self._conversion_range_types = [self.character_type,
                                        self.length_type]
        self._conversion_range_names = ["ranges",
                                        "range_lengths"]

    def complete_header(self, completion):
        self._header.append(completion)
        self._header.append("")  # for an extra empty line

    def add_whitepace_range(self, category, categorizer, units):
        self.add_range(category, categorizer.create_tables(units))

    def add_range(self, category, tables):
        idx = 0
        for table in tables:
            self.add_table(table,
                           f"/**\n * {self._range_table_descriptions[idx]} {category}.\n */",
                           self._range_table_types[idx],
                           category,
                           self._range_table_names[idx])
            idx += 1

    def add_conversion_range(self, category, tables, descriptions):
        self.add_named_conversion_range(category, tables, self._conversion_range_names, descriptions)

    def add_named_conversion_range(self, category, tables, table_names, descriptions):
        idx = 0
        for table in tables:
            self.add_table(table,
                           descriptions[idx],
                           self._conversion_range_types[idx],
                           category,
                           table_names[idx])
            idx += 1

    def add_table(self, table, description, table_type, category, table_name):
        if table and sum(table) != 0:
            self._data.append(description)
            self._data.append(f"static const {table_type} lit_unicode_{category.lower()}"
                              f"{'_' + table_name if table_name else ''}{self._table_name_suffix}"
                              f"[] JERRY_ATTR_CONST_DATA =")
            self._data.append("{")
            self._data.append(format_code(table, 1, 6 if self._table_name_suffix else 4))
            self._data.append("};")
            self._data.append("")  # for an extra empty line

    def generate(self):
        with open(self._filepath, 'w', encoding='utf8') as generated_source:
            generated_source.write("\n".join(self._header))
            generated_source.write("\n".join(self._data))


class UnicodeSupplementarySource(UnicodeBasicSource):
    def __init__(self, filepath):
        UnicodeBasicSource.__init__(self, filepath, "uint32_t", "uint16_t")
        self._table_name_suffix = "_sup"

    def add_whitepace_range(self, category, categorizer, units):
        self.add_range(category, categorizer.create_tables(units))

class UnicodeBasicCategorizer:
    def __init__(self):
        self._length_limit = 0xff
        self.extra_id_continue_units = set([0x200C, 0x200D])

    #pylint: disable=no-self-use
    def in_range(self, i):
        return 0x80 <= i < 0x10000

    def _group_ranges(self, units):
        """
        Convert an increasing list of integers into a range list
        :return: List of ranges.
        """
        for _, group in itertools.groupby(enumerate(units), lambda q: (q[1] - q[0])):
            group = list(group)
            yield group[0][1], group[-1][1]

    def create_tables(self, units):
        """
        Split list of ranges into intervals and single char lists.
        :return: A tuple containing the following info:
            - list of interval starting points
            - list of interval lengths
            - list of single chars
        """

        interval_sps = []
        interval_lengths = []
        chars = []

        for element in self._group_ranges(units):
            interval_length = element[1] - element[0]
            if interval_length == 0:
                chars.append(element[0])
            elif interval_length > self._length_limit:
                for i in range(element[0], element[1], self._length_limit + 1):
                    length = min(self._length_limit, element[1] - i)
                    interval_sps.append(i)
                    interval_lengths.append(length)
            else:
                interval_sps.append(element[0])
                interval_lengths.append(interval_length)

        return interval_sps, interval_lengths, chars

    def read_units(self, file_path, categories, subcategories=None):
        """
        Read the Unicode Derived Core Properties file and extract the ranges
        for the given categories.

        :param file_path: Path to the Unicode "DerivedCoreProperties.txt" file.
        :param categories: A list of category strings to extract from the Unicode file.
        :param subcategories: A list of subcategory strings to restrict categories.
        :return: A dictionary each string from the :param categories: is a key and for each
                key list of code points are stored.
        """
        # Create a dictionary in the format: { category[0]: [ ], ..., category[N]: [ ] }
        units = {}
        for category in categories:
            units[category] = []

        # Formats to match:
        #  <HEX>     ; <category> #
        #  <HEX>..<HEX>     ; <category> # <subcategory>
        matcher = r"(?P<start>[\dA-F]+)(?:\.\.(?P<end>[\dA-F]+))?\s+; (?P<category>[\w]+) # (?P<subcategory>[\w&]{2})"

        with open(file_path, "r", encoding='utf8') as src_file:
            for line in src_file:
                match = re.match(matcher, line)

                if (match
                        and match.group("category") in categories
                        and (not subcategories or match.group("subcategory") in subcategories)):
                    start = int(match.group("start"), 16)
                    # if no "end" found use the "start"
                    end = int(match.group("end") or match.group("start"), 16)

                    matching_code_points = [
                        code_point for code_point in range(start, end + 1) if self.in_range(code_point)
                    ]

                    units[match.group("category")].extend(matching_code_points)

        return units

    def read_case_mappings(self, unicode_data_file, special_casing_file):
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
        with open(unicode_data_file, encoding='utf8') as unicode_data:
            reader = csv.reader(unicode_data, delimiter=';')

            for line in reader:
                letter_id = int(line[0], 16)

                if not self.in_range(letter_id):
                    continue

                capital_letter = line[12]
                small_letter = line[13]

                if capital_letter:
                    upper_case_mapping[letter_id] = parse_unicode_sequence(capital_letter)

                if small_letter:
                    lower_case_mapping[letter_id] = parse_unicode_sequence(small_letter)

        # Update the conversion tables with the special cases
        with open(special_casing_file, encoding='utf8') as special_casing:
            reader = csv.reader(special_casing, delimiter=';')

            for line in reader:
                # Skip comment sections and empty lines
                if not line or line[0].startswith('#'):
                    continue

                # Replace '#' character with empty string
                for idx, fragment in enumerate(line):
                    if fragment.find('#') >= 0:
                        line[idx] = ''

                letter_id = int(line[0], 16)
                condition_list = line[4]

                if not self.in_range(letter_id) or condition_list:
                    continue

                original_letter = parse_unicode_sequence(line[0])
                small_letter = parse_unicode_sequence(line[1])
                capital_letter = parse_unicode_sequence(line[3])

                if small_letter != original_letter:
                    lower_case_mapping[letter_id] = small_letter
                if capital_letter != original_letter:
                    upper_case_mapping[letter_id] = capital_letter

        return lower_case_mapping, upper_case_mapping

class UnicodeSupplementaryCategorizer(UnicodeBasicCategorizer):
    def __init__(self):
        UnicodeBasicCategorizer.__init__(self)
        self._length_limit = 0xffff
        self.extra_id_continue_units = set()

    def in_range(self, i):
        return i >= 0x10000

def generate_ranges(script_args, plane_type):
    if plane_type == UNICODE_PLANE_TYPE_SUPPLEMENTARY:
        c_source = UnicodeSupplementarySource(RANGES_SUP_C_SOURCE)
        categorizer = UnicodeSupplementaryCategorizer()
    else:
        c_source = UnicodeBasicSource(RANGES_C_SOURCE)
        categorizer = UnicodeBasicCategorizer()

    header_completion = [f"/* This file is automatically generated by the {os.path.basename(__file__)} script",
                         f" * from {DERIVED_PROPS_FILE}. Do not edit! */",
                         ""]

    c_source.complete_header("\n".join(header_completion))

    derived_props_path = os.path.join(script_args.unicode_dir, DERIVED_PROPS_FILE)
    units = categorizer.read_units(derived_props_path, ["ID_Start", "ID_Continue"])

    units["ID_Continue"] = sorted(set(units["ID_Continue"]).union(categorizer.extra_id_continue_units)
                                  - set(units["ID_Start"]))

    for category, unit in units.items():
        c_source.add_range(category, categorizer.create_tables(unit))

    prop_list_path = os.path.join(script_args.unicode_dir, PROP_LIST_FILE)

    white_space_units = categorizer.read_units(prop_list_path, ["White_Space"], ["Zs"])["White_Space"]

    c_source.add_whitepace_range("White_Space", categorizer, white_space_units)

    c_source.generate()


# functions for unicode conversions

def make_char(hex_val):
    """
    Create a unicode character from a hex value

    :param hex_val: Hex value of the character.
    :return: Unicode character corresponding to the value.
    """

    try:
        return unichr(hex_val)
    except NameError:
        return chr(hex_val)


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
        result += make_char(hex_val)

    return result

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


def generate_conversions(script_args, plane_type):
    if plane_type == UNICODE_PLANE_TYPE_SUPPLEMENTARY:
        c_source = UnicodeSupplementarySource(CONVERSIONS_SUP_C_SOURCE)
        categorizer = UnicodeSupplementaryCategorizer()
    else:
        c_source = UnicodeBasicSource(CONVERSIONS_C_SOURCE)
        categorizer = UnicodeBasicCategorizer()

    header_completion = [f"/* This file is automatically generated by the {os.path.basename(__file__)} script",
                         f" * from {UNICODE_DATA_FILE} and {SPECIAL_CASING_FILE} files. Do not edit! */",
                         ""]

    c_source.complete_header("\n".join(header_completion))

    unicode_data_path = os.path.join(script_args.unicode_dir, UNICODE_DATA_FILE)
    special_casing_path = os.path.join(script_args.unicode_dir, SPECIAL_CASING_FILE)

    # Read the corresponding unicode values of lower and upper case letters and store these in tables
    lower_case, upper_case = categorizer.read_case_mappings(unicode_data_path, special_casing_path)

    c_source.add_conversion_range("character_case",
                                  extract_ranges(lower_case, upper_case),
                                  [("/* Contains start points of character case ranges "
                                    "(these are bidirectional conversions). */"),
                                   "/* Interval lengths of start points in `character_case_ranges` table. */"])
    c_source.add_conversion_range("character_pair",
                                  extract_character_pair_ranges(lower_case, upper_case),
                                  ["/* Contains the start points of bidirectional conversion ranges. */",
                                   "/* Interval lengths of start points in `character_pair_ranges` table. */"])

    c_source.add_table(extract_character_pairs(lower_case, upper_case),
                       "/* Contains lower/upper case bidirectional conversion pairs. */",
                       c_source.character_type,
                       "character_pairs",
                       "")

    c_source.add_conversion_range("upper_case_special",
                                  extract_special_ranges(upper_case),
                                  [("/* Contains start points of one-to-two uppercase ranges where the "
                                    "second character\n"
                                    " * is always the same.\n"
                                    " */"),
                                   "/* Interval lengths for start points in `upper_case_special_ranges` table. */"])

    c_source.add_conversion_range("lower_case",
                                  extract_ranges(lower_case),
                                  ["/* Contains start points of lowercase ranges. */",
                                   "/* Interval lengths for start points in `lower_case_ranges` table. */"])

    c_source.add_named_conversion_range("lower_case",
                                        extract_conversions(lower_case),
                                        ["conversions", "conversion_counters"],
                                        [("/* The remaining lowercase conversions. The lowercase variant can "
                                          "be one-to-three character long. */"),
                                         ("/* Number of one-to-one, one-to-two, and one-to-three lowercase "
                                          "conversions. */")])

    c_source.add_named_conversion_range("upper_case",
                                        extract_conversions(upper_case),
                                        ["conversions", "conversion_counters"],
                                        [("/* The remaining uppercase conversions. The uppercase variant can "
                                          "be one-to-three character long. */"),
                                         ("/* Number of one-to-one, one-to-two, and one-to-three uppercase "
                                          "conversions. */")])

    if lower_case:
        warnings.warn('Not all elements extracted from the lowercase table!')
    if upper_case:
        warnings.warn('Not all elements extracted from the uppercase table!')

    c_source.generate()


def generate_folding(script_args, plane_type):
    if plane_type == UNICODE_PLANE_TYPE_SUPPLEMENTARY:
        c_source = UnicodeSupplementarySource(FOLDING_SUP_C_SOURCE)
        categorizer = UnicodeSupplementaryCategorizer()
    else:
        c_source = UnicodeBasicSource(FOLDING_C_SOURCE)
        categorizer = UnicodeBasicCategorizer()

    header_completion = [f"/* This file is automatically generated by the {os.path.basename(__file__)} script",
                         f" * from the {CASE_FOLDING_FILE} file. Do not edit! */",
                         ""]

    c_source.complete_header("\n".join(header_completion))

    unicode_data_path = os.path.join(script_args.unicode_dir, UNICODE_DATA_FILE)
    special_casing_path = os.path.join(script_args.unicode_dir, SPECIAL_CASING_FILE)
    case_folding_path = os.path.join(script_args.unicode_dir, CASE_FOLDING_FILE)

    # Read the corresponding unicode values of lower and upper case letters and store these in tables
    lower_case, upper_case = categorizer.read_case_mappings(unicode_data_path, special_casing_path)

    folding = {}

    with open(case_folding_path, 'r', encoding='utf8') as case_folding:
        case_folding_re = re.compile(r'(?P<code_point>[^;]*);\s*(?P<type>[^;]*);\s*(?P<folding>[^;]*);')
        for line in case_folding:
            match = case_folding_re.match(line)
            if match and match.group('type') in ('S', 'C'):
                code_point = int(match.group('code_point'), 16)

                if categorizer.in_range(code_point):
                    folding[code_point] = parse_unicode_sequence(match.group('folding'))

    should_to_upper = []
    should_skip_to_lower = []

    for code_point in lower_case:
        if code_point not in folding:
            should_skip_to_lower.append(code_point)

    for code_point, folded in folding.items():
        if lower_case.get(code_point, make_char(code_point)) != folded:
            should_to_upper.append(code_point)

            if upper_case.get(code_point, '') == folded:
                should_skip_to_lower.append(code_point)

    c_source.add_range('folding_skip_to_lower', categorizer.create_tables(should_skip_to_lower))
    c_source.add_range('folding_to_upper', categorizer.create_tables(should_to_upper))

    c_source.generate()


# entry point


def main():
    parser = argparse.ArgumentParser(description='lit-unicode-{conversions,ranges}-{sup}.inc.h generator',
                                     epilog='''
                                        The input data must be retrieved from
                                        http://www.unicode.org/Public/<VERSION>/ucd/UCD.zip.
                                        The last known good version is 13.0.0.
                                        ''')
    def check_dir(path):
        if not os.path.isdir(path) or not os.access(path, os.R_OK):
            raise argparse.ArgumentTypeError(f'The {path} directory does not exist or is not readable!')
        return path

    parser.add_argument('--unicode-dir', metavar='DIR', action='store', required=True,
                        type=check_dir, help='specify the unicode data directory')

    script_args = parser.parse_args()

    generate_ranges(script_args, UNICODE_PLANE_TYPE_BASIC)
    generate_ranges(script_args, UNICODE_PLANE_TYPE_SUPPLEMENTARY)
    generate_conversions(script_args, UNICODE_PLANE_TYPE_BASIC)
    generate_conversions(script_args, UNICODE_PLANE_TYPE_SUPPLEMENTARY)
    generate_folding(script_args, UNICODE_PLANE_TYPE_BASIC)
    # There are currently no code points in the supplementary planes that require special folding
    # generate_folding(script_args, UNICODE_PLANE_TYPE_SUPPLEMENTARY)


if __name__ == "__main__":
    main()
