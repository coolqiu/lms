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

#ifndef DATABASE_MEDIA_DIRECTORY_HPP
#define DATABASE_MEDIA_DIRECTORY_HPP

#include <vector>

#include <boost/filesystem/path.hpp>

#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/WtSqlTraits>

namespace Database {

class MediaDirectory;

class MediaDirectorySettings
{
	public:

		enum UpdatePeriod {
			Never,
			Daily,
			Weekly,
			Monthly
		};

		typedef Wt::Dbo::ptr<MediaDirectorySettings> pointer;

		MediaDirectorySettings();

		// accessors
		static pointer get(Wt::Dbo::Session& session);

		// write accessors
		void	setManualScanRequested(bool value)		{ _manualScanRequested = value;}
		void	setUpdatePeriod(UpdatePeriod period)		{ _updatePeriod = period;}
		void	setUpdateStartTime(boost::posix_time::time_duration dur)	{ _updateStartTime = dur;}
		void	setLastUpdate(boost::posix_time::ptime time)	{ _lastUpdate = time; }
		void	setLastScan(boost::posix_time::ptime time)	{ _lastScan = time; }
		void	setAudioFileExtensions(std::vector<boost::filesystem::path> extensions);
		void	setVideoFileExtensions(std::vector<boost::filesystem::path> extensions);

		// Read accessors
		bool				getManualScanRequested(void) const	{ return _manualScanRequested; }
		UpdatePeriod			getUpdatePeriod(void) const		{ return _updatePeriod; }
		boost::posix_time::time_duration	getUpdateStartTime(void) const	{ return _updateStartTime; }
		boost::posix_time::ptime	getLastUpdated(void) const		{ return _lastUpdate; }
		boost::posix_time::ptime	getLastScan(void) const			{ return _lastScan; }
		std::vector<boost::filesystem::path>	getAudioFileExtensions(void) const;
		std::vector<boost::filesystem::path>	getVideoFileExtensions(void) const;

		template<class Action>
			void persist(Action& a)
			{
				Wt::Dbo::field(a, _manualScanRequested,	"manual_scan_requested");
				Wt::Dbo::field(a, _updatePeriod,	"update_period");
				Wt::Dbo::field(a, _updateStartTime,	"update_start_time");
				Wt::Dbo::field(a, _audioFileExtensions,	"audio_file_extensions");
				Wt::Dbo::field(a, _videoFileExtensions,	"video_file_extensions");
				Wt::Dbo::field(a, _lastUpdate,		"last_update");
				Wt::Dbo::field(a, _lastScan,		"last_scan");
				Wt::Dbo::hasMany(a, _mediaDirectories, Wt::Dbo::ManyToOne, "media_directory_settings");
			}

	private:

		bool					_manualScanRequested;	// Immadiate scan has been requested by user
		UpdatePeriod				_updatePeriod;		// How long between updates
		boost::posix_time::time_duration	_updateStartTime;	// Time of day to begin the update
		std::string				_audioFileExtensions;	// Extension of the audio files to be scanned
		std::string				_videoFileExtensions;	// Extension of the video files to be scanned
		boost::posix_time::ptime		_lastUpdate;		// last time the database has changed
		boost::posix_time::ptime		_lastScan;		// last time the database has been scanned
		Wt::Dbo::collection< Wt::Dbo::ptr<MediaDirectory> > _mediaDirectories;	// list of media directories
};


class MediaDirectory
{
	public:

		typedef Wt::Dbo::ptr<MediaDirectory> pointer;

		enum Type {
			Audio = 1,
			Video = 2,
		};

		MediaDirectory() {}
		MediaDirectory(boost::filesystem::path p, Type type);

		// Accessors
		static pointer create(Wt::Dbo::Session& session, boost::filesystem::path p, Type type);
		static std::vector<MediaDirectory::pointer>	getAll(Wt::Dbo::Session& session);
		static std::vector<MediaDirectory::pointer>	getByType(Wt::Dbo::Session& session, Type type);
		static pointer get(Wt::Dbo::Session& session, boost::filesystem::path p, Type type);

		static void eraseAll(Wt::Dbo::Session& session);

		Type			getType(void) const	{ return _type; }
		boost::filesystem::path	getPath(void) const	{ return boost::filesystem::path(_path); }

		template<class Action>
			void persist(Action& a)
			{
				Wt::Dbo::field(a, _type,		"type");
				Wt::Dbo::field(a, _path,		"path");
				Wt::Dbo::belongsTo(a, _settings, "media_directory_settings", Wt::Dbo::OnDeleteCascade);
			}

	private:

		Type		_type;
		std::string	_path;

		MediaDirectorySettings::pointer		_settings;		// back pointer

};

} // namespace Database

#endif

