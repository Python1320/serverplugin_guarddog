dofile("../common.lua")

RequireDefaultlibs()

SOLUTION"guarddog"
	
	targetdir	"Release"
	INCLUDES	"source_sdk"
	defines		{"NDEBUG"}
	
	WINDOWS()
	LINUX()

	PROJECT()
		SOURCE_SDK_LINKS()
		configuration 		"windows"
		configuration 		"linux"
