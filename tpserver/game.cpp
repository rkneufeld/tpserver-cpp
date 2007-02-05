/*  Game controller for tpserver-cpp
 *
 *  Copyright (C) 2003-2006, 2007  Lee Begg and the Thousand Parsec Project
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

#include <iostream>
#include <stdlib.h>
#include <ctime>
#include <cassert>

#include "logging.h"
#include "player.h"
#include "object.h"
#include "order.h"
#include "universe.h"
#include "frame.h"
#include "net.h"
#include "vector3d.h"
#include "objectmanager.h"
#include "ordermanager.h"
#include "objectdatamanager.h"
#include "boardmanager.h"
#include "resourcemanager.h"
#include "playermanager.h"
#include "ownedobject.h"
#include "combatstrategy.h"
#include "designstore.h"
#include "ruleset.h"
#include "persistence.h"
#include "tpscheme.h"
#include "settings.h"
#include "timercallback.h"
#include "prng.h"

#include "game.h"

Game *Game::myInstance = NULL;

Game *Game::getGame()
{
	if (myInstance == NULL) {
		myInstance = new Game();
	}
	return myInstance;
}

bool Game::setRuleset(Ruleset* rs){
  if(!loaded){
    if(ruleset != NULL)
      delete ruleset;
    ruleset = rs;
    return true;
  }else{
    return false;
  }
}

Ruleset* Game::getRuleset() const{
  return ruleset;
}

bool Game::load()
{
  if(!loaded && ruleset != NULL && tpscheme != NULL){
    Logger::getLogger()->info("Loading Game");  

    if(persistence->init()){
      Logger::getLogger()->debug("Persistence initialised");
    }else{
      Logger::getLogger()->error("Problem initialising Persistence, game not loaded.");
      return false;
    }

    ruleset->initGame();

    //if nothing loaded from database
    //init game
        IGObject* universe = objectmanager->getObject(0);
        if(universe != NULL){
            objectmanager->doneWithObject(0);
            objectmanager->init();
            ordermanager->init();
            boardmanager->init();
            resourcemanager->init();
            playermanager->init();
            designstore->init();
        }else{
            Logger::getLogger()->info("Creating Game");
            ruleset->createGame();
        }
    
    loaded = true;
    return true;
  }else{
    if(loaded){
      Logger::getLogger()->warning("Game already loaded");
    }
    if(ruleset == NULL){
      Logger::getLogger()->warning("Game not loaded, no ruleset set");
    }
    if(tpscheme == NULL){
      Logger::getLogger()->warning("Game not loaded, no TpScheme implementation set");
    }
    return false;
  }

}

bool Game::start(){
  if(loaded && !started){
    Logger::getLogger()->info("Starting Game");

    ruleset->startGame();
    
    uint32_t tl = atoi(Settings::getSettings()->get("turn_length").c_str());
    if(tl != 0){
      setTurnLength(tl);
    }

    resetEOTTimer();

    started = true;
    return true;
  }else{
    Logger::getLogger()->warning("Game not starting, not loaded or already started");
    return false;
  }
}


void Game::save()
{
	Logger::getLogger()->info("Game saved");
}

ObjectManager* Game::getObjectManager() const{
    return objectmanager;
}

OrderManager* Game::getOrderManager() const{
  return ordermanager;
}

ObjectDataManager* Game::getObjectDataManager() const{
  return objectdatamanager;
}

BoardManager* Game::getBoardManager() const{
    return boardmanager;
}

ResourceManager* Game::getResourceManager() const{
    return resourcemanager;
}

PlayerManager* Game::getPlayerManager() const{
    return playermanager;
}

CombatStrategy* Game::getCombatStrategy() const{
  return combatstrategy;
}

void Game::setCombatStrategy(CombatStrategy* cs){
  if(combatstrategy != NULL)
    delete combatstrategy;
  combatstrategy = cs;
}

DesignStore* Game::getDesignStore() const{
  return designstore;
}

Persistence* Game::getPersistence() const{
    return persistence;
}

bool Game::setPersistence(Persistence* p){
  if(!loaded){
    delete persistence;
    persistence = p;
    return true;
  }else{
    Logger::getLogger()->warning("Could not set new Persistence method.");
    return false;
  }
}

TpScheme* Game::getTpScheme() const{
  return tpscheme;
}

/*!Sets the implementation of TpScheme to use.

  If there is an implementation set already, it is not replaced and
  false is returned.

  \param imp The TpScheme implementation to use
  \returns True if set, false if not set
    
 */
