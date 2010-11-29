assert = require('assert');
edf = require('edfparser');

j = edf.parse('<edf="on"><winter="discontent"/><best="worst"></></>');
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
    test.equals(e.trees[0].children.length, 2, 'edf node: one child');
    test.done();
}

exports['test child[0] node'] = function(test){
    test.expect(3);
    child = e.trees[0].children[0];
    test.equals(child.tag, 'winter', 'child node');
    test.equals(child.value, 'discontent', 'child node');
    test.equals(child.children, undefined, 'child node: one child');
    test.done();
}

exports['test child[0][0] node'] = function(test){
    test.expect(3);
    child = e.trees[0].children[1];
    test.equals(child.tag, 'best', 'child node');
    test.equals(child.value, 'worst', 'child node');
    test.equals(child.children, undefined, 'open node, no children');
    test.done();
}

