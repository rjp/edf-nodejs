assert = require('assert');
edf = require('edfparser');

j = edf.parse('<edf="on"><winter="discontent"/></>');
e = eval('('+j+')');

exports['test parse count'] = function(test){
    test.expect(1);
    test.equals(e.parsed, 1, 'number of parsed nodes');
    test.done();
}

exports['test top node'] = function(test){
    test.expect(3);
    test.equals(e.trees[0].tag, 'edf', 'edf node');
    test.equals(e.trees[0].value, 'on', 'edf node');
    test.equals(e.trees[0].children.length, 1, 'edf node: one child');
    test.done();
}

exports['test child node'] = function(test){
    test.expect(3);
    child = e.trees[0].children[0];
    test.equals(child.tag, 'winter', 'child node');
    test.equals(child.value, 'discontent', 'child node');
    test.equals(child.children, undefined, 'child node: no children');
    test.done();
}

