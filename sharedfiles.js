/*
'   This file is provided under the LGPL license ver 2.1.',
'   Written by Katsumi.',
   http://hp.vector.co.jp/authors/VA016157/
'   kmorimatsu@users.sourceforge.jp',
*/

/*
'	This script is to copy shared files.',
*/

var filearray=[
	'args.c',
	'class.c',
	'cmpdata.c',
	'compiler.c',
	'debug.c',
	'error.c',
	'exception.c',
	'file.c',
	'float.c',
	'function.c',
	'globalvars.c',
	'library.c',
	'linker.c',
	'memory.c',
	'operator.c',
	'run.c',
	'string.c',
	'statement.c',
	'timer.c',
	'value.c',
	'varname.c',
	'compiler.h',
	'debug.h',
	'reservednames.js',
	'sharedfiles.js',
	'class.txt',
];

var WshShell = WScript.CreateObject("WScript.Shell");
var FSO = WScript.CreateObject("Scripting.FileSystemObject");


// Create a folder on desktop
try {
	var SharedFiles=WshShell.SpecialFolders("Desktop") + '\\sharedfiles\\';
	FSO.CreateFolder(SharedFiles);
} catch (e) {
	WScript.Echo('"sharedfiles" folder already exists on desktop!');
	WScript.Quit();
}

// Copy files to 'sharedfiles' directory
for(var i in filearray){
	FSO.CopyFile (filearray[i],SharedFiles + filearray[i]);
}

// All done
WScript.Echo('Done!');

