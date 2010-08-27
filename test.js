assert = require('assert');
sys = require('sys');
edf = require('edfparser');
parser = new edf.EDFParser();


//j = parser.parse("<edf=\"on\"><test=42/></>");
j = parser.parse("<edf=\"on\"><test=42/></><reply=\"fish\"><age=44/></>");
e = eval('('+j+')');

assert.equal(2, e.parsed);

assert.equal('edf', e.trees[0].tag);
assert.equal('on', e.trees[0].value);
assert.equal(1, e.trees[0].children.length);

assert.equal('reply', e.trees[1].tag);
assert.equal('fish', e.trees[1].value);
