/* Copyright JS Foundation and other contributors, http://js.foundation
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

#ifndef JERRYSCRIPT_CONFIG_H
#define JERRYSCRIPT_CONFIG_H

// @JERRY_BUILD_CFG@

/**
 * Built-in configurations
 *
 * Allowed values for built-in defines:
 *  0: Disable the given built-in.
 *  1: Enable the given built-in.
 */
/*
 * By default all built-ins are enabled if they are not defined.
 */
#ifndef JERRY_BUILTINS
#define JERRY_BUILTINS 1
#endif /* !defined (JERRY_BUILTINS) */

#ifndef JERRY_BUILTIN_ANNEXB
#define JERRY_BUILTIN_ANNEXB JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_ANNEXB) */

#ifndef JERRY_BUILTIN_ARRAY
#define JERRY_BUILTIN_ARRAY JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_ARRAY) */

#ifndef JERRY_BUILTIN_BOOLEAN
#define JERRY_BUILTIN_BOOLEAN JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_BOOLEAN) */

#ifndef JERRY_BUILTIN_DATE
#define JERRY_BUILTIN_DATE JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_DATE) */

#ifndef JERRY_BUILTIN_ERRORS
#define JERRY_BUILTIN_ERRORS JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_ERRORS) */

#ifndef JERRY_BUILTIN_JSON
#define JERRY_BUILTIN_JSON JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_JSON) */

#ifndef JERRY_BUILTIN_MATH
#define JERRY_BUILTIN_MATH JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_MATH) */

#ifndef JERRY_BUILTIN_NUMBER
#define JERRY_BUILTIN_NUMBER JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_NUMBER) */

#ifndef JERRY_BUILTIN_REGEXP
#define JERRY_BUILTIN_REGEXP JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_REGEXP) */

#ifndef JERRY_BUILTIN_STRING
#define JERRY_BUILTIN_STRING JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_STRING) */

#ifndef JERRY_BUILTIN_BIGINT
#define JERRY_BUILTIN_BIGINT JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_BIGINT) */

#ifndef JERRY_BUILTIN_CONTAINER
#define JERRY_BUILTIN_CONTAINER JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_CONTAINER) */

#ifndef JERRY_BUILTIN_DATAVIEW
#define JERRY_BUILTIN_DATAVIEW JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_DATAVIEW) */

#ifndef JERRY_BUILTIN_GLOBAL_THIS
#define JERRY_BUILTIN_GLOBAL_THIS JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_GLOBAL_THIS) */

#ifndef JERRY_BUILTIN_PROXY
#define JERRY_BUILTIN_PROXY JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_PROXY) */

#ifndef JERRY_BUILTIN_REALMS
#define JERRY_BUILTIN_REALMS JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_REALMS) */

#ifndef JERRY_BUILTIN_REFLECT
#define JERRY_BUILTIN_REFLECT JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_REFLECT) */

#ifndef JERRY_BUILTIN_TYPEDARRAY
#define JERRY_BUILTIN_TYPEDARRAY JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_TYPEDARRAY) */

#ifndef JERRY_BUILTIN_SHAREDARRAYBUFFER
#define JERRY_BUILTIN_SHAREDARRAYBUFFER JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_SHAREDARRAYBUFFER) */

#ifndef JERRY_BUILTIN_ATOMICS
#define JERRY_BUILTIN_ATOMICS JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_ATOMICS) */

#ifndef JERRY_BUILTIN_WEAKREF
#define JERRY_BUILTIN_WEAKREF JERRY_BUILTINS
#endif /* !defined (JERRY_BUILTIN_WEAKREF) */

#ifndef JERRY_MODULE_SYSTEM
#define JERRY_MODULE_SYSTEM JERRY_BUILTINS
#endif /* !defined (JERRY_MODULE_SYSTEM) */

/**
 * Engine internal and misc configurations.
 */

/**
 * Specifies the compressed pointer representation
 *
 * Allowed values:
 *  0: use 16 bit representation
 *  1: use 32 bit representation
 *
 * Default value: 0
 * For more details see: jmem/jmem.h
 */
#ifndef JERRY_CPOINTER_32_BIT
#define JERRY_CPOINTER_32_BIT 0
#endif /* !defined (JERRY_CPOINTER_32_BIT) */

