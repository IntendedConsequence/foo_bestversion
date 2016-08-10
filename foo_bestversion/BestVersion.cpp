#include "BestVersion.h"

#include "Maths.h"
#include "ToString.h"

#include <limits>
#include <map>

namespace bestversion {

//------------------------------------------------------------------------------

std::string getMainArtist(metadb_handle_list_cref tracks)
{
	// Keep track of the number of each artist name we encounter.
	std::map<const char*, t_size> artists;

	// For each track, increment the number of occurrences of its artist.
	for(t_size i = 0; i < tracks.get_count(); i++)
	{
		service_ptr_t<metadb_info_container> outInfo;
		if(tracks[i]->get_async_info_ref(outInfo))
		{
			const file_info& fileInfo = outInfo->info();
			const bool has_artist_tag = fileInfo.meta_exists("artist");
			const bool has_album_artist_tag = fileInfo.meta_exists("album artist");
			if(has_artist_tag || has_album_artist_tag)
			{
				const char * artist = has_artist_tag ? fileInfo.meta_get("artist", 0) : fileInfo.meta_get("album artist", 0);
				++artists[artist];
			}
		}
	}

	// Find the artist that occurred the most.
	t_size maxCount = 0;
	const char* maxArtist = "";
	for(auto iter = artists.begin(); iter != artists.end(); iter++)
	{
		if((iter->second) > maxCount)
		{
			maxCount = iter->second;
			maxArtist = iter->first;
		}
	}

	// We've kept the database locked this whole time because we're just storing char* in the map, not copying the strings.
	return maxArtist;
}

//------------------------------------------------------------------------------

inline bool isTrackByArtist(const std::string& artist, const metadb_handle_ptr& track)
{
	// todo: ignore slight differences, e.g. in punctuation

	service_ptr_t<metadb_info_container> outInfo;
	if(track->get_async_info_ref(outInfo))
	{
		const file_info& fileInfo = outInfo->info();
		for(t_size j = 0; j < fileInfo.meta_get_count_by_name("artist"); j++)
		{
			if(stricmp_utf8(fileInfo.meta_get("artist", j), artist.c_str()) == 0)
			{
				return true;
			}
		}

		for(t_size j = 0; j < fileInfo.meta_get_count_by_name("album artist"); j++)
		{
			if(stricmp_utf8(fileInfo.meta_get("album artist", j), artist.c_str()) == 0)
			{
				return true;
			}
		}
	}

	return false;
}

//------------------------------------------------------------------------------
bool doesTrackHaveExactTrackNumber(const std::string& track_number, const metadb_handle_ptr& track)
{
	service_ptr_t<metadb_info_container> outInfo;
	if (!track->get_async_info_ref(outInfo))
	{
		return false;
	}

	const file_info& fileInfo = outInfo->info();

	if (!fileInfo.meta_exists("tracknumber"))
	{
		return false;
	}

	auto track_number_noleading = track_number;
	track_number_noleading.erase(0, track_number_noleading.find_first_not_of('0')); //remove leading zeros

	std::string fileTag = fileInfo.meta_get("tracknumber", 0);
	fileTag.erase(0, fileTag.find_first_not_of('0')); //remove leading zeros


	if (stricmp_utf8(fileTag.c_str(), track_number_noleading.c_str()) == 0)
	{
		return true;
	}

	return false;
}

bool doesTrackHaveExactTagFieldValue(const std::string& field_name, const std::string& field_value, const metadb_handle_ptr& track) {
	// todo: ignore slight differences, e.g. in punctuation
	service_ptr_t<metadb_info_container> outInfo;
	if (!track->get_async_info_ref(outInfo))
	{
		return false;
	}

	const file_info& fileInfo = outInfo->info();

	if (!fileInfo.meta_exists(field_name.c_str()))
	{
		return false;
	}


	const std::string fileTag = fileInfo.meta_get(field_name.c_str(), 0);

	if (stricmp_utf8(fileTag.c_str(), field_value.c_str()) == 0)
	{
		return true;
	}

	return false;
}

bool doesTrackHaveSimilarTitle(const std::string& title, const metadb_handle_ptr& track, bool exact)
{
	// todo: ignore slight differences, e.g. in punctuation
	service_ptr_t<metadb_info_container> outInfo;
	if (!track->get_async_info_ref(outInfo))
	{
		return false;
	}

	const file_info& fileInfo = outInfo->info();
	
	if(!fileInfo.meta_exists("title"))
	{
		return false;
	}


	const std::string fileTitle = fileInfo.meta_get("title", 0);

	if(stricmp_utf8(fileTitle.c_str(), title.c_str()) == 0)
	{
		return true;
	}
	else if(!exact && fileTitlesMatchExcludingBracketsOnLhs(fileTitle, title))
	{
		return true;
	}

	return false;
}

//------------------------------------------------------------------------------

void filterTracksByTagField(const std::string& field_name, const std::string& field_value, pfc::list_base_t<metadb_handle_ptr>& tracks)
{
	const t_size n = tracks.get_count();
	bit_array_bittable deleteMask(n);

	for (t_size i = 0; i < n; i++)
	{
		deleteMask.set(i, !doesTrackHaveExactTagFieldValue(field_name, field_value, tracks[i]));
	}

	tracks.remove_mask(deleteMask);
}

void filterTracksByArtist(const std::string& artist, pfc::list_base_t<metadb_handle_ptr>& tracks)
{
	const t_size n = tracks.get_count();
	bit_array_bittable deleteMask(n);

	for(t_size i = 0; i < n; i++)
	{
		deleteMask.set(i, !isTrackByArtist(artist, tracks[i]));
	}

	tracks.remove_mask(deleteMask);
}

void filterTracksByTrackNumber(const std::string& track_number, pfc::list_base_t<metadb_handle_ptr>& tracks) {
	const t_size n = tracks.get_count();
	bit_array_bittable deleteMask(n);

	for (t_size i = 0; i < n; i++)
	{
		deleteMask.set(i, !doesTrackHaveExactTrackNumber(track_number, tracks[i]));
	}

	tracks.remove_mask(deleteMask);
}

//------------------------------------------------------------------------------

void filterTracksByCloseTitle(const std::string& title, pfc::list_base_t<metadb_handle_ptr>& tracks, bool exact)
{
	const t_size n = tracks.get_count();
	bit_array_bittable deleteMask(n);

	for(t_size i = 0; i < n; i++)
	{
		deleteMask.set(i, !doesTrackHaveSimilarTitle(title, tracks[i], exact));
	}

	tracks.remove_mask(deleteMask);
}

//------------------------------------------------------------------------------

bool fileTitlesMatchExcludingBracketsOnLhs(const std::string& lhs, const std::string& rhs)
{
	auto lhsBracketPos = strcspn(lhs.c_str(),"([");
	if(lhsBracketPos != pfc_infinite)
	{
		if((stricmp_utf8_ex(lhs.c_str(), lhsBracketPos - 1u, rhs.c_str(), pfc_infinite) == 0)
			|| (stricmp_utf8_ex(lhs.c_str(), lhsBracketPos, rhs.c_str(), pfc_infinite) == 0))
		{
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------

float calculateTrackRating(const std::string& title, const metadb_handle_ptr& track)
{
	service_ptr_t<metadb_info_container> outInfo;
	if(!track->get_async_info_ref(outInfo))
	{
		// Don't pick it we can't get any info.
		return -1.0f;
	}

	const file_info& fileInfo = outInfo->info();

	if (!fileInfo.meta_exists("title"))
	{
		// Don't pick it if it doesn't have a title.
		return -1.0f;
	}

	float totalRating = 0.0f;

	const std::string fileTitle = fileInfo.meta_get("title", 0);

	// Assume title is already roughly correct.
	if(stricmp_utf8(fileTitle.c_str(), title.c_str()) == 0)
	{
		static const float ratingForExactTitleMatch = 2.0f;

		totalRating += ratingForExactTitleMatch;
	}
	else
	{
		static const float ratingForTitleMatchWithBrackets = 1.0f;

		totalRating += ratingForTitleMatchWithBrackets;
	}

	if(fileInfo.meta_exists("PLAY_COUNTER"))
	{
		const int playCount = atoi(fileInfo.meta_get("PLAY_COUNTER",0));

		static const float lowPlayCount = 0.0f;
		static const float highPlayCount = 10.0f;

		static const float lowPlayCountRating = 0.0f;
		static const float highPlayCountRating = 0.5f;

		const float playCountRating = maths::map(static_cast<float>(playCount), lowPlayCount, highPlayCount, lowPlayCountRating, highPlayCountRating);

		totalRating += playCountRating;
	}

	const auto bitrate = fileInfo.info_get_bitrate();

	static const float lowBitrate = 0.0f;
	static const float highBitrate = 1000.0f;

	static const float lowBitrateRating = 0.0f;
	static const float highBitrateRating = 2.0f;

	const float bitrateRating = maths::map(static_cast<float>(bitrate), lowBitrate, highBitrate, lowBitrateRating, highBitrateRating);

	totalRating += bitrateRating;

	static const float releaseTypeWeighting = 3.0f;
	float releaseTypeRating = 0.55f;	// Default for if nothing is set; assume it's somewhere between a live album and a soundtrack.

	if(fileInfo.meta_exists("musicbrainz album type") || fileInfo.meta_exists("releasetype"))
	{
		const std::string albumType = fileInfo.meta_exists("musicbrainz album type") ? fileInfo.meta_get("musicbrainz album type", 0) : fileInfo.meta_get("releasetype", 0);

		if(albumType == "album")
		{
			releaseTypeRating = 1.0f;
		}
		else if(albumType == "single")
		{
			releaseTypeRating = 0.9f;
		}
		else if(albumType == "compilation")
		{
			releaseTypeRating = 0.8f;
		}
		else if(albumType == "ep")
		{
			releaseTypeRating = 0.7f;
		}
		else if(albumType == "soundtrack")
		{
			releaseTypeRating = 0.6f;
		}
		else if(albumType == "live")
		{
			releaseTypeRating = 0.5f;
		}
		else if(albumType == "other")
		{
			releaseTypeRating = 0.4f;
		}
		else if(albumType == "remix")
		{
			releaseTypeRating = 0.3f;
		}
		else
		{
			releaseTypeRating = 1.0f;
		}
	}

	totalRating += releaseTypeRating * releaseTypeWeighting;
	
	static float albumArtistWeighting = 1.0f;
	float albumArtistRating = 1.0f;

	if(fileInfo.meta_exists("album artist") && fileInfo.meta_exists("artist"))
	{
		const std::string artist = fileInfo.meta_get("artist", 0);
		const std::string albumArtist = fileInfo.meta_get("album artist", 0);

		if(albumArtist == artist)
		{
			albumArtistRating = 1.0f;
		}
		else if(albumArtist == "Various Artists")
		{
			albumArtistRating = 0.333f;
		}
		else
		{
			// Artist appearing on someone else's album?
			albumArtistRating = 0.666f;
		}
	}

	totalRating += albumArtistRating * albumArtistWeighting;

	return totalRating;
}

//------------------------------------------------------------------------------

// all tracks will be analysed; try to cut the size of the list down before calling.
metadb_handle_ptr getBestTrackByTitle(const std::string& title, const pfc::list_base_t<metadb_handle_ptr>& tracks)
{
	if(tracks.get_count() == 1)
	{
		console::info(("Only one version of " + title + " exists in library").c_str());
		return tracks[0];
	}

	console::info(("Finding best version of " + title + ". " + to_string(tracks.get_count()) + " candidates").c_str());

	metadb_handle_ptr bestTrack = 0;
	float bestTrackRating = std::numeric_limits<float>::min();

	for(t_size index = 0; index < tracks.get_count(); index++)
	{
		const float trackRating = calculateTrackRating(title, tracks[index]);

		console::info(("Rating: " + to_string(trackRating, 2) + ": " + tracks[index]->get_path()).c_str());

		if(trackRating >= 0.0f && trackRating > bestTrackRating)
		{
			bestTrackRating = trackRating;
			bestTrack = tracks[index];
		}
	}

	if(bestTrack == 0)
	{
		console::info(("Couldn't find a match for " + title).c_str());
	}
	else
	{
		console::info(("Picked track with rating: " + to_string(bestTrackRating, 2) + ": " + bestTrack->get_path()).c_str());
	}

	return bestTrack;
}

void findAllDeadItemsInAllPlaylists()
{
	static_api_ptr_t<playlist_manager> pm;
	t_size playlist_count = pm->get_playlist_count();
	pfc::list_t<metadb_handle_ptr> dead_tracks;
	std::string output_playlist_name = "[foo_bestversion]DEAD ITEMS";
	pfc::string8 current_playlist_name;
	for (t_size playlist_index = 0; playlist_index < playlist_count; playlist_index++)
	{
		pm->playlist_get_name(playlist_index, current_playlist_name);
		console::info((std::string("Processing playlist: ") + current_playlist_name.c_str()).c_str());
		findDeadItemsInPlaylist(playlist_index, dead_tracks);
	}
	t_size output_playlist = pm->find_playlist(output_playlist_name.c_str(), pfc_infinite);

	if (output_playlist != pfc_infinite)
	{
		pm->playlist_undo_backup(output_playlist);
		pm->playlist_clear(output_playlist);
	}
	else
	{
		output_playlist = pm->create_playlist(
			output_playlist_name.c_str(),
			pfc_infinite,
			pfc_infinite
		);
	}

	pm->playlist_add_items(output_playlist, dead_tracks, bit_array_true());
	pm->set_active_playlist(output_playlist);
	pm->set_playing_playlist(output_playlist);
}

void findDeadItemsInPlaylist(t_size playlist, pfc::list_base_t<metadb_handle_ptr>& track_list) {
	static_api_ptr_t<playlist_manager> pm;
	pfc::list_t<metadb_handle_ptr> all_tracks;
	pm->playlist_get_all_items(playlist, all_tracks);
	abort_callback_dummy abort;
	for (t_size index = 0; index < all_tracks.get_count(); index++)
	{
		if (strstr(all_tracks[index]->get_path(), "3dydfy") == NULL)
		{
			if (!filesystem::g_exists(all_tracks[index]->get_path(), abort))
			{
				track_list.add_item(all_tracks[index]);
			}
		}
	}
}

void selectDeadItemsInActivePlaylist()
{
	static_api_ptr_t<playlist_manager> pm;
	size_t active_playlist = pm->get_active_playlist();
	pfc::list_t<metadb_handle_ptr> all_tracks;
	pm->playlist_get_all_items(active_playlist, all_tracks);
	abort_callback_dummy abort;

	unsigned total = pm->activeplaylist_get_item_count();
	bit_array_bittable mask(total);

	for (t_size index = 0; index < all_tracks.get_count(); index++)
	{
		if (strstr(all_tracks[index]->get_path(), "3dydfy") == NULL && !filesystem::g_exists(all_tracks[index]->get_path(), abort))
		{
			mask.set(index, true);
		}
		else
		{
			mask.set(index, false);
		}
	}
	
	pm->activeplaylist_set_selection(bit_array_true(), mask);
}

void selectNonLibraryItemsInActivePlaylist()
{
	static_api_ptr_t<playlist_manager> pm;
	static_api_ptr_t<library_manager> lm;
	size_t active_playlist = pm->get_active_playlist();
	pfc::list_t<metadb_handle_ptr> all_tracks;
	pm->playlist_get_all_items(active_playlist, all_tracks);

	unsigned total = pm->activeplaylist_get_item_count();
	bit_array_bittable mask(total);

	for (t_size index = 0; index < all_tracks.get_count(); index++)
	{
		if (strstr(all_tracks[index]->get_path(), "3dydfy") == NULL && !lm->is_item_in_library(all_tracks[index]))
		{
			mask.set(index, true);
		}
		else
		{
			mask.set(index, false);
		}
	}

	pm->activeplaylist_set_selection(bit_array_true(), mask);
}

void findAllNonLibraryItemsInAllPlaylists()
{
	static_api_ptr_t<playlist_manager> pm;
	t_size playlist_count = pm->get_playlist_count();
	pfc::list_t<metadb_handle_ptr> outside_tracks;
	std::string output_playlist_name = "[foo_bestversion]OUTSIDE ITEMS";
	pfc::string8 current_playlist_name;
	for (t_size playlist_index = 0; playlist_index < playlist_count; playlist_index++)
	{
		pm->playlist_get_name(playlist_index, current_playlist_name);
		console::info((std::string("Processing playlist: ") + current_playlist_name.c_str()).c_str());
		findNonLibraryItemsInPlaylist(playlist_index, outside_tracks);
	}
	t_size output_playlist = pm->find_playlist(output_playlist_name.c_str(), pfc_infinite);

	if (output_playlist != pfc_infinite)
	{
		pm->playlist_undo_backup(output_playlist);
		pm->playlist_clear(output_playlist);
	}
	else
	{
		output_playlist = pm->create_playlist(
			output_playlist_name.c_str(),
			pfc_infinite,
			pfc_infinite
		);
	}

	pm->playlist_add_items(output_playlist, outside_tracks, bit_array_true());
	pm->set_active_playlist(output_playlist);
	pm->set_playing_playlist(output_playlist);
}

void findNonLibraryItemsInPlaylist(t_size playlist, pfc::list_base_t<metadb_handle_ptr>& track_list)
{
	static_api_ptr_t<playlist_manager> pm;
	static_api_ptr_t<library_manager> lm;
	pfc::list_t<metadb_handle_ptr> all_tracks;
	pm->playlist_get_all_items(playlist, all_tracks);

	for (t_size index = 0; index < all_tracks.get_count(); index++)
	{
		if (strstr(all_tracks[index]->get_path(), "3dydfy") == NULL)
		{
			if (!lm->is_item_in_library(all_tracks[index]))
			{
				track_list.add_item(all_tracks[index]);
			}
		}
	}
}

//------------------------------------------------------------------------------

} // namespace bestversion
