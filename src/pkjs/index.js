/**
 * Pebble Trivia — PebbleKit JS
 * Fetches questions from Open Trivia Database (opentdb.com)
 * Free, no API key required.
 *
 * Encoding received from watch:
 *   value = 1                   → send next question
 *   value = cat_id + diff*200   → category/difficulty change
 *     diff: 0=any 1=easy 2=medium 3=hard
 *     cat_id 102 = True/False (type=boolean)
 *     cat_id 101 = Easy Kids  (always easy, type=multiple)
 */

var KEY_CATEGORY     = 'CATEGORY';
var KEY_QUESTION     = 'QUESTION';
var KEY_ANSWER       = 'ANSWER';
var KEY_REQUEST_NEXT = 'REQUEST_NEXT';

var queue         = [];
var fetching      = false;
var waitingToSend = false;
var currentCatId  = 0;
var currentDiff   = 0;   // 0=any 1=easy 2=medium 3=hard
var fetchGen      = 0;   // incremented on category change to discard stale XHRs

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
  var diff = '';
  if (currentDiff === 1)      diff = '&difficulty=easy';
  else if (currentDiff === 2) diff = '&difficulty=medium';
  else if (currentDiff === 3) diff = '&difficulty=hard';

  if (currentCatId === 102) {
    // True / False category
    return 'https://opentdb.com/api.php?amount=20&type=boolean' + diff;
  }
  if (currentCatId === 101) {
    // Easy (Kids) — always easy multiple-choice regardless of difficulty picker
    return 'https://opentdb.com/api.php?amount=20&type=multiple&difficulty=easy';
  }
  var base = 'https://opentdb.com/api.php?amount=20&type=multiple' + diff;
  if (currentCatId > 0) {
    return base + '&category=' + currentCatId;
  }
  return base;
}

function doFetch() {
  if (fetching) return;
  fetching = true;
  var gen = fetchGen;

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
            var correct      = decodeHTML(q.correct_answer);
            var questionText = decodeHTML(q.question);

            if (q.type === 'boolean') {
              // True/False — always the same two options
              questionText += '\nA) True\nB) False';
            } else {
              // Multiple choice — shuffle all four options
              var choices = q.incorrect_answers.map(function(a) {
                return decodeHTML(a);
              });
              choices.push(correct);
              for (var i = choices.length - 1; i > 0; i--) {
                var j = Math.floor(Math.random() * (i + 1));
                var tmp = choices[i]; choices[i] = choices[j]; choices[j] = tmp;
              }
              var labels = ['A', 'B', 'C', 'D'];
              questionText += '\n' + choices.map(function(c, idx) {
                return labels[idx] + ') ' + c;
              }).join('\n');
            }

            queue.push({
              category: decodeHTML(q.category),
              question: questionText,
              answer:   correct
            });
          });
          console.log('Fetched ' + data.results.length +
                      ' questions. Queue: ' + queue.length);
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
    console.log('Fetch error');
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
  if (queue.length < 5) doFetch();  // prefetch in background

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
  var raw = parseInt(e.payload[KEY_REQUEST_NEXT], 10);
  if (raw === 1) {
    // Next question request
    sendNextQuestion();
  } else if (!isNaN(raw)) {
    // Category + difficulty change
    currentDiff  = Math.floor(raw / 200);
    currentCatId = raw % 200;
    fetchGen++;
    queue         = [];
    fetching      = false;
    waitingToSend = false;
    console.log('Cat: ' + currentCatId + '  Diff: ' + currentDiff);
    sendNextQuestion();
  }
});
