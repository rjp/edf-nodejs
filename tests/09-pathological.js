sys = require('sys');
assert = require('assert');
edf = require('edfparser');
fs = require('fs');

function js2edf(tree) {
    var edf = '<' + tree.tag;
    if (tree.value != undefined) {
        if (isNaN(parseFloat(tree.value))) {
            if (tree.value.length > 0) {
                var x = tree.value.replace(/"/g, "\\\"");
                edf = edf + '="' + x + '"';
            }
        } else {
            edf = edf + '=' + tree.value;
        }
    }
    if (tree.children && tree.children.length > 0) {
        var i;
        edf = edf + '>';
        for(i=0; i < tree.children.length; i++) {
            edf = edf + js2edf(tree.children[i]);
        }
        edf = edf + '</>';
    } else {
        edf = edf + '/>';
    }
    return edf;
}

// { trees: [ { tag: 'edf', value: '', end: 1 }, { end: 1 } ] , parsed: 1 }
function trees2edf(trees) {
    var edf = '';
    for(var i=0; i < trees.parsed; i++) {
        edf = edf + js2edf(trees.trees[i]);
    }
    return edf;
}

exports.pathological = {}

var test_strings = fs.readFileSync('testlines', 'utf8').split('\n');
// why do we end up with a bogus empty string?

function make_test(i, wanted, got) {
    return function(test) {
	    test.expect(1);
//        sys.puts('W:'+wanted+' == '+got+' => '+(wanted==got));
	    test.equals(wanted, got, 'parse/reverse '+i+' matches');
	    test.done();
    }
}

for(var i=0; i < test_strings.length; i++) {
    if (test_strings[i].length == 0) { continue; }

	var j = edf.parse(test_strings[i]);
    if (j == -1) { throw("FATAL: "+test_strings[i]); }

	var e = JSON.parse(j);
    var wanted = test_strings[i];
    var got = trees2edf(e);

    var x = make_test(i, wanted, got);
    exports.pathological['parse '+i] = x;
}
