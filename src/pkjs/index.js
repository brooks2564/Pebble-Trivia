/**
 * Pebble Trivia — PebbleKit JS
 * Fetches questions from Open Trivia Database (opentdb.com)
 * Free, no API key required.
 */

// Message keys — must match build/src/message_keys.auto.c
var KEY_CATEGORY     = 10000;
var KEY_QUESTION     = 10001;
var KEY_ANSWER       = 10002;
var KEY_REQUEST_NEXT = 10003;
// Protocol: REQUEST_NEXT value=1 → send next question
//           REQUEST_NEXT value=anything else → switch to that category ID

var queue = [];
var fetching = false;
var waitingToSend = false;
var currentCategoryId = 0;
var fetchGen = 0;  // incremented on category change to discard stale XHR responses

function decodeHTML(str) {
  return str
    .replace(/&amp;/g,    '&')
    .replace(/&lt;/g,     '<')
    .replace(/&gt;/g,     '>')
    .replace(/&quot;/g,   '"')
    .replace(/&#039;/g,   "'")
    .replace(/&ldquo;/g,  '\u201c')
    .replace(/&rdquo;/g,  '\u201d')
    .replace(/&lsquo;/g,  '\u2018')
    .replace(/&rsquo;/g,  '\u2019')
    .replace(/&hellip;/g, '...')
    .replace(/&eacute;/g, 'e')
    .replace(/&egrave;/g, 'e')
    .replace(/&uuml;/g,   'u')
    .replace(/&ouml;/g,   'o')
    .replace(/&auml;/g,   'a')
    .replace(/&ntilde;/g, 'n')
    .replace(/&oacute;/g, 'o')
    .replace(/&aacute;/g, 'a')
    .replace(/&iacute;/g, 'i')
    .replace(/&ndash;/g,  '-')
    .replace(/&mdash;/g,  '-');
}

function buildUrl() {
  var base = 'https://opentdb.com/api.php?amount=20&type=multiple';
  if (currentCategoryId === 101) {
    return base + '&difficulty=easy';
  } else if (currentCategoryId > 0) {
    return base + '&category=' + currentCategoryId;
  }
  return base;
}

function doFetch() {
  if (fetching) return;
  fetching = true;
  var gen = fetchGen;  // capture generation at time of request

  var xhr = new XMLHttpRequest();
  xhr.onload = function() {
    fetching = false;
    if (gen !== fetchGen) {
      console.log('Discarding stale fetch (category changed)');
      return;
    }
    if (xhr.status === 200) {
      try {
        var data = JSON.parse(xhr.responseText);
        if (data.response_code === 0 && data.results) {
          data.results.forEach(function(q) {
            queue.push({
              category: decodeHTML(q.category),
              question: decodeHTML(q.question),
              answer:   decodeHTML(q.correct_answer)
            });
          });
          console.log('Fetched ' + data.results.length + ' questions. Queue: ' + queue.length);
        }
      } catch(e) {
        console.log('Parse error: ' + e);
      }
    }
    if (waitingToSend) {
      waitingToSend = false;
      sendNextQuestion();
    }
  };
  xhr.onerror = function() {
    fetching = false;
    console.log('Fetch failed');
  };
  xhr.open('GET', buildUrl());
  xhr.send();
}

function sendNextQuestion() {
  if (queue.length === 0) {
    waitingToSend = true;
    doFetch();
    return;
  }

  var q = queue.shift();

  // Prefetch in background when running low
  if (queue.length < 5) {
    doFetch();
  }

  var payload = {};
  payload[KEY_CATEGORY] = q.category.substring(0, 63);
  payload[KEY_QUESTION] = q.question.substring(0, 510);
  payload[KEY_ANSWER]   = q.answer.substring(0, 254);
  Pebble.sendAppMessage(payload, function() {
    console.log('Question sent OK');
  }, function(e) {
    console.log('Send failed: ' + JSON.stringify(e));
  });
}

Pebble.addEventListener('ready', function() {
  console.log('JS ready — waiting for category selection');
});

Pebble.addEventListener('appmessage', function(e) {
  var val = parseInt(e.payload[KEY_REQUEST_NEXT], 10);
  if (val === 1) {
    // Normal next-question request
    sendNextQuestion();
  } else if (!isNaN(val)) {
    // Category change — val is the new category ID
    currentCategoryId = val;
    fetchGen++;
    queue = [];
    fetching = false;
    waitingToSend = false;
    console.log('Category set to: ' + currentCategoryId + ', fetching...');
    sendNextQuestion();
  }
});