/**
 * Enable/Disable the engine's JavaScript debugger interface
 *
 * Allowed values:
 *  0: Disable the debugger parts.
 *  1: Enable the debugger.
 */
#ifndef JERRY_DEBUGGER
#define JERRY_DEBUGGER 0
#endif /* !defined (JERRY_DEBUGGER) */

/**
 * Enable/Disable built-in error messages for error objects.
 *
 * Allowed values:
 *  0: Disable error messages.
 *  1: Enable error message.
 *
 * Default value: 0
 */
#ifndef JERRY_ERROR_MESSAGES
#define JERRY_ERROR_MESSAGES 0
#endif /* !defined (JERRY_ERROR_MESSAGES) */

/**
 * Enable/Disable external context.
 *
 * Allowed values:
 *  0: Disable external context.
 *  1: Enable external context support.
 *
 * Default value: 0
 */
#ifndef JERRY_EXTERNAL_CONTEXT
#define JERRY_EXTERNAL_CONTEXT 0
#endif /* !defined (JERRY_EXTERNAL_CONTEXT) */

/**
 * Maximum size of heap in kilobytes
 *
 * Default value: 512 KiB
 */
#ifndef JERRY_GLOBAL_HEAP_SIZE
#define JERRY_GLOBAL_HEAP_SIZE (512)
#endif /* !defined (JERRY_GLOBAL_HEAP_SIZE) */

/**
 * The allowed heap usage limit until next garbage collection, in bytes.
 *
 * If value is 0, the default is 1/32 of JERRY_HEAP_SIZE
 */
#ifndef JERRY_GC_LIMIT
#define JERRY_GC_LIMIT 0
#endif /* !defined (JERRY_GC_LIMIT) */

/**
 * Maximum stack usage size in kilobytes
 *
 * Note: This feature cannot be used when 'detect_stack_use_after_return=1' ASAN option is enabled.
 * For more detailed description:
 *   - https://github.com/google/sanitizers/wiki/AddressSanitizerUseAfterReturn#compatibility
 *
 * Default value: 0, unlimited
 */
#ifndef JERRY_STACK_LIMIT
#define JERRY_STACK_LIMIT (0)
#endif /* !defined (JERRY_STACK_LIMIT) */

/**
 * Maximum depth of recursion during GC mark phase
 *
 * Default value: 8
 */
#ifndef JERRY_GC_MARK_LIMIT
#define JERRY_GC_MARK_LIMIT (8)
#endif /* !defined (JERRY_GC_MARK_LIMIT) */

/**
 * Enable/Disable property lookup cache.
 *
 * Allowed values:
 *  0: Disable lookup cache.
 *  1: Enable lookup cache.
 *
 * Default value: 1
 */
#ifndef JERRY_LCACHE
#define JERRY_LCACHE 1
#endif /* !defined (JERRY_LCACHE) */

/**
 * Enable/Disable function toString operation.
 *
 * Allowed values:
 *  0: Disable function toString operation.
 *  1: Enable function toString operation.
 *
 * Default value: 0
 */
#ifndef JERRY_FUNCTION_TO_STRING
#define JERRY_FUNCTION_TO_STRING 0
#endif /* !defined (JERRY_FUNCTION_TO_STRING) */

/**
 * Enable/Disable line-info management inside the engine.
 *
 * Allowed values:
 *  0: Disable line-info in the engine.
 *  1: Enable line-info management.
 *
 * Default value: 0
 */
#ifndef JERRY_LINE_INFO
#define JERRY_LINE_INFO 0
#endif /* !defined (JERRY_LINE_INFO) */

/**
 * Enable/Disable logging inside the engine.
 *
 * Allowed values:
 *  0: Disable internal logging.
 *  1: Enable internal logging.
 *
 * Default value: 0
 */
#ifndef JERRY_LOGGING
#define JERRY_LOGGING 0
#endif /* !defined (JERRY_LOGGING) */

/**
 * Enable/Disable gc call before every allocation.
 *
 * Allowed values:
 *  0: Disable gc call before each allocation.
 *  1: Enable and force gc call before each allocation.
 *
 * Default value: 0
 * Warning!: This is an advanced option and will slow down the engine!
 *           Only enable it for debugging purposes.
 */
