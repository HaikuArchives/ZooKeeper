#define DEBUG 0
#include <Debug.h>

#include <Application.h>
#include <Roster.h>
#include <Window.h>
#include <Box.h>
#include <TextControl.h>
#include <Button.h>
#include <CheckBox.h>
#include <Node.h>
#include <NodeInfo.h>
#include <MenuItem.h>
#include <Bitmap.h>
#include <String.h>
#include <Beep.h>
#include <fs_attr.h>

#include "ZKDefs.h"
#include "EditView.h"

EditView::EditView(entry_ref ref)
: BBox(BRect(5,5,330,155), "EditView", B_FOLLOW_ALL),
	m_ref				(ref),
	m_menu_field		(NULL),
	m_dropdown_menu		(new BMenu ("Menu")),
	m_settings_item		(new BMenuItem("Settings", new BMessage(ZK_SETTINGS_MODE))),
	m_dropzone_item		(new BMenuItem("Dropzone", new BMessage(ZK_DROPZONE_MODE))),
	m_dir				(new BTextControl(BRect(5,55,318,75), "Working dir", "Working dir", "", NULL, B_FOLLOW_LEFT_RIGHT)),
	m_command			(new BTextControl(BRect(5,30,318,50), "Command", "Command", "", NULL, B_FOLLOW_LEFT_RIGHT)),
	m_in_terminal		(new BCheckBox (BRect(5,85,100,110), "in_terminal", "In Terminal", new BMessage(ZK_IN_TERMINAL))),
	m_keep_open			(new BCheckBox (BRect(5,105,100,125), "keep_open", "Keep Open", new BMessage(ZK_KEEP_TERMINAL))),
	m_filetypes_button	(new BButton(BRect(93,95,313,120), "filetypes", 
							"Edit Supported Filetypes, Icon, etc" B_UTF8_ELLIPSIS, 
							new BMessage(ZK_OPEN_FILETYPES), B_FOLLOW_LEFT_RIGHT)),
	m_views_visible		(true)
{
	PRINT(("EditView::EditView()\n"));

	m_dropdown_menu->SetRadioMode(true);
	m_dropdown_menu->SetLabelFromMarked(true);
	
	m_settings_item->SetMarked(true);
	
	m_dropdown_menu->AddItem(m_settings_item);
	m_dropdown_menu->AddItem(m_dropzone_item);
	m_dropdown_menu->AddSeparatorItem();
	m_dropdown_menu->AddItem(new BMenuItem("Help" B_UTF8_ELLIPSIS, new BMessage(ZK_HELP_REQUESTED)));
	m_dropdown_menu->AddItem(new BMenuItem("About" B_UTF8_ELLIPSIS, new BMessage(B_ABOUT_REQUESTED)));
	
	m_menu_field = new BMenuField(BRect(0,0,1,1), "menufield", "label", m_dropdown_menu);
	SetLabel(m_menu_field);
	
	float str_width	= StringWidth("Working dir");
	m_dir->SetDivider(str_width-5);
	m_command->SetDivider(str_width-5);
	
	m_filetypes_button->SetFlags(m_filetypes_button->Flags() | B_FULL_UPDATE_ON_RESIZE);
//	m_filetypes_button->MakeDefault(true);
	
	m_in_terminal->SetValue(true);
	m_keep_open->SetValue(true);
	
	AddChild(m_command);
	AddChild(m_dir);
	AddChild(m_in_terminal);
	AddChild(m_keep_open);
	AddChild(m_filetypes_button);
}

void 
EditView::AllAttached	(void)
{
	PRINT(("EditView::AllAttached()\n"));

	m_in_terminal->			SetTarget(this);
	m_keep_open->			SetTarget(this);
	m_filetypes_button->	SetTarget(this);
	m_dropdown_menu->		SetTargetForItems(this);
	
	Init();
}

EditView::~EditView()	
{
	PRINT(("EditView::~EditView()\n"));
	
	// void
}

