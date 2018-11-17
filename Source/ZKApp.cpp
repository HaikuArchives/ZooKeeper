#define DEBUG 0
#include <Debug.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <Alert.h>
#include <Application.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <fs_attr.h>
#include <Node.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>

#include "ZKApp.h"
#include "ZKWindow.h"

ZooKeeperApp::ZooKeeperApp	(const char * app_sig)
 : BApplication	(app_sig),
	m_argv_received		(false),
	m_refs_received		(false),
	m_tracker_addon		(false),
	m_tmp_dir			(NULL),
	m_window			(NULL),
	m_app_ref			(),
	m_dir				(),
	m_command			(),
	m_keep_open			(false),
	m_in_terminal		(false)
{
	if (MakeTempFolder() != B_OK)
	{
		fprintf(stderr, "Unable to create temp folder.");
		exit(-1);	
	}
	
	m_window = 	new ZKWindow (BRect(300,200,636,360));
	
	GetPreferences();
}

ZooKeeperApp::~ZooKeeperApp	(void)	
{
	// void
}

void 
ZooKeeperApp::ArgvReceived	(int32 argc, char **argv)
{
	m_argv_received = true;
	
	BMessage msg (B_REFS_RECEIVED);

	for (int32 i = 1;  i < argc;  i++)
	{
		BEntry entry (argv[i]);

		entry_ref ref;
		entry.GetRef (& ref);
		
		if (entry.Exists())
			msg.AddRef ("refs", & ref);
		else
			printf("File not found: %s\n", argv[i]);
	}
	
	RefsReceived(& msg);
}

void 
ZooKeeperApp::RefsReceived	(BMessage * message)
{
	entry_ref dir_ref;
	
	if (message->FindRef("dir_ref", & dir_ref) == B_OK)
		m_tracker_addon = true;
	
	ProcessRefs(message);
}

void
ZooKeeperApp::ReadyToRun	(void)
{
	if (! m_argv_received && ! m_refs_received && ! m_tracker_addon)
	{
		m_window->Show();
		return;
	}

	PostMessage(B_QUIT_REQUESTED);
}

void 
ZooKeeperApp::MessageReceived	(BMessage * message)
{
	switch(message->what)
	{
		case B_SIMPLE_DATA:
				ProcessRefs(message);
				break;
	
		case ZK_HELP_REQUESTED:
				Help();
				break;
				
		default:
				BApplication::MessageReceived(message);
				break;
	}
}

void
ZooKeeperApp::AboutRequested	(void)
{
	PRINT(("ZooKeeperApp::AboutRequested()\n"));

	BAlert * alert = new BAlert("ZooKeeper", "ZooKeeper 2.1.1\n\n"
	
	"created by Jonas Sundström\n"
	"www.kirilla.com\n"
	"jonas@kirilla.com\n\n"
	
	"icon stolen from early BeOS\n\n"
	
	"copyleft 2004, 2005\n"
	"released to the public domain", 
	"Go to website", "Close", NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);

	int32 reply = alert->Go();
	
	if (reply == 0)
	{
		BMessage msg(B_ARGV_RECEIVED); 
		msg.AddString("argv", "app"); 
		msg.AddString("argv", "http://www.kirilla.com/");  
		msg.AddInt32("argc", 2); 
		be_roster->Launch ("application/x-vnd.Be.URL.http", &msg ); 			
	}
}

void 
ZooKeeperApp::Help	(void)
{
	PRINT(("ZooKeeperApp::Help()\n"));

	(new BAlert("ZooKeeper", 
	
	"ZooKeeper is a glue application (or frontend) that lets you specify "
	"a shell command or script to be executed on a set of files.\n\n"
	
	"It's designed to be cloned and customized.\n\n"
	
	"When launched without any arguments or files "
	"ZooKeeper provides a window where you add "
	"a shell command, a working directory, and other "
	"preferences. These settings are stored with the executable "
	"making it possible to keep multiple copies of ZooKeeper for "
	"different purposes.\n\n"
	
	"Ways to use ZooKeeper:\n"
	"* Filedrops on a ZooKeeper window\n"
	"* Filedrops on a ZooKeeper icon\n"
	"* Using Tracker's Open With... menu\n"
	"* As the Preferred Application of a filetype\n"
	"* As Tracker add-ons\n\n"
	
	"If you want ZooKeeper to do anything useful at all you "
	"need to supply a shell command, including the $zkfiles variable "
	"which will be substituted by the set of files "
	"given to ZooKeeper by any of its launch mechanisms.\n\n"

	"Example:\n"
	"Dropping 'wow.ps' on a ZooKeeper "
	"where the command is set to\n"
	"~/config/bin/psviewer $zkfiles\n"
	"Your file 'wow.ps' will open in psviewer\n\n"
	
	"As a Tracker add-on, ZooKeeper also provides the $zkfolder "
	"variable, should you ever need it.\n\n"
	
	"It's best to give each copy of ZooKeeper a "
	"a unique name, describing its purpose, "
	"and a unique Application Signature.", "Ok", 
	
	NULL, NULL, B_WIDTH_AS_USUAL, B_IDEA_ALERT))->Go();
}

