/*
 * ICanvas.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/TextAlignment.h"
#include "../../lib/Rect.h"
#include "../../lib/Color.h"

struct SDL_Surface;
class IImage;
class IVideoInstance;
enum EFonts : int8_t;

enum class CanvasScalingPolicy
{
	AUTO,  // automatically scale canvas operations by global scaling factor
	IGNORE // disable any scaling processing. Scaling factor will be set to 1

};

/// Class that represents surface for drawing on
class ICanvas : boost::noncopyable
{
protected:
	friend class CanvasClipGuard;
	friend class CanvasViewGuard;

	virtual void setClipRect(const Rect & rect) = 0;
	virtual Rect getClipRect() const = 0;

	virtual void setViewRect(const Rect & rect) = 0;
	virtual Rect getViewRect() const = 0;

public:
	virtual ~ICanvas() = default;

	/// if set to true, drawing this canvas onto another canvas will use alpha channel information
	virtual void applyTransparency(bool on) = 0;

	/// applies grayscale filter onto current image
	virtual void applyGrayscale() = 0;

	/// renders image onto this canvas at specified position
	virtual void draw(const std::shared_ptr<IImage>& image, const Point & pos) = 0;
	virtual void draw(const IImage& image, const Point & pos) = 0;

	virtual void draw(IVideoInstance & video, const Point & pos) = 0;

	/// renders section of image bounded by sourceRect at specified position
	virtual void draw(const std::shared_ptr<IImage>& image, const Point & pos, const Rect & sourceRect) = 0;

	/// renders another canvas onto this canvas
	virtual void draw(const ICanvas &image, const Point & pos) = 0;

	/// renders another canvas onto this canvas with transparency
	virtual void drawTransparent(const ICanvas & image, const Point & pos, double transparency) = 0;

	/// renders another canvas onto this canvas with scaling
	virtual void drawScaled(const ICanvas &image, const Point & pos, const Point & targetSize) = 0;

	/// renders single pixels with specified color
	virtual void drawPoint(const Point & dest, const ColorRGBA & color) = 0;

	/// renders continuous, 1-pixel wide line with color gradient
	virtual void drawLine(const Point & from, const Point & dest, const ColorRGBA & colorFrom, const ColorRGBA & colorDest) = 0;

	/// renders rectangular, solid-color border in specified location
	virtual void drawBorder(const Rect & target, const ColorRGBA & color, int width = 1) = 0;

	/// renders rectangular, dashed border in specified location
	virtual void drawBorderDashed(const Rect & target, const ColorRGBA & color) = 0;

	/// renders single line of text with specified parameters
	virtual void drawText(const Point & position, const EFonts & font, const ColorRGBA & colorDest, ETextAlignment alignment, const std::string & text ) = 0;

	/// renders multiple lines of text with specified parameters
	virtual void drawText(const Point & position, const EFonts & font, const ColorRGBA & colorDest, ETextAlignment alignment, const std::vector<std::string> & text ) = 0;

	/// fills selected area with solid color
	virtual void drawColor(const Rect & target, const ColorRGBA & color) = 0;

	/// fills selected area with blended color
	virtual void drawColorBlended(const Rect & target, const ColorRGBA & color) = 0;

	/// fills canvas with texture
	virtual void fillTexture(const std::shared_ptr<IImage>& image) = 0;

	virtual Rect getRenderArea() const = 0;
};

class CanvasClipGuard : boost::noncopyable
{
	ICanvas & canvas;
	Rect oldRect;

public:
	CanvasClipGuard(ICanvas & canvas, const Rect & rect)
		: canvas(canvas)
		, oldRect(canvas.getClipRect())
	{
		canvas.setClipRect(rect);
	}
	~CanvasClipGuard()
	{
		canvas.setClipRect(oldRect);
	}
};

class CanvasViewGuard : boost::noncopyable
{
	ICanvas & canvas;
	Rect oldRect;

public:
	CanvasViewGuard(ICanvas & canvas, const Rect & rect)
		: canvas(canvas)
		, oldRect(canvas.getClipRect())
	{
		canvas.setViewRect(rect);
	}
	~CanvasViewGuard()
	{
		canvas.setViewRect(oldRect);
	}
};
