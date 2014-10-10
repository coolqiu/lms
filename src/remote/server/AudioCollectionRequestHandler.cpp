/*
 * Copyright (C) 2013 Emeric Poupon
 *
 * This file is part of LMS.
 *
 * LMS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LMS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LMS.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <algorithm>    // std::min

#include <boost/locale.hpp>
#include <boost/uuid/sha1.hpp>
#include <boost/foreach.hpp>

#include "logger/Logger.hpp"

#include "AudioCollectionRequestHandler.hpp"

#include "database/AudioTypes.hpp"
#include "database/MediaDirectory.hpp"
#include "cover/CoverArtGrabber.hpp"

namespace Remote {
namespace Server {

AudioCollectionRequestHandler::AudioCollectionRequestHandler(Database::Handler& db)
: _db(db)
{}


bool
AudioCollectionRequestHandler::process(const AudioCollectionRequest& request, AudioCollectionResponse& response)
{

	bool res = false;

	switch (request.type())
	{
		case AudioCollectionRequest::TypeGetRevision:
			// No payload
			res = processGetRevision(*response.mutable_revision());
			if (res)
				response.set_type(AudioCollectionResponse::TypeRevision);
			break;

		case AudioCollectionRequest::TypeGetGenreList:
			if (request.has_get_genres())
			{
				res = processGetGenres(request.get_genres(), *response.mutable_genre_list());
				if (res)
					response.set_type(AudioCollectionResponse::TypeArtistList);
			}
			else
				LMS_LOG(MOD_REMOTE, SEV_ERROR) << "Bad AudioCollectionRequest::TypeGetGenreList";
			break;

		case AudioCollectionRequest::TypeGetArtistList:
			if (request.has_get_artists())
			{
				res = processGetArtists(request.get_artists(), *response.mutable_artist_list());
				if (res)
					response.set_type(AudioCollectionResponse::TypeArtistList);

			}
			else
				LMS_LOG(MOD_REMOTE, SEV_ERROR) << "Bad AudioCollectionRequest::TypeGetArtistList message!";
			break;

		case AudioCollectionRequest::TypeGetReleaseList:
			if (request.has_get_releases())
			{
				res = processGetReleases(request.get_releases(), *response.mutable_release_list());
				if (res)
					response.set_type(AudioCollectionResponse::TypeReleaseList);

			}
			else
				LMS_LOG(MOD_REMOTE, SEV_ERROR) << "Bad AudioCollectionRequest::TypeGetReleaseList message!";
			break;

		case AudioCollectionRequest::TypeGetTrackList:
			if (request.has_get_tracks())
			{
				res = processGetTracks(request.get_tracks(), *response.mutable_track_list());
				if (res)
					response.set_type(AudioCollectionResponse::TypeTrackList);

			}
			else
				LMS_LOG(MOD_REMOTE, SEV_ERROR) << "Bad AudioCollectionRequest::TypeGetTrackList message!";
			break;

		case AudioCollectionRequest::TypeGetCoverArt:
			if (request.has_get_cover_art())
				res = processGetCoverArt(request.get_cover_art(), response);
			else
				LMS_LOG(MOD_REMOTE, SEV_ERROR) << "Bad AudioCollectionRequest::TypeGetCoverArt message!";
			break;

		default:
			LMS_LOG(MOD_REMOTE, SEV_ERROR) << "Unhandled AudioCollectionRequest_Type = " << request.type();

	}

	return res;
}


bool
AudioCollectionRequestHandler::processGetGenres(const AudioCollectionRequest::GetGenreList& request, AudioCollectionResponse::GenreList& response)
{
	// sanity checks
	if (!request.has_batch_parameter())
	{
		LMS_LOG(MOD_REMOTE, SEV_ERROR) << "No batch parameters found!";
		return false;
	}

	std::size_t  size = request.batch_parameter().size();
	if (!size)
		size = _maxListArtists;
	size = std::min(size, _maxListArtists);

	Wt::Dbo::Transaction transaction( _db.getSession() );

	Wt::Dbo::collection<Database::Genre::pointer> genres = Database::Genre::getAll( _db.getSession(), request.batch_parameter().offset(), static_cast<int>(size));

	typedef Wt::Dbo::collection< Database::Genre::pointer > Genres;

	for (Genres::const_iterator it = genres.begin(); it != genres.end(); ++it)
	{
		AudioCollectionResponse_Genre* genre = response.add_genres();

		genre->set_name( std::string( boost::locale::conv::to_utf<char>((*it)->getName(), "UTF-8") ) );
		genre->set_id(it->id());
	}

	return true;
}

bool
AudioCollectionRequestHandler::processGetArtists(const AudioCollectionRequest::GetArtistList& request, AudioCollectionResponse::ArtistList& response)
{
	// sanity checks
	if (!request.has_batch_parameter())
	{
		LMS_LOG(MOD_REMOTE, SEV_ERROR) << "No batch parameters found!";
		return false;
	}

	std::size_t  size = request.batch_parameter().size();
	if (!size)
		size = _maxListArtists;
	size = std::min(size, _maxListArtists);

	// Now fetch requested data...

	Wt::Dbo::Transaction transaction( _db.getSession() );

	Wt::Dbo::collection<Database::Artist::pointer> artists = Database::Artist::getAll( _db.getSession(), request.batch_parameter().offset(), static_cast<int>(size) );

	typedef Wt::Dbo::collection< Database::Artist::pointer > Artists;

	for (Artists::const_iterator it = artists.begin(); it != artists.end(); ++it)
	{
		AudioCollectionResponse_Artist* artist = response.add_artists();

		artist->set_name( std::string( boost::locale::conv::to_utf<char>((*it)->getName(), "UTF-8") ) );
		artist->set_id(it->id());
	}

	return true;
}

bool
AudioCollectionRequestHandler::processGetReleases(const AudioCollectionRequest::GetReleaseList& request, AudioCollectionResponse::ReleaseList& response)
{
	// sanity checks
	if (!request.has_batch_parameter())
	{
		LMS_LOG(MOD_REMOTE, SEV_ERROR) << "No batch parameters found!";
		return false;
	}

	std::size_t  size = request.batch_parameter().size();
	if (!size)
		size = _maxListReleases;
	size = std::min(size, _maxListReleases);

	std::vector<Database::Artist::id_type> artistIds;
	for (int id = 0; id < request.artist_id_size(); ++id)
		artistIds.push_back( request.artist_id(id) );

	Wt::Dbo::Transaction transaction( _db.getSession() );

	Wt::Dbo::collection<Database::Release::pointer> releases = Database::Release::getAll( _db.getSession(), artistIds, request.batch_parameter().offset(), static_cast<int>(size));

	typedef Wt::Dbo::collection< Database::Release::pointer > Releases;

	for (Releases::const_iterator it = releases.begin(); it != releases.end(); ++it)
	{
		AudioCollectionResponse_Release* release = response.add_releases();

		release->set_name( std::string( boost::locale::conv::to_utf<char>((*it)->getName(), "UTF-8") ) );
		release->set_id(it->id());
	}

	return true;
}

bool
AudioCollectionRequestHandler::processGetTracks(const AudioCollectionRequest::GetTrackList& request, AudioCollectionResponse::TrackList& response)
{
	// sanity checks
	if (!request.has_batch_parameter())
	{
		LMS_LOG(MOD_REMOTE, SEV_ERROR) << "No batch parameters found!";
		return false;
	}

	std::size_t  size = request.batch_parameter().size();
	if (!size)
		size = _maxListTracks;
	size = std::min(size, _maxListTracks);

	// Get filters
	std::vector<Database::Artist::id_type> artistIds;
	for (int id = 0; id < request.artist_id_size(); ++id)
		artistIds.push_back( request.artist_id(id) );

	std::vector<Database::Release::id_type> releaseIds;
	for (int id = 0; id < request.release_id_size(); ++id)
		releaseIds.push_back( request.release_id(id) );

	std::vector<Database::Release::id_type> genreIds;
	for (int id = 0; id < request.genre_id_size(); ++id)
		genreIds.push_back( request.genre_id(id) );

	Wt::Dbo::Transaction transaction( _db.getSession() );

	Wt::Dbo::collection<Database::Track::pointer> tracks
		= Database::Track::getAll( _db.getSession(),
				artistIds,
				releaseIds,
				genreIds,
				request.batch_parameter().offset(),
				static_cast<int>(size));

	typedef Wt::Dbo::collection< Database::Track::pointer > Tracks;

	for (Tracks::const_iterator it = tracks.begin(); it != tracks.end(); ++it)
	{
		AudioCollectionResponse_Track* track = response.add_tracks();

		track->set_id(it->id());
		track->set_disc_number( (*it)->getDiscNumber() );
		track->set_track_number( (*it)->getTrackNumber() );
		track->set_artist_id( (*it)->getArtist().id() );
		track->set_release_id( (*it)->getRelease().id() );

		track->set_name( std::string( boost::locale::conv::to_utf<char>((*it)->getName(), "UTF-8") ) );
		track->set_duration_secs( (*it)->getDuration().total_seconds() );
//		if (!(*it)->getCreationTime().is_special())
//			track->set_release_date(  boost::posix_time::to_simple_string((*it)->getCreationTime()) );

		BOOST_FOREACH(Database::Genre::pointer genre, (*it)->getGenres())
			track->add_genre_id( genre.id() );

	}

	return true;
}

bool
AudioCollectionRequestHandler::processGetCoverArt(const AudioCollectionRequest::GetCoverArt& request, AudioCollectionResponse& response)
{
	bool res = false;

	response.set_type(AudioCollectionResponse::TypeCoverArt);

	switch(request.type())
	{
		case AudioCollectionRequest::GetCoverArt::TypeGetCoverArtRelease:

			if (request.has_release_id())
			{
				Wt::Dbo::Transaction transaction( _db.getSession() );

				// Get the request release
				Database::Release::pointer release = Database::Release::getById( _db.getSession(), request.release_id());

				std::vector<CoverArt::CoverArt> coverArts = CoverArt::Grabber::getFromRelease(release);

				BOOST_FOREACH(CoverArt::CoverArt& coverArt, coverArts)
				{
					AudioCollectionResponse_CoverArt* cover_art = response.add_cover_art();

					if (request.has_size())
					{
						std::size_t size = request.size();
						if (size > _maxCoverArtSize || size == 0)
							size = _maxCoverArtSize;
						if (size < _minCoverArtSize)
							size = _minCoverArtSize;

						coverArt.scale(size);
					}

					cover_art->set_mime_type(coverArt.getMimeType());
					cover_art->set_data( std::string( coverArt.getData().begin(), coverArt.getData().end()) );
				}
			}
			res = true;
			break;

		case AudioCollectionRequest::GetCoverArt::TypeGetCoverArtTrack:
			if (request.has_track_id())
			{
				Wt::Dbo::Transaction transaction( _db.getSession() );

				// Get the request release
				Database::Track::pointer track = Database::Track::getById( _db.getSession(), request.track_id());

				std::vector<CoverArt::CoverArt> coverArts = CoverArt::Grabber::getFromTrack(track);

				BOOST_FOREACH(CoverArt::CoverArt& coverArt, coverArts)
				{
					AudioCollectionResponse_CoverArt* cover_art = response.add_cover_art();

					if (request.has_size())
					{
						std::size_t size = request.size();
						if (size > _maxCoverArtSize || size == 0)
							size = _maxCoverArtSize;
						if (size < _minCoverArtSize)
							size = _minCoverArtSize;

						coverArt.scale(size);
					}

					cover_art->set_mime_type(coverArt.getMimeType());
					cover_art->set_data( std::string( coverArt.getData().begin(), coverArt.getData().end()) );
				}
			}
			res = true;
			break;
	}

	return res;
}

bool
AudioCollectionRequestHandler::processGetRevision(AudioCollectionResponse::Revision& response)
{

	bool res = false;

	Wt::Dbo::Transaction transaction( _db.getSession() );

	Database::MediaDirectorySettings::pointer settings = Database::MediaDirectorySettings::get( _db.getSession() );

	std::string hashStr = boost::posix_time::to_iso_string(settings->getLastUpdated());

	boost::uuids::detail::sha1 s;
	BOOST_FOREACH(const char c, hashStr)
		s.process_byte(c);

	unsigned int digest[5];
	s.get_digest(digest);

	std::ostringstream oss;
	for (std::size_t i = 0; i < 5; ++i)
		oss << std::hex << std::setfill('0') << std::setw(4) << digest[i];

	response.set_rev(oss.str());

	res = true;

	return res;
}



} // namespace Remote
} // namespace Server

