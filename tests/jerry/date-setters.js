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

assert (isNaN (Date.prototype.setTime()));
assert (isNaN (Date.prototype.setMilliseconds()));
assert (isNaN (Date.prototype.setUTCMilliseconds()));
assert (isNaN (Date.prototype.setSeconds()));
assert (isNaN (Date.prototype.setUTCSeconds()));
assert (isNaN (Date.prototype.setMinutes()));
assert (isNaN (Date.prototype.setUTCMinutes()));
assert (isNaN (Date.prototype.setHours()));
assert (isNaN (Date.prototype.setUTCHours()));
assert (isNaN (Date.prototype.setDate()));
assert (isNaN (Date.prototype.getUTCDate()));
assert (isNaN (Date.prototype.setMonth()));
assert (isNaN (Date.prototype.setUTCMonth()));
assert (isNaN (Date.prototype.setFullYear()));
assert (isNaN (Date.prototype.setUTCFullYear()));
