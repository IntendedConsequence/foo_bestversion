//#include "ContextMenu.h"

#include "FoobarSDKWrapper.h"

#include "BestVersion.h"
//#include "LastFm.h"
#include "PlaylistGenerator.h"

//#include <map>

using namespace bestversion;


void findAllDeadItemsInAllPlaylists();

// {EC9EB231-1C89-47BA-AF87-811D80096B86}
static const GUID g_mainmenu_group_id = { 0xec9eb231, 0x1c89, 0x47ba,{ 0xaf, 0x87, 0x81, 0x1d, 0x80, 0x9, 0x6b, 0x86 } };


static mainmenu_group_popup_factory g_mainmenu_group(g_mainmenu_group_id, mainmenu_groups::edit, mainmenu_commands::sort_priority_dontcare, "Best Version");


class mainmenu_edit_commands : public mainmenu_commands {
public:
	enum {
		cmd_select_dead = 0,
		cmd_select_nonlibrary,
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

		// {22D1A566-BED6-46DA-BD9E-0CCC9B890ECB}
		static const GUID guid_select_nonlibrary = { 0x22d1a566, 0xbed6, 0x46da,{ 0xbd, 0x9e, 0xc, 0xcc, 0x9b, 0x89, 0xe, 0xcb } };


		// {177EF689-FB2A-41B7-BF28-7A3BEB511B72}
		static const GUID guid_findallnonlibrary = { 0x177ef689, 0xfb2a, 0x41b7,{ 0xbf, 0x28, 0x7a, 0x3b, 0xeb, 0x51, 0x1b, 0x72 } };


		switch (p_index) {
		case cmd_select_dead: return guid_select_dead;
		case cmd_select_nonlibrary: return guid_select_nonlibrary;
		case cmd_findalldead: return guid_findalldead;
		case cmd_findallnonlibrary: return guid_findallnonlibrary;
		default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	void get_name(t_uint32 p_index, pfc::string_base & p_out) {
		switch (p_index) {
		case cmd_select_dead: p_out = "Select dead items in current playlist"; break;
		case cmd_select_nonlibrary: p_out = "Select non-library items in current playlist"; break;
		case cmd_findalldead: p_out = "Find dead items across all playlists"; break;
		case cmd_findallnonlibrary: p_out = "Find all non-library items in all playlists"; break;
		default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	bool get_description(t_uint32 p_index, pfc::string_base & p_out) {
		switch (p_index) {
		case cmd_select_dead: p_out = "Selects all dead items in current playlist."; return true;
		case cmd_select_nonlibrary: p_out = "Selects all non-library items in current playlist."; return true;
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
			selectDeadItemsInActivePlaylist();
			break;
		case cmd_select_nonlibrary:
			selectNonLibraryItemsInActivePlaylist();
			break;
		case cmd_findalldead:
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


class DeadItemsPlaylistGenerator : public threaded_process_callback
{
private:
	static_api_ptr_t<playlist_manager> pm;
	pfc::list_t<metadb_handle_ptr> all_playlist_tracks;
	pfc::list_t<metadb_handle_ptr> dead_tracks;
	bool success;

public:
	DeadItemsPlaylistGenerator()
		: success(false)
	{
	}

	virtual void on_init(HWND /*p_wnd*/)
	{
		pfc::list_t<metadb_handle_ptr> tracks;
		t_size playlist_count = pm->get_playlist_count();
		for (t_size playlist_index = 0; playlist_index < playlist_count; playlist_index++)
		{
			pm->playlist_get_all_items(playlist_index, tracks);
			all_playlist_tracks.add_items(tracks);
		}
	}

	virtual void run(threaded_process_status& p_status, abort_callback& p_abort)
	{
		try
		{
			console::info("Finding all dead items in all playlists...");

			p_status.set_item("Finding all dead items in all playlists...");
			p_abort.check();

			for (t_size index = 0; index < all_playlist_tracks.get_count(); index++)
			{
				p_status.set_progress(index, all_playlist_tracks.get_count());
				p_abort.check();
				if (strstr(all_playlist_tracks[index]->get_path(), "3dydfy") == NULL)
				{
					if (!filesystem::g_exists(all_playlist_tracks[index]->get_path(), p_abort))
					{
						dead_tracks.add_item(all_playlist_tracks[index]);
					}
				}
			}

			if (dead_tracks.get_count() == 0)
			{
				throw pfc::exception("Did not find enough dead tracks to make a playlist");
			}

			success = true;
			p_abort.check();
		}
		catch (exception_aborted&)
		{
			success = false;
		}
	}

	virtual void on_done(HWND /*p_wnd*/, bool /*p_was_aborted*/)
	{
		if (!success)
		{
			return;
		}

		generatePlaylistFromTracks(dead_tracks, "[foo_bestversion]DEAD ITEMS");
	}
};

void findAllDeadItemsInAllPlaylists()
{
	std::string title("Generating dead tracks playlist");

	// New this up since it's going to live on another thread, which will delete it when it's ready.
	auto generator = new service_impl_t<DeadItemsPlaylistGenerator>();

	static_api_ptr_t<threaded_process> tp;

	tp->run_modeless(
		generator,
		tp->flag_show_abort | tp->flag_show_item | tp->flag_show_progress | tp->flag_show_delayed,
		core_api::get_main_window(),
		title.c_str(),
		pfc_infinite
	);
}