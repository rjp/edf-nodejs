sys = require('sys');
edf = require('edfparser');
parser = new edf.EDFParser();
sys.print("<edf=\"on\"><test=42/></>\n");
j = parser.parse("<edf=\"on\"><test=42/></>");
sys.print(j+"\n");
e = eval('('+j+')');
sys.print("chunk is '"+e.tag+"'\n");
