/*
Copyright © 2013 Igor Paliychuk

This file is part of FLARE.

FLARE is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

FLARE is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
FLARE.  If not, see http://www.gnu.org/licenses/
*/

#include <iostream>

#include <stdio.h>
#include <stdlib.h>

#include "SharedResources.h"
#include "Settings.h"

#include "SDLHardwareRenderDevice.h"

#if SDL_VERSION_ATLEAST(2,0,0)

using namespace std;

SDLHardwareImage::SDLHardwareImage(RenderDevice *_device)
	: Image(_device)
	, surface(NULL) {
}

SDLHardwareImage::~SDLHardwareImage() {
}

int SDLHardwareImage::getWidth() const {
	int w, h;
	SDL_QueryTexture(surface, NULL, NULL, &w, &h);
	return (surface ? w : 0);
}

int SDLHardwareImage::getHeight() const {
	int w, h;
	SDL_QueryTexture(surface, NULL, NULL, &w, &h);
	return (surface ? h : 0);
}

SDLHardwareRenderDevice::SDLHardwareRenderDevice()
	: screen(NULL)
	, titlebar_icon(NULL) {
	cout << "Using Render Device: SDLHardwareRenderDevice (hardware, SDL 2)" << endl;
	textures_count = 0;
}

int SDLHardwareRenderDevice::createContext(int width, int height) {
	int set_fullscreen = 0;

	if (is_initialized) {
		if (textures_count != 0) {
			cout << "Trying to change video mode. Number of not freed textures:" << textures_count << endl;
			cout << "This can cause graphical issues or even game exit" << endl;
		}
		SDL_DestroyRenderer(renderer);
		Uint32 flags = 0;

		if (FULLSCREEN) flags = SDL_WINDOW_FULLSCREEN;
		else flags = SDL_WINDOW_SHOWN;

		SDL_DestroyWindow(screen);
		screen = SDL_CreateWindow(msg->get(WINDOW_TITLE).c_str(),
									SDL_WINDOWPOS_CENTERED,
									SDL_WINDOWPOS_CENTERED,
									width, height,
									flags);

		if (HWSURFACE) flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE;
		else flags = SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE;

		if (DOUBLEBUF) flags = flags | SDL_RENDERER_PRESENTVSYNC;

		renderer = SDL_CreateRenderer(screen, -1, flags);

	}
	else {
		Uint32 flags = 0;

		if (FULLSCREEN) flags = SDL_WINDOW_FULLSCREEN;
		else flags = SDL_WINDOW_SHOWN;

		screen = SDL_CreateWindow(msg->get(WINDOW_TITLE).c_str(),
									SDL_WINDOWPOS_CENTERED,
									SDL_WINDOWPOS_CENTERED,
									width, height,
									flags);

		if (HWSURFACE) flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE;
		else flags = SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE;

		if (DOUBLEBUF) flags = flags | SDL_RENDERER_PRESENTVSYNC;

		if (screen != NULL) renderer = SDL_CreateRenderer(screen, -1, flags);
	}

	if (screen != NULL && renderer != NULL) {
		is_initialized = true;

		// Add Window Titlebar Icon
		if (titlebar_icon == NULL) {
			titlebar_icon = IMG_Load(mods->locate("images/logo/icon.png").c_str());
			SDL_SetWindowIcon(screen, titlebar_icon);
		}

		return (set_fullscreen == 0 ? 0 : -1);
	}
	else {
		fprintf(stderr, "createContext() failed: %s\n", SDL_GetError());
		SDL_Quit();
		exit(1);
	}
}

Rect SDLHardwareRenderDevice::getContextSize() {
	Rect size;
	size.x = size.y = 0;
	SDL_GetWindowSize(screen, &size.w, &size.h);

	return size;
}

int SDLHardwareRenderDevice::render(Renderable& r, Rect dest) {
	dest.w = r.src.w;
	dest.h = r.src.h;
    SDL_Rect src = r.src;
    SDL_Rect _dest = dest;
	return SDL_RenderCopy(renderer, static_cast<SDLHardwareImage *>(r.sprite)->surface, &src, &_dest);
}

