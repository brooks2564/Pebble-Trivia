/**
 * Pebble Trivia — PebbleKit JS
 * Fetches questions from Open Trivia Database (opentdb.com)
 * Free, no API key required.
 */

var queue = [];
var fetching = false;

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

function fetchQuestions(callback) {
  if (fetching) return;
  fetching = true;

  var xhr = new XMLHttpRequest();
  xhr.onload = function() {
    fetching = false;
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
    if (callback) callback();
  };
  xhr.onerror = function() {
    fetching = false;
    console.log('Fetch failed');
    if (callback) callback();
  };
  xhr.open('GET', 'https://opentdb.com/api.php?amount=20&type=multiple');
  xhr.send();
}

function sendNextQuestion() {
  if (queue.length === 0) {
    fetchQuestions(sendNextQuestion);
    return;
  }

  var q = queue.shift();

  // Prefetch in background when running low
  if (queue.length < 5) {
    fetchQuestions(null);
  }

  var payload = {
    '0': q.category.substring(0, 63),
    '1': q.question.substring(0, 510),
    '2': q.answer.substring(0, 254)
  };

  Pebble.sendAppMessage(payload,
    function() { console.log('Question sent OK'); },
    function(e) { console.log('Send failed: ' + JSON.stringify(e)); }
  );
}

Pebble.addEventListener('ready', function() {
  console.log('JS ready, fetching questions...');
  fetchQuestions(sendNextQuestion);
});

Pebble.addEventListener('appmessage', function(e) {
  if (e.payload['3'] === 1) {
    sendNextQuestion();
  }
});
