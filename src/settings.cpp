/*  Setting class
 *
 *  Copyright (C) 2003-2005  Lee Begg and the Thousand Parsec Project
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

#include <cassert>
#include <iostream>
#include <fstream>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define VERSION "0.0.0"
#endif

#include "logging.h"

#include "settings.h"

Settings *Settings::myInstance = NULL;

Settings *Settings::getSettings()
{
	if (myInstance == NULL) {
		myInstance = new Settings();
		myInstance->setDefaultValues();
	}
	return myInstance;
}

bool Settings::readArgs(int argc, char** argv){

  for(int i = 1; i < argc; i++){
    if(argv[i][0] == '-'){
      if(argv[i][1] == '-'){
	//long option
	if(strncmp(argv[i] + 2, "help", 4) == 0){
	  printHelp();
	  store["NEVER_START"] = "!";
	}else if(strncmp(argv[i] + 2, "version", 7) == 0){
	  std::cout << "tpserver-cpp " VERSION << std::endl;
	  store["NEVER_START"] = "!";
	}else if(strncmp(argv[i] + 2, "port", 4) == 0){
	  store["listen_port"] = std::string(argv[++i]);
	}else if(strncmp(argv[i] + 2, "host" , 4) == 0){
	  store["listen_addr"] = std::string(argv[++i]);
	}else if(strncmp(argv[i] + 2, "configure", 9) == 0){
	  store["config_file"] = std::string(argv[++i]);
	}else if(strncmp(argv[i] + 2, "logging", 7) == 0){
	  store["log_level"] = std::string(argv[++i]);
#ifdef HAVE_LIBMYSQL
        }else if(strncmp(argv[i] + 2, "mydb", 4) == 0){
            store["mysql_db"] = std::string(argv[++i]);
        }else if(strncmp(argv[i] + 2, "mypass", 6) == 0){
            store["mysql_pass"] = std::string(argv[++i]);
        }else if(strncmp(argv[i] + 2, "myuser", 6) == 0){
            store["mysql_user"] = std::string(argv[++i]);
        }else if(strncmp(argv[i] + 2, "myhost", 6) == 0){
            store["mysql_host"] = std::string(argv[++i]);
        }else if(strncmp(argv[i] + 2, "mysocket", 8) == 0){
            store["mysql_socket"] = std::string(argv[++i]);
        }else if(strncmp(argv[i] + 2, "myport", 6) == 0){
            store["mysql_port"] = std::string(argv[++i]);
#endif
	}

      }else{
	//short option
	if(strncmp(argv[i] + 1, "h", 2) == 0){
	  printHelp();
	  store["NEVER_START"] = "!";
	}else if(strncmp(argv[i] + 1, "P", 2) == 0){
	  store["listen_port"] = std::string(argv[++i]);
	}else if(strncmp(argv[i] + 1, "H", 2) == 0){
	  store["listen_addr"] = std::string(argv[++i]);
	}else if(strncmp(argv[i] + 1, "C", 2) == 0){
	  store["config_file"] = std::string(argv[++i]);
	}else if(strncmp(argv[i] + 1, "l", 2) == 0){
	  store["log_level"] = std::string(argv[++i]);
	}

      }
    }
  }

  return true;
}

bool Settings::readConfFile(){
  return readConfFile(store["config_file"]);
}

void Settings::gripeOnLine(const std::string& line, const char* complaint) {
    Logger::getLogger()->error( "Invalid configuration file line, %s: \"%s\"", complaint, line.c_str());
}


bool Settings::readConfFile(const std::string& fname){
    std::ifstream  configFile( fname.c_str());
    std::string   configString;
    unsigned long      lineCount = 0;
    const char* validKeyChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";

    while ( ! configFile.eof() && getline( configFile, configString)) {
        std::string    savedConfigString = configString;
        std::string    key = "";
        std::string    value = "";
        std::string::size_type valuelength = 0;
        std::string::size_type keylength = 0;

        /* remove leading spaces */
        configString.erase( 0, configString.find_first_not_of( " 	"));

        /* comment/empty line? */
        if ( configString == "" || configString[0] == '#') {
            continue;
        }

        lineCount++;
        keylength = configString.find_first_of( " 	=");
        key = configString.substr( 0, keylength);
        configString.erase( 0, keylength);
        if ( key == "") {
            gripeOnLine( savedConfigString, "empty key");
            continue;
        }
        if ( key.find_first_not_of( validKeyChars) < keylength ||
             key.find_first_of( "0123456789") == 0) {
            gripeOnLine( savedConfigString, "invalid characters in key");
            continue;
        }

        /* remove whitespace */
        configString.erase( 0, configString.find_first_not_of( " 	"));

        /* this next character better be an equals sign */
        if ( configString[0] != '=') {
            gripeOnLine( savedConfigString, "missing equals");
            continue;
        }
        configString.erase( 0, 1);

        /* remove whitespace... */
        configString.erase( 0, configString.find_first_not_of( " 	"));

        /* Are we quoted? */
        if ( configString[0] == '"') {
            configString.erase( 0, 1);
            valuelength = configString.find_first_of( "\"");
            value = configString.substr( 0, valuelength);
            configString.erase( 0, valuelength + 1);

            /* remove whitespace... */
            configString.erase( 0, configString.find_first_not_of( " 	"));

            /* What is left ought to be a comment */
            if ( !configString.empty() && configString[0] != '#') {
                gripeOnLine( savedConfigString, "extraneous characters");
            }
            /* But if it isn't, we just let it pass... */
        }
        else if ( configString[0] == '\'') {
            configString.erase( 0, 1);
            valuelength = configString.find_first_of( "\'");
            value = configString.substr( 0, valuelength);
            configString.erase( 0, valuelength + 1);

            /* remove whitespace... */
            configString.erase( 0, configString.find_first_not_of( " 	"));

            /* What is left ought to be a comment */
            if ( !configString.empty() && configString[0] != '#') {
                gripeOnLine( savedConfigString, "extraneous characters");
            }
            /* But if it isn't, we just let it pass... */
        }
        else {
            valuelength = configString.find_first_of( " 	");
            value = configString.substr( 0, valuelength);
            configString.erase( 0, valuelength);

            /* remove whitespace... */
            configString.erase( 0, configString.find_first_not_of( " 	"));

            /* What is left ought to be a comment */
            if ( !configString.empty() && configString[0] != '#') {
                gripeOnLine( savedConfigString, "extraneous characters");
            }
            /* But if it isn't, we just let it pass... */
        }
        store[key] = value;
    }

  Logger::getLogger()->reconfigure();
  Logger::getLogger()->info("%d lines from configuration file %s read", lineCount, fname.c_str());

  return true;
}

