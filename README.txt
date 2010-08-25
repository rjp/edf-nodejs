qUAck - Telnet look and feel client for UNaXcess II

Links
-----

Public version -      http://ua2.org/clients/
Development version - http://ua2.org/UADN/
Buck stops here -     mike@compsoc.man.ac.uk (Techno on UA)

FAQ
---

Q: It doesn't compile
A: Probably the carriage return / line feed thing. Try this:

   perl -pli~ -e 's/^V^M//' Makefile* */Makefile useful/useful.h useful/useful.cpp qUAck/CmdIO.cpp qUAck/qUAck.h EDF/EDF.cpp EDF/EDFElement.[ch]* Conn/Conn.cpp

Q: How do I configure it?
A: Under Unix you can put configuration options in a .qUAckrc file in your
   home directory. Under Windows put them in a qUAck.edf file in the same
   directory as the executable. Here's an example:
   
   <>
     <server="localhost"/>
     <port=4040/>
     <username="sysop"/>
   </>
   
   Other settings:
   
   <password="password"/>
   
   If you don't specify a password in the config file you will be prompted
   for one
   
   <secure=1/>
   <certificate="qUAck.pem"/>
   
   The certificate field is optional for secure connections
   
   <editor="/bin/vi"/>
   
   Use an external editor for text composition
   
   <browse=1/>
   <browser="/usr/bin/netscape"/>
   <browserwait=1/>
   
   Specify a browser for viewing URLs. browserwait tells qUAck to wait for
   the launched browser to finish before carrying on (useful for terminal
   stuff like lynx)
   
   <attachmentsize=n/>
   <attachmentdir="/tmp"/>
   
   This sets the size of attachment you are prepared to recieved in a page.
   By default no attachments will be sent to you. Use a setting of -1 for
   any size. You can also specify a directory for saving attachments
   
   <busy=1/>
   <silent=1/>
   <shadow=1/>
   
   Login with busy / silent / shadow flag on (the ability to send and
   recieve pages is diabled with shadow on)
   
   <thinpipe=1/>
   
   Use reduced data requests which refresh cached data (user list, folder
   list) in case you're on a slow link
   
   <screensize=1/>
   
   Obey screen width / height settings stored on the server and ignore the
   queried ones qUAck gets from your terminal
   
   <usepid=1/>
   
   Append the PID to the filename qUAck puts debug information into
   

Q: Backspace / delete / etc doesn't work in the editor (Unix)
A: Your version of curses is broken :-)

Credits
-------

qUAck looks the way it does as a result of 10 years UI "refinement" beginning
with old UNaXcess (circa 1984) and continuing until 1999 when we [the Manc CS
UA bods] were dragged kicking and screaming onto UNaXcess II

Old UA guilty parties: Brandon S Allbery, Andrew G Minter, Rob Partington,
Gryn Davies, Michael Wood, Andrew Armitage, Francis Cook, Brian Widdas

Special mention to Jason Williams (Khendon) and Tim Bannister (isoma) - my
favourite beta testers ever - who put up with bugs, compilation problems and
a host of other obstacles to help me fix things before public release,
Rob Partington (Phaedrus) for providing patches and using the exact opposite
of all my settings and forcing me to fix all the code I never see or use,
David Walters for his first time user suggestions.

Enjoy!

        Mike
