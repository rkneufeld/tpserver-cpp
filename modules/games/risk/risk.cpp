/*  Risk rulesset class
 *
 *  Copyright (C) 2008  Ryan Neufeld and the Thousand Parsec Project
 *  Copyright (C) 2005, 2007  Lee Begg and the Thousand Parsec Project
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
 
// System includes 
#include <sstream> 

// tpserver includes 
#include <tpserver/player.h>
#include <tpserver/playermanager.h>
#include <tpserver/playerview.h>
#include <tpserver/game.h>
#include <tpserver/logging.h>

#include <tpserver/property.h>
#include <tpserver/component.h>
#include <tpserver/design.h>
#include <tpserver/designview.h>
#include <tpserver/category.h>
#include <tpserver/designstore.h>

#include <tpserver/object.h>
#include <tpserver/objectview.h>
#include <tpserver/objecttypemanager.h>
#include <tpserver/ordermanager.h>
#include <tpserver/orderqueue.h>
#include <tpserver/orderqueueobjectparam.h>
#include <tpserver/objectmanager.h>
#include <tpserver/objectparameter.h>
#include <tpserver/resourcemanager.h>
#include <tpserver/resourcedescription.h>
#include <tpserver/prng.h>
#include <tpserver/settings.h>
#include <tpserver/message.h>

// risk includes 
#include "risk.h"
#include "riskturn.h" 
#include "colonize.h"
#include "move.h"
#include "reinforce.h"
#include "containertypes.h"
#include "map.h"
#include "staticobject.h"
#include "universe.h"
#include "ownedobject.h"
#include "planet.h"
#include "galaxy.h"

//Tyler's "hacky define :p"
#define DEBUG_FN_PRINT() (Logger::getLogger()->debug(__PRETTY_FUNCTION__))

//the libtool magic required
extern "C" { 
   #define tp_init librisk_LTX_tp_init 
   bool tp_init(){ 
      return Game::getGame()->setRuleset(new RiskRuleset::Risk());
   } 
}

namespace RiskRuleset {
  
using std::string;
using std::map;
using std::set;
using std::vector;
using std::advance;
using std::pair;

Risk::Risk(){
   //Minisec has a parent of random(NULL), whats with that?	
}

Risk::~Risk(){
   //minisec simply deletes the random if not NULL    
}

std::string Risk::getName(){
   return "Risk";
}

std::string Risk::getVersion(){
   return "0.1";
}

void Risk::initGame(){
   Logger::getLogger()->info("Risk initialised");

   Game::getGame()->setTurnProcess(new RiskTurn());

   setObjectTypes();

   setOrderTypes();
}

void Risk::setObjectTypes() const{
   ObjectTypeManager* obdm = Game::getGame()->getObjectTypeManager();
   StaticObjectType* eo;    

   obdm->addNewObjectType(new UniverseType());

   //old
   /*  eo = new StaticObjectType();
   eo->setTypeName("Galaxy");
   eo->setTypeDescription("A set of star systems that provides a bonus of reinforcements to any player completely controlling it.");
   obdm->addNewObjectType(eo); */
   //new
   obdm->addNewObjectType(new GalaxyType());

   eo = new StaticObjectType();
   eo->setTypeName("Star System");
   eo->setTypeDescription("A territory capable of being controlled and having any number of armies.");
   obdm->addNewObjectType(eo);

   obdm->addNewObjectType(new PlanetType);   //may need to special some stuff here
   //There are no fleets in risk - hence no fleet type
}

void Risk::setOrderTypes() const{
   OrderManager* orm = Game::getGame()->getOrderManager();

   //To be an action availible on all unowned planets
   //With planet selected order is to colonize with NUMBER armies
   orm->addOrderType(new Colonize());

   //To be an action availible on all player owned planets
   //With planet selected order is to reinforce with NUMBER armies
   orm->addOrderType(new Reinforce());

   //To be an action availible on all player owned planets
   //With planet selected order is to reinforce with NUMBER armies
   orm->addOrderType(new Move());
}

void Risk::createGame(){
   Logger::getLogger()->info("Risk created");

   createResources();
 
   //set up universe (universe->galaxies->star sys->planet)
   createUniverse();
}

