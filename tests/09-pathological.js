sys = require('sys');
assert = require('assert');
edf = require('edfparser');
fs = require('fs');

parser = new edf.EDFParser();

function js2edf(tree) {
    var edf = '<' + tree.tag;
    if (tree.value != undefined) {
        if (isNaN(parseFloat(tree.value))) {
            if (tree.value.length > 0) {
                edf = edf + '="' + tree.value + '"';
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
    for(var i=0; i < 1; i++) { // trees.parsed; i++) {
        edf = edf + js2edf(trees.trees[i]);
    }
    return edf;
}

exports.pathological = {}

var test_strings = fs.readFileSync('testlines', 'utf8').split('\n');
// why do we end up with a bogus empty string?

for(var i=0; i < test_strings.length; i++) {
    if (test_strings[i].length == 0) { continue; }

	var j = parser.parse(test_strings[i]);
    if (j == -1) { throw("FATAL: "+test_strings[i]); }

	var e = JSON.parse(j);
    var wanted = test_strings[i];
    var got = trees2edf(e);

    if (i == 18) {
        sys.puts(wanted);
        sys.puts(j);
        sys.puts(got);
        sys.puts(wanted == got);
    }

    var tObj = new Object();
    tObj.wanted = wanted;
    tObj.got = got;
	
    var x = function(test){ // why aren't you closing?
	    test.expect(1);
        sys.puts(tObj.wanted+' == '+tObj.got+' => '+(tObj.wanted==tObj.got));
	    test.equals(tObj.wanted, tObj.got, 'parse/reverse '+i+' matches');
	    test.done();
	}
    exports.pathological['parse '+i] = x;
}
