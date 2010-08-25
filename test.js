sys = require('sys');
edf = require('edfparser');
parser = new edf.EDFParser();
sys.print(parser.parse("Fish"));