#ifndef JERRY_MEM_GC_BEFORE_EACH_ALLOC
#define JERRY_MEM_GC_BEFORE_EACH_ALLOC 0
#endif /* !defined (JERRY_MEM_GC_BEFORE_EACH_ALLOC) */

/**
 * Enable/Disable the collection if run-time memory statistics.
 *
 * Allowed values:
 *  0: Disable run-time memory information collection.
 *  1: Enable run-time memory statistics collection.
 *
 * Default value: 0
 */
#ifndef JERRY_MEM_STATS
#define JERRY_MEM_STATS 0
#endif /* !defined (JERRY_MEM_STATS) */

/**
 * Use 32-bit/64-bit float for ecma-numbers
 * This option is for expert use only!
 *
 * Allowed values:
 *  1: use 64-bit floating point number mode
 *  0: use 32-bit floating point number mode
 *
 * Default value: 1
 */
#ifndef JERRY_NUMBER_TYPE_FLOAT64
#define JERRY_NUMBER_TYPE_FLOAT64 1
#endif /* !defined (JERRY_NUMBER_TYPE_FLOAT64 */

/**
 * Enable/Disable the JavaScript parser.
 *
 * Allowed values:
 *  0: Disable the JavaScript parser and all related functionallity.
 *  1: Enable the JavaScript parser.
 *
 * Default value: 1
 */
#ifndef JERRY_PARSER
#define JERRY_PARSER 1
#endif /* !defined (JERRY_PARSER) */

/**
 * Enable/Disable JerryScript byte code dump functions during parsing.
 * To dump the JerryScript byte code the engine must be initialized with opcodes
 * display flag. This option does not influence RegExp byte code dumps.
 *
 * Allowed values:
 *  0: Disable all bytecode dump functions.
 *  1: Enable bytecode dump functions.
 *
 * Default value: 0
 */
#ifndef JERRY_PARSER_DUMP_BYTE_CODE
#define JERRY_PARSER_DUMP_BYTE_CODE 0
#endif /* defined (JERRY_PARSER_DUMP_BYTE_CODE) */

/**
 * Enable/Disable ECMA property hashmap.
 *
 * Allowed values:
 *  0: Disable property hasmap.
 *  1: Enable property hashmap.
 *
 * Default value: 1
 */
#ifndef JERRY_PROPERTY_HASHMAP
#define JERRY_PROPERTY_HASHMAP 1
#endif /* !defined (JERRY_PROPERTY_HASHMAP) */

/**
 * Enables/disables the Promise event callbacks
 *
 * Default value: 0
 */
#ifndef JERRY_PROMISE_CALLBACK
#define JERRY_PROMISE_CALLBACK 0
#endif /* !defined (JERRY_PROMISE_CALLBACK) */

/**
 * Enable/Disable byte code dump functions for RegExp objects.
 * To dump the RegExp byte code the engine must be initialized with
 * regexp opcodes display flag. This option does not influence the
 * JerryScript byte code dumps.
 *
 * Allowed values:
 *  0: Disable all bytecode dump functions.
 *  1: Enable bytecode dump functions.
 *
 * Default value: 0
 */
#ifndef JERRY_REGEXP_DUMP_BYTE_CODE
#define JERRY_REGEXP_DUMP_BYTE_CODE 0
#endif /* !defined (JERRY_REGEXP_DUMP_BYTE_CODE) */

/**
 * Enables/disables the RegExp strict mode
 *
 * Default value: 0
 */
#ifndef JERRY_REGEXP_STRICT_MODE
#define JERRY_REGEXP_STRICT_MODE 0
#endif /* !defined (JERRY_REGEXP_STRICT_MODE) */

/**
 * Enable/Disable the snapshot execution functions.
 *
 * Allowed values:
 *  0: Disable snapshot execution.
 *  1: Enable snapshot execution.
 *
 * Default value: 0
 */
#ifndef JERRY_SNAPSHOT_EXEC
#define JERRY_SNAPSHOT_EXEC 0
#endif /* !defined (JERRY_SNAPSHOT_EXEC) */

/**
 * Enable/Disable the snapshot save functions.
 *
 * Allowed values:
 *  0: Disable snapshot save functions.
 *  1: Enable snapshot save functions.
 */
