/*  Planet
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

#include <map>

#include <tpserver/frame.h>
#include <tpserver/position3dobjectparam.h>
#include <tpserver/sizeobjectparam.h>
#include <tpserver/refsys.h>
#include <tpserver/referenceobjectparam.h>
#include <tpserver/position3dobjectparam.h>
#include <tpserver/sizeobjectparam.h>
#include <tpserver/orderqueueobjectparam.h>
#include <tpserver/integerobjectparam.h>
#include <tpserver/objectparametergroup.h>
#include <tpserver/game.h>
#include <tpserver/ordermanager.h>
#include <tpserver/orderqueue.h>
#include <tpserver/resourcemanager.h>
#include <tpserver/resourcedescription.h>
#include <tpserver/prng.h>

#include "resourcelistparam.h"
#include "containertypes.h"
#include "nop.h"
#include "rfts.h"

#include "planet.h"


namespace RFTS_ {

using std::string;
using std::pair;
using std::map;

Planet::Planet() : StaticObject() {
   nametype = "Planet";
   typedesc = "A planet object";
   setSize(2);

   ObjectParameterGroup* group = new ObjectParameterGroup();
   
   playerref = new ReferenceObjectParam();
   playerref->setName("Owner");
   playerref->setDescription("The owner of this object");
   playerref->setReferenceType(rst_Player);
   group = new ObjectParameterGroup();
   group->setGroupId(2);
   group->setName("Ownership");
   group->setDescription("The ownership of this object");
   group->addParameter(playerref);
   paramgroups.push_back(group);
      
   group = new ObjectParameterGroup();
   group->setGroupId(3);
   group->setName("Resources");
   group->setDescription("The planets stats");
   resources = new ResourceListParam();
   resources->setName("Resource List");
   resources->setDescription("The stats for this planet");
   group->addParameter(resources);
   paramgroups.push_back(group);
   
   group = new ObjectParameterGroup();
   group->setGroupId(4);
   group->setName("Orders");
   group->setDescription("The order queues of the planet");
   orderqueue = new OrderQueueObjectParam();
   orderqueue->setName("Order Queue");
   orderqueue->setDescription("The queue of orders for this planet");
   
   group->addParameter(orderqueue);
   paramgroups.push_back(group); 
   
}

Planet::~Planet() {

}

void Planet::setDefaultOrderTypes() {
   OrderManager *om = Game::getGame()->getOrderManager();
   std::set<uint32_t> allowedlist;
   allowedlist.insert(om->getOrderTypeByName("No Operation"));
   allowedlist.insert(om->getOrderTypeByName("Build Fleet"));
   allowedlist.insert(om->getOrderTypeByName("Produce"));
   orderqueue->setAllowedOrders(allowedlist);
}

void Planet::doOnceATurn(IGObject* obj) {

   // next turn is odd - do RP production
   if(Game::getGame()->getTurnNumber() % 2 == 0)
   {
      // calc RP for next turn
      calcRP();
   }
   else // this turn was odd - take care of prod. order
   {
      calcPopuation();
      
   }

}

void Planet::calcRP() {
   resources->setResource("Resource Point",
         (resources->getResource("Population").first * 2) +
         (resources->getResource("Industry").first * resources->getResource("Social Environment").first) 
                                             / 16);
}

void Planet::calcPopuation() {
   uint32_t newPop = resources->getResource("Population").first;
   const pair<uint32_t,uint32_t> &planetary = resources->getResource("Planetary Environment");
   pair<uint32_t,uint32_t> &social = resources->getResource("Social Environment");

            // social + midpoint of difference of percentage of planetary and social
   social.first = static_cast<uint32_t>(social.first + .5 *
                   ((planetary.first / planetary.second) - (social.first / social.second)));

   uint32_t& popMaint = resources->getResource("Population Maintenance").first;

   if(newPop != 0)
      newPop *= (popMaint / newPop);

   popMaint = 0; // use up maint points
   
      // social < 40 => pop goes down, else goes up
   newPop *= static_cast<uint32_t>((social.first / 40.) / 10.);

   resources->setResource("Population", newPop);
}

int Planet::getContainerType() {
   return ContainerTypes_::Planet;
}

ObjectData* Planet::clone() const {
   return new Planet();
}

void Planet::packExtraData(Frame * frame){
  ObjectData::packExtraData(frame);
  
  frame->packInt((playerref->getReferencedId() == 0) ? 0xffffffff : playerref->getReferencedId());
  
  map<uint32_t, pair<uint32_t, uint32_t> > reslist = resources->getResources();
    frame->packInt(reslist.size());
    for(map<uint32_t, pair<uint32_t, uint32_t> >::iterator itcurr = reslist.begin();
            itcurr != reslist.end(); ++itcurr){
        frame->packInt(itcurr->first);
        frame->packInt(itcurr->second.first);
        frame->packInt(itcurr->second.second);
        frame->packInt(0);
    }
}

uint32_t Planet::getOwner() const{
  return playerref->getReferencedId();
}

void Planet::setOwner(uint32_t no){
  playerref->setReferencedId(no);
  Game::getGame()->getOrderManager()->getOrderQueue(orderqueue->getQueueId())->addOwner(no);
  touchModTime();
}

void Planet::setDefaultResources() {

   Random* rand = Game::getGame()->getRandom();
   const ProductionInfo& po = Rfts::getProductionInfo();
   
   setResource("Resource Point", rand->getInRange(static_cast<uint32_t>(0), static_cast<uint32_t>(60)) );
   
   setResource("Industry",
               rand->getInRange(static_cast<uint32_t>(5), po.getMinResources("Industry")*3/4),
               po.getRandResourceVal("Industry"));
                        
   setResource("Population",
               rand->getInRange(static_cast<uint32_t>(10), po.getMinResources("Population")*3/4),
               po.getRandResourceVal("Population"));
                        
   setResource("Social Environment",
               rand->getInRange(static_cast<uint32_t>(15), po.getMinResources("Social Environment")*3/4),
               po.getRandResourceVal("Social Environment"));
                        
   setResource("Planetary Environment",
               rand->getInRange(static_cast<uint32_t>(5),po.getMinResources("Planetary Environment")*3/4),
               po.getRandResourceVal("Planetary Environment"));
}

const uint32_t Planet::getCurrentRP() const {
   return resources->getResource("Resource Point").first;
}

const pair<uint32_t, uint32_t> Planet::getResource(uint32_t resTypeId) const {
   return resources->getResource(resTypeId);
}

const pair<uint32_t, uint32_t> Planet::getResource(const string& resTypeName) const {
   return resources->getResource(resTypeName);
}

void Planet::setResource(uint32_t resTypeId, uint32_t currVal, uint32_t maxVal) {
   resources->setResource(resTypeId, currVal, maxVal);
}

void Planet::setResource(const string& resType, uint32_t currVal, uint32_t maxVal) {
   resources->setResource(resType, currVal, maxVal);
}

const map<uint32_t, pair<uint32_t, uint32_t> > Planet::getResources() const{
    return resources->getResources();
}

void Planet::addResource(uint32_t resTypeId, uint32_t amount){
   resources->getResource(resTypeId).first += amount;
   touchModTime();
}

void Planet::addResource(const string& resType, uint32_t amount){
   resources->getResource(resType).first += amount;
   touchModTime();
}

bool Planet::removeResource(uint32_t resTypeId, uint32_t amount){
   std::map<uint32_t, std::pair<uint32_t, uint32_t> > reslist = resources->getResources();
   
      if(reslist.find(resTypeId) != reslist.end()){
         touchModTime();
         if(reslist[resTypeId].first >= amount){
            std::pair<uint32_t, uint32_t> respair = reslist[resTypeId];
            respair.first -= amount;
            reslist[resTypeId] = respair;
            resources->setResources(reslist);
         } else {
            reslist[resTypeId].first = 0;
            resources->setResources(reslist);
         }
         return true;
      }
      return false;
}

bool Planet::removeResource(const string& resTypeName, uint32_t amount){

   return removeResource(Game::getGame()->getResourceManager()->
                        getResourceDescription(resTypeName)->getResourceType(),
                        amount);
}

}
