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
  mov    %eax,(%eax);
  mov    %ebx,0x4(%eax);
  mov    %ecx,0x8(%eax);
  mov    %edx,0xc(%eax);
  mov    %esi,0x10(%eax);
  mov    %edi,0x14(%eax);
  mov    %ebp,0x18(%eax);
  mov    %esp,0x1c(%eax);
  push   %edx;
  mov    0x4(%esp),%edx;
  mov    %edx,0x20(%eax);
  pop    %edx;
  xor    %eax,%eax;
  ret
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
  test   %edx, %edx;
  jne    . + 0x3;
  inc    %edx;
  mov    0x4(%eax),%ebx;
  mov    0x8(%eax),%ecx;
  mov    0x10(%eax),%esi;
  mov    0x14(%eax),%edi;
  mov    0x18(%eax),%ebp;
  mov    0x1c(%eax),%esp;
  push   %edx;
  mov    0x20(%eax),%edx;
  mov    %edx,0x4(%esp);
  mov    0xc(%eax),%edx;
  pop    %eax;
  ret
endfunc longjmp
