/*
 * CanvasSoftware.h, part of VCMI engine
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
#include "../render/ICanvas.h"

/// Class that represents surface for drawing on
class CanvasSoftware final : public ICanvas
{
	/// Upscaler awareness. Must be first member for initialization
	CanvasScalingPolicy scalingPolicy;

	/// Target surface
	SDL_Surface * surface;

	/// Current rendering area, all rendering operations will be moved into selected area
	Rect renderArea;

	Point transformPos(const Point & input);
	Point transformSize(const Point & input);

	void setClipRect(const Rect & rect) override;
	Rect getClipRect() const override;
	void setViewRect(const Rect & rect) override;
	Rect getViewRect() const override;

public:
	/// constructs canvas of specified size
	CanvasSoftware(const Point & size, CanvasScalingPolicy scalingPolicy);

	/// constructs canvas using existing surface. Caller maintains ownership on the surface
	CanvasSoftware(SDL_Surface * surface, CanvasScalingPolicy scalingPolicy);

	~CanvasSoftware();

	/// if set to true, drawing this canvas onto another canvas will use alpha channel information
	void applyTransparency(bool on) override;

	/// applies grayscale filter onto current image
	void applyGrayscale() override;

	/// renders image onto this canvas at specified position
	void draw(const std::shared_ptr<IImage>& image, const Point & pos) override;
	void draw(const IImage& image, const Point & pos) override;

	void draw(IVideoInstance & video, const Point & pos) override;

	/// renders section of image bounded by sourceRect at specified position
	void draw(const std::shared_ptr<IImage>& image, const Point & pos, const Rect & sourceRect) override;

	/// renders another canvas onto this canvas
	void draw(const ICanvas &image, const Point & pos) override;

	/// renders another canvas onto this canvas with transparency
	void drawTransparent(const ICanvas & image, const Point & pos, double transparency) override;

	/// renders another canvas onto this canvas with scaling
	void drawScaled(const ICanvas &image, const Point & pos, const Point & targetSize) override;

	/// renders single pixels with specified color
	void drawPoint(const Point & dest, const ColorRGBA & color) override;

	/// renders continuous, 1-pixel wide line with color gradient
	void drawLine(const Point & from, const Point & dest, const ColorRGBA & colorFrom, const ColorRGBA & colorDest) override;

	/// renders rectangular, solid-color border in specified location
	void drawBorder(const Rect & target, const ColorRGBA & color, int width = 1) override;

	/// renders rectangular, dashed border in specified location
	void drawBorderDashed(const Rect & target, const ColorRGBA & color) override;

	/// renders single line of text with specified parameters
	void drawText(const Point & position, const EFonts & font, const ColorRGBA & colorDest, ETextAlignment alignment, const std::string & text ) override;

	/// renders multiple lines of text with specified parameters
	void drawText(const Point & position, const EFonts & font, const ColorRGBA & colorDest, ETextAlignment alignment, const std::vector<std::string> & text ) override;

	/// fills selected area with solid color
	void drawColor(const Rect & target, const ColorRGBA & color) override;

	/// fills selected area with blended color
	void drawColorBlended(const Rect & target, const ColorRGBA & color) override;

	/// fills canvas with texture
	void fillTexture(const std::shared_ptr<IImage>& image) override;

	int getScalingFactor() const;

	/// get the render area
	Rect getRenderArea() const override;

	std::shared_ptr<ISharedImage> toSharedImage() override;
};
