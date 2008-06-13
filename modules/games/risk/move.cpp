/*  move class
 *
 *  Copyright (C) 2008  Ryan Neufeld and the Thousand Parsec Project
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

#include <tpserver/frame.h>
#include <tpserver/objectorderparameter.h>
#include <tpserver/game.h>
#include <tpserver/object.h>
#include <tpserver/objectmanager.h>
#include <tpserver/objecttypemanager.h>
#include <tpserver/player.h>
#include <tpserver/playermanager.h>
#include <tpserver/playerview.h>
#include <tpserver/message.h>
#include <tpserver/ordermanager.h>
#include <tpserver/orderqueue.h>
#include <tpserver/logging.h>
#include <tpserver/listparameter.h>
#include <tpserver/orderqueueobjectparam.h>
#include <tpserver/orderqueue.h>
#include <tpserver/prng.h>
#include <tpserver/settings.h>

#include <string>
#include <boost/format.hpp>
#include "move.h"
#include "planet.h"

//TODO: Make Move order display correctly in client
namespace RiskRuleset {

using std::map;
using std::pair;
using std::string;
using std::set;
using boost::format;

Move::Move() : Order() {
   name = "Move";
   description = "Move any number of units to an adjacent planet";

   targetPlanet = new ListParameter();
   targetPlanet->setName("Planet");
   targetPlanet->setDescription("The Planet to move to.");
   targetPlanet->setListOptionsCallback(ListOptionCallback(this,
      &Move::generateListOptions));
   addOrderParameter(targetPlanet);

   turns = 1;
}

Move::~Move() {

}

//This function is commented to aid new ruleset developers in understanding
//how to construct a generateListOptions function
//CHECK: On move to empty planet with 2 units:
   //planet empty planet recieves 2 units, updates reinforcements properly
   //attemping to move from newly colonized planet does not allow user to pick any value
map<uint32_t, pair<string, uint32_t> >Move::generateListOptions() {
   //This map will be filled with options to be displayed
   //The pair is made up of a title and an integer max
   map<uint32_t, pair<string,uint32_t> > options;
   
   Game* game = Game::getGame();
   
   //Get the current object in question. This is done so we have access
   //to the object in which the list is being generated for, letting us
   //populate the list in a context specific matter 
   IGObject* selectedObj = game->getObjectManager()->getObject(
      game->getOrderManager()->getOrderQueue(orderqueueid)->getObjectId());
   //In the Risk ruleset the Move order is only used on Planets, and here
   //I capture a pointer to the Planet my IGObject is.
   Planet* planet = dynamic_cast<Planet*>(selectedObj->getObjectBehaviour());
   
   //Here we ensure that the dynamic_cast went ok
   assert(planet);
   
   //And now we tell the object manager we are done with our selectedObj'ect
   game->getObjectManager()->doneWithObject(selectedObj->getID());

   /* I'm now grabbing the set of planets adjacent to the current planet.
   This operation is fairly specific to risk, as risk restricts movement
   to adjacent territories, or in my case, planets. */
   set<Planet*> adjacent = planet->getAdjacent();

   /* I now grab the number of "Army" resources on the planet, as I intend to
   use it as a maximum for each pair I add to the map.
      I add the current and max together to get an absolute maximum that the planet could have */
   uint32_t availibleUnits = planet->getResource("Army").first 
      + planet->getResource("Army").second - 1;

   /* This for loop will iterate over every adjacent planet. 
   This is where the majority of the work occurs, and we populate our list.
   You see here we select an item of the map, in my case (*i)->getID(), and
   for that item we create a pair.
      If its a little hard to read, here is what I am doing:
            options[#] = pair<string,uint32_t>( "title", max# );

   For my pair I set the title as the adjacent planet to move to, and set the
   max to availible units. */
   for(set<Planet*>::iterator i = adjacent.begin(); i != adjacent.end(); i++) {
      options[(*i)->getID()] = pair<string,uint32_t>(
         (*i)->getName(), availibleUnits );
   }   
   //CHECK: how to get more than a single digit display

   return options;
}

Order* Move::clone() const {
   Move* o = new Move();
   o->type = type;
   return o;
}