int SDLHardwareRenderDevice::render(Sprite *r) {
	if (r == NULL) {
		return -1;
	}
	if ( !localToGlobal(r) ) {
		return -1;
	}

	// negative x and y clip causes weird stretching
	// adjust for that here
	if (m_clip.x < 0) {
		m_clip.w -= abs(m_clip.x);
		m_dest.x += abs(m_clip.x);
		m_clip.x = 0;
	}
	if (m_clip.y < 0) {
		m_clip.h -= abs(m_clip.y);
		m_dest.y += abs(m_clip.y);
		m_clip.y = 0;
	}

	m_dest.w = m_clip.w;
	m_dest.h = m_clip.h;

    SDL_Rect src = m_clip;
    SDL_Rect dest = m_dest;
	return SDL_RenderCopy(renderer, static_cast<SDLHardwareImage *>(r->getGraphics())->surface, &src, &dest);
}

int SDLHardwareRenderDevice::renderImage(Image* image, Rect& src) {
	if (!image) return -1;

    SDL_Rect _src = src;
	SDL_Rect dest;
	dest.x = 0;
	dest.y = 0;

	// negative x and y clip causes weird stretching
	// adjust for that here
	if (src.x < 0) {
		src.w -= abs(src.x);
		dest.x += abs(src.x);
		src.x = 0;
	}
	if (src.y < 0) {
		src.h -= abs(src.y);
		dest.y += abs(src.y);
		src.y = 0;
	}

	dest.w = src.w;
	dest.h = src.h;

	return SDL_RenderCopy(renderer, static_cast<SDLHardwareImage *>(image)->surface, &_src, &dest);
}

int SDLHardwareRenderDevice::renderToImage(Image* src_image, Rect& src, Image* dest_image, Rect& dest, bool dest_is_transparent) {
	if (!src_image || !dest_image) return -1;
	if (SDL_SetRenderTarget(renderer, static_cast<SDLHardwareImage *>(dest_image)->surface) != 0) return -1;
	SDL_SetTextureBlendMode(static_cast<SDLHardwareImage *>(dest_image)->surface, SDL_BLENDMODE_BLEND);
	dest.w = src.w;
	dest.h = src.h;
    SDL_Rect _src = src;
    SDL_Rect _dest = dest;
	SDL_RenderCopy(renderer, static_cast<SDLHardwareImage *>(src_image)->surface, &_src, &_dest);
	SDL_SetRenderTarget(renderer, NULL);
	return 0;
}

int SDLHardwareRenderDevice::renderText(
	TTF_Font *ttf_font,
	const std::string& text,
	Color color,
	Rect& dest
) {
	int ret = 0;
	SDL_Color _color = color;
	SDL_Texture *surface;

	SDL_Surface *cleanup = TTF_RenderUTF8_Blended(ttf_font, text.c_str(), color);
	if (cleanup) {
		surface = SDL_CreateTextureFromSurface(renderer,cleanup);
		textures_count+=1;
		SDL_FreeSurface(cleanup);
	}

	if (surface == NULL)
		return -1;

	SDL_Rect clip;
	int w, h;
	SDL_QueryTexture(surface, NULL, NULL, &w, &h);

	clip.x = clip.y = 0;
	clip.w = w;
	clip.h = h;

	dest.w = clip.w;
	dest.h = clip.h;
	SDL_Rect _dest = dest;

	ret = SDL_RenderCopy(renderer, surface, &clip, &_dest);

	SDL_DestroyTexture(surface);
	textures_count-=1;

	return ret;
}

Image * SDLHardwareRenderDevice::renderTextToImage(TTF_Font* ttf_font, const std::string& text, Color color, bool blended) {
	SDLHardwareImage *image = new SDLHardwareImage(this);
	SDL_Color _color = color;

	SDL_Surface *cleanup;

	if (blended) {
		cleanup = TTF_RenderUTF8_Blended(ttf_font, text.c_str(), color);
	}
	else {
		cleanup = TTF_RenderUTF8_Solid(ttf_font, text.c_str(), color);
	}

	if (cleanup) {
		image->surface = SDL_CreateTextureFromSurface(renderer, cleanup);
		textures_count+=1;
		SDL_FreeSurface(cleanup);
		return image;
	}

	delete image;
	return NULL;
}

