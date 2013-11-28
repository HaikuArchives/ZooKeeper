#ifndef __EDIT_VIEW_H__
#define __EDIT_VIEW_H__

#include <Box.h>
#include <MenuField.h>

class EditView : public BBox
{
public:
					EditView	(entry_ref ref);
					~EditView	();

	virtual void	MessageReceived 	(BMessage * message);
	virtual void 	AllAttached			(void);

			void	Save				(void);
protected:
private:
			void	Init				(void);
			void	OpenFileTypes		(void);
			void	GotoSettingsMode	(void);
			void	GotoDropzoneMode	(void);

	entry_ref	m_ref;

	BMenuField	*	m_menu_field;
	BMenu 		*	m_dropdown_menu;
	
	BMenuItem	*	m_settings_item;
	BMenuItem	*	m_dropzone_item;
	
	BTextControl *	m_dir;
	BTextControl *	m_command;
	
	BCheckBox	*	m_in_terminal;
	BCheckBox 	*	m_keep_open;

	BButton		* 	m_filetypes_button;
	
	bool		m_views_visible;
};

#endif

