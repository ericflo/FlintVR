#!/usr/bin/python

# This first bit of code is common bootstrapping code
# to determine the SDK root, and to set up the import
# path for additional python code.

#begin bootstrap
import os
import sys

def init():
	root = "../"
	while( os.path.isdir( root + "bin/scripts/build" ) == False ):
		root += "../"
		if( len(root) > 30 ):
			print "Unable to find SDK root. Exiting."
			sys.exit(1)
	root = os.path.abspath(root)
	os.environ['OCULUS_SDK_PATH'] = root
	os.environ['NDK_MODULE_PATH'] = root
	sys.path.append( root + "/bin/scripts/build" )

init()
import ovrbuild
ovrbuild.init()
#end bootstrap


ovrbuild.build()