void SDLHardwareRenderDevice::drawPixel(
	int x,
	int y,
	Uint32 color
) {
	Uint32 u_format = SDL_GetWindowPixelFormat(screen);
	SDL_PixelFormat* format = SDL_AllocFormat(u_format);

	if (!format) return;

	SDL_Color rgba;
	SDL_GetRGBA(color, format, &rgba.r, &rgba.g, &rgba.b, &rgba.a);
	SDL_FreeFormat(format);

	SDL_SetRenderDrawColor(renderer, rgba.r, rgba.g, rgba.b, rgba.a);
	SDL_RenderDrawPoint(renderer, x, y);
}

/*
 * Set the pixel at (x, y) to the given value
 */
void SDLHardwareRenderDevice::drawPixel(Image *image, int x, int y, Uint32 pixel) {
	if (!image) return;

	Uint32 u_format;
	SDL_QueryTexture(static_cast<SDLHardwareImage *>(image)->surface, &u_format, NULL, NULL, NULL);
	SDL_PixelFormat* format = SDL_AllocFormat(u_format);

	if (!format) return;

	SDL_Color rgba;
	SDL_GetRGBA(pixel, format, &rgba.r, &rgba.g, &rgba.b, &rgba.a);
	SDL_FreeFormat(format);

	SDL_SetRenderTarget(renderer, static_cast<SDLHardwareImage *>(image)->surface);
	SDL_SetTextureBlendMode(static_cast<SDLHardwareImage *>(image)->surface, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, rgba.r, rgba.g, rgba.b, rgba.a);
	SDL_RenderDrawPoint(renderer, x, y);
	SDL_SetRenderTarget(renderer, NULL);
}

void SDLHardwareRenderDevice::drawLine(
	int x0,
	int y0,
	int x1,
	int y1,
	Uint32 color
) {
	Uint32 u_format = SDL_GetWindowPixelFormat(screen);
	SDL_PixelFormat* format = SDL_AllocFormat(u_format);

	if (!format) return;

	SDL_Color rgba;
	SDL_GetRGBA(color, format, &rgba.r, &rgba.g, &rgba.b, &rgba.a);
	SDL_FreeFormat(format);

	SDL_SetRenderDrawColor(renderer, rgba.r, rgba.g, rgba.b, rgba.a);
	SDL_RenderDrawLine(renderer, x0, y0, x1, y1);
}

void SDLHardwareRenderDevice::drawLine(
	const Point& p0,
	const Point& p1,
	Uint32 color
) {
	drawLine(p0.x, p0.y, p1.x, p1.y, color);
}

void SDLHardwareRenderDevice::drawRectangle(
	const Point& p0,
	const Point& p1,
	Uint32 color
) {
	drawLine(p0.x, p0.y, p1.x, p0.y, color);
	drawLine(p1.x, p0.y, p1.x, p1.y, color);
	drawLine(p0.x, p0.y, p0.x, p1.y, color);
	drawLine(p0.x, p1.y, p1.x, p1.y, color);
}

void SDLHardwareRenderDevice::blankScreen() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);
	return;
}

void SDLHardwareRenderDevice::commitFrame() {
	SDL_RenderPresent(renderer);
	return;
}

void SDLHardwareRenderDevice::destroyContext() {
	SDL_FreeSurface(titlebar_icon);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(screen);

	return;
}

void SDLHardwareRenderDevice::fillImageWithColor(Image *dst, Rect *dstrect, Uint32 color) {
	if (!dst) return;

	Uint32 u_format;
	SDL_QueryTexture(static_cast<SDLHardwareImage *>(dst)->surface, &u_format, NULL, NULL, NULL);
	SDL_PixelFormat* format = SDL_AllocFormat(u_format);

	if (!format) return;

	SDL_Color rgba;
	SDL_GetRGBA(color, format, &rgba.r, &rgba.g, &rgba.b, &rgba.a);
	SDL_FreeFormat(format);

	SDL_SetRenderTarget(renderer, static_cast<SDLHardwareImage *>(dst)->surface);
	SDL_SetTextureBlendMode(static_cast<SDLHardwareImage *>(dst)->surface, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, rgba.r, rgba.g , rgba.b, rgba.a);
	SDL_RenderClear(renderer);
	SDL_SetRenderTarget(renderer, NULL);
}