//TODO: Get move messaging working.
bool Move::doOrder(IGObject* obj) {
   Logger::getLogger()->debug("Starting a Move::dorOrder");
   --turns;
   
   Game* game = Game::getGame();
   ObjectManager* om = game->getObjectManager();
   PlayerManager* pm = game->getPlayerManager();
   Planet* origin = dynamic_cast<Planet*>(obj->getObjectBehaviour());
   assert(origin);
   
   //Get the list of planetIDs and the # of units to move
   map<uint32_t,uint32_t> list = targetPlanet->getList();
   
   //Origin message setup
   Message* originMessage; //message for origin planet owner.
   assert(originMessage);
   //TODO: Restore original message after segfault is fixed
   string originSubject = "test subject";/* "Move order(s) via " + origin->getName() + " completed";*/
   string originBody = "";
   //TODO: Fix seg fault on setSubject()
   assert(originSubject != "");
   originMessage->setSubject(originSubject);
   
   
   //Target Messaging setup
   map<uint32_t,string> targetMessages;   //owner=>subject, body is always the same
   string targetSubject = "You have been attacked!";
   
   //Iterate over all planetIDs and # of units to move to them
   for(map<uint32_t,uint32_t>::iterator i = list.begin(); i != list.end(); ++i) {
      uint32_t planetID = i->first;
      uint32_t numUnits = i->second;
      
      //Restrain the number of units moved off of origin
      uint32_t maxUnits = origin->getResource("Army").first;
      if ( numUnits >= maxUnits && maxUnits > 0) {
         if ( maxUnits > 0)
            numUnits = maxUnits - 1;
         else
            numUnits = 0;
      }
      
      Planet* target = dynamic_cast<Planet*>(
         om->getObject(planetID)->getObjectBehaviour());
      assert(target);

      //Friendly Move  - origin and target owner are the same    
      if (origin->getOwner() == target->getOwner()) {
         Logger::getLogger()->debug("The move is a Friendly move.");

         //Move the units
         origin->removeResource("Army",numUnits);
         target->addResource("Army",numUnits);
         
         //Produce the message
         format message("You have moved %1% units from %2% to %3%.\n");
         message % numUnits; message % origin->getName(); message % target->getName();
         originBody += message.str();
      }
      //Attack Move - origin and target owners are not the same, target is owned
      else if (target->getOwner() != 0){ 
         Logger::getLogger()->debug("The move is an Attack move.");
         //TODO: Implement more attack mechanics
                  
         pair<uint32_t,uint32_t> rollResult;
         uint32_t damage = atoi(Settings::getSettings()->get("risk_attack_dmg").c_str() );
         bool targetIsAttackingOrigin = 
            isTargetAttackingOrigin(obj, om->getObject(i->first));
         //Establish default odds
         //origin is restricted to only use as many units as it has availible, and cap to 3*damage
         uint32_t originOdds = (origin->getResource("Army").first - 1) / 3*damage;
         if (originOdds > 3) { originOdds = 3;}
         //target is restricted to only as many units as there are avail and capped to 2*damage
         uint32_t targetOdds = target->getResource("Army").first / 2*damage;
         if (targetOdds > 2) { targetOdds = 2;}
         //Do we let the roll continue if the owner has 0 odds?

         if ( targetIsAttackingOrigin) {
            Logger::getLogger()->debug("The target planet is also attacking the origin");
            targetOdds += 1;        //Increase defenders odds
            //TODO: do we remove order on target planet to attack current planet, or just change odds?
         }
         else
         {
             Logger::getLogger()->debug("The target planet is not attacking the origin");
         }  
         rollResult = attackRoll(originOdds,targetOdds);
               
         Logger::getLogger()->debug("In the attack the attacker will take %d damage and the defender will take %d. Damage per roll is %d",
            rollResult.first*damage, rollResult.second*damage,damage);
            
         //Apply the damages of the attack
         origin->removeResource("Army",rollResult.first*damage);
         target->removeResource("Army",rollResult.second*damage);
         numUnits -= rollResult.first*damage;
         
         string attacker = pm->getPlayer(origin->getOwner())->getName();
         //Check for change of ownership
         if (target->getResource("Army").first <= 0) {
            target->setOwner(origin->getOwner());
            target->setResource("Army",numUnits, origin->getResource("Army").second);
            //TODO:Clear order queue for target planet
            
            //Produce target 'loss of ownership' message
            format message("You have lost %1% to %2%");
            message % target->getName(); message % attacker;
            targetMessages[target->getOwner()] += message.str();
         }
         else
         {
            //Produce target 'you've been attacked' message
            format message(
               "%1% has been attacked by %2% via %3%. You lost %4% units, your opponent lost %5%.\n");
            message % target->getName(); message % attacker;
            message % origin->getName(); message % (rollResult.second*damage);
            message % (rollResult.first*damage);
            targetMessages[target->getOwner()] += message.str();
         }
         //Produce origin message
         format message(
            "Your attack on %1% destroyed %2% of your opponents units. You lost %3% units in retaliation.");
         message % target->getName(); message % (rollResult.second*damage);
         message % (rollResult.first*damage);
         originBody += message.str();
      }
      //Colonize Move - origin and target owners are not the same, target is unowned
      else {
         Logger::getLogger()->debug("The move is a Colonize move.");
         
         origin->removeResource("Army",numUnits);
         target->setOwner(origin->getOwner());
         target->addResource("Army",numUnits);
         
         //Produce message for origin
         format message("You have colonized %1% with %2% units.\n");
         message % target->getName(); message % numUnits;
         originBody += message.str();
      }
   }
   
   Logger::getLogger()->debug("Origin message to be sent. Body is:\n%s\n Subject is: %s",originSubject.c_str(),originBody.c_str());

   Logger::getLogger()->debug("set subject of message");
   originMessage->setBody(originBody);        //don't try setting a body with an empty string
   pm->getPlayer(origin->getOwner())->postToBoard(originMessage);
   
   //Send message to target player(s)
   for(map<uint32_t,string>::iterator i = targetMessages.begin(); i != targetMessages.end(); ++i) {
      Message* targetMessage;
      assert(targetMessage);
      targetMessage->setSubject(targetSubject);
      targetMessage->setBody((*i).second);
      pm->getPlayer((*i).first)->postToBoard(targetMessage);
   }
   
   return true;   //moves are always completed, no matter what happens
}

