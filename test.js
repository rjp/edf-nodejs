assert = require('assert');
sys = require('sys');
require('./json_parse_state.js');
profiler = require('profiler');

sys.puts(sys.inspect(json_parse));

var pmem = 0;
var psec = 0;

function rm(x, y) {
    var mem = process.memoryUsage().rss;
    var now = new Date().getTime();
    if (y !== undefined) {
        sys.puts(now+" "+x+": "+mem+", delta: "+(mem-pmem)+", per: "+(mem-pmem)/y+", ms/per: "+(now-psec)/y);
    } else {
        sys.puts(now+" "+x+": "+mem+", delta: "+(mem-pmem));
    }
    pmem = mem;
    psec = now;
}

rm("before require edfparser");
edf = require('edfparser');

function do_it() {
rm("before simple parse");
j = edf.parse("<edf=\"on\"><test=42/></><reply=\"fish\"><age=44/></>");
// e = eval('('+j+')');
rm("after simple parse");

var x = "<reply=\"message_list\"><message=1991721><date=1286960325/><fromid=1030/><toid=358/><text=\"> Yuck, ick, etc ;) \n\nWell, quite.  Must be something subliminal going on.\n\nAs far as I remember, there was me and another female person who I can't remember who it was (I /think/ it was someone I knew, not a generic random female) and we were making a big showy \\\"both of us kissing him on the cheek\\\" thing, deliberately for the camera, to say thank you for something.  No idea what.  It's all going a bit fuzzy now, as these things do ...\"/><subject=\"Mock the Week\"/><fromname=\"Alluveai\"/><toname=\"kat\"/><replyto=1991702><fromid=358/><fromname=\"kat\"/></><replyto=1991696><read=1/><fromid=1030/><fromname=\"Alluveai\"/></><replyto=1991694><fromid=358/><fromname=\"kat\"/></><replyto=1991693><read=1/><fromid=1030/><fromname=\"Alluveai\"/></><replyto=1991655><fromid=358/><fromname=\"kat\"/></><replyto=1991654><fromid=2013/><fromname=\"rjp\"/></><replyto=1991652><fromid=358/><fromname=\"kat\"/></><replyto=1991651><fromid=2013/><fromname=\"rjp\"/></><replyto=1991650><fromid=358/><fromname=\"kat\"/></><msgpos=21/></><searchtype=0/><folderid=229384/><foldername=\"Here-And-Now\"/><nummsgs=21/></>";

var y = "<edf=\"on\"><test=42/></><reply=\"fish\"><age=44/></>";

var t_json = '{"trees":[{"tag":"reply","value":"message_list", "children":[{"tag":"message","value":1991721, "children":[{"tag":"date","value":1286960325}, {"tag":"fromid","value":1030}, {"tag":"toid","value":358}, {"tag":"text","value":"> Yuck, ick, etc ;) \n\nWell, quite.  Must be something subliminal going on.\n\nAs far as I remember, there was me and another female person who I can\'t remember who it was (I /think/ it was someone I knew, not a generic random female) and we were making a big showy \"both of us kissing him on the cheek\" thing, deliberately for the camera, to say thank you for something.  No idea what.  It\'s all going a bit fuzzy now, as these things do ..."}, {"tag":"subject","value":"Mock the Week"}, {"tag":"fromname","value":"Alluveai"}, {"tag":"toname","value":"kat"}, {"tag":"replyto","value":1991702, "children":[{"tag":"fromid","value":358}, {"tag":"fromname","value":"kat", "end":1}]}, {"tag":"replyto","value":1991696, "children":[{"tag":"read","value":1}, {"tag":"fromid","value":1030}, {"tag":"fromname","value":"Alluveai", "end":1}]}, {"tag":"replyto","value":1991694, "children":[{"tag":"fromid","value":358}, {"tag":"fromname","value":"kat", "end":1}]}, {"tag":"replyto","value":1991693, "children":[{"tag":"read","value":1}, {"tag":"fromid","value":1030}, {"tag":"fromname","value":"Alluveai", "end":1}]}, {"tag":"replyto","value":1991655, "children":[{"tag":"fromid","value":358}, {"tag":"fromname","value":"kat", "end":1}]}, {"tag":"replyto","value":1991654, "children":[{"tag":"fromid","value":2013}, {"tag":"fromname","value":"rjp", "end":1}]}, {"tag":"replyto","value":1991652, "children":[{"tag":"fromid","value":358}, {"tag":"fromname","value":"kat", "end":1}]}, {"tag":"replyto","value":1991651, "children":[{"tag":"fromid","value":2013}, {"tag":"fromname","value":"rjp", "end":1}]}, {"tag":"replyto","value":1991650, "children":[{"tag":"fromid","value":358}, {"tag":"fromname","value":"kat", "end":1}]}, {"tag":"msgpos","value":21, "end":1}]}, {"tag":"searchtype","value":0}, {"tag":"folderid","value":229384}, {"tag":"foldername","value":"Here-And-Now"}, {"tag":"nummsgs","value":21, "end":1}], "end":1}, {"end":1}], "parsed":1}';

var gc = 0;
var q;
var reps = 10001;
rm("before "+reps);
for(var i=0; i<reps; i++) {
    var j = edf.parse(x);
    var e = JSON.parse(j);
    q = j;
    if (i % 100 == 0) { profiler.gc(); gc = gc + 1;}
}
rm("after "+reps, reps);
sys.puts("GC: "+gc);
sys.puts(typeof q);
sys.puts("q length is "+q.length);
rm("before "+reps+" json");
for(var i=0; i<reps; i++) {
    var e = JSON.parse(q);
}
rm("after "+reps+" json", reps);

}

setTimeout(do_it, 15000);