Uint32 SDLHardwareRenderDevice::MapRGB(Image *src, Uint8 r, Uint8 g, Uint8 b) {
	if (!src || !static_cast<SDLHardwareImage *>(src)->surface) return 0;

	Uint32 u_format;
	SDL_QueryTexture(static_cast<SDLHardwareImage *>(src)->surface, &u_format, NULL, NULL, NULL);
	SDL_PixelFormat* format = SDL_AllocFormat(u_format);

	if (format) {
		Uint32 ret = SDL_MapRGB(format, r, g, b);
		SDL_FreeFormat(format);
		return ret;
	}
	else {
		return 0;
	}
}

Uint32 SDLHardwareRenderDevice::MapRGB(Uint8 r, Uint8 g, Uint8 b) {
	Uint32 u_format = SDL_GetWindowPixelFormat(screen);
	SDL_PixelFormat* format = SDL_AllocFormat(u_format);

	if (format) {
		Uint32 ret = SDL_MapRGB(format, r, g, b);
		SDL_FreeFormat(format);
		return ret;
	}
	else {
		return 0;
	}
}

Uint32 SDLHardwareRenderDevice::MapRGBA(Image *src, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	if (!src || !static_cast<SDLHardwareImage *>(src)->surface) return 0;

	Uint32 u_format;
	SDL_QueryTexture(static_cast<SDLHardwareImage *>(src)->surface, &u_format, NULL, NULL, NULL);
	SDL_PixelFormat* format = SDL_AllocFormat(u_format);

	if (format) {
		Uint32 ret = SDL_MapRGBA(format, r, g, b, a);
		SDL_FreeFormat(format);
		return ret;
	}
	else {
		return 0;
	}
}

Uint32 SDLHardwareRenderDevice::MapRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	Uint32 u_format = SDL_GetWindowPixelFormat(screen);
	SDL_PixelFormat* format = SDL_AllocFormat(u_format);

	if (format) {
		Uint32 ret = SDL_MapRGBA(format, r, g, b, a);
		SDL_FreeFormat(format);
		return ret;
	}
	else {
		return 0;
	}
}

/**
 * create blank surface
 */
Image *SDLHardwareRenderDevice::createAlphaSurface(int width, int height) {

	SDLHardwareImage *image = new SDLHardwareImage(this);

	if (width > 0 && height > 0) {
		image->surface = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, width, height);
		textures_count+=1;
		if(image->surface == NULL) {
			fprintf(stderr, "CreateRGBSurface failed: %s\n", SDL_GetError());
		}
		else {
				SDL_SetRenderTarget(renderer, image->surface);
				SDL_SetTextureBlendMode(image->surface, SDL_BLENDMODE_BLEND);
				SDL_SetRenderDrawColor(renderer, 0,0,0,0);
				SDL_RenderClear(renderer);
				SDL_SetRenderTarget(renderer, NULL);
		}
	}

	return image;
}

void SDLHardwareRenderDevice::setGamma(float g) {
	Uint16 ramp[256];
	SDL_CalculateGammaRamp(g, ramp);
	SDL_SetWindowGammaRamp(screen, ramp, ramp, ramp);
}

