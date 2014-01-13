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

#include "CommonIncludes.h"
#include "FileParser.h"
#include "GameStateConfig.h"
#include "GameStateResolution.h"
#include "GameStateTitle.h"
#include "MenuConfirm.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsParsing.h"

GameStateResolution::GameStateResolution(int width, int height, bool fullscreen, bool hwsurface, bool doublebuf)
	: GameState()
	, confirm(NULL)
	, confirm_align("")
	, confirm_ticks(0)
	, old_w(VIEW_W)
	, old_h(VIEW_H)
	, old_fullscreen(fullscreen)
	, old_hwsurface(hwsurface)
	, old_doublebuf(doublebuf)
	, new_w(width)
	, new_h(height)
	, initialized(false)
{
}

void GameStateResolution::logic() {
	if (!initialized) {
		initialized = true;

		// Apply the new resolution
		// if it fails, don't create the dialog box (this will make the game continue straight to the title screen)
		if (compareVideoSettings() && applyVideoSettings(new_w, new_h))
			confirm = new MenuConfirm(msg->get("OK"),msg->get("Use this resolution?"));

		if (confirm) {
			// Load the MenuConfirm positions and alignments from menus/menus.txt
			FileParser infile;
			if (infile.open("menus/menus.txt")) {
				int menu_index = -1;
				while (infile.next()) {
					if (infile.key == "id") {
						if (infile.val == "confirm") menu_index = 0;
						else menu_index = -1;
					}

					if (menu_index == -1)
						continue;

					if (infile.key == "layout") {
						infile.val = infile.val + ',';
						confirm_area.x = eatFirstInt(infile.val, ',');
						confirm_area.y = eatFirstInt(infile.val, ',');
						confirm_area.w = eatFirstInt(infile.val, ',');
						confirm_area.h = eatFirstInt(infile.val, ',');
					}

					if (infile.key == "align") {
						confirm_align = infile.val;
					}
				}
				infile.close();
			}

			confirm->visible = true;
			confirm->window_area = confirm_area;
			confirm->alignment = confirm_align;
			confirm->align();
			confirm->update();

			confirm_ticks = MAX_FRAMES_PER_SEC * 10;
		}
		else {
			delete requestedGameState;
			requestedGameState = new GameStateTitle();
			return;
		}
	}

	if (confirm) {
		confirm->logic();

		if (confirm_ticks > 0) confirm_ticks--;

		if (confirm->confirmClicked) {
			saveSettings();
			delete requestedGameState;
			requestedGameState = new GameStateTitle();
		}
		else if (confirm->cancelClicked || confirm_ticks == 0) {
			cleanup();
			FULLSCREEN = old_fullscreen;
			HWSURFACE = old_hwsurface;
			DOUBLEBUF = old_doublebuf;
			if (applyVideoSettings(old_w, old_h)) {
				saveSettings();
			}
			delete requestedGameState;
			requestedGameState = new GameStateConfig();
		}
	}
	else {
		delete requestedGameState;
		requestedGameState = new GameStateTitle();
	}
}

void GameStateResolution::render() {
	if (confirm)
		confirm->render();
}

/**
 * Tries to apply the selected video settings, reverting back to the old settings upon failure
 */
bool GameStateResolution::applyVideoSettings(int width, int height) {
	if (MIN_VIEW_W > width && MIN_VIEW_H > height) {
		fprintf (stderr, "A mod is requiring a minimum resolution of %dx%d\n", MIN_VIEW_W, MIN_VIEW_H);
		if (width < MIN_VIEW_W) width = MIN_VIEW_W;
		if (height < MIN_VIEW_H) height = MIN_VIEW_H;
	}

	// Attempt to apply the new settings
	int status = render_device->createContext(width, height);

	// If the new settings fail, revert to the old ones
	if (status == -1) {
		fprintf (stderr, "Error during SDL_SetVideoMode: %s\n", SDL_GetError());
		render_device->createContext(VIEW_W, VIEW_H);
		return false;

	}
	else {

		// If the new settings succeed, adjust the view area
		VIEW_W = width;
		VIEW_W_HALF = width/2;
		VIEW_H = height;
		VIEW_H_HALF = height/2;

		return true;
	}
}

/**
 * Checks if the video settings have changed
 * Returns true if they have, otherwise returns false
 */
bool GameStateResolution::compareVideoSettings() {
	return (!(old_w == new_w && old_h == new_h) ||
			 FULLSCREEN != old_fullscreen ||
			 HWSURFACE != old_hwsurface ||
			 DOUBLEBUF != old_doublebuf);
}

void GameStateResolution::cleanup() {
	if (confirm) {
		delete confirm;
		confirm = NULL;
	}
}

GameStateResolution::~GameStateResolution() {
	cleanup();
}
