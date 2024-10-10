[![Support](https://img.shields.io/badge/Support-Official-green.svg)](mailto:support@perforce.com)

# P4DTG
Helix Defect Tracking Gateway (P4DTG) provides better visibility and control over your defect tracking process by easily sharing information from Helix Core projects to your external defect tracking system.

The Helix Defect Tracking Gateway integrates Perforce with third-party defect tracking systems. Perforce currently supports Bugzilla, Jira Cloud, and Jira on-premises (Data Center).
Other defect tracking systems are supported by third-parties (e.g., Fogbugz from Fogcreek).

## Requirements
For P4DTG requirements see "Compatibility Statements" in [RELNOTES](https://github.com/perforce/p4dtg/blob/master/RELNOTES.txt#L99)

## Documentation
Official documentation is located on the [Perforce website](https://www.perforce.com/manuals/p4dtg/)

## Support
P4DTG is officially supported by Perforce.
Pull requests will be managed by Perforce's engineering teams. We will do our best to acknowledge these in a timely manner based on available capacity.  
Issues will not be managed on GitHub. All issues should be recorded via [Perforce's standard support process](https://www.perforce.com/support/request-support).

## Content
```
p4-dtg/ 
    README.md   	- This file


sdk/
    p4jobdt/	    - Perforce plugin, used to build both
                      p4jobs.(so|dll) and p4generic.(so.dll)
    bugz/		    - Plugin for Bugzilla 2 MySQL 4 or 5
    mysql/		    - Plugin for generic MySQL 5 connection
    jira-rest/      - Plugin for Jira REST
    include/	    - Plugin Interface Definition
    share/		    - Utility functions for plugin development
    mydts/		    - Complete C++ Framework for Plugin Development

    test/		    - Commandline plugin test tool

src/
    p4dtg-config/   - GUI Configuration tool source
    p4dtg-repl/     - Replication engine source
    share/          - Utility library used by both of the above
    ntservice/      - Utlity to create Windows services
    doc/            - Misc P4DTG docs
```