void Risk::createResources() {
   ResourceManager* resMan = Game::getGame()->getResourceManager();

   ResourceDescription* res = new ResourceDescription();
   res->setNameSingular("Army");
   res->setNamePlural("Armies");
   res->setUnitSingular("unit");
   res->setUnitPlural("units");
   res->setDescription("Armies");
   res->setMass(0);
   res->setVolume(0);
   resMan->addResourceDescription(res);
}

void Risk::createUniverse() {
    DEBUG_FN_PRINT();
   ObjectManager *objman = Game::getGame()->getObjectManager();
   ObjectTypeManager *otypeman = Game::getGame()->getObjectTypeManager();

   uint32_t uniType = otypeman->getObjectTypeByName("Universe");
   IGObject *universe = objman->createNewObject();

   //TODO: Adjust default position of universe
   //TODO: Perhaps push constellations away from eachother for visibility 
   otypeman->setupObject(universe, uniType);
   universe->setName("The Universe");
   StaticObject* uniData = static_cast<StaticObject*>(universe->getObjectBehaviour());
   uniData->setSize(123456789123ll);
   uniData->setUnitPos(.5,.5);
   objman->addObject(universe);

   //TODO: LATER: create some sort of import function to create map from file 
   //create galaxies and keep reference for system creation
   IGObject *gal_cassiopeia = createGalaxy(*universe, "Cassiopeia", 5); //North America
   IGObject *gal_cygnus = createGalaxy(*universe, "Cygnus", 2); //South America
   IGObject *gal_cepheus = createGalaxy(*universe, "Cepheus", 5); //Europe
   IGObject *gal_orion = createGalaxy(*universe, "Orion",3); //Africa
   IGObject *gal_draco = createGalaxy(*universe, "Draco", 7); //Russia
   IGObject *gal_crux = createGalaxy(*universe, "Crux Australis", 2); //Australia

   Logger::getLogger()->info("Galaxies Created");

   //create systems - (I don't know why I chose to do numbers from -1 to 1!?)
   // Cassiopeia Systems (North America, Bonus 5)
   createStarSystem(*gal_cassiopeia, "Shedir",         -0.321, 0.670);
   createStarSystem(*gal_cassiopeia, "Caph",           -0.213, 0.751);
   createStarSystem(*gal_cassiopeia, "Ruchbah",        -0.447, 0.724);
   createStarSystem(*gal_cassiopeia, "Gamma Cas",      -0.339, 0.760);
   createStarSystem(*gal_cassiopeia, "Segin",          -0.519, 0.807);
   createStarSystem(*gal_cassiopeia, "Zeta Cas",       -0.303, 0.571);
   createStarSystem(*gal_cassiopeia, "Marfak",         -0.420, 0.616);
   createStarSystem(*gal_cassiopeia, "Xi Cas",         -0.357, 0.481);  
   createStarSystem(*gal_cassiopeia, "Sigma Cas",      -0.222, 0.643);

   // Cygnus Systems (South America, Bonus 2)
   createStarSystem(*gal_cygnus, "Deneb",              -0.321, 0.373);
   createStarSystem(*gal_cygnus, "Albireo",            -0.249, 0.049);
   createStarSystem(*gal_cygnus, "Sadr",               -0.294, 0.256);
   createStarSystem(*gal_cygnus, "Gienah Cygni",       -0.402, 0.238);
 
   // Cepheus Systems (Europe, Bonus 5)
   createStarSystem(*gal_cepheus, "Alderamin",         0.045,  0.472);
   createStarSystem(*gal_cepheus, "Alfirk",            0.063,  0.625);
   createStarSystem(*gal_cepheus, "Al Kalb al Rai",    -0.018, 0.724);
   createStarSystem(*gal_cepheus, "Alrai",             -0.081, 0.724);
   createStarSystem(*gal_cepheus, "The Garnet Star",   -0.045, 0.445);
   createStarSystem(*gal_cepheus, "Alkurhah",          -0.036, 0.499);
   createStarSystem(*gal_cepheus, "Iota Cep",          -0.090, 0.598);

   // Orion Systens (Africa, Bonus 3)
   createStarSystem(*gal_orion, "Betelgeuse",          -0.069, 0.328);
   createStarSystem(*gal_orion, "Rigel",               0.126,  0.094);
   createStarSystem(*gal_orion, "Bellatrix",           0.084,  0.337);
   createStarSystem(*gal_orion, "Mintaka",             0.048,  0.220);
   createStarSystem(*gal_orion, "Alnitak",             -0.015, 0.202);
   createStarSystem(*gal_orion, "Saiph",               -0.015, 0.058);

   // Draco Systems (Russia, Bonus 7)
   createStarSystem(*gal_draco, "Etamin",              0.147, 0.382);
   createStarSystem(*gal_draco, "Rastaban",            0.246, 0.382);
   createStarSystem(*gal_draco, "Arrakis",             0.300, 0.402);
   createStarSystem(*gal_draco, "Kuma",                0.246, 0.436);
   createStarSystem(*gal_draco, "Grumium",             0.147, 0.454);
   createStarSystem(*gal_draco, "Nodus Secundus",      0.111, 0.634);
   createStarSystem(*gal_draco, "Tyl",                 0.102, 0.697);
   createStarSystem(*gal_draco, "Dsibin",              0.204, 0.670);
   createStarSystem(*gal_draco, "Aldhibah",            0.273, 0.544);
   createStarSystem(*gal_draco, "Ed Asiach",           0.399, 0.472);
   createStarSystem(*gal_draco, "Thubah",              0.444, 0.634);
   createStarSystem(*gal_draco, "Gianfar",             0.498, 0.778);

   // Crux Systens (Australia, Bonus 2)
   createStarSystem(*gal_crux, "Acrux",                0.506, 0.049);
   createStarSystem(*gal_crux, "Becrux",               0.366, 0.148);
   createStarSystem(*gal_crux, "Gacrux",               0.434, 0.292);
   createStarSystem(*gal_crux, "Delta Cru",            0.590, 0.211); 
}

