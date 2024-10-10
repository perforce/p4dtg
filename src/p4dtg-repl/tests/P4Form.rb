#!/usr/local/bin/ruby
#
# Copyright 2005 Perforce Software.  All rights reserved.
#
#
# P4 Form Data Class and associated useful functions
#

#
# Extract a set from a string
#
def extract_set(str)
	list = []
	if str != nil && str.length > 0 then
		list = str.split(/[\t \n,]+/).compact      # Drop nil
		list.delete_if { |val| val =~ /^[\t ]*$/ } # Drop ""
		list.uniq!                                 # Drop dups
	end
	return list
end

#
# Lookup email address for userid
#
def get_email(userid)
	if !(userid =~ /@/) then
		#
		# p4 user will always return a value so this may not
		# be a real email address. 
		# P4D: if p4 user -o uid would return an error for non-existant
		#      users, then this would need to change to handle that case
		cmd=`p4 user -o #{userid} | egrep "^Email:"`.chomp!
		parts = cmd.split(/[ \t][ \t]*/)
		if parts.length > 1 then
			return parts[1]
		else
			return nil
		end
	else
		return userid
	end
end

#
# Tests if the strlist string is a list of the specified kind
#
def islistof(strlist, kind)
	list=extract_set(strlist)
	list.each do |item|
		cmd="#{kind}(\"#{item}\")"
		if (item =~ /["]/) || (not eval(cmd)) then
			return false
		end
	end
	return true
end

#
# Tests if value string is a changelist
#
def ischangelist(in_value)
	if in_value == nil || in_value.strip == "" then
		return true
	end
	value = in_value.strip
	if value =~ /^[1-9][0-9]*$/ then
		#
		# p4 change -o cid returns an error if cid does not exist
		#
		cmd=`p4 change -o #{value} > /dev/null 2>&1 ; echo $?`.chomp!
		if cmd == "0" then
			return true
		else
			return false
		end
	else
		return false
	end
	return false
end

#
# Tests if value string is a userid
#
def isuserid(in_value)
	if in_value == nil || in_value.strip == "" then
		return true
	end
	value = in_value.strip
	if value =~ /^[-_.a-zA-Z0-9]+$/ then
		#
		# p4 user -o uid always returns something even if uid does not exist
		#   so this uses p4 users to list them all and see if the one exists
		#   This is terribly inefficient
		# P4D: add support so that p4 user -o uid returns an error if the
		#      user does not exist. If this is done then the change would
		#      be to make this like ischangelist above
		#
		cmd=`p4 users | egrep "^#{value} " > /dev/null 2>&1 ; echo $?`.chomp!
		if cmd == "0" then
			return true
		else
			return false
		end
	else
		return false
	end
	return false
end

#
# Tests if value string is a jobid
#
def isjobid(in_value)
	if in_value == nil || in_value.strip == "" then
		return true
	end
	value = in_value.strip
	if value =~ /^[-_.a-zA-Z0-9]+$/ then
		#
		# This requires the spec depot to be enabled in order to work
		# P4D: add support such that p4 job -o jid returns an error if the
		#      jid does not exist. If this is done then the change would be
		#      to make this like ischangelist above
		#
		cmd=
			`p4 fstat //spec/job/#{value}.p4s 2>&1 | wc -l | awk '{ print $1; }'`.to_i
		if cmd > 1 then
			return true
		else
			return false
		end
	else
		return false
	end
	return false
end

#
# Tests if value string is a depot path
#
def isdepotpath(in_value)
	if in_value == nil || in_value.strip == "" then
		return true
	end
	value = in_value.strip
	#
	# This does not do much checking, just basic syntax of // followed by
	#  something that is not whitespace
	#
	if value =~ /^\/\/[^\t ]+.*$/ then
		return true
	else
		return false
	end
	return false
end

#
# Tests if value string is a path to an existing file
#
def isfilepath(in_value)
	if in_value == nil || in_value.strip == "" then
		return true
	end
	value = in_value.strip
	if value =~ /.*[.][.][.]$/ then
		return false
	end
	if value =~ /^\/\/[^\t ]+.*$/ then
		#
		# This is a bit hokey, there may be a better way to check to 
		# see if a file exists in the depot
		#
		cmd=`p4 fstat #{value} 2>&1 | wc -l | awk '{ print $1; }'`.to_i
		if cmd > 1 then
			return true
		else
			return false
		end
	else
		return false
	end
	return false
end

#
# Applies the test specified by check to the string value
#
def checkfield( check, value )
	if check == nil then
		return true
	end
	thetest = check.gsub(/[$]VALUE/, "#{value}")
	result=eval(thetest)
	if result then
		return true
	else
		return false
	end
end

class P4Form
	#
	# A form is composed of a set of fields interspersed with
	# comments delimited by the # character
	#
	# Fields are of the following two varieties:
	#   ^field: value
	#
	#   ^field:
	#   [\t ]value...
	#
	#	EJPUserID: user  Special field indicating that EJP is updating job
	#                  so allow read-only fields to be updated and use
	#                  this value as the user

	def initialize( formdata )
		@fields = {}
		@userid = nil			# Special field, see below for description
		@chain = []				# Special field, see below for description
		@revnum = nil			# Special field, see below for description

		infield=false
		curfield=nil
		curvalue=nil

		formdata.each do |line|
			line.chomp!
			case line
				when /^#/
					# ignore comment lines
				when /^[^\t ]/
					# begin field
					if infield && 
							curvalue != nil && curvalue.strip != "" then
						@fields[curfield] = curvalue
					end
					infield=true
					curfield = line[/^[^:]*/].strip
					curvalue = line.sub(/^[^:]*:[\t ]*/, "")
				when /^[\t ]/
					# continue field
					if infield then
						curvalue = "#{curvalue}\n#{line}"
					else
						# unknown
					end
				else
					# unknown
			end
		end
		
		# If in a field, make sure to save it
		if infield &&
				curvalue != nil && curvalue.strip != "" then
			@fields[curfield] = curvalue
		end

		#
		# Special Field indicating overriding of readonly fields and the
		# UserID to be used for such changes
		#
		if @fields.has_key?("EJPUserID") then
			@userid = @fields["EJPUserID"]
			@fields.delete("EJPUserID")
		end

		#
		# Special Field indicating previous Jobs that should not be modified
		# as a result of any changes/additions/deletions
		#
		if @fields.has_key?("EJPChain") then
			@chain = extract_set(@fields["EJPChain"])
			@fields.delete("EJPChain")
		end

		#
		# Special Field indicating which revision in the spec depot to 
		# use for determining changes
		#
		if @fields.has_key?("EJPRevNum") then
			@revnum = extract_set(@fields["EJPRevNum"])
			@fields.delete("EJPRevNum")
		end
	end
	
	attr_reader :fields
	attr_reader :userid
	attr_reader :chain
	attr_reader :revnum

	def print(obj)
		for f in @fields.keys do
			obj.printf("%s: %s\n\n", f, fields[f])
		end
	end

	def validate( checks )
		invalid = []
		invalid << @fields.keys
		invalid.delete_if { |val| checkfield(checks[val], @fields[val]) }
		return invalid
	end

	def validate_fields( thesefields, checks )
		invalid = []
		invalid = thesefields.clone
		invalid.delete_if { |val| checkfield(checks[val], @fields[val]) }
		return invalid
	end
		
end

