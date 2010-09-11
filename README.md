EDF parser implemented as a synchronous node.js extension.

    edf = require('edfparser');
    parser = new edf.EDFParser();
    j = parser.parse("<edf=\"on\"><test=42/></>");
    # now you have the JSON-equivalent of your EDF
    e = eval('('+j+')');

This is what you get back

    {"trees":[{"tag":"edf","value":"on", "children":[{"tag":"test","value":42, "end":1}], "end":1}, {"tag":"reply","value":"fish", "children":[{"tag":"age","value":44, "end":1}], "end":1}, {"end":1}], "parsed":2}

e.parsed is the number of top-level EDF trees returned. e.trees is an array of trees. Each node is represented
as a tag (name of the EDF node), a value (if specified) and children which is a list of child nodes.

Currently there are no helper functions for finding specific nodes - they are planned.
