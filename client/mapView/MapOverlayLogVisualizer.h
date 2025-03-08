/*
 * MapOverlayLogVisualizer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/logging/VisualLogger.h"

VCMI_LIB_NAMESPACE_BEGIN

class int3;

VCMI_LIB_NAMESPACE_END

class MapViewModel;
class ICanvas;

class MapOverlayLogVisualizer : public IMapOverlayLogVisualizer
{
private:
	ICanvas & target;
	std::shared_ptr<MapViewModel> model;

public:
	MapOverlayLogVisualizer(ICanvas & target, std::shared_ptr<MapViewModel> model);
	void drawLine(int3 start, int3 end) override;
	void drawText(int3 tile, int lineNumber, const std::string & text, const std::optional<ColorRGBA> & color) override;
};
