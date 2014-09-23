dofile("../common.lua")

RequireDefaultlibs()

SOLUTION"guarddog"
	
	INCLUDES	"source_sdk"
	
	WINDOWS()
	LINUX()

	PROJECT()
		SOURCE_SDK_LINKS()
		configuration 		"windows"
	        targetprefix    	"serverplugin_"
			targetsuffix    	""
			targetextension 	".dll"
		configuration 		"linux"
	        targetprefix    	"serverplugin_"
			targetsuffix    	""
			targetextension 	".so"