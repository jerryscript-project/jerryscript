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

var NISLFuzzingFunc = function () {
  var hookCallback;
  function hooks() {
      return hookCallback.apply();
  }
  function setHookCallback() {
  }
  function isArray() {
  }
  function isObject() {
  }
  function isObjectEmpty() {
  }
  function isUndefined() {
  }
  function isNumber() {
  }
  function isDate() {
  }
  function map() {
  }
  function hasOwnProp() {
  }
  function extend() {
      for (var i in b) {
          if (hasOwnProp()) {
          }
      }
  }
  function createUTC() {
  }
  function defaultParsingFlags() {
  }
  function getParsingFlags() {
      if (m._pf == null) {
          m._pf = defaultParsingFlags();
      }
  }
  if (Array.prototype.some) {
  } else {
      some = function () {
      };
  }
  function isValid() {
      if (m._isValid == null) {
          var flags = getParsingFlags();
      }
  }
  function createInvalid() {
      var m = createUTC();
      if (flags != null) {
          extend();
      } else {
      }
  }
  var momentProperties = hooks.momentProperties = [];
  function copyConfig() {
      if (!isUndefined()) {
          to._isAMomentObject = from._isAMomentObject;
      }
  }
  var updateInProgress = false;
  function Moment() {
      copyConfig();
      if (updateInProgress === false) {
      }
  }
  function isMoment() {
  }
  function absFloor() {
  }
  function toInt() {
  }
  function compareArrays() {
  }
  function warn() {
  }
  function deprecate() {
  }
  var deprecations = {};
  function deprecateSimple() {
  }
  hooks.suppressDeprecationWarnings = false;
  hooks.deprecationHandler = null;
  function isFunction() {
  }
  function set() {
  }
  function mergeConfigs() {
  }
  function Locale() {
  }
  if (Object.keys) {
  } else {
      keys = function () {
      };
  }
  var defaultCalendar = {
      sameDay: '[Today at] LT',
      nextDay: '[Tomorrow at] LT',
      nextWeek: 'dddd [at] LT',
      lastDay: '[Yesterday at] LT',
      lastWeek: '[Last] dddd [at] LT',
      sameElse: 'L'
  };
  function calendar() {
  }
  var defaultLongDateFormat = {
      LTS: 'h:mm:ss A',
      LT: 'h:mm A',
      L: 'MM/DD/YYYY',
      LL: 'MMMM D, YYYY',
      LLL: 'MMMM D, YYYY h:mm A',
      LLLL: 'dddd, MMMM D, YYYY h:mm A'
  };
  function longDateFormat() {
  }
  var defaultInvalidDate = 'Invalid date';
  function invalidDate() {
  }
  var defaultOrdinal = '%d';
  var defaultDayOfMonthOrdinalParse = /\d{1,2}/;
  function ordinal() {
  }
  var defaultRelativeTime = {
      future: 'in %s',
      past: '%s ago',
      s: 'a few seconds',
      ss: '%d seconds',
      m: 'a minute',
      mm: '%d minutes',
      h: 'an hour',
      hh: '%d hours',
      d: 'a day',
      dd: '%d days',
      M: 'a month',
      MM: '%d months',
      y: 'a year',
      yy: '%d years'
  };
  function relativeTime() {
  }
  function pastFuture() {
  }
  var aliases = {};
  function addUnitAlias() {
  }
  function normalizeUnits() {
  }
  function normalizeObjectUnits() {
  }
  var priorities = {};
  function addUnitPriority() {
  }
  function getPrioritizedUnits() {
  }
  function zeroFill() {
  }
  var formattingTokens = /(\[[^\[]*\])|(\\)?([Hh]mm(ss)?|Mo|MM?M?M?|Do|DDDo|DD?D?D?|ddd?d?|do?|w[o|w]?|W[o|W]?|Qo?|YYYYYY|YYYYY|YYYY|YY|gg(ggg?)?|GG(GGG?)?|e|E|a|A|hh?|HH?|kk?|mm?|ss?|S{1,9}|x|X|zz?|ZZ?|.)/g;
  var localFormattingTokens = /(\[[^\[]*\])|(\\)?(LTS|LT|LL?L?L?|l{1,4})/g;
  var formatFunctions = {};
  var formatTokenFunctions = {};
  function addFormatToken() {
  }
  function removeFormattingTokens() {
  }
  function makeFormatFunction() {
  }
  function formatMoment() {
  }
  function expandFormat() {
  }
  var match1 = /\d/;
  var match2 = /\d\d/;
  var match3 = /\d{3}/;
  var match4 = /\d{4}/;
  var match6 = /[+-]?\d{6}/;
  var match1to2 = /\d\d?/;
  var match3to4 = /\d\d\d\d?/;
  var match5to6 = /\d\d\d\d\d\d?/;
  var match1to3 = /\d{1,3}/;
  var match1to4 = /\d{1,4}/;
  var match1to6 = /[+-]?\d{1,6}/;
  var matchUnsigned = /\d+/;
  var matchSigned = /[+-]?\d+/;
  var matchOffset = /Z|[+-]\d\d:?\d\d/gi;
  var matchShortOffset = /Z|[+-]\d\d(?::?\d\d)?/gi;
  var matchTimestamp = /[+-]?\d+(\.\d{1,3})?/;
  var matchWord = /[0-9]{0,256}['a-z\u00A0-\u05FF\u0700-\uD7FF\uF900-\uFDCF\uFDF0-\uFF07\uFF10-\uFFEF]{1,256}|[\u0600-\u06FF\/]{1,256}(\s*?[\u0600-\u06FF]{1,256}){1,2}/i;
  var regexes = {};
  function addRegexToken() {
  }
  function getParseRegexForToken() {
  }
  function unescapeFormat() {
  }
  function regexEscape() {
  }
  var tokens = {};
  function addParseToken() {
  }
  function addWeekParseToken() {
  }
  function addTimeToArrayFromToken() {
  }
  var YEAR = 0;
  var MONTH = 1;
  var DATE = 2;
  var HOUR = 3;
  var MINUTE = 4;
  var SECOND = 5;
  var MILLISECOND = 6;
  var WEEK = 7;
  var WEEKDAY = 8;
  addFormatToken('Y', function () {
  });
  addFormatToken(['YY'], function () {
  });
  addFormatToken(['YYYY'], 'year');
  addFormatToken(['YYYYY']);
  addFormatToken(['YYYYYY']);
  addParseToken(function () {
  });
  addParseToken(function () {
  });
  addParseToken(function () {
  });
  function daysInYear() {
  }
  function isLeapYear() {
  }
  hooks.parseTwoDigitYear = function () {
  };
  var getSetYear = makeGetSet('FullYear');
  function getIsLeapYear() {
  }
  function makeGetSet() {
  }
  function get() {
  }
  function set$1() {
  }
  function stringGet() {
  }
  function stringSet() {
  }
  function mod() {
  }
  if (Array.prototype.indexOf) {
  } else {
      indexOf = function () {
      };
  }
  function daysInMonth() {
  }
  addFormatToken('Mo', function () {
  });
  addFormatToken('MMM', function () {
  });
  addFormatToken('MMMM', function () {
  });
  addUnitAlias('month');
  addRegexToken(function () {
  });
  addRegexToken(function () {
  });
  addParseToken(function () {
  });
  addParseToken(function () {
  });
  var MONTHS_IN_FORMAT = /D[oD]?(\[[^\[\]]*\]|\s)+MMMM?/;
  var defaultLocaleMonths = 'January_February_March_April_May_June_July_August_September_October_November_December'.split('_');
  function localeMonths() {
  }
  var defaultLocaleMonthsShort = 'Jan_Feb_Mar_Apr_May_Jun_Jul_Aug_Sep_Oct_Nov_Dec'.split();
  function localeMonthsShort() {
      if (!m) {
          return isArray() ? this._monthsShort : this._monthsShort['standalone'];
      }
  }
  function handleStrictParse() {
  }
  function localeMonthsParse() {
  }
  function setMonth() {
  }
  function getSetMonth() {
  }
  function getDaysInMonth() {
  }
  var defaultMonthsShortRegex = matchWord;
  function monthsShortRegex() {
  }
  var defaultMonthsRegex = matchWord;
  function monthsRegex() {
  }
  function computeMonthsParse() {
  }
  function createDate() {
  }
  function createUTCDate() {
  }
  function firstWeekOffset() {
  }
  function dayOfYearFromWeeks() {
  }
  function weekOfYear() {
  }
  function weeksInYear() {
  }
  addFormatToken('w', ['ww'], 'wo', 'week');
  addFormatToken('W', ['WW'], 'Wo', 'isoWeek');
  addWeekParseToken(function () {
  });
  function localeWeek() {
  }
  var defaultLocaleWeek = {
      dow: 0,
      doy: 6
  };
  function localeFirstDayOfWeek() {
  }
  function localeFirstDayOfYear() {
  }
  function getSetWeek() {
  }
  function getSetISOWeek() {
  }
  addFormatToken('do', 'day');
  addFormatToken(function () {
  });
  addFormatToken('ddd', function () {
  });
  addFormatToken('dddd', function () {
  });
  addFormatToken('e', 'weekday');
  addFormatToken('E', 'isoWeekday');
  addRegexToken(function () {
  });
  addRegexToken(function () {
  });
  function parseWeekday() {
  }
  function parseIsoWeekday() {
  }
  var defaultLocaleWeekdays = 'Sunday_Monday_Tuesday_Wednesday_Thursday_Friday_Saturday'.split();
  function localeWeekdays() {
  }
  var defaultLocaleWeekdaysShort = 'Sun_Mon_Tue_Wed_Thu_Fri_Sat'.split();
  function localeWeekdaysShort() {
  }
  var defaultLocaleWeekdaysMin = 'Su_Mo_Tu_We_Th_Fr_Sa'.split();
  function localeWeekdaysMin() {
  }
  function handleStrictParse$1() {
  }
  function getSetLocaleDayOfWeek() {
  }
  var defaultWeekdaysRegex = matchWord;
  function weekdaysRegex() {
  }
  var defaultWeekdaysMinRegex = matchWord;
  function hFormat() {
  }
  addParseToken('hmmss', function () {
  });
  function getSetGlobalLocale() {
  }
  var tzRegex = /Z|[+-]\d\d(?::?\d\d)?/;
  var aspNetJsonRegex = /^\/?Date\((\-?\d+)/i;
  function configFromISO() {
  }
  function configFromStringAndArray() {
  }
  function configFromObject() {
  }
  function createFromConfig() {
  }
  function prepareConfig() {
  }
  var prototypeMin = deprecate('moment().min is deprecated, use moment.max instead. http://momentjs.com/guides/#/warnings/min-max/', function () {
  });
  var prototypeMax = deprecate('moment().max is deprecated, use moment.min instead. http://momentjs.com/guides/#/warnings/min-max/', function () {
  });
  function pickBy() {
  }
  function max() {
  }
  var now = function () {
  };
  var ordering = [];
  function isValid$1() {
  }
  function offset() {
  }
  function offsetFromString() {
  }
  function cloneWithOffset() {
  }
  function getSetZone() {
  }
  function setOffsetToUTC() {
  }
  function setOffsetToLocal() {
  }
  function hasAlignedHourOffset() {
  }
  function isDaylightSavingTime() {
  }
  function isDaylightSavingTimeShifted() {
  }
  function createAdder() {
  }
  function addSubtract() {
  }
  var add = createAdder();
  var subtract = createAdder();
  function getCalendarFormat() {
  }
  function calendar$1() {
  }
  function clone() {
  }
  function isAfter() {
  }
  function isBefore() {
  }
  function isBetween() {
  }
  function isSame() {
  }
  function isSameOrAfter() {
  }
  function isSameOrBefore() {
  }
  function diff() {
  }
  hooks.defaultFormat = 'YYYY-MM-DDTHH:mm:ssZ';
  function toString() {
  }
  function inspect() {
  }
  function format() {
  }
  function from() {
  }
  function fromNow() {
  }
  function to() {
  }
  function toNow() {
  }
  function locale() {
  }
  var lang = deprecate('moment().lang() is deprecated. Instead, use moment().localeData() to get the language configuration. Use moment().locale() to change languages.', function () {
  });
  function localeData() {
  }
  function startOf() {
  }
  function endOf() {
  }
  function valueOf() {
  }
  function unix() {
  }
  function toDate() {
  }
  function toArray() {
  }
  function toObject() {
  }
  function toJSON() {
  }
  function isValid$2() {
  }
  function parsingFlags() {
  }
  function invalidAt() {
  }
  var proto = Moment.prototype;
  proto.parsingFlags = parsingFlags;
  return "42"
};
var NISLCallingResult = NISLFuzzingFunc();
assert(NISLCallingResult === "42");
