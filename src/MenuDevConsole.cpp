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

/**
 * class MenuDevConsole
 */

#include "FileParser.h"
#include "MenuDevConsole.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "Settings.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"

#include <limits>

MenuDevConsole::MenuDevConsole() : Menu() {

	// Load config settings
	FileParser infile;
	// @CLASS MenuDevConsole|Description of menus/devconsole.txt
	if(infile.open("menus/devconsole.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR close|x (integer), y (integer)|Position of the close button.
			if(infile.key == "close") close_pos = toPoint(infile.val);
			// @ATTR label_title|label|Position of the "Developer Console" label.
			else if(infile.key == "label_title") title = eatLabelInfo(infile.val);
			// @ATTR confirm|x (integer), y (integer)|Position of the "Execute" button.
			else if(infile.key == "confirm") confirm_pos = toPoint(infile.val);
			// @ATTR input|x (integer), y (integer)|Position of the command entry widget.
			else if(infile.key == "input") input_pos = toPoint(infile.val);
			// @ATTR history|x (integer), y (integer), w (integer), h (integer)|Position and dimensions of the command history.
			else if(infile.key == "history") history_area = toRect(infile.val);
		}
		infile.close();
	}

	button_close = new WidgetButton("images/menus/buttons/button_x.png");
	tablist.add(button_close);

	input_box = new WidgetInput("images/menus/input_console.png");
	tablist.add(input_box);

	button_confirm = new WidgetButton("images/menus/buttons/button_default.png");
	button_confirm->label = msg->get("Execute");
	tablist.add(button_confirm);

	log_history = new WidgetLog(history_area.w, history_area.h);
	tablist.add(log_history->getWidget());

	setBackground("images/menus/dev_console.png");

	align();
	alignElements();

	color_echo = font->getColor("widget_disabled");
	color_error = font->getColor("menu_penalty");
}

MenuDevConsole::~MenuDevConsole() {
	delete button_close;
	delete button_confirm;
	delete input_box;
	delete log_history;
}

void MenuDevConsole::alignElements() {
	button_close->pos.x = window_area.x + close_pos.x;
	button_close->pos.y = window_area.y + close_pos.y;

	button_confirm->pos.x = window_area.x + confirm_pos.x;
	button_confirm->pos.y = window_area.y + confirm_pos.y;
	button_confirm->refresh();

	input_box->setPosition(window_area.x + input_pos.x, window_area.y + input_pos.y);
	input_box->inFocus = true;

	log_history->setPosition(window_area.x + history_area.x, window_area.y + history_area.y);

	label.set(window_area.x+title.x, window_area.y+title.y, title.justify, title.valign, msg->get("Developer Console"), font->getColor("menu_normal"));
}

void MenuDevConsole::logic() {
	if (visible) {
		if (!input_box->inFocus) {
			tablist.logic();
		}

		input_box->logic();
		log_history->logic();

		if (button_close->checkClick()) {
			visible = false;
			reset();
		}
		else if (button_confirm->checkClick()) {
			execute();
		}
		else if (input_box->inFocus && inpt->pressing[ACCEPT] && !inpt->lock[ACCEPT]) {
			inpt->lock[ACCEPT] = true;
			execute();
		}
		else if (input_box->inFocus && inpt->pressing[CANCEL] && !inpt->lock[CANCEL]) {
			inpt->lock[CANCEL] = true;
			input_box->inFocus = false;
		}
	}
}

void MenuDevConsole::render() {
	if (visible) {
		// background
		Menu::render();

		label.render();
		button_close->render();
		button_confirm->render();
		input_box->render();
		log_history->render();
	}
}

bool MenuDevConsole::inputFocus() {
	return visible && input_box->inFocus;
}

void MenuDevConsole::reset() {
	input_box->setText("");
	input_box->inFocus = true;
	// log_history->clear();
}

