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

#pragma once

#include <map>

#include <Wt/WSignal>
#include <Wt/WDialog>
#include <Wt/WComboBox>
#include <Wt/WStringListModel>
#include <Wt/WString>

#include "av/AvTranscoder.hpp"

namespace UserInterface {

class VideoParametersDialog : public Wt::WDialog
{
	public:
		// parameters to be edited
		VideoParametersDialog(Wt::WString windowTitle, Wt::WDialog* parent = 0);

		// Populates widget contents using these paramaters
		void load(const Av::MediaFile& mediaFile, Av::TranscodeParameters currentParameters);

		// Save widgets contents into these parameters
		void save(Av::TranscodeParameters& parameters);

		// Signal to be emitted if parameters are changed
		Wt::Signal<void>& apply()	{ return _apply; }

	private:

		void handleApply(void);

		// Stream handling
		void createStreamWidgets(const Wt::WString& label, Av::Stream::Type type, Wt::WTable* layout);
		void addStreams(Wt::WStringListModel* model, const std::vector<Av::Stream>& streams);
		void selectStream(const Wt::WStringListModel* model, int streamId, Wt::WComboBox* combo);

		Wt::Signal<void>	_apply;

		std::map<Av::Stream::Type, std::pair<Wt::WComboBox*, Wt::WStringListModel* > >	_streamSelection;
};

} // namespace UserInterface

