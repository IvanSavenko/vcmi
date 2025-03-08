/*
 * CanvasSoftware.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CanvasSoftware.h"

#include "SDL_Extensions.h"

#include "../GameEngine.h"
#include "../media/IVideoPlayer.h"
#include "../render/Colors.h"
#include "../render/IFont.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"
#include "../render/IScreenHandler.h"

#include <SDL_surface.h>
#include <SDL_pixels.h>

CanvasSoftware::CanvasSoftware(SDL_Surface * surface, CanvasScalingPolicy scalingPolicy):
	scalingPolicy(scalingPolicy),
	surface(surface),
	renderArea(0,0, surface->w, surface->h)
{
	surface->refcount++;
}

CanvasSoftware::CanvasSoftware(const CanvasSoftware & other):
	scalingPolicy(other.scalingPolicy),
	surface(other.surface),
	renderArea(other.renderArea)
{
	surface->refcount++;
}

void CanvasSoftware::setClipRect(const Rect & rect)
{
	CSDL_Ext::setClipRect(surface, rect * ENGINE->screenHandler().getScalingFactor());
}

Rect CanvasSoftware::getClipRect() const
{
	Rect oldRect;
	CSDL_Ext::getClipRect(surface, oldRect);
	return oldRect / ENGINE->screenHandler().getScalingFactor();
}

void CanvasSoftware::setViewRect(const Rect & rect)
{
	renderArea = rect;
}

Rect CanvasSoftware::getViewRect() const
{
	return renderArea;
}

CanvasSoftware::CanvasSoftware(const Point & size, CanvasScalingPolicy scalingPolicy):
	scalingPolicy(scalingPolicy),
	surface(CSDL_Ext::newSurface(size * getScalingFactor())),
	renderArea(Point(0,0), size * getScalingFactor())
{
	CSDL_Ext::fillSurface(surface, CSDL_Ext::toSDL(Colors::TRANSPARENCY) );
	SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
}

int CanvasSoftware::getScalingFactor() const
{
	if (scalingPolicy == CanvasScalingPolicy::IGNORE)
		return 1;
	return ENGINE->screenHandler().getScalingFactor();
}

Point CanvasSoftware::transformPos(const Point & input)
{
	return renderArea.topLeft() + input * getScalingFactor();
}

Point CanvasSoftware::transformSize(const Point & input)
{
	return input * getScalingFactor();
}

CanvasSoftware CanvasSoftware::createFromSurface(SDL_Surface * surface, CanvasScalingPolicy scalingPolicy)
{
	return CanvasSoftware(surface, scalingPolicy);
}

void CanvasSoftware::applyTransparency(bool on)
{
	if (on)
		SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
	else
		SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
}

void CanvasSoftware::applyGrayscale()
{
	CSDL_Ext::convertToGrayscale(surface, renderArea);
}

CanvasSoftware::~CanvasSoftware()
{
	SDL_FreeSurface(surface);
}

void CanvasSoftware::draw(IVideoInstance & video, const Point & pos)
{
	video.show(pos, surface);
}

void CanvasSoftware::draw(const IImage& image, const Point & pos)
{
	image.draw(surface, transformPos(pos), nullptr, getScalingFactor());
}

void CanvasSoftware::draw(const std::shared_ptr<IImage>& image, const Point & pos)
{
	assert(image);
	if (image)
		image->draw(surface, transformPos(pos), nullptr, getScalingFactor());
}

void CanvasSoftware::draw(const std::shared_ptr<IImage>& image, const Point & pos, const Rect & sourceRect)
{
	Rect realSourceRect = sourceRect * getScalingFactor();
	assert(image);
	if (image)
		image->draw(surface, transformPos(pos), &realSourceRect, getScalingFactor());
}

void CanvasSoftware::draw(const ICanvas & image, const Point & pos)
{
	auto actualImage = dynamic_cast<const CanvasSoftware&>(image);
	CSDL_Ext::blitSurface(actualImage.surface, actualImage.renderArea, surface, transformPos(pos));
}

void CanvasSoftware::drawTransparent(const ICanvas & image, const Point & pos, double transparency)
{
	auto actualImage = dynamic_cast<const CanvasSoftware&>(image);

	SDL_BlendMode oldMode;

	SDL_GetSurfaceBlendMode(actualImage.surface, &oldMode);
	SDL_SetSurfaceBlendMode(actualImage.surface, SDL_BLENDMODE_BLEND);
	SDL_SetSurfaceAlphaMod(actualImage.surface, 255 * transparency);
	CSDL_Ext::blitSurface(actualImage.surface, actualImage.renderArea, surface, transformPos(pos));
	SDL_SetSurfaceAlphaMod(actualImage.surface, 255);
	SDL_SetSurfaceBlendMode(actualImage.surface, oldMode);
}

void CanvasSoftware::drawScaled(const ICanvas & image, const Point & pos, const Point & targetSize)
{
	auto actualImage = dynamic_cast<const CanvasSoftware&>(image);

	SDL_Rect targetRect = CSDL_Ext::toSDL(Rect(transformPos(pos), transformSize(targetSize)));
	SDL_BlitScaled(actualImage.surface, nullptr, surface, &targetRect);
}

void CanvasSoftware::drawPoint(const Point & dest, const ColorRGBA & color)
{
	Point point = transformPos(dest);
	CSDL_Ext::putPixelWithoutRefreshIfInSurf(surface, point.x, point.y, color.r, color.g, color.b, color.a);
}

void CanvasSoftware::drawLine(const Point & from, const Point & dest, const ColorRGBA & colorFrom, const ColorRGBA & colorDest)
{
	CSDL_Ext::drawLine(surface, transformPos(from), transformPos(dest), CSDL_Ext::toSDL(colorFrom), CSDL_Ext::toSDL(colorDest), getScalingFactor());
}

void CanvasSoftware::drawBorder(const Rect & target, const ColorRGBA & color, int width)
{
	Rect realTarget = target * getScalingFactor() + renderArea.topLeft();

	CSDL_Ext::drawBorder(surface, realTarget.x, realTarget.y, realTarget.w, realTarget.h, CSDL_Ext::toSDL(color), width * getScalingFactor());
}

void CanvasSoftware::drawBorderDashed(const Rect & target, const ColorRGBA & color)
{
	Rect realTarget = target * getScalingFactor() + renderArea.topLeft();

	CSDL_Ext::drawLineDashed(surface, realTarget.topLeft(),    realTarget.topRight(),    CSDL_Ext::toSDL(color));
	CSDL_Ext::drawLineDashed(surface, realTarget.bottomLeft(), realTarget.bottomRight(), CSDL_Ext::toSDL(color));
	CSDL_Ext::drawLineDashed(surface, realTarget.topLeft(),    realTarget.bottomLeft(),  CSDL_Ext::toSDL(color));
	CSDL_Ext::drawLineDashed(surface, realTarget.topRight(),   realTarget.bottomRight(), CSDL_Ext::toSDL(color));
}

void CanvasSoftware::drawText(const Point & position, const EFonts & font, const ColorRGBA & colorDest, ETextAlignment alignment, const std::string & text )
{
	const auto & fontPtr = ENGINE->renderHandler().loadFont(font);

	switch (alignment)
	{
	case ETextAlignment::TOPLEFT:      return fontPtr->renderTextLeft  (surface, text, colorDest, transformPos(position));
	case ETextAlignment::TOPCENTER:    return fontPtr->renderTextCenter(surface, text, colorDest, transformPos(position));
	case ETextAlignment::CENTER:       return fontPtr->renderTextCenter(surface, text, colorDest, transformPos(position));
	case ETextAlignment::BOTTOMRIGHT:  return fontPtr->renderTextRight (surface, text, colorDest, transformPos(position));
	}
}

void CanvasSoftware::drawText(const Point & position, const EFonts & font, const ColorRGBA & colorDest, ETextAlignment alignment, const std::vector<std::string> & text )
{
	const auto & fontPtr = ENGINE->renderHandler().loadFont(font);

	switch (alignment)
	{
	case ETextAlignment::TOPLEFT:      return fontPtr->renderTextLinesLeft  (surface, text, colorDest, transformPos(position));
	case ETextAlignment::TOPCENTER:    return fontPtr->renderTextLinesCenter(surface, text, colorDest, transformPos(position));
	case ETextAlignment::CENTER:       return fontPtr->renderTextLinesCenter(surface, text, colorDest, transformPos(position));
	case ETextAlignment::BOTTOMRIGHT:  return fontPtr->renderTextLinesRight (surface, text, colorDest, transformPos(position));
	}
}

void CanvasSoftware::drawColor(const Rect & target, const ColorRGBA & color)
{
	Rect realTarget = target * getScalingFactor() + renderArea.topLeft();

	CSDL_Ext::fillRect(surface, realTarget, CSDL_Ext::toSDL(color));
}

void CanvasSoftware::drawColorBlended(const Rect & target, const ColorRGBA & color)
{
	Rect realTarget = target * getScalingFactor() + renderArea.topLeft();

	CSDL_Ext::fillRectBlended(surface, realTarget, CSDL_Ext::toSDL(color));
}

void CanvasSoftware::fillTexture(const std::shared_ptr<IImage>& image)
{
	assert(image);
	if (!image)
		return;
		
	Rect imageArea(Point(0, 0), image->dimensions());
	for (int y=0; y < surface->h; y+= imageArea.h)
	{
		for (int x=0; x < surface->w; x+= imageArea.w)
			image->draw(surface, Point(renderArea.x + x * getScalingFactor(), renderArea.y + y * getScalingFactor()), nullptr, getScalingFactor());
	}
}

Rect CanvasSoftware::getRenderArea() const
{
	return renderArea;
}
