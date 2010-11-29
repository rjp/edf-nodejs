assert = require('assert');
edf = require('edfparser');

j = edf.parse('<edf="on"/>');
e = eval('('+j+')');

exports['test parse count'] = function(test){
    test.expect(1);
    test.equals(e.parsed, 1, 'number of parsed nodes');
    test.done();
}

exports['test parsed node'] = function(test){
    test.expect(3);
    test.equals(e.trees[0].tag, 'edf', 'edf node');
    test.equals(e.trees[0].value, 'on', 'edf node');
    test.equals(e.trees[0].children, undefined, 'edf node: no children');
    test.done();
}
