#!/usr/local/bin/ruby
#
# Copyright 2005 Perforce Software.  All rights reserved.
#

require "P4Form.rb"

failures = 0

if ischangelist("1") then 
# SKIP INIITIALIZATION
	printf "System currently populated with test data\n"
else
	printf "Populating system with test data\n"
	`sh p4user.setup karen`  	# Add user
	`sh newjob.sh`                  # Create job000001
	`sh newjob.sh`                  # Create job000002
	`sh newjob.sh`                  # Create job000003
	`sh changelist.sh one`          # Create 1
	`sh changelist.sh two`					# Create 2
	`sh changelist.sh three`				# Create 3
	`sh changelist.sh four`					# Create 4
end 

for n in 1..3 do
	jobid = "job00000#{n}"
	
	`p4 -p :3344 job -o #{jobid} | sed -e 's/	/  /g' | sed -e 's=..../../.. ..:..:..=yyyy/mm/dd hh:mm:ss=' | awk '/^[^#]/ { print $0; }'`
	print `p4 -p :3344 fix -c #{n} #{jobid}`
	if n == 3 then
		print `p4 -p :3344 fix -c 4 #{jobid}`
	end
	`p4 -p :3344 job -o #{jobid} | sed -e 's/	/  /g' | sed -e 's=..../../.. ..:..:..=yyyy/mm/dd hh:mm:ss=' | awk '/^[^#]/ { print $0; }'`

end