void Settings::set(std::string item, std::string value){
  store[item] = value;
}


std::string Settings::get(std::string item){
  std::map<std::string, std::string>::iterator itcurr = store.find(item);
  if(itcurr == store.end()){
    return std::string("");
  }
  return itcurr->second;
}

Settings::Settings()
{

}

Settings::~Settings()
{

}

Settings::Settings(Settings & rhs)
{
}

Settings Settings::operator=(Settings & rhs)
{
  //please don't call me
  assert(0);
  return *this;
}

void Settings::printHelp(){
  std::cout << "tpserver-cpp <options>" << std::endl;
  std::cout << " Options:" << std::endl;
  std::cout << "\t-h\t--help\t\tPrint this help then exit" << std::endl;
  std::cout << "\t\t--version\tPrint version then exit" << std::endl;
  std::cout << "\t-H\t--host\t\tThe host address to listen on (default all)" << std::endl;
  std::cout << "\t-P\t--port\t\tThe port to listen on (default 6923)" << std::endl;
  std::cout << "\t-C\t--configure\tConfiguration file to read" << std::endl;
  std::cout << "\t-l\t--logging\tSets the logging level (default 0)" << std::endl;
#ifdef HAVE_LIBMYSQL
    std::cout << "\t\t--mydb\t\tSets the mysql database to use" << std::endl;
    std::cout << "\t\t--mypass\tSets the mysql password to use" << std::endl;
    std::cout << "\t\t--myuser\tSets the mysql user to use" << std::endl;
    std::cout << "\t\t--myhost\tSets the mysql host to use" << std::endl;
    std::cout << "\t\t--mysocket\tSets the mysql socket to use" << std::endl;
    std::cout << "\t\t--myport\tSets the mysql port to use" << std::endl;
#endif
}

void Settings::setDefaultValues(){
  store["NEVER_START"] = "0";
  store["config_file"] = "/etc/tpserver-cpp/tpserver.conf";
}
