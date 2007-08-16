/*  colonise
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

#include <tpserver/objectorderparameter.h>
#include <tpserver/message.h>
#include <tpserver/game.h>
#include <tpserver/object.h>
#include <tpserver/designstore.h>
#include <tpserver/objectmanager.h>
#include <tpserver/player.h>
#include <tpserver/playermanager.h>

#include "fleet.h"
#include "planet.h"

#include "colonise.h"

namespace RFTS_ {

using std::string;
using std::set;

Colonise::Colonise() {
   name = "Colonise";
   description = "Colonise a planet";

   planet = new ObjectOrderParameter();
   planet->setName("Planet");
   planet->setDescription("The planet to colonise");
   addOrderParameter(planet);

   turns = 1;
}

Colonise::~Colonise() {

}

Order* Colonise::clone() const {
   Colonise *c = new Colonise();
   c->type = this->type;
   return c;
}

bool Colonise::doOrder(IGObject *obj) {
   
   Game *game = Game::getGame();
   ObjectManager *om = game->getObjectManager();
   DesignStore *ds = game->getDesignStore();
   
   Fleet *fleetData = dynamic_cast<Fleet*>(obj->getObjectData());

   IGObject *planetObj = om->getObject(planet->getObjectId());
   Planet *planetData = dynamic_cast<Planet*>(planetObj->getObjectData());

   Player *player = game->getPlayerManager()->getPlayer(fleetData->getOwner());
   
   Message *msg = new Message();
   
   if(planetData == NULL)
   {
      msg->setSubject("Colonise failed!");
      msg->setBody("Tried to colonise a non-planet!");
      msg->addReference(rst_Action_Order, rsorav_Canceled);
   }
   else
   {
      // FIXME
      uint32_t transId = 3; // ds->getDesignByName("Transport")->getDesignId();
      fleetData->removeShips(transId, fleetData->numShips(transId));

      if(fleetData->totalShips() == 0)
         om->scheduleRemoveObject(obj->getID());

      // TODO - check population of planet against number of colonists
      planetData->setOwner(player->getID());

      msg->setSubject("Colonise order complete");
      msg->setBody(string("Transports from ") + obj->getName() + " colonised "  + planetObj->getName());
      msg->addReference(rst_Action_Order, rsorav_Completion);

   }

   om->doneWithObject(planetObj->getID());

   msg->addReference(rst_Object, planetObj->getID());
   msg->addReference(rst_Object, obj->getID());
   
   player->postToBoard(msg);

   return true;
}

}