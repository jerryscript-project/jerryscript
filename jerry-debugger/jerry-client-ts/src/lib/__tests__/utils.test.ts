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

import { getFormatSize, getUint32, setUint32, decodeMessage, encodeMessage,
  cesu8ToString, stringToCesu8, assembleUint8Arrays } from '../utils';

const defConfig = {
  cpointerSize: 2,
  littleEndian: true,
};
const altConfig = {
  cpointerSize: 4,
  littleEndian: true,
};

describe('getFormatSize', () => {
  it('returns 0 for an empty string', () => {
    expect(getFormatSize(defConfig, '')).toEqual(0);
  });

  it('throws on unexpected format character', () => {
    expect(() => {
      getFormatSize(defConfig, 'Q');
    }).toThrow();
  });

  it('returns 1 for B', () => {
    expect(getFormatSize(defConfig, 'B')).toEqual(1);
  });

  it('returns 2 for C with default configuration', () => {
    expect(getFormatSize(defConfig, 'C')).toEqual(2);
  });

  it('returns 4 for C with alternate configuration', () => {
    expect(getFormatSize(altConfig, 'C')).toEqual(4);
  });

  it('returns 4 for I', () => {
    expect(getFormatSize(defConfig, 'I')).toEqual(4);
  });

  it('returns sum for longer format', () => {
    expect(getFormatSize(defConfig, 'BCIIIBBCC')).toEqual(21);
  });

  it('returns sum for longer format', () => {
    expect(getFormatSize(altConfig, 'BCIIIBBCC')).toEqual(27);
  });
});

describe('getUint32', () => {
  it('reads little endian values', () => {
    const array = Uint8Array.from([0xef, 0xbe, 0xad, 0xde]);
    expect(getUint32(true, array, 0)).toEqual(0xdeadbeef);
  });

  it('reads big endian values', () => {
    const array = Uint8Array.from([0xde, 0xad, 0xbe, 0xef]);
    expect(getUint32(false, array, 0)).toEqual(0xdeadbeef);
  });

  it('reads at an offset', () => {
    const array = Uint8Array.from([0x00, 0x00, 0xde, 0xad, 0xbe, 0xef]);
    expect(getUint32(false, array, 2)).toEqual(0xdeadbeef);
  });
});

describe('setUint32', () => {
  it('writes little endian values', () => {
    const array = new Uint8Array(4);
    setUint32(true, array, 0, 0xdeadbeef);
    expect(array).toEqual(Uint8Array.from([0xef, 0xbe, 0xad, 0xde]));
  });

  it('writes big endian values', () => {
    const array = new Uint8Array(4);
    setUint32(false, array, 0, 0xdeadbeef);
    expect(array).toEqual(Uint8Array.from([0xde, 0xad, 0xbe, 0xef]));
  });

  it('writes at an offset', () => {
    const array = new Uint8Array(6);
    setUint32(false, array, 2, 0xdeadbeef);
    expect(array).toEqual(Uint8Array.from([0x00, 0x00, 0xde, 0xad, 0xbe, 0xef]));
  });
});

describe('decodeMessage', () => {
  it('throws if message too short', () => {
    const array = Uint8Array.from([0, 1, 2]);
    expect(() => {
      decodeMessage(defConfig, 'I', array);
    }).toThrow();
  });

  it('throws on unexpected format character', () => {
    const array = Uint8Array.from([0, 1, 2, 3, 4, 5, 6, 7]);
    expect(() => {
      decodeMessage(defConfig, 'Q', array);
    }).toThrow();
  });

  it('returns a byte with B character', () => {
    const array = Uint8Array.from([42]);
    expect(decodeMessage(defConfig, 'B', array)).toEqual([42]);
  });

  it('returns two bytes for C with default config', () => {
    const array = Uint8Array.from([1, 2, 3]);  // 3 ignored
    expect(decodeMessage(defConfig, 'C', array)).toEqual([1 + (2 << 8)]);
  });

  it('returns two bytes for C with big endian', () => {
    const array = Uint8Array.from([1, 2, 3]);  // 3 ignored
    expect(decodeMessage({
      cpointerSize: 2,
      littleEndian: false,
    }, 'C', array)).toEqual([(1 << 8) + 2]);
  });

  it('returns four bytes for C with default config', () => {
    const array = Uint8Array.from([1, 2, 3, 4, 5]);  // 5 ignored
    expect(decodeMessage(altConfig, 'C', array)).toEqual(
      [1 + (2 << 8) + (3 << 16) + (4 << 24)],
    );
  });

  it('returns four bytes for C with big endian', () => {
    const array = Uint8Array.from([1, 2, 3, 4, 5]);  // 5 ignored
    expect(decodeMessage({
      cpointerSize: 4,
      littleEndian: false,
    }, 'C', array)).toEqual(
      [(1 << 24) + (2 << 16) + (3 << 8) + 4],
    );
  });

  it('handles multiple format characters', () => {
    const array = Uint8Array.from([1, 2, 3, 4, 5, 6, 7, 8]);  // 8 ignored
    expect(decodeMessage(defConfig, 'IBC', array)).toEqual([
      1 + (2 << 8) + (3 << 16) + (4 << 24),
      5,
      6 + (7 << 8),
    ]);
  });

  it('throws on unexpected pointer size', () => {
    const array = Uint8Array.from([0, 1, 2, 3, 4, 5, 6, 7]);
    expect(() => {
      decodeMessage({
        cpointerSize: 6,
        littleEndian: true,
      }, 'C', array);
    }).toThrow();
  });
});

