/* Copyright 2014 Samsung Electronics Co., Ltd.
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

/* 
 * File:   interpreter.h
 * Author: egavrin
 *
 * Created on July 2, 2014, 3:10 PM
 */

#ifndef INTERPRETER_H
#define	INTERPRETER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opcode.h"

#define FILE_NAME "application.bin"

void safe_opcode(FILE *, opcode_ptr, int, int);
void gen_bytecode(FILE*);
void run_int();

#endif	/* INTERPRETER_H */