void 
ZooKeeperApp::ProcessRefs	(BMessage * message)
{
	PRINT(("ZooKeeperApp::ProcessRefs()\n"));

	GetPreferences();

	entry_ref	dir_ref;
	BPath		dir_path;
	entry_ref	ref;
	int32		ref_count = 0;
	type_code	type;

	// do we have refs?
	
	message->GetInfo("refs", & type, & ref_count);
	if (ref_count < 1)
	{
		fprintf(stderr, "No files to process.\n");
		return;
	}
	
	if (m_tracker_addon)	
	{
		if ((m_command.FindFirst(ZK_FILES) < 0)
		 && (m_command.FindFirst(ZK_FOLDER) < 0))
		{
			fprintf(stderr, "No command to process files with.\n");
			return;
		}
		
		if (message->FindRef("dir_ref", & dir_ref) == B_OK)
		{
			BEntry entry (& dir_ref);
			dir_path.SetTo(& entry);
		}
	}
	else
	{
		if (m_command.FindFirst(ZK_FILES) < 0)
		{
			fprintf(stderr, "No command to process files with.\n");
			return;
		}
	}	
	
	BPath	path;
	BString	temp;
	BString	files;

	ref_count = 0;

	while(message->FindRef("refs", ref_count, & ref) == B_OK)
	{
		m_refs_received = true;
	
		path.SetTo(& ref);		
		temp	=	path.Path();
		MakeShellSafe(& temp);
		files << " " << temp;
		ref_count++;	
	}

	m_command.ReplaceAll(ZK_FILES, files.String());
	m_command.ReplaceAll(ZK_FOLDER, dir_path.Path());

	RunScript();
}

void 
ZooKeeperApp::GetPreferences(void)	
{
	PRINT(("ZooKeeperApp::GetPreferences()\n"));

	// app ref
	app_info	my_app_info;
	be_app->GetAppInfo(& my_app_info);
	m_app_ref	=	my_app_info.ref;

	// app node
	BNode	node	(& m_app_ref);
	
	char *	buffer	=	new char [4096];
	off_t	bytes	=	0;

	// dir
	bytes = node.ReadAttr("zook:dir", B_STRING_TYPE, 0, buffer, 4095);
	if (bytes >= 0)
	{
		buffer[bytes] = '\0';
		m_dir	=	buffer;
	}
	else
		m_dir.Truncate(0);
	
	// command
	bytes = node.ReadAttr("zook:command", B_STRING_TYPE, 0, buffer, 4095);
	if (bytes >= 0)
	{
		buffer[bytes] = '\0';
		m_command	=	buffer;
	}
	else
		m_command.Truncate(0);
	
	// in terminal
	attr_info	info;
	if (node.GetAttrInfo("zook:terminal", & info) == B_OK)
		m_in_terminal	=	true;
	else
		m_in_terminal	=	false;
	
	// keep open
	if (node.GetAttrInfo("zook:keepopen", & info) == B_OK)
		m_keep_open	=	true;
	else
		m_keep_open	=	false;
	
	delete [] buffer;
}

void 
ZooKeeperApp::RunScript(void)
{
	PRINT(("ZooKeeperApp::RunScript()\n"));

	// create kickstart_script
	BString kickstart_script	("#!/bin/sh\n");

	// Remove ourselves on signal and termination
	kickstart_script.Append("trap 'rm -f \"$0\"' 0 1 2 3 13 15\n");

	// working dir
	kickstart_script.Append("cd ");	
	kickstart_script.Append(m_dir);	
	kickstart_script.Append("\n");	

	// command
	kickstart_script.Append(m_command);
	kickstart_script.Append("\n");
	
	// keep open
	if (m_keep_open && m_in_terminal)
		kickstart_script.Append("read -p \"\n(Press Enter to close)\"\n");

	//	create tempfile name
	int32	counter	=	1;
	BString tempfile_name;
	tempfile_name << counter;
	while(m_tmp_dir->Contains(tempfile_name.String()))
	{
		counter	++;
		tempfile_name.Truncate(0);
		tempfile_name << counter;
	}
	
	// write kickstart script to file
	BFile	tmp_file;
	if (m_tmp_dir->CreateFile(tempfile_name.String(), & tmp_file) != B_OK)
		exit(-1);
		
	tmp_file.Write(kickstart_script.String(), kickstart_script.Length());
	printf("%s", kickstart_script.String());
	// open Terminal with kickstart script
	
	BPath	tempfile_path	(m_tmp_dir, tempfile_name.String());
	BString command_string	(tempfile_path.Path());
	
	BString	app_name = m_app_ref.name;
	MakeShellSafe(& app_name);

	if (m_in_terminal)
	{
		command_string.Prepend(" ");
		command_string.Prepend(app_name.String());
		command_string.Prepend("/boot/system/apps/Terminal -t");
	}
	// This should solve the permission error.
	command_string.Prepend("sh ");
	command_string.Append(" &");
	printf("%s", command_string.String());
	PRINT(("system(%s)\n", command_string.String()));
	system(command_string.String());	
}

void
ZooKeeperApp::MakeShellSafe	(BString * filename)
{
	filename->CharacterEscape("'", '\\');
	filename->Prepend("'"); 
	filename->Append("'"); 
}

status_t
ZooKeeperApp::MakeTempFolder(void)
{
	PRINT(("ZooKeeperApp::MakeTempFolder()\n"));

	status_t status;
	BPath p;
	if (find_directory(B_SYSTEM_TEMP_DIRECTORY, &p) != B_OK)	return B_ERROR;
	
	m_tmp_dir	=	new BDirectory();
	BDirectory temp_dir;
	
	if ((status = m_tmp_dir->	SetTo(p.Path())) != B_OK) 	return status;
	status	=	m_tmp_dir->	CreateDirectory("Kirilla_ZooKeeper", &temp_dir);
	if (status != B_OK && status != B_FILE_EXISTS)	return status;

	p.SetTo(p.Path(),"Kirilla_ZooKeeper");
	if ((status = m_tmp_dir->	SetTo(p.Path())) != B_OK)	return status;
		
	BEntry entry(p.Path());
	if (entry.Exists()	&&	entry.IsDirectory())	return B_OK;
	else											return B_ERROR;
}