#ifndef JERRY_SNAPSHOT_SAVE
#define JERRY_SNAPSHOT_SAVE 0
#endif /* !defined (JERRY_SNAPSHOT_SAVE) */

/**
 * Enable/Disable usage of system allocator.
 *
 * Allowed values:
 *  0: Disable usage of system allocator.
 *  1: Enable usage of system allocator.
 *
 * Default value: 0
 */
#ifndef JERRY_SYSTEM_ALLOCATOR
#define JERRY_SYSTEM_ALLOCATOR 0
#endif /* !defined (JERRY_SYSTEM_ALLOCATOR) */

/**
 * Enables/disables the unicode case conversion in the engine.
 * By default Unicode case conversion is enabled.
 */
#ifndef JERRY_UNICODE_CASE_CONVERSION
#define JERRY_UNICODE_CASE_CONVERSION 1
#endif /* !defined (JERRY_UNICODE_CASE_CONVERSION) */

/**
 * Configures if the internal memory allocations are exposed to Valgrind or not.
 *
 * Allowed values:
 *  0: Disable the Valgrind specific memory allocation notifications.
 *  1: Enable the Valgrind specific allocation notifications.
 */
#ifndef JERRY_VALGRIND
#define JERRY_VALGRIND 0
#endif /* !defined (JERRY_VALGRIND) */

/**
 * Enable/Disable the vm execution stop callback function.
 *
 * Allowed values:
 *  0: Disable vm exec stop callback support.
 *  1: Enable vm exec stop callback support.
 */
#ifndef JERRY_VM_HALT
#define JERRY_VM_HALT 0
#endif /* !defined (JERRY_VM_HALT) */

/**
 * Enable/Disable the vm throw callback function.
 *
 * Allowed values:
 *  0: Disable vm throw callback support.
 *  1: Enable vm throw callback support.
 */
#ifndef JERRY_VM_THROW
#define JERRY_VM_THROW 0
#endif /* !defined (JERRY_VM_THROW) */

/**
 * Advanced section configurations.
 */

/**
 * Allow configuring attributes on a few constant data inside the engine.
 *
 * One of the main usages:
 * Normally compilers store const(ant)s in ROM. Thus saving RAM.
 * But if your compiler does not support it then the directive below can force it.
 *
 * For the moment it is mainly meant for the following targets:
 *      - ESP8266
 *
 * Example configuration for moving (some) constatns into a given section:
 *  # define JERRY_ATTR_CONST_DATA __attribute__((section(".rodata.const")))
 */
#ifndef JERRY_ATTR_CONST_DATA
#define JERRY_ATTR_CONST_DATA
#endif /* !defined (JERRY_ATTR_CONST_DATA) */

/**
 * The JERRY_ATTR_GLOBAL_HEAP allows adding extra attributes for the Jerry global heap.
 *
 * Example on how to move the global heap into it's own section:
 *   #define JERRY_ATTR_GLOBAL_HEAP __attribute__((section(".text.globalheap")))
 */
#ifndef JERRY_ATTR_GLOBAL_HEAP
#define JERRY_ATTR_GLOBAL_HEAP
#endif /* !defined (JERRY_ATTR_GLOBAL_HEAP) */

/**
 * Sanity check for macros to see if the values are 0 or 1
 *
 * If a new feature is added this should be updated.
 */
/**
 * Check base builtins.
 */