bool Game::setTpScheme(TpScheme* imp){
  if(tpscheme != NULL)
    return false;
  tpscheme = imp;
  return true;
}

Random* Game::getRandom() const{
  return random;
}

bool Game::isLoaded() const{
  return loaded;
}

bool Game::isStarted() const{
  return (loaded && started);
}

void Game::doEndOfTurn()
{
    if(loaded && started){
        Logger::getLogger()->info("End Of Turn started");

        // send frame to all connections that the end of turn has started
        Frame * frame = new Frame(fv0_2);
        frame->setType(ft02_Time_Remaining);
        frame->packInt(0);
        Network::getNetwork()->sendToAll(frame);

	// DO END OF TURN STUFF HERE
	std::set<uint32_t>::iterator itcurr;

        //do orders
        std::set<uint32_t> objects = ordermanager->getObjectsWithOrders();
	for(itcurr = objects.begin(); itcurr != objects.end(); ++itcurr) {
            IGObject * ob = objectmanager->getObject(*itcurr);
            Order * currOrder = ordermanager->getFirstOrder(ob);
            if(currOrder != NULL){
                if(currOrder->doOrder(ob)){
                ordermanager->removeFirstOrder(ob);
                }else{
                    ordermanager->updateFirstOrder(ob);
                }
            }
            objectmanager->doneWithObject(ob->getID());
	}

	objectmanager->clearRemovedObjects();

	// update positions and velocities
        objects = objectmanager->getAllIds();
	for(itcurr = objects.begin(); itcurr != objects.end(); ++itcurr) {
            IGObject * ob = objectmanager->getObject(*itcurr);
            ob->updatePosition();
            objectmanager->doneWithObject(ob->getID());
        }

	for(itcurr = objects.begin(); itcurr != objects.end(); ++itcurr) {
            IGObject * ob = objectmanager->getObject(*itcurr);
            //ob->updatePosition();
            std::set<unsigned int> cont = ob->getContainedObjects();
            for(std::set<uint32_t>::iterator ita = cont.begin(); ita != cont.end(); ++ita){
                IGObject* itaobj = objectmanager->getObject(*ita);
                if(itaobj->getType() == obT_Fleet || (itaobj->getType() == obT_Planet && ((OwnedObject*)(itaobj->getObjectData()))->getOwner() != 0)){
                    for(std::set<unsigned int>::iterator itb = ita; itb != cont.end(); ++itb){
                        IGObject* itbobj = objectmanager->getObject(*itb);
                        if((*ita != *itb) && (itbobj->getType() == obT_Fleet || (itbobj->getType() == obT_Planet && ((OwnedObject*)(itbobj->getObjectData()))->getOwner() != 0))){
                            if(((OwnedObject*)(itaobj->getObjectData()))->getOwner() != ((OwnedObject*)(itbobj->getObjectData()))->getOwner()){
                                uint64_t diff = itaobj->getPosition().getDistance(itbobj->getPosition());
                                if(diff <= itaobj->getSize() / 2 + itbobj->getSize() / 2){
                                    combatstrategy->setCombatants(itaobj, itbobj);
                                    combatstrategy->doCombat();
                                    if(!combatstrategy->isAliveCombatant1()){
                                        if(itaobj->getType() == obT_Planet){
                                            ((OwnedObject*)(itaobj->getObjectData()))->setOwner(0);
                                        }else{
                                            objectmanager->scheduleRemoveObject(*ita);
                                        }
                                    }
                                    if(!combatstrategy->isAliveCombatant2()){
                                        if(itbobj->getType() == obT_Planet){
                                            ((OwnedObject*)(itbobj->getObjectData()))->setOwner(0);
                                        }else{
                                            objectmanager->scheduleRemoveObject(*itb);
                                        }
                                    }
                                }
                            }
                        }
                        objectmanager->doneWithObject(itbobj->getID());
                    }
                    
                    //combat between object and container (ie planet)
                    if(ob->getType() == obT_Fleet || (ob->getType() == obT_Planet && ((OwnedObject*)(ob->getObjectData()))->getOwner() != 0)){
                        if(((OwnedObject*)(itaobj->getObjectData()))->getOwner() != ((OwnedObject*)(ob->getObjectData()))->getOwner()){
                            combatstrategy->setCombatants(itaobj, ob);
                            combatstrategy->doCombat();
                            if(!combatstrategy->isAliveCombatant1()){
                                if(itaobj->getType() == obT_Planet){
                                    ((OwnedObject*)(itaobj->getObjectData()))->setOwner(0);
                                }else{
                                    objectmanager->scheduleRemoveObject(*ita);
                                }
                            }
                            if(!combatstrategy->isAliveCombatant2()){
                                if(ob->getType() == obT_Planet){
                                    ((OwnedObject*)(ob->getObjectData()))->setOwner(0);
                                }else{
                                    objectmanager->scheduleRemoveObject(*itcurr);
                                }
                            }
                        }
                    }
                }
                objectmanager->doneWithObject(itaobj->getID());
            }
            objectmanager->doneWithObject(ob->getID());
        }

        objectmanager->clearRemovedObjects();
	
	// to once a turn (right at the end)
        objects = objectmanager->getAllIds();
	for(itcurr = objects.begin(); itcurr != objects.end(); ++itcurr) {
	  IGObject * ob = objectmanager->getObject(*itcurr);
	  ob->getObjectData()->doOnceATurn(ob);
	  objectmanager->doneWithObject(ob->getID());
	}

	// find the objects that are visible to each player
	std::set<uint32_t> vis = objectmanager->getAllIds();
        std::set<uint32_t> players = playermanager->getAllIds();
	for(std::set<uint32_t>::iterator itplayer = players.begin(); itplayer != players.end(); ++itplayer){
            playermanager->getPlayer(*itplayer)->setVisibleObjects(vis);
	}
        playermanager->updateAll();

	// increment the time to the next turn
	turnTime += turnIncrement;
        delete timer;
        timer = NULL;
        if(secondsToEOT() <= 0)
            resetEOTTimer();
        else
          setEOTTimer();

	// send frame to all connections that the end of turn has finished
	frame = new Frame(fv0_2);
	frame->setType(ft02_Time_Remaining);
	frame->packInt(secondsToEOT());
	Network::getNetwork()->sendToAll(frame);
        Network::getNetwork()->doneEOT();

	Logger::getLogger()->info("End Of Turn finished");
    }else{
        Logger::getLogger()->info("End Of Turn not run because game not started");
        turnTime += turnIncrement;
        setEOTTimer();
    }
}

