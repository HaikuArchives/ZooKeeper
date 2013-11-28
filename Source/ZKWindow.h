#ifndef __ZK_WINDOW_H__
#define __ZK_WINDOW_H__

#include <Window.h>
#include <Box.h>

#include "ZKDefs.h"
#include "EditView.h"

class ZKWindow : public BWindow
{
public:
					ZKWindow			(BRect winframe);
					~ZKWindow			();
	virtual void	MessageReceived 	(BMessage * message);
	virtual bool	QuitRequested		();
			
private:
	status_t	MakeSettingsFolder	(void);
	status_t	ReadSettings		(void);
	status_t	SaveSettings		(void);
	void		FileDropped			(BMessage * message);	
	void		DoneEditing			(void);
			
	BBox		*	m_topbox;
	EditView	*	m_editbox;
	
	bool dropmode;
	
};

#endif

