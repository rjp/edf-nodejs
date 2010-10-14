sys = require('sys');
assert = require('assert');
edf = require('edfparser');
parser = new edf.EDFParser();

j = parser.parse('<reply="user_list"><user=1><name="Gryn"/><accesslevel=3/><status=1/><timeoff=1285110898/></><user=2><name="Crackpot"/><accesslevel=2/><timeoff=1276382989/></><user=3><name="Homicide"/><accesslevel=3/><gender=1/><timeoff=1282890124/></><user=4><name="Techno"/><accesslevel=4/><gender=1/><timeoff=1285337394/></><user=6><name="Ivan"/><accesslevel=2/><gender=1/><timeoff=1286351773/></><user=7><name="Rincewind"/><accesslevel=3/><gender=1/><status=5/><timeidle=1286519939/><timeoff=1283259668/></><user=9><name="Sniper"/><accesslevel=3/><gender=1/><timeoff=969054360/></><user=10><name="Wolf\'n"/><accesslevel=2/><timeoff=1142518193/></><user=12><name="Excess Luggage"/><accesslevel=2/><gender=1/><timeoff=1228231391/></><user=14><name="Bong"/><accesslevel=1/><gender=1/><timeoff=980199970/></><user=15><name="Mark"/><accesslevel=2/><gender=1/><timeoff=1286468591/></><user=16><name="Flood"/><accesslevel=1/><timeoff=1036837341/></><user=18><name="Cheradenine"/><accesslevel=3/><gender=1/><timeoff=1278956013/></><user=19><name="Max"/><accesslevel=2/><timeoff=1103120200/></><user=23><name="Mince"/><accesslevel=3/><gender=1/><timeoff=1199316968/></><user=24><name="Nige"/><accesslevel=2/><gender=1/><timeoff=1169090935/></><user=25><name="Refresher"/><accesslevel=2/><accessname="Pimp"/><status=5/><timeidle=1286478159/><timeoff=1285922086/></><user=28><name="Falcon"/><accesslevel=3/><gender=1/><status=7/><timebusy=1286007403/><statusmsg="Work"/><ti');

exports['broken parse'] = function(test){
    test.expect(1);
    test.equals(j, '-1', 'broken parse returns -1');
    test.done();
}
