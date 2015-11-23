///////////////////////////////////////////
// helper functions
///////////////////////////////////////////

Array.prototype.clone = function() {
  return this.slice(0);
};

Array.prototype.sum = function() {
  return this.reduce(function(a, b) { return a + b; });
};

Array.prototype.max = function() {
  return this.reduce(function(a, b) { return Math.max(a, b); });
};

Date.prototype.toFormattedString = function() {
  var yyyy = this.getFullYear().toString();
  var mm = (this.getMonth() + 1).toString();
  var dd = this.getDate().toString();
  mm = mm[1] ? mm : '0' + mm;
  dd = dd[1] ? dd : '0' + dd;
  return yyyy + '-' + mm + '-' + dd;
};

Object.values = function(obj) {
  return Object.keys(obj).map(function(key) { return obj[key]; });
};

function getParameterByName(name) {
  name = name.replace(/[\[]/, "\\[").replace(/[\]]/, "\\]");
  var regex = new RegExp("[\\?&]" + name + "=([^&#]*)"),
      results = regex.exec(location.search);
  return results === null ? "" : decodeURIComponent(results[1].replace(/\+/g, " "));
}

function wrapTooltip(content) {
  return '<div style="padding: 0.5em; font-family: consolas; line-height: 1.5em;">' + content + '</div>';
}

function wrapHyperlink(address, text) {
  return '<a href="' + address + '" target="_blank">' + text + '</a>'
}

///////////////////////////////////////////
// main module
///////////////////////////////////////////

var isSum = true;

var benchmarks = ['sunspider-1.0.2', 'ubench'];
var measureTypes = ['memory', 'performance'];
var measureUnits = {'memory': 'kb', 'performance': 's'};
var engines = ['jerryscript', 'jerryscript-snapshot', 'duktape'];
var link_main = {
  'jerryscript': 'http://www.jerryscript.net',
  'duktape':     'http://duktape.org'
};
var link_code = {
  'jerryscript': 'https://github.com/Samsung/jerryscript/commit/',
  'duktape':     'https://github.com/svaarala/duktape/commit/'
};
var beginDate = new Date('2015-07-10');

var benchmarkData = {};

google.load('visualization', '1', {packages: ['corechart', 'line']});
google.setOnLoadCallback(main);

function main() {
  // get params
  var data_src = getParameterByName('src') || 'data';
  var show = getParameterByName('show') || 'sum';

  isSum = show === 'sum';

  // fetch data via ajax
  var today = new Date();
  for (var d = beginDate; d <= today; d.setDate(d.getDate() + 1)) {
    var curDate = (new Date(d)).toFormattedString();
    $.ajax({
      _date: curDate,
      url: data_src + '/' + curDate + '.json',
      dataType: 'json',
      success: function(data) {
        benchmarkData[this._date] = data;
      },
      error: function(request, status, error) {
      }
    });
  }
}

$(document).ajaxStop(function () {
  benchmarks.forEach(function(benchmark) {
  measureTypes.forEach(function(measureType) {
    // transform data for internal use
    var transData = {}, transInfo = {};
    $.each(benchmarkData, function(date, element) {
      transData[date] = [];
      transInfo[date] = element['info'];
      engines.forEach(function(engine, index) {
        var repVal = undefined;   // default value
        var numTests = 0;
        var benchmark_obj = element[benchmark];
        if (benchmark_obj) {
          var record =  benchmark_obj[measureType][engine];
          if (record) {
            // record object contains a lot of subtests,
            // we use sum of all values as representative value
            var values = Object.values(record);
            repVal = isSum ? values.sum() : values.max();
            numTests = values.length;
          }
        }
        transData[date][index] = {tests: numTests, score: repVal};
      });
    });

    // transform data for google charts
    var data = new google.visualization.DataTable();
    data.addColumn('date', 'Date');
    engines.forEach(function(engine) {
      data.addColumn('number', engine);
      data.addColumn({type: 'string', role: 'tooltip', 'p': {'html': true}});
    });
    var arrayData = Object.keys(transData).sort().map(function(date) {
      // data
      var row = Object.values(transData[date]);
      // tooltip
      var tooltips = row.clone().map(function(value, index) {
        var engine = engines[index];
        var engine_pure = engine.split('-')[0];
        var info = transInfo[date] ? transInfo[date][engine_pure] : undefined;
        var score = value.score ? value.score.toFixed(2) + measureUnits[measureType] : '';
        var tests = value.tests ? value.tests : 0;
        var info_text = '';
        if (info && info.version)
          info_text = wrapHyperlink(link_code[engine_pure] + info.version, info.version);
        var engine_text = wrapHyperlink(link_main[engine_pure], engine);
        var textData = [
            ['source&nbsp; ', engine_text],
            ['version ', info_text],
            ['date&nbsp;&nbsp;&nbsp; ', date],
            ['score&nbsp;&nbsp; ', score + ' (' + tests + ' subtests)']];
        return wrapTooltip(textData.map(function(v) { return v.join(': '); }).join('<br />'));
      });

      // zip data and tooltips
      // so the array will be like [data, tooltip, data, tooltip, ...]
      row = row.map(function (v, i) {
        return [v.score, tooltips[i]];
      }).reduce(function(a, b) {
        return a.concat(b)
      });
      return [new Date(date)].concat(row);
    });
    data.addRows(arrayData);

    // chart options
    var options = {
      title: measureType,
      titleTextStyle: {
        fontSize: 20,
        bold: true
      },
      legend: { position: 'bottom' },
      backgroundColor: '#f8f8f8',
      hAxis: {
        title: 'Date',
        textStyle: {
          fontSize: 12,
          bold: false
        },
        titleTextStyle: {
          fontSize: 16,
          bold: true
        },
        format: 'yyyy-MM-dd'
      },
      vAxis: {
        minValue: 0,
        title: measureType + ' (' + measureUnits[measureType] + ')',
        textStyle: {
          fontSize: 12,
          bold: false
        },
        titleTextStyle: {
          fontSize: 16,
          bold: true
        }
      },
      tooltip: {
        isHtml: true,
        trigger: 'both'
      }
    };

    // draw chart
    var divObj = document.getElementById(
        ['chart', benchmark, measureType].join('_'));
    if (divObj) {
      var chart = new google.visualization.LineChart(divObj);
      chart.draw(data, options);
    }
  });
  });
});
