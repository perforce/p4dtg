#
# Copyright 2005 Perforce Software.  All rights reserved.
#
echo $1 >> one/main/$1.$$.txt
p4 add //depot/main/$1.$$.txt
p4 submit -i <<FOO
Change: new
Client: oneclient
User: one
Status: new
Description:
  sample changelist
Files:
  //depot/main/$1.$$.txt  # add
FOO
