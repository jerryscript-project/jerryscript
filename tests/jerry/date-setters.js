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

var ms = 1;
var sec = 1000 * ms;
var min = 60 * sec;
var hour = 60 * min;
var day = 24 * hour; /* 86400000 */
var d = new Date();

/* 15.9.5.27 Date.prototype.setTime (time) */
assert (d.setTime(0) == 0);
d.setTime(0);
assert (d.setTime(day) == day);
assert (d.getDate() == 2);

/* 15.9.5.28 Date.prototype.setMilliseconds (ms) */
d.setTime(0);
assert (d.setMilliseconds(1) == ms);
assert (d.getMilliseconds() == 1);

/* 15.9.5.29 Date.prototype.setUTCMilliseconds (ms) */
d.setTime(0);
assert (d.setUTCMilliseconds(1) == ms);
assert (d.getUTCMilliseconds() == 1);

/* 15.9.5.30 Date.prototype.setSeconds (sec [, ms ] ) */
d.setTime(0);
assert (d.setSeconds(1) == sec);
assert (d.getSeconds() == 1);
d.setTime(0);
assert (d.setSeconds(1, 1) == sec + ms);
assert (d.getSeconds() == 1);
assert (d.getMilliseconds() == 1);

/* 15.9.5.31 Date.prototype.setUTCSeconds (sec [, ms ] ) */
d.setTime(0);
assert (d.setUTCSeconds(1) == sec);
assert (d.getUTCSeconds() == 1);
d.setTime(0);
assert (d.setUTCSeconds(1, 1) == sec + ms);
assert (d.getUTCSeconds() == 1);
assert (d.getUTCMilliseconds() == 1);

/* 15.9.5.32 Date.prototype.setMinutes (min [, sec [, ms ] ] ) */
d.setTime(0);
assert (d.setMinutes(1) == min);
assert (d.getMinutes() == 1);
d.setTime(0);
assert (d.setMinutes(1, 1) == min + sec);
assert (d.getMinutes() == 1);
assert (d.getSeconds() == 1);
d.setTime(0);
assert (d.setMinutes(1, 1, 1) == min + sec + ms);
assert (d.getMinutes() == 1);
assert (d.getSeconds() == 1);
assert (d.getMilliseconds() == 1);

/* 15.9.5.33 Date.prototype.setUTCMinutes (min [, sec [, ms ] ] ) */
d.setTime(0);
assert (d.setUTCMinutes(1) == min);
assert (d.getUTCMinutes() == 1);
d.setTime(0);
assert (d.setUTCMinutes(1, 1) == min + sec);
assert (d.getUTCMinutes() == 1);
assert (d.getUTCSeconds() == 1);
d.setTime(0);
assert (d.setUTCMinutes(1, 1, 1) == min + sec + ms);
assert (d.getUTCMinutes() == 1);
assert (d.getUTCSeconds() == 1);
assert (d.getUTCMilliseconds() == 1);

/* 15.9.5.34 Date.prototype.setHours (hour [, min [, sec [, ms ] ] ] ) */
d.setTime(0);
assert (d.setHours(1) == hour + d.getTimezoneOffset() * 60000);
assert (d.getHours() == 1);
d.setTime(0);
assert (d.setHours(1, 1) == hour + min + d.getTimezoneOffset() * 60000);
assert (d.getHours() == 1);
assert (d.getMinutes() == 1);
d.setTime(0);
assert (d.setHours(1, 1, 1) == hour + min + sec + d.getTimezoneOffset() * 60000);
assert (d.getHours() == 1);
assert (d.getMinutes() == 1);
assert (d.getSeconds() == 1);
d.setTime(0);
assert (d.setHours(1, 1, 1, 1) == hour + min + sec + ms + d.getTimezoneOffset() * 60000);
assert (d.getHours() == 1);
assert (d.getMinutes() == 1);
assert (d.getSeconds() == 1);
assert (d.getMilliseconds() == 1);

/* 15.9.5.35 Date.prototype.setUTCHours (hour [, min [, sec [, ms ] ] ] ) */
d.setTime(0);
assert (d.setUTCHours(1) == hour);
assert (d.getUTCHours() == 1);
d.setTime(0);
assert (d.setUTCHours(1, 1) == hour + min);
assert (d.getUTCHours() == 1);
assert (d.getUTCMinutes() == 1);
d.setTime(0);
assert (d.setUTCHours(1, 1, 1) == hour + min + sec);
assert (d.getUTCHours() == 1);
assert (d.getUTCMinutes() == 1);
assert (d.getUTCSeconds() == 1);
d.setTime(0);
assert (d.setUTCHours(1, 1, 1, 1) == hour + min + sec + ms);
assert (d.getUTCHours() == 1);
assert (d.getUTCMinutes() == 1);
assert (d.getUTCSeconds() == 1);
assert (d.getUTCMilliseconds() == 1);

/* 15.9.5.36 Date.prototype.setDate (date) */
d.setTime(0);
assert (d.setDate(0) == -day);
assert (d.getDate() == 31);
d.setTime(0);
assert (d.setDate(1) == 0);
assert (d.getDate() == 1);

/* 15.9.5.37 Date.prototype.setUTCDate (date) */
d.setTime(0);
assert (d.setUTCDate(0) == -day);
assert (d.getUTCDate() == 31);
d.setTime(0);
assert (d.setUTCDate(1) == 0);
assert (d.getUTCDate() == 1);