void Game::resetEOTTimer(){
  turnTime = time(NULL) + turnIncrement;
  setEOTTimer();
}

int Game::secondsToEOT(){
  return turnTime - time(NULL);
}

int Game::getTurnNumber(){
    int turnnum = ((Universe*)(objectmanager->getObject(0)->getObjectData()))->getYear();
    objectmanager->doneWithObject(0);
    return turnnum;
}

void Game::setTurnLength(unsigned int sec){
    if(sec == 0){
        Logger::getLogger()->warning("Tried to set turn length to zero seconds, setting to 1 minute instead.");
        sec = 60;
    }
  turnIncrement = sec;
}

void Game::saveAndClose()
{
	save();
	//remove and delete players

	//remove and delete objects
        
        //shutdown the persistence method.
        if(loaded){
          persistence->shutdown();
        }
        
	Logger::getLogger()->info("Game saved & closed");
}

Game::Game()
{
    objectmanager = new ObjectManager();
  ordermanager = new OrderManager();
  objectdatamanager = new ObjectDataManager();
    boardmanager = new BoardManager();
    resourcemanager = new ResourceManager();
    playermanager = new PlayerManager();
  designstore = new DesignStore();
  combatstrategy = NULL;
  ruleset = NULL;
  persistence = new Persistence();
  random = new Random();
  tpscheme = NULL;
  loaded = false;
  started = false;

  turnIncrement = 86400; //24 hours
  timer = NULL;
  resetEOTTimer();
  //this is a good place to seed the PNRG
  srand((getpid() + time(NULL)) % RAND_MAX);
  random->seed(getpid() + time(NULL));
}

Game::Game(Game & rhs)
{

}

Game::~Game()
{
    delete objectmanager;
  delete ordermanager;
  delete objectdatamanager;
    delete boardmanager;
    delete resourcemanager;
    delete playermanager;
  if(combatstrategy != NULL)
    delete combatstrategy;
  if(ruleset != NULL)
    delete ruleset;
  delete designstore;
    if(persistence != NULL)
        delete persistence;
  if(timer != NULL){
    timer->setValid(false);
    delete timer;
  }
  delete random;
}

Game Game::operator=(Game & rhs)
{
  //only here to stop people doing funny things...
  assert(0);
  return *this;
}

void Game::setEOTTimer(){
  if(timer != NULL){
    timer->setValid(false);
    delete timer;
  }
  timer = new TimerCallback(this, &Game::doEndOfTurn, turnTime - time(NULL));
  Network::getNetwork()->addTimer(*timer);
}