#if (JERRY_BUILTIN_ANNEXB != 0) && (JERRY_BUILTIN_ANNEXB != 1)
#error "Invalid value for JERRY_BUILTIN_ANNEXB macro."
#endif /* (JERRY_BUILTIN_ANNEXB != 0) && (JERRY_BUILTIN_ANNEXB != 1) */
#if (JERRY_BUILTIN_ARRAY != 0) && (JERRY_BUILTIN_ARRAY != 1)
#error "Invalid value for JERRY_BUILTIN_ARRAY macro."
#endif /* (JERRY_BUILTIN_ARRAY != 0) && (JERRY_BUILTIN_ARRAY != 1) */
#if (JERRY_BUILTIN_BOOLEAN != 0) && (JERRY_BUILTIN_BOOLEAN != 1)
#error "Invalid value for JERRY_BUILTIN_BOOLEAN macro."
#endif /* (JERRY_BUILTIN_BOOLEAN != 0) && (JERRY_BUILTIN_BOOLEAN != 1) */
#if (JERRY_BUILTIN_DATE != 0) && (JERRY_BUILTIN_DATE != 1)
#error "Invalid value for JERRY_BUILTIN_DATE macro."
#endif /* (JERRY_BUILTIN_DATE != 0) && (JERRY_BUILTIN_DATE != 1) */
#if (JERRY_BUILTIN_ERRORS != 0) && (JERRY_BUILTIN_ERRORS != 1)
#error "Invalid value for JERRY_BUILTIN_ERRORS macro."
#endif /* (JERRY_BUILTIN_ERRORS != 0) && (JERRY_BUILTIN_ERRORS != 1) */
#if (JERRY_BUILTIN_JSON != 0) && (JERRY_BUILTIN_JSON != 1)
#error "Invalid value for JERRY_BUILTIN_JSON macro."
#endif /* (JERRY_BUILTIN_JSON != 0) && (JERRY_BUILTIN_JSON != 1) */
#if (JERRY_BUILTIN_MATH != 0) && (JERRY_BUILTIN_MATH != 1)
#error "Invalid value for JERRY_BUILTIN_MATH macro."
#endif /* (JERRY_BUILTIN_MATH != 0) && (JERRY_BUILTIN_MATH != 1) */
#if (JERRY_BUILTIN_NUMBER != 0) && (JERRY_BUILTIN_NUMBER != 1)
#error "Invalid value for JERRY_BUILTIN_NUMBER macro."
#endif /* (JERRY_BUILTIN_NUMBER != 0) && (JERRY_BUILTIN_NUMBER != 1) */
#if (JERRY_BUILTIN_REGEXP != 0) && (JERRY_BUILTIN_REGEXP != 1)
#error "Invalid value for JERRY_BUILTIN_REGEXP macro."
#endif /* (JERRY_BUILTIN_REGEXP != 0) && (JERRY_BUILTIN_REGEXP != 1) */
#if (JERRY_BUILTIN_STRING != 0) && (JERRY_BUILTIN_STRING != 1)
#error "Invalid value for JERRY_BUILTIN_STRING macro."
#endif /* (JERRY_BUILTIN_STRING != 0) && (JERRY_BUILTIN_STRING != 1) */
#if (JERRY_BUILTINS != 0) && (JERRY_BUILTINS != 1)
#error "Invalid value for JERRY_BUILTINS macro."
#endif /* (JERRY_BUILTINS != 0) && (JERRY_BUILTINS != 1) */
#if (JERRY_BUILTIN_REALMS != 0) && (JERRY_BUILTIN_REALMS != 1)
#error "Invalid value for JERRY_BUILTIN_REALMS macro."
#endif /* (JERRY_BUILTIN_REALMS != 0) && (JERRY_BUILTIN_REALMS != 1) */
#if (JERRY_BUILTIN_DATAVIEW != 0) && (JERRY_BUILTIN_DATAVIEW != 1)
#error "Invalid value for JERRY_BUILTIN_DATAVIEW macro."
#endif /* (JERRY_BUILTIN_DATAVIEW != 0) && (JERRY_BUILTIN_DATAVIEW != 1) */
#if (JERRY_BUILTIN_GLOBAL_THIS != 0) && (JERRY_BUILTIN_GLOBAL_THIS != 1)
#error "Invalid value for JERRY_BUILTIN_GLOBAL_THIS macro."
#endif /* (JERRY_BUILTIN_GLOBAL_THIS != 0) && (JERRY_BUILTIN_GLOBAL_THIS != 1) */
#if (JERRY_BUILTIN_REFLECT != 0) && (JERRY_BUILTIN_REFLECT != 1)
#error "Invalid value for JERRY_BUILTIN_REFLECT macro."
#endif /* (JERRY_BUILTIN_REFLECT != 0) && (JERRY_BUILTIN_REFLECT != 1) */
#if (JERRY_BUILTIN_WEAKREF != 0) && (JERRY_BUILTIN_WEAKREF != 1)
#error "Invalid value for JERRY_BUILTIN_WEAKREF macro."
#endif /* (JERRY_BUILTIN_WEAKREF != 0) && (JERRY_BUILTIN_WEAKREF != 1) */
#if (JERRY_BUILTIN_PROXY != 0) && (JERRY_BUILTIN_PROXY != 1)
#error "Invalid value for JERRY_BUILTIN_PROXY macro."
#endif /* (JERRY_BUILTIN_PROXY != 0) && (JERRY_BUILTIN_PROXY != 1) */
#if (JERRY_BUILTIN_TYPEDARRAY != 0) && (JERRY_BUILTIN_TYPEDARRAY != 1)
#error "Invalid value for JERRY_BUILTIN_TYPEDARRAY macro."
#endif /* (JERRY_BUILTIN_TYPEDARRAY != 0) && (JERRY_BUILTIN_TYPEDARRAY != 1) */
#if (JERRY_BUILTIN_SHAREDARRAYBUFFER != 0) && (JERRY_BUILTIN_SHAREDARRAYBUFFER != 1)
#error "Invalid value for JERRY_BUILTIN_SHAREDARRAYBUFFER macro."
#endif /* (JERRY_BUILTIN_SHAREDARRAYBUFFER != 0) && (JERRY_BUILTIN_SHAREDARRAYBUFFER != 1) */
#if (JERRY_BUILTIN_ATOMICS != 0) && (JERRY_BUILTIN_ATOMICS != 1)
#error "Invalid value for JERRY_BUILTIN_ATOMICS macro."
#endif /* (JERRY_BUILTIN_ATOMICS != 0) && (JERRY_BUILTIN_ATOMICS != 1) */
#if (JERRY_BUILTIN_BIGINT != 0) && (JERRY_BUILTIN_BIGINT != 1)
#error "Invalid value for JERRY_BUILTIN_BIGINT macro."
#endif /* (JERRY_BUILTIN_BIGINT != 0) && (JERRY_BUILTIN_BIGINT != 1) */
#if (JERRY_MODULE_SYSTEM != 0) && (JERRY_MODULE_SYSTEM != 1)
#error "Invalid value for JERRY_MODULE_SYSTEM macro."
#endif /* (JERRY_MODULE_SYSTEM != 0) && (JERRY_MODULE_SYSTEM != 1) */
#if (JERRY_BUILTIN_TYPEDARRAY == 0) && (JERRY_BUILTIN_SHAREDARRAYBUFFER == 1)
#error "JERRY_BUILTIN_TYPEDARRAY should be enabled too to enable JERRY_BUILTIN_SHAREDARRAYBUFFER macro."
#endif /* (JERRY_BUILTIN_TYPEDARRAY == 0) && (JERRY_BUILTIN_SHAREDARRAYBUFFER == 1) */
#if (JERRY_BUILTIN_SHAREDARRAYBUFFER == 0) && (JERRY_BUILTIN_ATOMICS == 1)
#error "JERRY_BUILTIN_SHAREDARRAYBUFFER should be enabled too to enable JERRY_BUILTIN_ATOMICS macro."
#endif /* (JERRY_BUILTIN_SHAREDARRAYBUFFER == 0) && (JERRY_BUILTIN_ATOMICS == 1) */

