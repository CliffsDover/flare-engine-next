/*
Copyright © 2014 Justin Jacobs

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

#include <stdio.h>
#include <string>

#include "RenderDeviceList.h"

#include "SDLRenderDevice.h"

#if SDL_VERSION_ATLEAST(2,0,0)
#include "SDL2RenderDevice.h"
#endif

RenderDevice* getRenderDevice(std::string name) {
	// "sdl" is the default
	if (name != "") {
		if (name == "sdl") return new SDLRenderDevice();
#if SDL_VERSION_ATLEAST(2,0,0)
		else if (name == "sdl2") return new SDL2RenderDevice();
#endif
		else {
			fprintf(stderr, "Render device '%s' not found. Falling back to the default.\n", name.c_str());
			return new SDLRenderDevice();
		}
	}
	else {
		return new SDLRenderDevice();
	}
}
