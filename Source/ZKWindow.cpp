#define DEBUG 0
#include <Debug.h>

#include <stdio.h>
#include <stdlib.h>

#include <Application.h>
#include <Roster.h>
#include <InterfaceKit.h>
#include <String.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <Path.h>
#include <File.h>
#include <Beep.h>
#include <Deskbar.h>

#include "ZKDefs.h"
#include "ZKWindow.h"
#include "EditView.h"

ZKWindow::ZKWindow(BRect winframe)
	:	BWindow(winframe, "ZooKeeper", B_TITLED_WINDOW, B_NOT_V_RESIZABLE),
	m_topbox	(NULL),
	m_editbox	(NULL),
	dropmode	(false)
{
	PRINT(("ZKWindow::ZKWindow()\n"));

	app_info app;
	be_app->GetAppInfo(& app);
	SetTitle(app.ref.name);
	
	BRect rect (Bounds());

	m_topbox = new BBox(rect, "view", B_FOLLOW_ALL_SIDES, B_NAVIGABLE, B_FANCY_BORDER);
	m_editbox = new	EditView(app.ref);
	
	m_topbox->AddChild(m_editbox);
	AddChild(m_topbox);

	MakeSettingsFolder();
	ReadSettings();
	
	SetSizeLimits(336, 800, 100, 5000);
}

ZKWindow::~ZKWindow()	
{
	
}

void 
ZKWindow::MessageReceived		(BMessage * message)
{
	// PRINT(("ZKWindow::MessageReceived()\n"));
	
	switch(message->what)
	{
		case ZK_DROPZONE_MODE:
			dropmode = true;
			break;
		
		case ZK_SETTINGS_MODE:
			dropmode = false;
			break;
			
		case B_SIMPLE_DATA:
			//if (dropmode)
			{
				m_editbox->Save();
				be_app->PostMessage(message);
			}
			break;
	
		case ZK_HELP_REQUESTED:
			be_app->PostMessage(message);
			break; 
			
		case B_ABOUT_REQUESTED:
			be_app->PostMessage(message);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

bool
ZKWindow::QuitRequested	(void)
{
	PRINT(("ZKWindow::QuitRequested()\n"));

	SaveSettings();
	m_editbox->Save();

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

status_t 
ZKWindow::MakeSettingsFolder	(void)
{
	PRINT(("ZKWindow::MakeSettingsFolder()\n"));

	status_t	status;
	BPath		path;
	if ((status = find_directory(B_USER_SETTINGS_DIRECTORY, & path)) != B_OK)
		return status;
	
	BEntry 		entry	(path.Path());

	// settings
	if (entry.Exists()	==	false	||	entry.IsDirectory()	==	false)	
		return B_ERROR;
	
	BDirectory	mother	(path.Path());
	BDirectory	baby;
	
	// Kirilla
	path.SetTo(path.Path(), "Kirilla");
	entry.SetTo(path.Path());
	if (! entry.Exists())
	{
		status	=	mother.CreateDirectory("Kirilla", & baby);
		if (status != B_OK && status != B_FILE_EXISTS)				return status;
	}
	else 
		if (! entry.IsDirectory())
			return B_FILE_EXISTS;
	
	if ((status = mother.SetTo(path.Path())) != B_OK)			return status;

	// ZooKeeper
	path.SetTo(path.Path(), "ZooKeeper");
	entry.SetTo(path.Path());
	if (! entry.Exists())
	{
		status	=	mother.CreateDirectory("ZooKeeper", & baby);
		if (status != B_OK && status != B_FILE_EXISTS)				return status;
	}
	else 
		if (! entry.IsDirectory())
			return B_FILE_EXISTS;
	
	if ((status = mother.SetTo(path.Path())) != B_OK)			return status;

	entry.SetTo(path.Path());
	if (entry.Exists()	&&	entry.IsDirectory())	return B_OK;
	else											return B_ERROR;
}

status_t
ZKWindow::ReadSettings	(void)
{
	PRINT(("ZKWindow::ReadSettings()\n"));

	status_t	status;
	BMessage	settings;
	BPath		path;
	BFile		file;
	BRect 		frame;

	// get path to settings-file
	if ((status = find_directory(B_USER_SETTINGS_DIRECTORY, & path)) != B_OK)
		return status;

	if ((status = path.SetTo(path.Path(),"Kirilla/ZooKeeper/settings")) != B_OK)
		return status;
		
	if ((status	= file.SetTo(path.Path(), B_READ_ONLY)) != B_OK)
		return status;

	// get message from file
	if ((status	=	settings.Unflatten(& file)) != B_OK)
		return status;

	// get settings from message
	if (settings.FindRect("winframe", & frame) == B_OK)
	{
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	}

	return B_OK;
}

status_t 
ZKWindow::SaveSettings	(void)
{
	PRINT(("ZKWindow::SaveSettings()\n"));

	status_t	status;
	BMessage 	settings;
	BPath		path;
	BFile		file;

	// add settings to message
	settings.AddRect("winframe", Frame());
	
	// get path to settings-file
	if ((status = find_directory(B_USER_SETTINGS_DIRECTORY, & path)) != B_OK)
		return status;

	if ((status = path.SetTo(path.Path(),"Kirilla/ZooKeeper/settings")) != B_OK)
		return status;
		
	if ((status	= file.SetTo(path.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE)) != B_OK)
		return status;

	if ((status	= settings.Flatten(& file))	!=	B_OK)
		return status;

	return B_OK;
}