/**
 * Internal options.
 */
#if (JERRY_CPOINTER_32_BIT != 0) && (JERRY_CPOINTER_32_BIT != 1)
#error "Invalid value for 'JERRY_CPOINTER_32_BIT' macro."
#endif /* (JERRY_CPOINTER_32_BIT != 0) && (JERRY_CPOINTER_32_BIT != 1) */
#if (JERRY_DEBUGGER != 0) && (JERRY_DEBUGGER != 1)
#error "Invalid value for 'JERRY_DEBUGGER' macro."
#endif /* (JERRY_DEBUGGER != 0) && (JERRY_DEBUGGER != 1) */
#if (JERRY_ERROR_MESSAGES != 0) && (JERRY_ERROR_MESSAGES != 1)
#error "Invalid value for 'JERRY_ERROR_MESSAGES' macro."
#endif /* (JERRY_ERROR_MESSAGES != 0) && (JERRY_ERROR_MESSAGES != 1) */
#if (JERRY_EXTERNAL_CONTEXT != 0) && (JERRY_EXTERNAL_CONTEXT != 1)
#error "Invalid value for 'JERRY_EXTERNAL_CONTEXT' macro."
#endif /* (JERRY_EXTERNAL_CONTEXT != 0) && (JERRY_EXTERNAL_CONTEXT != 1) */
#if JERRY_GLOBAL_HEAP_SIZE <= 0
#error "Invalid value for 'JERRY_GLOBAL_HEAP_SIZE' macro."
#endif /* JERRY_GLOBAL_HEAP_SIZE <= 0 */
#if JERRY_GC_LIMIT < 0
#error "Invalid value for 'JERRY_GC_LIMIT' macro."
#endif /* JERRY_GC_LIMIT < 0 */
#if JERRY_STACK_LIMIT < 0
#error "Invalid value for 'JERRY_STACK_LIMIT' macro."
#endif /* JERRY_STACK_LIMIT < 0 */
#if JERRY_GC_MARK_LIMIT < 0
#error "Invalid value for 'JERRY_GC_MARK_LIMIT' macro."
#endif /* JERRY_GC_MARK_LIMIT < 0 */
#if (JERRY_LCACHE != 0) && (JERRY_LCACHE != 1)
#error "Invalid value for 'JERRY_LCACHE' macro."
#endif /* (JERRY_LCACHE != 0) && (JERRY_LCACHE != 1) */
#if (JERRY_FUNCTION_TO_STRING != 0) && (JERRY_FUNCTION_TO_STRING != 1)
#error "Invalid value for 'JERRY_FUNCTION_TO_STRING' macro."
#endif /* (JERRY_FUNCTION_TO_STRING != 0) && (JERRY_FUNCTION_TO_STRING != 1) */
#if (JERRY_LINE_INFO != 0) && (JERRY_LINE_INFO != 1)
#error "Invalid value for 'JERRY_LINE_INFO' macro."
#endif /* (JERRY_LINE_INFO != 0) && (JERRY_LINE_INFO != 1) */
#if (JERRY_LOGGING != 0) && (JERRY_LOGGING != 1)
#error "Invalid value for 'JERRY_LOGGING' macro."
#endif /* (JERRY_LOGGING != 0) && (JERRY_LOGGING != 1) */
#if (JERRY_MEM_GC_BEFORE_EACH_ALLOC != 0) && (JERRY_MEM_GC_BEFORE_EACH_ALLOC != 1)
#error "Invalid value for 'JERRY_MEM_GC_BEFORE_EACH_ALLOC' macro."
#endif /* (JERRY_MEM_GC_BEFORE_EACH_ALLOC != 0) && (JERRY_MEM_GC_BEFORE_EACH_ALLOC != 1) */
#if (JERRY_MEM_STATS != 0) && (JERRY_MEM_STATS != 1)
#error "Invalid value for 'JERRY_MEM_STATS' macro."
#endif /* (JERRY_MEM_STATS != 0) && (JERRY_MEM_STATS != 1) */
#if (JERRY_NUMBER_TYPE_FLOAT64 != 0) && (JERRY_NUMBER_TYPE_FLOAT64 != 1)
#error "Invalid value for 'JERRY_NUMBER_TYPE_FLOAT64' macro."
#endif /* (JERRY_NUMBER_TYPE_FLOAT64 != 0) && (JERRY_NUMBER_TYPE_FLOAT64 != 1) */
#if (JERRY_PARSER != 0) && (JERRY_PARSER != 1)
#error "Invalid value for 'JERRY_PARSER' macro."
#endif /* (JERRY_PARSER != 0) && (JERRY_PARSER != 1) */
#if (JERRY_PARSER_DUMP_BYTE_CODE != 0) && (JERRY_PARSER_DUMP_BYTE_CODE != 1)
#error "Invalid value for 'JERRY_PARSER_DUMP_BYTE_CODE' macro."
#endif /* (JERRY_PARSER_DUMP_BYTE_CODE != 0) && (JERRY_PARSER_DUMP_BYTE_CODE != 1) */
#if (JERRY_PROPERTY_HASHMAP != 0) && (JERRY_PROPERTY_HASHMAP != 1)
#error "Invalid value for 'JERRY_PROPERTY_HASHMAP' macro."
#endif /* (JERRY_PROPERTY_HASHMAP != 0) && (JERRY_PROPERTY_HASHMAP != 1) */
#if (JERRY_PROMISE_CALLBACK != 0) && (JERRY_PROMISE_CALLBACK != 1)
#error "Invalid value for 'JERRY_PROMISE_CALLBACK' macro."
#endif /* (JERRY_PROMISE_CALLBACK != 0) && (JERRY_PROMISE_CALLBACK != 1) */
#if (JERRY_REGEXP_DUMP_BYTE_CODE != 0) && (JERRY_REGEXP_DUMP_BYTE_CODE != 1)
#error "Invalid value for 'JERRY_REGEXP_DUMP_BYTE_CODE' macro."
#endif /* (JERRY_REGEXP_DUMP_BYTE_CODE != 0) && (JERRY_REGEXP_DUMP_BYTE_CODE != 1) */
#if (JERRY_REGEXP_STRICT_MODE != 0) && (JERRY_REGEXP_STRICT_MODE != 1)
#error "Invalid value for 'JERRY_REGEXP_STRICT_MODE' macro."
#endif /* (JERRY_REGEXP_STRICT_MODE != 0) && (JERRY_REGEXP_STRICT_MODE != 1) */
#if (JERRY_SNAPSHOT_EXEC != 0) && (JERRY_SNAPSHOT_EXEC != 1)
#error "Invalid value for 'JERRY_SNAPSHOT_EXEC' macro."
#endif /* (JERRY_SNAPSHOT_EXEC != 0) && (JERRY_SNAPSHOT_EXEC != 1) */
#if (JERRY_SNAPSHOT_SAVE != 0) && (JERRY_SNAPSHOT_SAVE != 1)
#error "Invalid value for 'JERRY_SNAPSHOT_SAVE' macro."
#endif /* (JERRY_SNAPSHOT_SAVE != 0) && (JERRY_SNAPSHOT_SAVE != 1) */
#if (JERRY_SYSTEM_ALLOCATOR != 0) && (JERRY_SYSTEM_ALLOCATOR != 1)
#error "Invalid value for 'JERRY_SYSTEM_ALLOCATOR' macro."
#endif /* (JERRY_SYSTEM_ALLOCATOR != 0) && (JERRY_SYSTEM_ALLOCATOR != 1) */
#if (JERRY_UNICODE_CASE_CONVERSION != 0) && (JERRY_UNICODE_CASE_CONVERSION != 1)
#error "Invalid value for 'JERRY_UNICODE_CASE_CONVERSION' macro."
#endif /* (JERRY_UNICODE_CASE_CONVERSION != 0) && (JERRY_UNICODE_CASE_CONVERSION != 1) */
#if (JERRY_VALGRIND != 0) && (JERRY_VALGRIND != 1)
#error "Invalid value for 'JERRY_VALGRIND' macro."
#endif /* (JERRY_VALGRIND != 0) && (JERRY_VALGRIND != 1) */
#if (JERRY_VM_HALT != 0) && (JERRY_VM_HALT != 1)
#error "Invalid value for 'JERRY_VM_HALT' macro."
#endif /* (JERRY_VM_HALT != 0) && (JERRY_VM_HALT != 1) */
#if (JERRY_VM_THROW != 0) && (JERRY_VM_THROW != 1)
#error "Invalid value for 'JERRY_VM_THROW' macro."
#endif /* (JERRY_VM_THROW != 0) && (JERRY_VM_THROW != 1) */

/**
 * Cross component requirements check.
 */

/**
 * The date module can only use the float 64 number types.
 */
#if JERRY_BUILTIN_DATE && !JERRY_NUMBER_TYPE_FLOAT64
#error "Date does not support float32"
#endif /* JERRY_BUILTIN_DATE && !JERRY_NUMBER_TYPE_FLOAT64 */

/**
 * Source name related types into a single guard
 */
#if JERRY_LINE_INFO || JERRY_ERROR_MESSAGES || JERRY_MODULE_SYSTEM
#define JERRY_SOURCE_NAME 1
#else /* !(JERRY_LINE_INFO || JERRY_ERROR_MESSAGES || JERRY_MODULE_SYSTEM) */
#define JERRY_SOURCE_NAME 0
#endif /* JERRY_LINE_INFO || JERRY_ERROR_MESSAGES || JERRY_MODULE_SYSTEM */

#endif /* !JERRYSCRIPT_CONFIG_H */
