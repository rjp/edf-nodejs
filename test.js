assert = require('assert');
sys = require('sys');
edf = require('edfparser');
parser = new edf.EDFParser();
j = parser.parse("<edf=\"on\"><test=42/></>");
e = eval('('+j+')');
assert.equal('edf', e.tag);
assert.equal('on', e.value);
assert.equal(1, e.children.length);
