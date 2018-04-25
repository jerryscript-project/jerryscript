// Copyright JS Foundation and other contributors, http://js.foundation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

export interface ByteConfig {
  cpointerSize: number;
  littleEndian: boolean;
}

/**
 * Calculates expected byte length given a format string
 */
export function getFormatSize(config: ByteConfig, format: string) {
  let length = 0;
  for (let i = 0; i < format.length; i++) {
    switch (format[i]) {
      case 'B':
        length++;
        break;

      case 'C':
        length += config.cpointerSize;
        break;

      case 'I':
        length += 4;
        break;

      default:
        throw new Error('unsupported message format');
    }
  }
  return length;
}

/**
 * Returns a 32-bit integer read from array at offset, in either direction
 *
 * @param littleEndian Byte order to use
 * @param array        Array with length >= offset + 3
 * @param offset       Offset at which to start reading
 */
export function getUint32(littleEndian: boolean, array: Uint8Array, offset: number) {
  let value = 0;
  if (littleEndian) {
    value = array[offset];
    value |= array[offset + 1] << 8;
    value |= array[offset + 2] << 16;
    value |= array[offset + 3] << 24;
  } else {
    value = array[offset] << 24;
    value |= array[offset + 1] << 16;
    value |= array[offset + 2] << 8;
    value |= array[offset + 3];
  }
  return value >>> 0;
}

/**
 * Writes a 32-bit integer to array at offset, in either direction
 *
 * @param littleEndian Byte order to use
 * @param array        Array with length >= offset + 3
 * @param offset       Offset at which to start writing
 * @param value        Value to write in 32-bit integer range
 */
export function setUint32(littleEndian: boolean, array: Uint8Array, offset: number, value: number) {
  if (littleEndian) {
    array[offset] = value & 0xff;
    array[offset + 1] = (value >> 8) & 0xff;
    array[offset + 2] = (value >> 16) & 0xff;
    array[offset + 3] = (value >> 24) & 0xff;
  } else {
    array[offset] = (value >> 24) & 0xff;
    array[offset + 1] = (value >> 16) & 0xff;
    array[offset + 2] = (value >> 8) & 0xff;
    array[offset + 3] = value & 0xff;
  }
}

/**
 * Parses values out of message array matching format
 *
 * Throws if message not big enough for specified format or bogus format character
 *
 * @param config  Byte order / size info
 * @param format  String of 'B', 'C', and 'I' characters
 * @param message Array containing message
 * @param offset  Optional offset at which to start reading
 */
export function decodeMessage(config: ByteConfig, format: string, message: Uint8Array, offset = 0) {
  // Format: B=byte I=int32 C=cpointer
  // Returns an array of decoded numbers

  const result = [];
  let value;

  if (offset + getFormatSize(config, format) > message.byteLength) {
    throw new Error('received message too short');
  }

  for (let i = 0; i < format.length; i++) {
    if (format[i] === 'B') {
      result.push(message[offset++]);
      continue;
    }

    if (format[i] === 'C' && config.cpointerSize === 2) {
      if (config.littleEndian) {
        value = message[offset] | (message[offset + 1] << 8);
      } else {
        value = (message[offset] << 8) | message[offset + 1];
      }

      result.push(value);
      offset += 2;
      continue;
    }

    if (format[i] !== 'I' && (format[i] !== 'C' || config.cpointerSize !== 4)) {
      throw new Error('unexpected decode request');
    }

    value = getUint32(config.littleEndian, message, offset);
    result.push(value);
    offset += 4;
  }

  return result;
}

/**
 * Packs values into new array according to format
 *
 * Throws if not enough values supplied or values exceed expected integer ranges
 *
 * @param config Byte order / size info
 * @param format String of 'B', 'C', and 'I' characters
 * @param values Array of values to format into message
 */
export function encodeMessage(config: ByteConfig, format: string, values: Array<number>) {
  const length = getFormatSize(config, format);
  const message = new Uint8Array(length);
  let offset = 0;

  if (values.length < format.length) {
    throw new Error('not enough values supplied');
  }

  for (let i = 0; i < format.length; i++) {
    const value = values[i];

    if (format[i] === 'B') {
      if ((value & 0xff) !== value) {
        throw new Error('expected byte value');
      }
      message[offset++] = value;
      continue;
    }

    if (format[i] === 'C' && config.cpointerSize === 2) {
      if ((value & 0xffff) !== value) {
        throw new Error('expected two-byte value');
      }
      const lowByte = value & 0xff;
      const highByte = (value >> 8) & 0xff;

      if (config.littleEndian) {
        message[offset++] = lowByte;
        message[offset++] = highByte;
      } else {
        message[offset++] = highByte;
        message[offset++] = lowByte;
      }
      continue;
    }

    if (format[i] !== 'I' && (format[i] !== 'C' || config.cpointerSize !== 4)) {
      throw new Error('unexpected encode request');
    }

    if ((value & 0xffffffff) !== value) {
      throw new Error('expected four-byte value');
    }

    setUint32(config.littleEndian, message, offset, value);
    offset += 4;
  }

  return message;
}

export function cesu8ToString(array: Uint8Array | undefined) {
  if (!array) {
    return '';
  }

  const length = array.byteLength;

  let i = 0;
  let result = '';

  while (i < length) {
    let chr = array[i++];

    if (chr >= 0x7f) {
      if (chr & 0x20) {
        // Three byte long character
        chr = ((chr & 0xf) << 12) | ((array[i] & 0x3f) << 6) | (array[i + 1] & 0x3f);
        i += 2;
      } else {
        // Two byte long character
        chr = ((chr & 0x1f) << 6) | (array[i] & 0x3f);
        ++i;
      }
    }

    result += String.fromCharCode(chr);
  }

  return result;
}

/**
 * Convert a string to Uint8Array buffer in cesu8 format, with optional left padding
 *
 * @param str String to convert
 * @param offset Optional number of padding bytes to allocate at the beginning
 */
export function stringToCesu8(str: string, offset: number = 0) {
  const length = str.length;
  let byteLength = length;
  for (let i = 0; i < length; i++) {
    const chr = str.charCodeAt(i);

    if (chr > 0x7ff) {
      byteLength++;
    }

    if (chr >= 0x7f) {
      byteLength++;
    }
  }

  const result = new Uint8Array(offset + byteLength);
  for (let i = 0; i < length; i++) {
    const chr = str.charCodeAt(i);

    if (chr > 0x7ff) {
      result[offset++] = 0xe0 | (chr >> 12);
      result[offset++] = 0x80 | ((chr >> 6) & 0x3f);
      result[offset++] = 0x80 | (chr & 0x3f);
    } else if (chr >= 0x7f) {
      result[offset++] = 0xc0 | (chr >> 6);
      result[offset++] = 0x80 | (chr & 0x3f);
    } else {
      result[offset++] = chr;
    }
  }
  return result;
}

// Concat the two arrays. The first byte (opcode) of nextArray is ignored.
export function assembleUint8Arrays(baseArray: Uint8Array | undefined, nextArray: Uint8Array) {
  if (!baseArray) {
    // Cut the first byte (opcode)
    return nextArray.slice(1);
  }

  if (nextArray.byteLength <= 1) {
    // Nothing to append
    return baseArray;
  }

  const baseLength = baseArray.byteLength;
  const nextLength = nextArray.byteLength - 1;

  const result = new Uint8Array(baseLength + nextLength);
  result.set(nextArray, baseLength - 1);

  // This set operation overwrites the opcode
  result.set(baseArray);

  return result;
}