void 
EditView::MessageReceived(BMessage * message)
{
	// PRINT(("EditView::MessageReceived()\n"));

	switch(message->what)
	{
		case ZK_SETTINGS_MODE:
			GotoSettingsMode();
			break;

		case ZK_DROPZONE_MODE:
			GotoDropzoneMode();
			break;

		case ZK_IN_TERMINAL:
			Save();
			break;

		case ZK_KEEP_TERMINAL:
			Save();
			break;

		case ZK_OPEN_FILETYPES:
			OpenFileTypes();
			break;

		case B_ABOUT_REQUESTED:
		case ZK_HELP_REQUESTED:
			if (m_views_visible)
				m_settings_item->SetMarked(true);
			else
				m_dropzone_item->SetMarked(true);
			be_app->PostMessage(message);			
			break;
		
		default:
			BBox::MessageReceived(message);
			break;			
	}
}

void 
EditView::Save	(void)
{
	PRINT(("EditView::Save()\n"));

	BNode		node		(& m_ref);
	off_t		bytes	=	0;	
	
	BString	command	=	m_command->Text();
	BString	dir		=	m_dir->Text();
	
	if (command.Length() != 0)
	{
		bytes = node.WriteAttr("zook:command", B_STRING_TYPE, 0, command.String(), command.Length()+1);
		if (! bytes == command.Length()+1)
			beep();
	}
	else
		node.RemoveAttr("zook:command");
	
	if (dir.Length() != 0)
	{
		bytes = node.WriteAttr("zook:dir", B_STRING_TYPE, 0, dir.String(), dir.Length()+1);
		if (! bytes == dir.Length()+1)
			beep();
	}
	else
		node.RemoveAttr("zook:dir");
	
	if (m_in_terminal->Value())
		bytes = node.WriteAttr("zook:terminal", B_STRING_TYPE, 0, "Y", 2);
	else
		node.RemoveAttr("zook:terminal");

	if (m_keep_open->Value())
		bytes = node.WriteAttr("zook:keepopen", B_STRING_TYPE, 0, "Y", 2);
	else
		node.RemoveAttr("zook:keepopen");

}

void 
EditView::Init	(void)
{
	PRINT(("EditView::Init()\n"));

	BNode		node		(& m_ref);
	BNodeInfo	nodeinfo	(& node);

	char * buffer	=	new char [4096];
	off_t	bytes	=	0;
	
	bytes = node.ReadAttr("zook:command", B_STRING_TYPE, 0, buffer, 4095);
	if (bytes >= 0)
	{
		buffer[bytes] = '\0';
		m_command->SetText(buffer);
	}
	
	bytes = node.ReadAttr("zook:dir", B_STRING_TYPE, 0, buffer, 4095);
	if (bytes >= 0)
	{
		buffer[bytes] = '\0';
		m_dir->SetText(buffer);
	}
	
	attr_info	info;
	if (node.GetAttrInfo("zook:terminal", & info) == B_OK)
		m_in_terminal->SetValue(true);
	else
		m_in_terminal->SetValue(false);
	
	
	if (node.GetAttrInfo("zook:keepopen", & info) == B_OK)
		m_keep_open->SetValue(true);
	else
		m_keep_open->SetValue(false);
	
	delete [] buffer;
}

void 
EditView::OpenFileTypes	(void)
{
	PRINT(("EditView::OpenFileTypes()\n"));

	BMessage	msg	(B_REFS_RECEIVED);
	msg.AddRef("refs", & m_ref);
	be_roster->Launch("application/x-vnd.Be-MIMA", & msg);
}

void 
EditView::GotoSettingsMode (void)
{
	PRINT(("EditView::GotoSettingsMode()\n"));
	
	Window()->PostMessage(ZK_SETTINGS_MODE);
	
	if (! m_views_visible)
	{
		m_dir->Show();
		m_command->Show();
		m_in_terminal->Show();
		m_keep_open->Show();
		m_filetypes_button->Show();
		
		m_views_visible = true;
	}
	
}

void 
EditView::GotoDropzoneMode	(void)
{
	PRINT(("EditView::GotoDropzoneMode()\n"));
	
	Window()->PostMessage(ZK_DROPZONE_MODE);
	
	if (m_views_visible)
	{
		m_dir->Hide();
		m_command->Hide();
		m_in_terminal->Hide();
		m_keep_open->Hide();
		m_filetypes_button->Hide();
		
		m_views_visible = false;
	}
}

