EDF parser implemented as a synchronous node.js extension.

edf = require('edfparser');
parser = new edf.EDFParser();
j = parser.parse("<edf=\"on\"><test=42/></>");
# now you have the JSON-equivalent of your EDF
e = eval('('+j+')');