void MenuDevConsole::execute() {
	std::string command = input_box->getText();
	if (command == "") return;

	input_box->setText("");

	log_history->add(command, false, &color_echo);

	std::vector<std::string> args;
	command += ' ';

	std::string arg = popFirstString(command, ' ');
	while (arg != "") {
		args.push_back(arg);
		arg = popFirstString(command, ' ');
	}


	if (args.empty()) {
		return;
	}

	if (args[0] == "help") {
		log_history->add("teleport - " + msg->get("teleports the player to a specific tile, and optionally, a specific map"), false);
		log_history->add("unset_status - " + msg->get("unsets the given campaign statuses if they are set"), false);
		log_history->add("set_status - " + msg->get("sets the given campaign statuses"), false);
		log_history->add("give_xp - " + msg->get("rewards the player with the specified amount of experience points"), false);
		log_history->add("give_currency - " + msg->get("adds the specified amount of currency to the player's inventory"), false);
		log_history->add("give_item - " + msg->get("adds an item to the player's inventory"), false);
		log_history->add("spawn_enemy - " + msg->get("spawns an enemy matching the given category next to the player"), false);
		log_history->add("toggle_devhud - " + msg->get("turns on/off the developer hud"), false);
		log_history->add("clear - " + msg->get("clears the command history"), false);
		log_history->add("help - " + msg->get("displays this text"), false);
	}
	else if (args[0] == "clear") {
		log_history->clear();
	}
	else if (args[0] == "toggle_devhud") {
		DEV_HUD = !DEV_HUD;
		log_history->add(msg->get("Toggled the developer hud"), false);
	}
	else if (args[0] == "spawn_enemy") {
		if (args.size() > 1) {
			int min_level = (args.size() > 2 && toInt(args[2]) > 0) ? toInt(args[2]) : 0;
			int max_level = (args.size() > 3 && toInt(args[3]) > 0) ? toInt(args[3]) : std::numeric_limits<int>::max();
			Enemy_Level el = enemyg->getRandomEnemy(args[1], min_level, max_level);
			if (el.type != "") {
				Point spawn_pos = floor(mapr->collider.get_random_neighbor(floor(pc->stats.pos), 1));
				powers->spawn(el.type, spawn_pos);
				log_history->add(msg->get("Spawned enemy: ") + el.type);
			}
			else {
				log_history->add(msg->get("ERROR: Invalid enemy category"), false, &color_error);
			}
		}
		else {
			log_history->add(msg->get("ERROR: Too few arguments"), false, &color_error);
		}
	}
	else if (args[0] == "give_item") {
		if (args.size() > 1) {
			int id = toInt(args[1]);
			if (id <= 0 || (unsigned)id >= items->items.size() || items->items[id].name == "") {
				log_history->add(msg->get("ERROR: Invalid item ID"), false, &color_error);
				return;
			}

			int quantity = (args.size() > 2) ? toInt(args[2]) : 1;

			if (quantity > 0) {
				if (id == CURRENCY_ID) {
					camp->rewardCurrency(quantity);
				}
				else {
					ItemStack stack;
					stack.item = id;
					stack.quantity = quantity;
					camp->rewardItem(stack);
				}
				log_history->add(msg->get("Added item: ") + items->items[id].name + " (" + toString(typeid(int), &quantity) + ")", false);
			}
		}
		else {
			log_history->add(msg->get("ERROR: Too few arguments"), false, &color_error);
		}
	}
	else if (args[0] == "give_currency") {
		int quantity = (args.size() > 1) ? toInt(args[1]) : 0;
		if (quantity > 0) {
			camp->rewardCurrency(quantity);
			log_history->add(msg->get("Added currency: ") + toString(typeid(int), &quantity), false);
		}
		if (args.size() < 2) {
			log_history->add(msg->get("ERROR: Too few arguments"), false, &color_error);
		}
	}
	else if (args[0] == "give_xp") {
		int quantity = (args.size() > 1) ? toInt(args[1]) : 0;
		if (quantity > 0) {
			camp->rewardXP(quantity, true);
			log_history->add(msg->get("Added XP: ") + toString(typeid(int), &quantity), false);
		}
		if (args.size() < 2) {
			log_history->add(msg->get("ERROR: Too few arguments"), false, &color_error);
		}
	}
	else if (args[0] == "set_status") {
		for (unsigned i = 1; i < args.size(); ++i) {
			camp->setStatus(args[i]);
			log_history->add(msg->get("Set campaign status: ") + args[i], false);
		}
		if (args.size() < 2) {
			log_history->add(msg->get("ERROR: Too few arguments"), false, &color_error);
		}
	}
	else if (args[0] == "unset_status") {
		for (unsigned i = 1; i < args.size(); ++i) {
			if (camp->checkStatus(args[i])) {
				camp->unsetStatus(args[i]);
				log_history->add(msg->get("Unset campaign status: ") + args[i], false);
			}
			else {
				log_history->add(msg->get("ERROR: Unknown campaign status: ") + args[i], false, &color_error);
			}
		}
		if (args.size() < 2) {
			log_history->add(msg->get("ERROR: Too few arguments"), false, &color_error);
		}
	}
	else if (args[0] == "teleport") {
		if (args.size() > 2) {
			FPoint dest;
			dest.x = toInt(args[1]) + 0.5f;
			dest.y = toInt(args[2]) + 0.5f;

			if (args.size() > 3) {
				if (fileExists(mods->locate(args[3]))) {
					mapr->teleportation = true;
					mapr->teleport_destination.x = dest.x;
					mapr->teleport_destination.y = dest.y;
					mapr->teleport_mapname = args[3];
					log_history->add(msg->get("Teleporting to: " + args[1] + ", " + args[2] + ", " + args[3]), false);
				}
				else {
					log_history->add(msg->get("ERROR: Unknown map: ") + args[3], false, &color_error);
				}
			}
			else {
				mapr->teleportation = true;
				mapr->teleport_destination.x = dest.x;
				mapr->teleport_destination.y = dest.y;
				log_history->add(msg->get("Teleporting to: " + args[1] + ", " + args[2]), false);
			}
		}
		else {
			log_history->add(msg->get("ERROR: Too few arguments"), false, &color_error);
		}
	}
	else {
		log_history->add(msg->get("ERROR: Unknown command"), false, &color_error);
	}
}
