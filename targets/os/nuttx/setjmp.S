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

.syntax unified

.macro func _name
.global \_name
.type \_name, %function
\_name:
.endm
.macro endfunc _name
.size \_name, .-\_name
.endm

/**
 * setjmp (jmp_buf env)
 *
 * See also:
 *          longjmp
 *
 * @return 0 - if returns from direct call,
 *         nonzero - if returns after longjmp.
 */
func setjmp
  stmia r0!, {r4 - r11, lr}
  str sp, [r0], #4
  vstm r0, {s16 - s31}
  mov r0, #0
  bx lr
endfunc setjmp

/**
 * longjmp (jmp_buf env, int val)
 *
 * Note:
 *      if val is not 0, then it would be returned from setjmp,
 *      otherwise - 0 would be returned.
 *
 * See also:
 *          setjmp
 */
func longjmp
  ldmia r0!, {r4 - r11, lr}
  ldr sp, [r0]
  add r0, r0, #4
  vldm r0, {s16 - s31}
  mov r0, r1
  cmp r0, #0
  bne 1f
  mov r0, #1
  1:
  bx lr
endfunc longjmp