void SDLHardwareRenderDevice::listModes(std::vector<Rect> &modes) {
	Rect** detect_modes;
	std::vector<Rect> vec_detect_modes;
	Rect detect_mode;
	SDL_DisplayMode mode;
	/* SDL_compat.c */
	for (int i = 0; i < SDL_GetNumDisplayModes(SDL_GetWindowDisplayIndex(screen)); ++i) {
		SDL_GetDisplayMode(SDL_GetWindowDisplayIndex(screen), i, &mode);

		if (!vec_detect_modes.empty())
		{
			if (vec_detect_modes.back().w == mode.w && vec_detect_modes.back().h == mode.h) {
			continue;
			}
		}

		detect_mode.x = 0;
		detect_mode.y = 0;
		detect_mode.w = mode.w;
		detect_mode.h = mode.h;
		vec_detect_modes.push_back(detect_mode);

	}
	detect_modes = (Rect**)calloc(vec_detect_modes.size(),sizeof(Rect));
	for (unsigned i = 0; i < vec_detect_modes.size(); ++i) {
		detect_modes[i] = &vec_detect_modes[i];
	}
	vec_detect_modes.clear();
	// Check if there are any modes available
	if (detect_modes == (Rect**)0) {
		fprintf(stderr, "No modes available!\n");
		return;
	}

	// Check if our resolution is restricted
	if (detect_modes == (Rect**)-1) {
		fprintf(stderr, "All resolutions available.\n");
	}

	for (unsigned i=0; detect_modes[i]; ++i) {
		modes.push_back(*detect_modes[i]);
		if (detect_modes[i]->w < MIN_VIEW_W || detect_modes[i]->h < MIN_VIEW_H) {
			// make sure the resolution fits in the constraints of MIN_VIEW_W and MIN_VIEW_H
			modes.pop_back();
		}
		else {
			// check previous resolutions for duplicates. If one is found, drop the one we just added
			for (unsigned j=0; j<modes.size()-1; ++j) {
				if (modes[j].w == detect_modes[i]->w && modes[j].h == detect_modes[i]->h) {
					modes.pop_back();
					break;
				}
			}
		}
	}
	if (detect_modes)
		free(detect_modes);
}

Image *SDLHardwareRenderDevice::loadGraphicSurface(std::string filename, std::string errormessage, bool IfNotFoundExit) {
	// lookup image in cache
	Image *img;
	img = cacheLookup(filename);
	if (img != NULL) return img;

	// load image
	SDLHardwareImage *image = new SDLHardwareImage(this);
	if (!image) return NULL;

	image->surface = IMG_LoadTexture(renderer, mods->locate(filename).c_str());
	textures_count+=1;

	if(image == NULL) {
		if (!errormessage.empty())
			fprintf(stderr, "%s: %s\n", errormessage.c_str(), IMG_GetError());
		if (IfNotFoundExit) {
			SDL_Quit();
			exit(1);
		}
	}

	// store image to cache
	cacheStore(filename, image);
	return image;
}

void SDLHardwareRenderDevice::scaleSurface(Image *source, int width, int height) {
	if(!source || !width || !height)
		return;

	Image *dest = createAlphaSurface(width, height);
	if (dest != NULL) {
		// copy the source texture to the new texture, stretching it in the process
		SDL_SetRenderTarget(renderer, static_cast<SDLHardwareImage *>(dest)->surface);
		SDL_RenderCopyEx(renderer, static_cast<SDLHardwareImage *>(source)->surface, NULL, NULL, 0, NULL, SDL_FLIP_NONE);
		SDL_SetRenderTarget(renderer, NULL);

		// Remove the old surface
		SDL_DestroyTexture(static_cast<SDLHardwareImage *>(source)->surface);
		textures_count-=1;
		static_cast<SDLHardwareImage *>(source)->surface = static_cast<SDLHardwareImage *>(dest)->surface;
	}
}

Uint32 SDLHardwareRenderDevice::readPixel(Image *image, int x, int y) {
	//Unimplemented
	return 0;
}

/*
 * Returns false if a pixel at Point px is transparent
 */
bool SDLHardwareRenderDevice::checkPixel(Point px, Image *image) {
	//Unimplemented
	return true;
}

void SDLHardwareRenderDevice::freeImage(Image *image) {
	if (!image) return;

	cacheRemove(image);

	if (static_cast<SDLHardwareImage *>(image)->surface)
	{
		SDL_DestroyTexture(static_cast<SDLHardwareImage *>(image)->surface);
		textures_count-=1;
	}
}

#endif
