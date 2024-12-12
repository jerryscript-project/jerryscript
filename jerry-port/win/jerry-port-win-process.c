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
#include "jerryscript-port.h"

#if defined(_WIN32)

#include <windows.h>

#include <crtdbg.h>
#include <stdio.h>

void
jerry_port_init (void)
{
  if (!IsDebuggerPresent ())
  {
    /* Make output streams unbuffered by default */
    setvbuf (stdout, NULL, _IONBF, 0);
    setvbuf (stderr, NULL, _IONBF, 0);

    /*
     * By default abort() only generates a crash-dump in *non* debug
     * builds. As our Assert() / ExceptionalCondition() uses abort(),
     * leaving the default in place would make debugging harder.
     *
     * MINGW's own C runtime doesn't have _set_abort_behavior(). When
     * targeting Microsoft's UCRT with mingw, it never links to the debug
     * version of the library and thus doesn't need the call to
     * _set_abort_behavior() either.
     */
#if !defined(__MINGW32__) && !defined(__MINGW64__)
    _set_abort_behavior (_CALL_REPORTFAULT | _WRITE_ABORT_MSG, _CALL_REPORTFAULT | _WRITE_ABORT_MSG);
#endif /* !defined(__MINGW32__) && !defined(__MINGW64__) */

    /*
     * SEM_FAILCRITICALERRORS causes more errors to be reported to
     * callers.
     *
     * We used to also specify SEM_NOGPFAULTERRORBOX, but that prevents
     * windows crash reporting from working. Which includes registered
     * just-in-time debuggers, making it unnecessarily hard to debug
     * problems on windows. Now we try to disable sources of popups
     * separately below (note that SEM_NOGPFAULTERRORBOX did not actually
     * prevent all sources of such popups).
     */
    SetErrorMode (SEM_FAILCRITICALERRORS);

    /*
     * Show errors on stderr instead of popup box (note this doesn't
     * affect errors originating in the C runtime, see below).
     */
    _set_error_mode (_OUT_TO_STDERR);

    /*
     * In DEBUG builds, errors, including assertions, C runtime errors are
     * reported via _CrtDbgReport. By default such errors are displayed
     * with a popup (even with NOGPFAULTERRORBOX), preventing forward
     * progress. Instead report such errors stderr (and the debugger).
     * This is C runtime specific and thus the above incantations aren't
     * sufficient to suppress these popups.
     */
    _CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile (_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode (_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile (_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode (_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile (_CRT_WARN, _CRTDBG_FILE_STDERR);
  }
} /* jerry_port_init */

void
jerry_port_sleep (uint32_t sleep_time)
{
  Sleep (sleep_time);
} /* jerry_port_sleep */

#endif /* defined(_WIN32) */
