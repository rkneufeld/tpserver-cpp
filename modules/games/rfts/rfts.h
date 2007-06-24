#ifndef RFTS_H
#define RFTS_H
/*  RFTS rulesset class
 *
 *  Copyright (C) 2007  Tyler Shaub and the Thousand Parsec Project
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

#include <tpserver/ruleset.h>

namespace RFTS_ {

class Rfts : public Ruleset {
 public:
   Rfts();
   virtual ~Rfts();

   virtual std::string getName();
   virtual std::string getVersion();
      
   virtual void initGame();
   virtual void createGame();
   virtual void startGame();

   virtual bool onAddPlayer(Player *player);
   virtual void onPlayerAdded(Player *player);
 private:
   
   void setObjectTypes();
   void setOrderTypes();

   void createDesignCategories();
   void createResources();

   static const unsigned MAX_PLAYERS = 4;
};

}

#endif
