//#include "ContextMenu.h"

#include "FoobarSDKWrapper.h"

#include "BestVersion.h"
//#include "LastFm.h"
//#include "PlaylistGenerator.h"

//#include <map>

using namespace bestversion;

// {EC9EB231-1C89-47BA-AF87-811D80096B86}
static const GUID g_mainmenu_group_id = { 0xec9eb231, 0x1c89, 0x47ba,{ 0xaf, 0x87, 0x81, 0x1d, 0x80, 0x9, 0x6b, 0x86 } };


static mainmenu_group_popup_factory g_mainmenu_group(g_mainmenu_group_id, mainmenu_groups::edit, mainmenu_commands::sort_priority_dontcare, "Best Version");


class mainmenu_edit_commands : public mainmenu_commands {
public:
	enum {
		cmd_select_dead = 0,
		cmd_findalldead,
		cmd_findallnonlibrary,
		cmd_total
	};
	t_uint32 get_command_count() {
		return cmd_total;
	}
	GUID get_command(t_uint32 p_index) {
		// {BEEA7056-79F6-45AD-BCC0-EEB5AD934656}
		static const GUID guid_select_dead = { 0xbeea7056, 0x79f6, 0x45ad,{ 0xbc, 0xc0, 0xee, 0xb5, 0xad, 0x93, 0x46, 0x56 } };

		// {E45AED1B-2A80-4A94-BAE0-0FD3C55BFAED}
		static const GUID guid_findalldead = { 0xe45aed1b, 0x2a80, 0x4a94,{ 0xba, 0xe0, 0xf, 0xd3, 0xc5, 0x5b, 0xfa, 0xed } };

		// {177EF689-FB2A-41B7-BF28-7A3BEB511B72}
		static const GUID guid_findallnonlibrary = { 0x177ef689, 0xfb2a, 0x41b7,{ 0xbf, 0x28, 0x7a, 0x3b, 0xeb, 0x51, 0x1b, 0x72 } };


		switch (p_index) {
		case cmd_select_dead: return guid_select_dead;
		case cmd_findalldead: return guid_findalldead;
		case cmd_findallnonlibrary: return guid_findallnonlibrary;
		default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	void get_name(t_uint32 p_index, pfc::string_base & p_out) {
		switch (p_index) {
		case cmd_select_dead: p_out = "Select dead items in current playlist"; break;
		case cmd_findalldead: p_out = "Find dead items across all playlists"; break;
		case cmd_findallnonlibrary: p_out = "Find all non-library items in all playlists"; break;
		default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	bool get_description(t_uint32 p_index, pfc::string_base & p_out) {
		switch (p_index) {
		case cmd_select_dead: p_out = "Selects all dead items in current playlist."; return true;
		case cmd_findalldead: p_out = "Finds all dead items in all playlists and dumps them into a new playlist. Doesn't remove duplicates."; return true;
		case cmd_findallnonlibrary: p_out = "Finds all non-library items in all playlists and dumps them into a new playlist. Doesn't remove duplicates."; return true;
		default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	GUID get_parent() {
		return g_mainmenu_group_id;
	}
	void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) {
		switch (p_index) {
		case cmd_select_dead:
			//popup_message::g_show("This is a sample menu command.", "Blah");
			selectDeadItemsInActivePlaylist();
			break;
		case cmd_findalldead:
			//popup_message::g_show("Here was a function call to RunPlaybackStateDemo().", "Blah");
			findAllDeadItemsInAllPlaylists();
			break;
		case cmd_findallnonlibrary:
			findAllNonLibraryItemsInAllPlaylists();
			break;
		default:
			uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
};

static mainmenu_commands_factory_t<mainmenu_edit_commands> g_mainmenu_commands_sample_factory;