//TODO: encapsulate this function so it is a lot more pretty
//CHECK: check if the function even works
bool Move::isTargetAttackingOrigin(IGObject* trueOrigin, IGObject* target) {
   Logger::getLogger()->debug("Checking if a target planet is attacking");
   
   bool result = false;
   //Get order queue from object
   OrderQueueObjectParam* oqop = dynamic_cast<OrderQueueObjectParam*>(target->getParameterByType(obpT_Order_Queue));
   OrderQueue* oq;
   OrderManager* ordM = Game::getGame()->getOrderManager();
   
   if(oqop != NULL && (oq = ordM->getOrderQueue(oqop->getQueueId())) != NULL)
   {
      //Iterate over all orders
      for (uint32_t j = 0; j < oq->getNumberOrders(); j++)
      {
         //Taken from RFTS
         OwnedObject *orderedObj = dynamic_cast<OwnedObject*>(target->getObjectBehaviour());
         assert(orderedObj);

         Order* order = oq->getOrder(j, orderedObj->getOwner());
         //if order is of type asked for then process it
         if (order->getName() == "Move")
         {
            Move* move = dynamic_cast<Move*>(order);
            assert(move);
            
            Planet* origin = dynamic_cast<Planet*>(target->getObjectBehaviour());
            assert(origin);

            //Get the list of planetIDs and the # of units to move
            map<uint32_t,uint32_t> list = targetPlanet->getList();
            
            for(map<uint32_t,uint32_t>::iterator i = list.begin(); i != list.end(); ++i) {
               uint32_t planetID = i->first;
               
               target = Game::getGame()->getObjectManager()->getObject(planetID);
               if ( target == trueOrigin ) {
                  //NOTE: Here is where any logic goes for dealing with two planets attacking eachother
                  //For now we just notify the function caller the target is attacking the trueOrigin
                  result = true;
                  //force the exit of the funciton
                  j = oq->getNumberOrders() + 1;
                  i = list.end();
               }
            } //End for each order part in the order
         }  //end if order name is "Move"
      } //end for all orders
   }//end if oqop is not null or oq is not null
   return result;
}

//The left member of the pair represents how many units are lost by the attacker
//The right member of the pair represents how many units are lost by the defender
pair<uint32_t,uint32_t> Move::attackRoll(uint32_t oddsAttacker, uint32_t oddsDefender) {
   Logger::getLogger()->debug("Starting Move::attackRoll. Odds are %d:%d",oddsAttacker,oddsDefender);
   Random* random = Game::getGame()->getRandom();  //ASK: seed this random?
   pair<uint32_t,uint32_t> result;           //pair representing the result
   result.first = 0;
   result.second = 0;
   
   std::list<uint32_t> attackRolls, defendRolls;  //lists for both players rolls

   uint32_t roll,attack,defend; //used for debugging
   
   //Get attacker's rolls
   for( uint32_t a = 0; a < oddsAttacker; a++) {
      roll = random->getInRange(1,6);
      attackRolls.push_front(roll);
      Logger::getLogger()->debug("Attacker rolls a %d",roll);
   }
   attackRolls.sort();
   attackRolls.reverse();
   
   //Get the defenders rolls
   for( uint32_t d = 0; d < oddsDefender; d++) {
      roll = random->getInRange(1,6);
      defendRolls.push_front(roll);
      Logger::getLogger()->debug("Defender rolls a %d",roll);
   }
   defendRolls.sort();
   defendRolls.reverse();
   
   //Evaluate the rolls and assign damage
   while ( !attackRolls.empty() && !defendRolls.empty()) {
      attack = attackRolls.front();
      defend = defendRolls.front();
      if ( attack > defend  ) { //Attacker wins on the die
         result.second++; //defender will take 1 damage
         Logger::getLogger()->debug("Attacker(%d) beats defender(%d)",attack,defend);
      }
      else {                                             //Defender wins on the die
         result.first++;   //attacker will take 1 damage
         Logger::getLogger()->debug("Defender(%d) beats attacker(%d)",defend,attack);
      }
      
      attackRolls.pop_front();   //remove the dice being looked at
      defendRolls.pop_front();
   }
   
   Logger::getLogger()->debug("In total defender lost %d rolls, attacker lost %d",result.second,result.first);
   
   return result;          //send back the results
}

void clearOrders(IGObject* obj) {
   
}
} //end namespace RiskRuleset
