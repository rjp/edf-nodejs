assert = require('assert');
sys = require('sys');
require('./json_parse_state.js');

sys.puts(sys.inspect(json_parse));

function rm(x) {
    sys.puts(x+" = "+sys.inspect(process.memoryUsage().rss));
}

rm("before require edfparser");
edf = require('edfparser');

rm("before EDFParser");
parser = new edf.EDFParser();
rm("after EDFParser");

rm("before simple parse");
j = parser.parse("<edf=\"on\"><test=42/></><reply=\"fish\"><age=44/></>");
e = eval('('+j+')');
rm("after simple parse");

var x = "<reply=\"message_list\"><message=1991721><date=1286960325/><fromid=1030/><toid=358/><text=\"> Yuck, ick, etc ;) \n\nWell, quite.  Must be something subliminal going on.\n\nAs far as I remember, there was me and another female person who I can't remember who it was (I /think/ it was someone I knew, not a generic random female) and we were making a big showy \\\"both of us kissing him on the cheek\\\" thing, deliberately for the camera, to say thank you for something.  No idea what.  It's all going a bit fuzzy now, as these things do ...\"/><subject=\"Mock the Week\"/><fromname=\"Alluveai\"/><toname=\"kat\"/><replyto=1991702><fromid=358/><fromname=\"kat\"/></><replyto=1991696><read=1/><fromid=1030/><fromname=\"Alluveai\"/></><replyto=1991694><fromid=358/><fromname=\"kat\"/></><replyto=1991693><read=1/><fromid=1030/><fromname=\"Alluveai\"/></><replyto=1991655><fromid=358/><fromname=\"kat\"/></><replyto=1991654><fromid=2013/><fromname=\"rjp\"/></><replyto=1991652><fromid=358/><fromname=\"kat\"/></><replyto=1991651><fromid=2013/><fromname=\"rjp\"/></><replyto=1991650><fromid=358/><fromname=\"kat\"/></><msgpos=21/></><searchtype=0/><folderid=229384/><foldername=\"Here-And-Now\"/><nummsgs=21/></>";

var y = "<edf=\"on\"><test=42/></><reply=\"fish\"><age=44/></>";

rm("before 1000");
for(var i=0; i<1000; i++) {
    var j = parser.parse(x);
    var e = JSON.parse(j);
}
rm("after 1000");
sys.puts(x.length);
sys.puts(y.length);

sys.puts(sys.inspect(e));

assert.equal(1, e.parsed);

assert.equal('reply', e.trees[0].tag);
assert.equal('message_list', e.trees[0].value);
assert.equal(1, e.trees[0].children.length);

assert.equal('reply', e.trees[1].tag);
assert.equal('fish', e.trees[1].value);
