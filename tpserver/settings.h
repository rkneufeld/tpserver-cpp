#ifndef SETTINGS_H
#define SETTINGS_H
/*  Settings class
 *
 *  Copyright (C) 2004-2005, 2006  Lee Begg and the Thousand Parsec Project
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <string>
#include <map>

class SettingsCallback;

class Settings {

      public:
	static Settings *getSettings();

	bool readArgs(int argc, char** argv);
	bool readConfFile();

	void set(std::string item, std::string value);
	std::string get(std::string item);
        
        void setCallback(std::string item, SettingsCallback cb);
        void removeCallback(std::string item);

      private:
	 Settings();
	~Settings();
	 Settings(Settings & rhs);
	Settings operator=(Settings & rhs);

	void printHelp();
        void gripeOnLine(const std::string& line, const char* complaint);
        bool readConfFile(const std::string& fname);

	void setDefaultValues();

	// settings storage
	std::map<std::string, std::string> store;
        std::map<std::string, SettingsCallback> callbacks;

	static Settings *myInstance;

};

#endif
