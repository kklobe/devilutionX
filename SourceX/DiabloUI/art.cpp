#include "DiabloUI/art.h"
#include "display.h"
#include "../3rdParty/Storm/Source/storm.h"

namespace devilution {

void LoadArt(const char *pszFile, Art *art, int frames, SDL_Color *pPalette)
{
	if (art == NULL || art->surface != NULL)
		return;

	art->frames = frames;

	DWORD width, height, bpp;
	if (!SBmpLoadImage(pszFile, 0, 0, 0, &width, &height, &bpp)) {
		SDL_Log("Failed to load image meta");
		return;
	}

	Uint32 format;
	switch (bpp) {
	case 8:
		format = SDL_PIXELFORMAT_INDEX8;
		break;
	case 24:
		format = SDL_PIXELFORMAT_RGB888;
		break;
	case 32:
		format = SDL_PIXELFORMAT_RGBA8888;
		break;
	default:
		format = 0;
		break;
	}
	SDLSurfaceUniquePtr art_surface { SDL_CreateRGBSurfaceWithFormat(SDL_SWSURFACE, width, height, bpp, format) };

	if (!SBmpLoadImage(pszFile, pPalette, static_cast<BYTE *>(art_surface->pixels),
	        art_surface->pitch * art_surface->format->BytesPerPixel * height, 0, 0, 0)) {
		SDL_Log("Failed to load image");
		return;
	}

	art->logical_width = art_surface->w;
	art->frame_height = height / frames;

	art->surface = ScaleSurfaceToOutput(std::move(art_surface));
}

void LoadMaskedArt(const char *pszFile, Art *art, int frames, int mask)
{
	LoadArt(pszFile, art, frames);
	if (art->surface != NULL)
		SDLC_SetColorKey(art->surface.get(), mask);
}

void LoadArt(Art *art, const BYTE *artData, int w, int h, int frames)
{
	art->frames = frames;
	art->surface = ScaleSurfaceToOutput(SDLSurfaceUniquePtr{SDL_CreateRGBSurfaceWithFormatFrom(
	    const_cast<BYTE *>(artData), w, h, 8, w, SDL_PIXELFORMAT_INDEX8)});
	art->logical_width = w;
	art->frame_height = h / frames;
}

} // namespace devilution
