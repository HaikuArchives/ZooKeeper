// ZooKeeper 2.1
// .............................
// created by Jonas Sundström
// www.kirilla.com
// jonas@kirilla.com
// 
// copyleft 2004, 2005
// released to the public domain
// .............................

#define DEBUG 0
#include <Debug.h>

#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <Application.h>
#include <Path.h>
#include <Query.h>
#include <Roster.h>
#include <String.h>
#include <TrackerAddOn.h>
#include <VolumeRoster.h>

#include <fs_attr.h>
#include <fs_info.h>
#include <fs_query.h>
#include <image.h>

#include "ZKApp.h"
#include "ZKDefs.h"

void app_sig_for_pathname(const char * path, BString * signature);
bool duplicates_exist (const char * signature);

int main(int argc, char *argv[], char *arge[]) 
{
	PRINT(("argv[0]: %s\n", argv[0]));

	BString signature = ZK_APP_SIG;

	if (duplicates_exist(signature.String()))
	{
		PRINT(("*****************************************************************\n"));
		PRINT(("******* MULTIPLE EXECUTABLES WITH THE SAME MIMETYPE !!!!! *******\n"));
		PRINT(("*****************************************************************\n"));
	}

	app_sig_for_pathname (argv[0], & signature);
	
	PRINT(("application signature: %s\n", signature.String()));
	
	new ZooKeeperApp(signature.String());
	be_app-> Run();
	
	delete	be_app;	

	return	(0);
}

void
process_refs(entry_ref dir_ref, BMessage* msg, void* reserved)
{
	PRINT(("process_refs()\n"));
	
	msg->AddRef("dir_ref", & dir_ref);
	
	// get the path of the Tracker add-on
	image_info image;
	int32 cookie = 0;
	status_t status = B_OK;

	status = get_next_image_info(0, &cookie, &image);

	while (status == B_OK)
	{
		if (((char*)process_refs >= (char*)image.text &&
			(char*)process_refs <= (char*)image.text + image.text_size)
			||
			((char*)process_refs >= (char*)image.data &&
			(char*)process_refs <= (char*)image.data + image.data_size))
			
		{
			PRINT(("****** found our image: %s\n", image.name));
			break;
		}
		
		status = get_next_image_info(0, &cookie, &image);
	}

	entry_ref addon_ref;
	
	if (get_ref_for_path(image.name, & addon_ref) == B_OK)
	{
		// it's better to launch Tracker add-ons by entry_ref
		// in the event of multiple instances with the same
		// applicatins signature
		
		PRINT(("Launching Tracker add-on by entry_ref.\n"));
		
		be_roster->Launch (& addon_ref, msg);
		
	}
	else
	{
		// if get_ref_for_path() fails we can launch by app sig
	
		PRINT(("Launching Tracker add-on by signature.\n"));
	
		BString signature = ZK_APP_SIG;
		app_sig_for_pathname(image.name, & signature);
		PRINT(("application signature: %s\n", signature.String()));
		be_roster->Launch (signature.String(), msg);
	}	

	fflush(stdout);
}

void app_sig_for_pathname(const char * path, BString * signature) 
{
	*signature = ZK_APP_SIG;

	BNode node (path);
	attr_info info;
	char buffer [B_MIME_TYPE_LENGTH];
	if (node.GetAttrInfo("BEOS:APP_SIG", & info) == B_OK)
	{
		PRINT(("node.GetAttrInfo(BEOS:APP_SIG, & info) == B_OK\n"));
	
		if (node.ReadAttr("BEOS:APP_SIG", info.type, 0, & buffer, info.size) == info.size)
		{
			PRINT(("node.ReadAttr(BEOS:APP_SIG, info.type, 0, & buffer, info.size) == B_OK\n"));
			
			PRINT(("attribute: %s \n", buffer));
			*signature = buffer;
		}
		else
			PRINT(("No BEOS:APP_SIG attribute. Fallback to default.\n"));
	}
	else
		PRINT(("No BEOS:APP_SIG attribute. Fallback to default.\n"));		

}

bool duplicates_exist (const char * signature)
{
	BVolumeRoster	roster;
	BVolume			volume;
	BQuery			query;
	BString			query_string	=	"BEOS:APP_SIG=";
	BEntry			entry;
	mode_t			permissions;
	uid_t			owner;
	gid_t			group;
	int32			query_hits		=	0;
	
	query_string += signature;
	
	while (roster.GetNextVolume(& volume) == B_OK)
	{
		if (volume.KnowsQuery())
		{
			PRINT(("volume.KnowsQuery()\n"));
			
			char volname [B_FILE_NAME_LENGTH];
			volume.GetName(volname);
			PRINT(("volume: %s\n", volname));
			
			query.Clear();
			
			if (query.SetVolume(& volume) == B_OK)
			{
				PRINT(("query.SetVolume(& volume) == B_OK\n"));
			
				if (query.SetPredicate(query_string.String()) == B_OK)
				{
					PRINT(("query.SetPredicate(%s) == B_OK\n", query_string.String()));
				
					if (query.Fetch() == B_OK)
					{
						PRINT(("query.Fetch() == B_OK\n"));
					
						while (query.GetNextEntry(& entry) == B_OK)
						{
							PRINT(("query.GetNextEntry(& entry) == B_OK\n"));
							
							entry.GetPermissions(& permissions);
							entry.GetOwner(& owner);
							entry.GetGroup(& group);
							
							BPath path (& entry);

							// if (access(path.Path(), X_OK))

							if	(((owner == getuid()) && (permissions & S_IXUSR))
							||	((group == getgid()) && (permissions & S_IXGRP))
							||	(permissions & S_IXOTH))
							{
								PRINT(("path is executable: %s\n", path.Path()));
								query_hits++;
							}
							else
							{
								PRINT(("path is NOT executable: %s\n", path.Path()));
							}
						}
					}
				}
			}
		}
	
		fflush(stdout);
	}
	
	if (query_hits > 1)
		return true;
	else
		return false;
}