/* 15.9.5.38 Date.prototype.setMonth (month [, date ] ) */
d.setTime(0);
assert (d.setMonth(0) == 0);
assert (d.getMonth() == 0);
d.setTime(0);
assert (d.setMonth(0, 0) == -day);
assert (d.getMonth() == 11);
assert (d.getDate() == 31);
d.setTime(0);
assert (d.setMonth(1) == 31 * day);
assert (d.getMonth() == 1);
d.setTime(0);
assert (d.setMonth(12) == 365 * day);
assert (d.getMonth() == 0);
d.setTime(0);
assert (d.setMonth(13) == (365 + 31) * day);
assert (d.getMonth() == 1);

/* 15.9.5.39 Date.prototype.setUTCMonth (month [, date ] ) */
d.setTime(0);
assert (d.setUTCMonth(0) == 0);
assert (d.getUTCMonth() == 0);
d.setTime(0);
assert (d.setUTCMonth(0, 0) == -day);
assert (d.getUTCMonth() == 11);
assert (d.getUTCDate() == 31);
d.setTime(0);
assert (d.setUTCMonth(1) == 31 * day);
assert (d.getUTCMonth() == 1);
d.setTime(0);
assert (d.setUTCMonth(12) == 365 * day);
assert (d.getUTCMonth() == 0);
d.setTime(0);
assert (d.setUTCMonth(13) == (365 + 31) * day);
assert (d.getUTCMonth() == 1);

/* 15.9.5.40 Date.prototype.setFullYear (year [, month [, date ] ] ) */
d.setTime(0);
assert (d.setFullYear(0) == -62167219200000);
assert (d.getFullYear() == 0);
d.setTime(0);
assert (d.setFullYear(0, 0) == -62167219200000);
assert (d.getFullYear() == 0);
assert (d.getMonth() == 0);
d.setTime(0);
assert (d.setFullYear(0, 0, 0) == -62167219200000 - day);
assert (d.getFullYear() == -1);
assert (d.getMonth() == 11);
assert (d.getDate() == 31);
d.setTime(0);
assert (d.setFullYear(1970) == 0);
assert (d.getFullYear() == 1970);

/* 15.9.5.41 Date.prototype.setUTCFullYear (year [, month [, date ] ] ) */
d.setTime(0);
assert (d.setUTCFullYear(0) == -62167219200000);
assert (d.getUTCFullYear() == 0);
d.setTime(0);
assert (d.setUTCFullYear(0, 0) == -62167219200000);
assert (d.getUTCFullYear() == 0);
assert (d.getUTCMonth() == 0);
d.setTime(0);
assert (d.setUTCFullYear(0, 0, 0) == -62167219200000 - day);
assert (d.getUTCFullYear() == -1);
assert (d.getUTCMonth() == 11);
assert (d.getUTCDate() == 31);
d.setTime(0);
assert (d.setUTCFullYear(1970) == 0);
assert (d.getUTCFullYear() == 1970);

/* ECMA262 v11 20.4.1.2 Day Number and Time within Day
   msPerDay = 86400000
   TimeWithinDay(t) = t modulo msPerDay

   ECMA262 v11 5.2.5 Mathematical Operations
   The notation “x modulo y” (y must be finite and nonzero) computes a value k of the same sign as y (or zero).

   Consequently TimeWithinDay(t) >= 0. It can be tested properly with dates close to 1970.
*/
d = new Date("1969-12-01T01:00:00.000Z");
d.setFullYear(1968);
assert (d.toISOString() == "1968-12-01T01:00:00.000Z");

d = new Date("1970-01-31T01:00:00.000Z");
d.setFullYear(1971);
assert (d.toISOString() == "1971-01-31T01:00:00.000Z");

/* Without argument */
d = new Date();
assert (isNaN (d.setTime()));
assert (isNaN (d.setMilliseconds()));
assert (isNaN (d.setUTCMilliseconds()));
assert (isNaN (d.setSeconds()));
assert (isNaN (d.setUTCSeconds()));
assert (isNaN (d.setMinutes()));
assert (isNaN (d.setUTCMinutes()));
assert (isNaN (d.setHours()));
assert (isNaN (d.setUTCHours()));
assert (isNaN (d.setDate()));
assert (isNaN (d.getUTCDate()));
assert (isNaN (d.setMonth()));
assert (isNaN (d.setUTCMonth()));
assert (isNaN (d.setFullYear()));
assert (isNaN (d.setUTCFullYear()));

var date = new Date('1975-08-19');
var date2 = new Date('1975-08-19');
var date3 = new Date('1975-08-19');
var date4 = new Date('1975-08-19');
var date5 = new Date('1975-08-19');
var date6 = new Date('1975-08-19');
date.setFullYear(275760, 8, 13);
date2.setFullYear(275760, 8, 14);
date3.setFullYear(-271820, 6570968, 13);
date4.setFullYear(-271820, 6570968, 14);
date5.setFullYear(-271821);
date6.setFullYear(-271822);
assert(date.getFullYear() == 275760);
assert(isNaN(date2.getFullYear()));
assert(date3.getFullYear() == 275760);
assert(isNaN(date4.getFullYear()));
assert(date5.getFullYear() == -271821);
assert(isNaN(date6.getFullYear()));
