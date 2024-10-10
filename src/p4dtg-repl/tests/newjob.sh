#
# Copyright 2005 Perforce Software.  All rights reserved.
#
p4 job -i <<FOO
Job:	new
Status: open
Type:   BUG
Severity:       C
Subsystem:      p4web
ReportedBy:     one
OwnedBy: one
Description:
        This is a test job
$1
FOO
