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

#pragma once
#ifndef GAMESTATERESOLUTION_H
#define GAMESTATERESOLUTION_H

#include "GameState.h"

class MenuConfirm;

class GameStateResolution : public GameState {
private:
	bool applyVideoSettings(int width, int height);
	void cleanup();
	MenuConfirm *confirm;
	SDL_Rect confirm_area;
	std::string confirm_align;
	int confirm_ticks;
	int old_w;
	int old_h;
	bool old_fs;

public:
	GameStateResolution(int width, int height, bool fullscreen);
	~GameStateResolution();
	void logic();
	void render();
};

#endif