IGObject* Risk::createGalaxy(IGObject& parent, const string& name, int bonus) {
   DEBUG_FN_PRINT();
   Game *game = Game::getGame();
   ObjectTypeManager *otypeman = game->getObjectTypeManager();

   IGObject *galaxy = game->getObjectManager()->createNewObject();

   otypeman->setupObject(galaxy, otypeman->getObjectTypeByName("Galaxy"));
   galaxy->setName(name);

   Galaxy* galaxyData = static_cast<Galaxy*>(galaxy->getObjectBehaviour());
   galaxyData->setBonus(bonus);

   galaxy->addToParent(parent.getID());
   game->getObjectManager()->addObject(galaxy);

   return galaxy;
}

IGObject* Risk::createStarSystem(IGObject& parent, const string& name, double unitX, double unitY) {
   DEBUG_FN_PRINT();
   Game *game = Game::getGame();
   ObjectTypeManager *otypeman = game->getObjectTypeManager();

   IGObject *starSys = game->getObjectManager()->createNewObject();

   otypeman->setupObject(starSys, otypeman->getObjectTypeByName("Star System"));
   starSys->setName(name);
   StaticObject* starSysData = dynamic_cast<StaticObject*>(starSys->getObjectBehaviour());
   starSysData->setUnitPos(unitX, unitY);

   starSys->addToParent(parent.getID());
   game->getObjectManager()->addObject(starSys);

   string planetName;

   planetName = starSys->getName() + " Prime";
   createPlanet(*starSys, planetName, starSysData->getPosition() + getRandPlanetOffset());
   return starSys;
}
IGObject* Risk::createPlanet(IGObject& parent, const string& name,double unitX, double unitY) {
   DEBUG_FN_PRINT();
   Game *game = Game::getGame();
   ObjectTypeManager *otypeman = game->getObjectTypeManager();

   IGObject *planet = game->getObjectManager()->createNewObject();

   otypeman->setupObject(planet, otypeman->getObjectTypeByName("Planet"));
   planet->setName(name);
   Planet* planetData = static_cast<Planet*>(planet->getObjectBehaviour());
   planetData->setUnitPos(unitX, unitY);
   planetData->setDefaultResources();

   OrderQueue *planetOrders = new OrderQueue();
   planetOrders->setObjectId(planet->getID());
   planetOrders->addOwner(0);
   game->getOrderManager()->addOrderQueue(planetOrders);
   OrderQueueObjectParam* oqop = static_cast<OrderQueueObjectParam*>
                                        (planet->getParameterByType(obpT_Order_Queue));
   oqop->setQueueId(planetOrders->getQueueId());
   planetData->setOrderTypes();

   planet->addToParent(parent.getID());
   game->getObjectManager()->addObject(planet);

   return planet;
}