describe('encodeMessage', () => {
  it('throws if value list too short', () => {
    expect(() => {
      encodeMessage(defConfig, 'BI', [42]);
    }).toThrow();
  });

  it('throws on unexpected format character', () => {
    expect(() => {
      encodeMessage(defConfig, 'Q', [42]);
    }).toThrow();
  });

  it('encodes a byte with B character', () => {
    const array = encodeMessage(defConfig, 'B', [42]);
    expect(array).toEqual(Uint8Array.from([42]));
  });

  it('throws on byte outside range', () => {
    expect(() => {
      encodeMessage(defConfig, 'B', [-1]);
    }).toThrow();
    expect(() => {
      encodeMessage(defConfig, 'B', [0x100]);
    }).toThrow();
  });

  it('encodes two bytes for C with default config', () => {
    const array = encodeMessage(defConfig, 'C', [1 + (2 << 8)]);
    expect(array).toEqual(Uint8Array.from([1, 2]));
  });

  it('encodes two bytes for C with big endian', () => {
    const array = encodeMessage({
      cpointerSize: 2,
      littleEndian: false,
    }, 'C', [(1 << 8) + 2]);
    expect(array).toEqual(Uint8Array.from([1, 2]));
  });

  it('throws on two bytes outside range', () => {
    expect(() => {
      encodeMessage(defConfig, 'C', [-1]);
    }).toThrow();
    expect(() => {
      encodeMessage(defConfig, 'C', [0x10000]);
    }).toThrow();
  });

  it('encodes four bytes for C with default config', () => {
    const array = encodeMessage(altConfig, 'C', [1 + (2 << 8) + (3 << 16) + (4 << 24)]);
    expect(array).toEqual(Uint8Array.from([1, 2, 3, 4]));
  });

  it('encodes four bytes for C with big endian', () => {
    const array = encodeMessage({
      cpointerSize: 4,
      littleEndian: false,
    }, 'C', [(1 << 24) + (2 << 16) + (3 << 8) + 4]);
    expect(array).toEqual(Uint8Array.from([1, 2, 3, 4]));
  });

  it('throws on float', () => {
    expect(() => {
      encodeMessage(defConfig, 'I', [4.2]);
    }).toThrow();
  });

  it('handles multiple format characters', () => {
    const array = encodeMessage(defConfig, 'IBC', [
      1 + (2 << 8) + (3 << 16) + (4 << 24),
      5,
      6 + (7 << 8),
    ]);
    expect(array).toEqual(Uint8Array.from([1, 2, 3, 4, 5, 6, 7]));
  });

  it('throws on byte outside range', () => {
    expect(() => {
      encodeMessage({
        cpointerSize: 6,
        littleEndian: true,
      }, 'C', [42]);
    }).toThrow();
  });

  it('throws on unexpected pointer size', () => {
    expect(() => {
      encodeMessage({
        cpointerSize: 6,
        littleEndian: true,
      }, 'C', [42]);
    }).toThrow();
  });
});

