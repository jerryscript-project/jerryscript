/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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
 */

#ifndef BYTECODE_DATA_H
#define BYTECODE_DATA_H

#include "opcodes.h"
#include "lit-id-hash-table.h"

/*
 * All literals are kept in the 'literals' array.
 * Literal structure doesn't hold real string. All program-specific strings
 * are kept in the 'strings_buffer' and literal has pointer to this buffer.
 *
 * Literal id is its index in 'literals' array of bytecode_data_t structure.
 *
 * Bytecode, which is kept in the 'opcodes' field, is divided into blocks
 * of 'BLOCK_SIZE' operands. Every block has its own numbering of literals.
 * Literal uid could be in range [0, 127] in every block.
 *
 * To map uid to literal id 'lit_id_hash' table is used.
 */
#define BLOCK_SIZE 64

/**
 * Pointer to lit_id_hash_table precedes every independent bytecode region
 */
typedef struct __attribute__ ((aligned (MEM_ALIGNMENT)))
{
  lit_id_hash_table *lit_id_hash;
} opcodes_header_t;

typedef struct
{
  const ecma_char_t *strings_buffer;
  const opcode_t *opcodes;
  opcode_counter_t opcodes_count;
} bytecode_data_t;

/**
 * Macros to get a hash table corresponding to a bytecode region
 */
#define GET_HASH_TABLE_FOR_BYTECODE(opcodes) (((opcodes_header_t *) (((uint8_t *) (opcodes)) - \
                                                                     sizeof (opcodes_header_t)))->lit_id_hash)

/**
 * Macros to get a pointer to bytecode header by pointer to opcodes start
 */
#define GET_BYTECODE_HEADER(opcodes) ((opcodes_header_t *) (((uint8_t *) (opcodes)) - sizeof (opcodes_header_t)))

#endif // BYTECODE_DATA_H