IGObject* Risk::createPlanet(IGObject& parent, const string& name,const Vector3d& location) {
   DEBUG_FN_PRINT();
   Game *game = Game::getGame();
   ObjectTypeManager *otypeman = game->getObjectTypeManager();

   IGObject *planet = game->getObjectManager()->createNewObject();

   otypeman->setupObject(planet, otypeman->getObjectTypeByName("Planet"));
   planet->setName(name);
   Planet* planetData = static_cast<Planet*>(planet->getObjectBehaviour());
   planetData->setPosition(location); // OK because unit pos isn't useful for planets
   planetData->setDefaultResources();

   OrderQueue *planetOrders = new OrderQueue();
   planetOrders->setObjectId(planet->getID());
   planetOrders->addOwner(0);
   game->getOrderManager()->addOrderQueue(planetOrders);
   OrderQueueObjectParam* oqop = static_cast<OrderQueueObjectParam*>
                                       (planet->getParameterByType(obpT_Order_Queue));
   oqop->setQueueId(planetOrders->getQueueId());
   planetData->setOrderTypes();

   planet->addToParent(parent.getID());
   game->getObjectManager()->addObject(planet);

   return planet;
}

void Risk::startGame(){
   Logger::getLogger()->info("Risk started");

   //Establish some defaults if user does not specify any in config
   Settings* settings = Settings::getSettings();
   if(settings->get("turn_length_over_threshold") == "")
   {
      settings->set("turn_length_over_threshold", "180");
      settings->set("turn_player_threshold", "0");
      settings->set("turn_length_under_threshold", "180");
   }

   if(settings->get("max_players") == "")
      settings->set("max_players", "4");
  
   if(settings->get("game_length") == "")
      settings->set("game_length", "60");
}

bool Risk::onAddPlayer(Player* player){
      Logger::getLogger()->debug("Risk onAddPlayer"); 
      Game* game = Game::getGame();

      bool canJoin = true;            

      /* Commented out for the time being until things start working properly

      uint32_t max_players = atoi(Settings::getSettings()->get("risk_max_players").c_str() );
      bool isStarted = game->isStarted();
      uint32_t cur_players = game->getPlayerManager()->getNumPlayers();

      //If ( max players exceeded OR (game's started AND there are no open spaces))    
         //disallow joining
      if ( (cur_players >= max_players)||(isStarted && !isBoardClaimed()) )
      canJoin = false;  
      else
      canJoin = true;
      */
      return canJoin; 
}

bool Risk::isBoardClaimed() const{
   //This method will run through board and check if all territories
    //are claimed or not

   //TODO: Check board to determine "claimedness"

   return false;
}

void Risk::onPlayerAdded(Player* player){
   Logger::getLogger()->debug("Risk onPlayerAdded");

   Message *welcome = new Message();
   welcome->setSubject("Welcome to Risk! Here's a brief reminder of some rules");
   welcome->setBody("<b><u>3 Turn Order</b></u>:<br />\
                     Part 1: Colonization orders are processed<br />\
                     Part 2: Reinforce orders are processed<br />\
                     Part 3: Non-attack movement orders are processed<br />\
                     Part 4: Attack movement orders are processed<br />\
                     *repeat*<br /><br />");

   player->postToBoard(welcome);

   Game::getGame()->getPlayerManager()->updatePlayer(player->getID()); 

   //This should make every object visible to an added player
   PlayerView* playerview = player->getPlayerView();
   std::set<uint32_t> objids = Game::getGame()->getObjectManager()->getAllIds();
   for(std::set<uint32_t>::iterator itcurr = objids.begin(); itcurr != objids.end();
         ++itcurr){
      ObjectView* obv = new ObjectView();
      obv->setObjectId(*itcurr);
      obv->setCompletelyVisible(true);
    playerview->addVisibleObject(obv);
   }
 
}

} //end namespace RiskRuleset