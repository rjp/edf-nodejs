sys = require('sys');
assert = require('assert');
edf = require('edfparser');
parser = new edf.EDFParser();

var x = "<reply=\"message_list\"><message=1991721><date=1286960325/><fromid=1030/><toid=358/><text=\"> Yuck, ick, etc ;) \n\nWell, quite.  Must be something subliminal going on.\n\nAs far as I remember, there was me and another female person who I can't remember who it was (I /think/ it was someone I knew, not a generic random female) and we were making a big showy \\\"both of us kissing him on the cheek\\\" thing, deliberately for the camera, to say thank you for something.  No idea what.  It's all going a bit fuzzy now, as these things do ...\"/><subject=\"Mock the Week\"/><fromname=\"Alluveai\"/><toname=\"kat\"/><replyto=1991702><fromid=358/><fromname=\"kat\"/></><replyto=1991696><read=1/><fromid=1030/><fromname=\"Alluveai\"/></><replyto=1991694><fromid=358/><fromname=\"kat\"/></><replyto=1991693><read=1/><fromid=1030/><fromname=\"Alluveai\"/></><replyto=1991655><fromid=358/><fromname=\"kat\"/></><replyto=1991654><fromid=2013/><fromname=\"rjp\"/></><replyto=1991652><fromid=358/><fromname=\"kat\"/></><replyto=1991651><fromid=2013/><fromname=\"rjp\"/></><replyto=1991650><fromid=358/><fromname=\"kat\"/></><msgpos=21/></><searchtype=0/><folderid=229384/><foldername=\"Here-And-Now\"/><nummsgs=21/></>";

j = parser.parse(x);
e = eval('('+j+')');

exports['test parse count'] = function(test){
    test.expect(1);
    test.equals(e.parsed, 1, 'number of parsed nodes');
    test.done();
}

exports['test top node'] = function(test){
    test.expect(3);
    test.equals(e.trees[0].tag, 'reply', 'edf node');
    test.equals(e.trees[0].value, 'message_list', 'edf node');
    test.equals(e.trees[0].children.length, 5, 'edf node: one child');
    test.done();
}

exports['test child[0] node'] = function(test){
    test.expect(2);
    child = e.trees[0].children[0];
    test.equals(child.tag, 'message', 'child node');
    test.equals(child.value, '1991721', 'child node');
    // test.equals(child.children, undefined, 'child node: one child');
    test.done();
}

