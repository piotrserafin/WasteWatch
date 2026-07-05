var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig, null, { autoHandleEvents: false });
var keys = require('message_keys');

var REQUEST_SCHEDULE = 1;
var DEFAULT_PROXY = 'https://fa4w4vfdrqxaksbqpnggorfzsm0qyhat.lambda-url.eu-central-1.on.aws';
var pendingSchedule = [];
var currentIndex = 0;

function getSettings() {
  return {
    token: (localStorage.getItem('token') || '').trim(),
    proxyUrl: (localStorage.getItem('proxyUrl') || '').trim(),
    streetName: (localStorage.getItem('streetName') || '').trim(),
    numberName: (localStorage.getItem('numberName') || '').trim()
  };
}

function saveSettings(token, proxyUrl, streetName, numberName) {
  localStorage.setItem('token', token);
  localStorage.setItem('proxyUrl', proxyUrl);
  localStorage.setItem('streetName', streetName);
  localStorage.setItem('numberName', numberName);
}

Pebble.addEventListener('ready', function() {
  var s = getSettings();
  if (s.token && s.streetName && s.numberName) {
    var msg = {};
    msg[keys.Token] = s.token;
    msg[keys.StreetName] = s.streetName;
    msg[keys.NumberName] = s.numberName;
    if (s.proxyUrl) msg[keys.ProxyUrl] = s.proxyUrl;
    Pebble.sendAppMessage(msg, function() {
      fetchSchedule(s);
    }, function() {
      fetchSchedule(s);
    });
  } else if (!s.token) {
    sendError('No token.\nOpen Settings.');
  } else {
    sendError('No address.\nOpen Settings.');
  }
});

Pebble.addEventListener('showConfiguration', function() {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e && !e.response) return;
  var dict = clay.getSettings(e.response);

  var token = (dict[keys.Token] || '').trim();
  var proxyUrl = (dict[keys.ProxyUrl] || '').trim();
  var streetName = (dict[keys.StreetName] || '').trim();
  var numberName = (dict[keys.NumberName] || '').trim();

  saveSettings(token, proxyUrl, streetName, numberName);

  var msg = {};
  msg[keys.Token] = token;
  msg[keys.ProxyUrl] = proxyUrl;
  msg[keys.StreetName] = streetName;
  msg[keys.NumberName] = numberName;

  Pebble.sendAppMessage(msg, function() {
    if (token && streetName && numberName) {
      fetchSchedule(getSettings());
    }
  });
});

Pebble.addEventListener('appmessage', function(e) {
  try {
    var t = e.payload[keys.Token];
    if (t) localStorage.setItem('token', t);
    var p = e.payload[keys.ProxyUrl];
    if (p) localStorage.setItem('proxyUrl', p);
    var s = e.payload[keys.StreetName];
    if (s) localStorage.setItem('streetName', s);
    var n = e.payload[keys.NumberName];
    if (n) localStorage.setItem('numberName', n);

    if (e.payload[keys.RequestType] === REQUEST_SCHEDULE) {
      var settings = getSettings();
      if (!settings.token) { sendError('No token.\nOpen Settings.'); return; }
      if (!settings.streetName || !settings.numberName) { sendError('No address.\nOpen Settings.'); return; }
      fetchSchedule(settings);
    }
  } catch (ex) {
    sendError('Error: ' + ex.message.substring(0, 50));
  }
});

function fetchSchedule(s) {
  var url = (s.proxyUrl || DEFAULT_PROXY).replace(/\/$/, '') + '/schedule';

  var body = 'token=' + encodeURIComponent(s.token) +
    '&street=' + encodeURIComponent(s.streetName) +
    '&number=' + encodeURIComponent(s.numberName);

  var req = new XMLHttpRequest();
  req.open('POST', url, true);
  req.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
  req.timeout = 55000;
  req.ontimeout = function() { sendError('Timeout.\nTry again.'); };
  req.onerror = function() { sendError('Network error'); };
  req.onload = function() {
    if (req.status === 200) {
      try {
        var data = JSON.parse(req.responseText);
        if (data.status === 'ok' && data.entries && data.entries.length > 0) {
          sendSchedule(data.entries);
        } else {
          sendError(data.error || 'Empty schedule');
        }
      } catch (ex) {
        sendError('Parse error');
      }
    } else {
      try { sendError(JSON.parse(req.responseText).error || 'HTTP ' + req.status); }
      catch (ex) { sendError('HTTP ' + req.status); }
    }
  };
  req.send(body);
}

function sendSchedule(entries) {
  var msg = {};
  msg[keys.ScheduleCount] = entries.length;
  Pebble.sendAppMessage(msg, function() {
    pendingSchedule = entries;
    currentIndex = 0;
    sendNext();
  }, function() { sendError('Msg error'); });
}

function sendNext() {
  if (currentIndex >= pendingSchedule.length) return;
  var e = pendingSchedule[currentIndex];
  var msg = {};
  msg[keys.ScheduleIndex] = currentIndex;
  msg[keys.ScheduleType] = e.type || '?';
  msg[keys.ScheduleDate] = e.date || '?';
  msg[keys.ScheduleDay] = e.day || '?';
  Pebble.sendAppMessage(msg, function() {
    currentIndex++;
    sendNext();
  }, function() { setTimeout(sendNext, 200); });
}

function sendError(message) {
  var msg = {};
  msg[keys.ErrorMessage] = message.substring(0, 63);
  Pebble.sendAppMessage(msg);
}