describe('cesu8ToString and stringToCesu8', () => {
  it('returns empty string for undefined input', () => {
    expect(cesu8ToString(undefined)).toEqual('');
  });

  it('returns empty array for empty string, and vice versa', () => {
    expect(cesu8ToString(new Uint8Array(0))).toEqual('');
    expect(stringToCesu8('')).toEqual(new Uint8Array(0));
  });

  it('returns ASCII from ASCII', () => {
    const sentence = 'The quick brown fox jumped over the lazy dog.';
    const array = new Uint8Array(sentence.length);
    for (let i = 0; i < sentence.length; i++) {
      array[i] = sentence.charCodeAt(i);
    }
    expect(cesu8ToString(array)).toEqual(sentence);
    expect(stringToCesu8(sentence)).toEqual(array);
  });

  it('acts like UTF-8 for two-byte encodings', () => {
    // 0x080 = 00010 000000 = 0x02, 0x00
    const lowTwoByte = Uint8Array.from([0xc0 + 0x02, 0x80 + 0x00]);
    // 0x7ff = 11111 111111 = 0x1f, 0x3f
    const highTwoByte = Uint8Array.from([0xc0 + 0x1f, 0x80 + 0x3f]);
    expect(cesu8ToString(lowTwoByte)).toEqual(String.fromCharCode(0x80));
    expect(cesu8ToString(highTwoByte)).toEqual(String.fromCharCode(0x7ff));
    expect(stringToCesu8(String.fromCharCode(0x80))).toEqual(lowTwoByte);
    expect(stringToCesu8(String.fromCharCode(0x7ff))).toEqual(highTwoByte);
  });

  it('acts like UTF-8 for three-byte encodings', () => {
    // 0x0800 = 0000 100000 000000 = 0x00, 0x20, 0x00
    const lowThreeByte = Uint8Array.from([0xe0 + 0x00, 0x80 + 0x20, 0x80 + 0x00]);
    // 0xffff = 1111 111111 111111 = 0x0f, 0x3f, 0x3f
    const highThreeByte = Uint8Array.from([0xe0 + 0x0f, 0x80 + 0x3f, 0x80 + 0x3f]);
    expect(cesu8ToString(lowThreeByte)).toEqual(String.fromCharCode(0x0800));
    expect(cesu8ToString(highThreeByte)).toEqual(String.fromCharCode(0xffff));
    expect(stringToCesu8(String.fromCharCode(0x0800))).toEqual(lowThreeByte);
    expect(stringToCesu8(String.fromCharCode(0xffff))).toEqual(highThreeByte);
  });

  it('decodes UTF-16 surrogate pairs', () => {
    // 😂 is encoded as a 'surrogate pair': \ud83d\ude02 -- this is valid
    // CESU-8 but invalid UTF-8 (a four-byte encoding should be used).
    const surrogatePairBytes = Uint8Array.from([
      // 0xd83d = 1101 100000 111101 = 0x0d, 0x20, 0x3d
      0xe0 + 0x0d, 0x80 + 0x20, 0x80 + 0x3d,
      // 0xde02 = 1101 111000 000010 = 0x0d, 0x38, 0x02
      0xe0 + 0x0d, 0x80 + 0x38, 0x80 + 0x02,
    ]);
    expect(cesu8ToString(surrogatePairBytes)).toEqual('😂');
    expect(stringToCesu8('😂')).toEqual(surrogatePairBytes);
  });
});

describe('assembleUint8Arrays', () => {
  it('drops the first byte of the second arg if first is undefined', () => {
    const array = Uint8Array.from([1, 2, 3]);
    expect(assembleUint8Arrays(undefined, array)).toEqual(Uint8Array.from([2, 3]));
  });

  it('returns first array if second empty', () => {
    const array1 = Uint8Array.from([1, 2, 3]);
    const array2 = new Uint8Array([]);
    expect(assembleUint8Arrays(array1, array2)).toEqual(Uint8Array.from([1, 2, 3]));
  });

  it('returns first array if second has one byte', () => {
    const array1 = Uint8Array.from([1, 2, 3]);
    const array2 = new Uint8Array([4]);
    expect(assembleUint8Arrays(array1, array2)).toEqual(Uint8Array.from([1, 2, 3]));
  });

  it('concatenates two arrays dropping first byte of the second one', () => {
    const array1 = Uint8Array.from([1, 2, 3]);
    const array2 = new Uint8Array([4, 5, 6]);
    expect(assembleUint8Arrays(array1, array2)).toEqual(Uint8Array.from([1, 2, 3, 5, 6]));
  });
});
