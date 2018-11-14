#ifndef __ZK_APP_H__
#define __ZK_APP_H__

#include <Application.h>
#include <StorageKit.h>
#include <String.h>
#include <Entry.h>

#include "ZKWindow.h"

class ZooKeeperApp : public BApplication
{
public:
	ZooKeeperApp (const char * app_sig);
	~ZooKeeperApp (void);
	
	virtual void	ArgvReceived	(int32 argc, char **argv);
	virtual void	RefsReceived	(BMessage * message);
	virtual	void	MessageReceived	(BMessage * message);
	virtual void	ReadyToRun		(void);
	virtual	void	AboutRequested	(void);

private:
	void			GetPreferences	(void);	
	void 			ProcessRefs		(BMessage * message);
	void			MakeShellSafe	(BString * filename);
	void 			RunScript		(void);
	void			Help			(void);
	status_t		MakeTempFolder	(void);

	bool	m_argv_received;
	bool	m_refs_received;
	bool	m_tracker_addon;
	
	BDirectory	*	m_tmp_dir;
	ZKWindow	*	m_window;

	entry_ref	m_app_ref;	
	BString		m_dir;
	BString		m_command;
	bool		m_keep_open;
	bool		m_in_terminal;


};

#endif
