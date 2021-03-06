/*  PlayerAgent object, processing and frame handling
 *
 *  Copyright (C) 2007, 2008  Lee Begg and the Thousand Parsec Project
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

#include <algorithm>
#include <ctime>

// #include "string.h"

#include "playerconnection.h"
#include "frame.h"
#include "logging.h"
#include "game.h"
#include "object.h"
#include "objectview.h"
#include "orderqueue.h"
#include "order.h"
#include "vector3d.h"
#include "board.h"
#include "message.h"
#include "objectmanager.h"
#include "objecttypemanager.h"
#include "ordermanager.h"
#include "boardmanager.h"
#include "playermanager.h"
#include "designstore.h"
#include "category.h"
#include "design.h"
#include "component.h"
#include "property.h"
#include "resourcedescription.h"
#include "resourcemanager.h"
#include "player.h"
#include "playerview.h"
#include "turntimer.h"

#include "orderqueueobjectparam.h"

#include "playeragent.h"

PlayerAgent::PlayerAgent(){
  curConnection = NULL;
  player = NULL;
}

PlayerAgent::~PlayerAgent(){
}

void PlayerAgent::setConnection(PlayerConnection * newcon){
  curConnection = newcon;
}

PlayerConnection *PlayerAgent::getConnection() const{
  return curConnection;
}

void PlayerAgent::setPlayer(Player * newpl){
  player = newpl;
}

Player *PlayerAgent::getPlayer() const{
  return player;
}


void PlayerAgent::processIGFrame(Frame * frame){
  if(player == NULL){
    Logger::getLogger()->warning("No Player for PlayerAgent to work on behalf of");
    return;
  }
  Logger::getLogger()->debug("IG Frame processor");
  
  switch (frame->getType()) {
  case ft02_Object_GetById:
    processGetObjectById(frame);
    break;
  case ft02_Object_GetByPos:
    processGetObjectByPos(frame);
    break;
  case ft02_OrderDesc_Get:
    processDescribeOrder(frame);
    break;
  case ft02_Order_Get:
    processGetOrder(frame);
    break;
  case ft02_Order_Insert:
    processAddOrder(frame);
    break;
  case ft02_Order_Remove:
    processRemoveOrder(frame);
    break;
  case ft02_Board_Get:
    processGetBoards(frame);
    break;
  case ft02_Message_Get:
    processGetMessages(frame);
    break;
  case ft02_Message_Post:
    processPostMessage(frame);
    break;
  case ft02_Message_Remove:
    processRemoveMessages(frame);
    break;
  case ft02_ResDesc_Get:
    processGetResourceDescription(frame);
    break;

  case ft03_ObjectIds_Get:
    processGetObjectIds(frame);
    break;
  case ft03_ObjectIds_GetByPos:
    processGetObjectIdsByPos(frame);
    break;
  case ft03_ObjectIds_GetByContainer:
    processGetObjectIdsByContainer(frame);
    break;
  case ft03_OrderTypes_Get:
    processGetOrderTypes(frame);
    break;
  case ft03_Order_Probe:
    processProbeOrder(frame);
    break;
  case ft03_BoardIds_Get:
    processGetBoardIds(frame);
    break;
  case ft03_ResType_Get:
    processGetResourceTypes(frame);
    break;
  case ft03_Player_Get:
    processGetPlayer(frame);
    break;

  case ft03_Category_Get:
    processGetCategory(frame);
    break;
  case ft03_Category_Add:
  case ft03_Category_Remove:
    // don't allow either of these
    processPermDisabled(frame);
    break;
  case ft03_CategoryIds_Get:
    processGetCategoryIds(frame);
    break;
  case ft03_Design_Get:
    processGetDesign(frame);
    break;
  case ft03_Design_Add:
    processAddDesign(frame);
    break;
  case ft03_Design_Modify:
    processModifyDesign(frame);
    break;
  case ft03_Design_Remove:
    //currently disabled
    processPermDisabled(frame);
    break;
  case ft03_DesignIds_Get:
    processGetDesignIds(frame);
    break;
  case ft03_Component_Get:
    processGetComponent(frame);
    break;
  case ft03_ComponentIds_Get:
    processGetComponentIds(frame);
    break;
  case ft03_Property_Get:
    processGetProperty(frame);
    break;
  case ft03_PropertyIds_Get:
    processGetPropertyIds(frame);
    break;

  case ft04_TurnFinished:
    processTurnFinished(frame);
    break;
    
  case ft04_ObjectDesc_Get:
    processGetObjectDesc(frame);
    break;
    
  case ft04_ObjectTypes_Get:
    processGetObjectTypes(frame);
    break;
    
    //object_modify
    
    case ft04_PlayerIds_Get:
        processGetPlayerIds(frame);
        break;
    
  default:
    Logger::getLogger()->warning("PlayerAgent: Discarded frame, not processed, was type %d", frame->getType());

    // Send a failed frame
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_ProtocolError, "Did not understand that frame type.");
    curConnection->sendFrame(of);
    break;
  }
}



void PlayerAgent::processPermDisabled(Frame * frame){
  Logger::getLogger()->debug("doing a frame that is disabled");

  Frame *of = curConnection->createFrame(frame);
  of->createFailFrame(fec_PermUnavailable, "Server does not support this frame type");
  curConnection->sendFrame(of);
}

void PlayerAgent::processGetObjectById (Frame * frame){
  Logger::getLogger()->debug ( "doing get object by id frame" );

  Frame *of = curConnection->createFrame ( frame );
  if ( frame->getDataLength() >= 4 ){
    int len = frame->unpackInt();

    // Check we have enough data
    if ( frame->getDataLength() >= 4 + 4*len ){

      // ListSeq Frame
      of->setType ( ft02_Sequence );
      of->packInt ( len );
      curConnection->sendFrame ( of );

      // Object frames
      for ( int i=0 ; i < len; ++i ){
        unsigned int objectID = frame->unpackInt();

        of = curConnection->createFrame ( frame );
        
        player->getPlayerView()->processGetObject(objectID, of);

        curConnection->sendFrame ( of );
      }

      return;
    }
  }

  // Fall through incase of error
  of->createFailFrame ( fec_FrameError, "Invalid frame, Get Object By Ids, Frame too short" );
  curConnection->sendFrame ( of );
}

void PlayerAgent::processGetObjectByPos(Frame * frame)
{
  Logger::getLogger()->debug("doing get object by pos frame");
  Frame *of = curConnection->createFrame(frame);
  if (frame->getDataLength() >= 36) {
    Vector3d pos;
    unsigned long long r;

    pos.unpack(frame);
    r = frame->unpackInt64();

    std::set<unsigned int> oblist = Game::getGame()->getObjectManager()->getObjectsByPos(pos, r);

    std::set<uint32_t> visibleObjects = player->getPlayerView()->getVisibleObjects();

    for(std::set<unsigned int>::iterator vischk = oblist.begin(); vischk != oblist.end();){
      if(visibleObjects.find(*vischk) == visibleObjects.end()){
        std::set<unsigned int>::iterator temp = vischk;
        ++temp;
        oblist.erase(vischk);
        vischk = temp;
      }else{
        ++vischk;
      }
    }

    of->setType(ft02_Sequence);
    of->packInt(oblist.size());
    curConnection->sendFrame(of);

    std::set<unsigned int>::iterator obCurr = oblist.begin();
    for( ; obCurr != oblist.end(); ++obCurr) {
      of = curConnection->createFrame(frame);
      player->getPlayerView()->processGetObject(*obCurr, of);
      curConnection->sendFrame(of);
    }

  } else {
    of->createFailFrame(fec_FrameError, "Invalid frame");
    curConnection->sendFrame(of);
  }
}

void PlayerAgent::processGetObjectIds(Frame * frame){
  Logger::getLogger()->debug("Doing get object ids frame");
  Frame *of = curConnection->createFrame(frame);
  player->getPlayerView()->processGetObjectIds(frame, of);
  curConnection->sendFrame(of);
}

void PlayerAgent::processGetObjectIdsByPos(Frame* frame){
 Logger::getLogger()->debug("doing get object ids by pos frame");
  Frame *of = curConnection->createFrame(frame);
  if (frame->getDataLength() >= 36) {
    Vector3d pos;
    unsigned long long r;

    pos.unpack(frame);
    r = frame->unpackInt64();

    std::set<unsigned int> oblist = Game::getGame()->getObjectManager()->getObjectsByPos(pos, r);
    std::set<uint32_t> visibleObjects = player->getPlayerView()->getVisibleObjects();
    for(std::set<unsigned int>::iterator vischk = oblist.begin(); vischk != oblist.end();){
      if(visibleObjects.find(*vischk) == visibleObjects.end()){
        std::set<unsigned int>::iterator temp = vischk;
        ++temp;
        oblist.erase(vischk);
        vischk = temp;
      }else{
        ++vischk;
      }
    }

    of->setType(ft03_ObjectIds_List);
    of->packInt(0);
    of->packInt(0);
    of->packInt(oblist.size());
    for(std::set<unsigned int>::iterator itcurr = oblist.begin(); itcurr != oblist.end(); ++itcurr){
      of->packInt(*itcurr);
      of->packInt64(Game::getGame()->getObjectManager()->getObject(*itcurr)->getModTime());
      Game::getGame()->getObjectManager()->doneWithObject(*itcurr);
    }
  }else{
    of->createFailFrame(fec_FrameError, "Invalid frame");
  }
  curConnection->sendFrame(of);
}

void PlayerAgent::processGetObjectIdsByContainer(Frame * frame){
  Logger::getLogger()->debug("doing get object ids by container frame");

  Frame *of = curConnection->createFrame(frame);
  if(frame->getDataLength() != 4){
    of->createFailFrame(fec_FrameError, "Invalid frame");
  }else{
    unsigned int objectID = frame->unpackInt();
    std::set<uint32_t> visibleObjects = player->getPlayerView()->getVisibleObjects();
    if(visibleObjects.find(objectID) != visibleObjects.end()){
      
        IGObject *o = Game::getGame()->getObjectManager()->getObject(objectID);
      
      if(o != NULL){
	std::set<unsigned int> contain = o->getContainedObjects();
	
	for(std::set<unsigned int>::iterator vischk = contain.begin(); vischk != contain.end();){
	  if(visibleObjects.find(*vischk) == visibleObjects.end()){
	    std::set<unsigned int>::iterator temp = vischk;
	    ++temp;
	    contain.erase(vischk);
	    vischk = temp;
	  }else{
	    ++vischk;
	  }
	}

	of->setType(ft03_ObjectIds_List);
	of->packInt(0);
	of->packInt(0);
	of->packInt(contain.size());
	for(std::set<unsigned int>::iterator itcurr = contain.begin(); itcurr != contain.end(); ++itcurr){
	  of->packInt(*itcurr);
            of->packInt64(Game::getGame()->getObjectManager()->getObject(*itcurr)->getModTime());
            Game::getGame()->getObjectManager()->doneWithObject(*itcurr);
	}
        Game::getGame()->getObjectManager()->doneWithObject(objectID);
	
      }else{
	of->createFailFrame(fec_NonExistant, "No such Object");
      }

    }else{
      of->createFailFrame(fec_NonExistant, "No such Object");
    }
  }
  curConnection->sendFrame(of);
}

void PlayerAgent::processGetObjectDesc(Frame * frame){
  Logger::getLogger()->debug("Doing get OrderDesc");
  
  if(frame->getVersion() < fv0_4){
    Logger::getLogger()->debug("Protocol version not high enough");
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Get ObjectDesc frame isn't supported in this protocol");
    curConnection->sendFrame(of);
    return;
  }
  
  Frame *of;
  
  if ( frame->getDataLength() < 4 ){
    of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Frame too short");
    curConnection->sendFrame(of);
    
    return;
    
  }
  
  int len = frame->unpackInt();

  // Check we have enough data
  if ( frame->getDataLength() < 4 + 4*len ){
    of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "List length incorrect");
    curConnection->sendFrame(of);
    
    return;
    
  }

  if(len > 1) {
    Logger::getLogger()->debug("Got multiple ObjectDesc, returning a sequence");
    Frame *seq = curConnection->createFrame(frame);
    seq->setType(ft02_Sequence);
    seq->packInt(len);
    curConnection->sendFrame(seq);
  } else {
    Logger::getLogger()->debug("Got single ObjectDesc, returning one objectdesc");
  }
  
  if(len == 0){
    of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "No objecttype to get");
    curConnection->sendFrame(of);
    
    return;
  }

  // Object frames
  for ( int i=0 ; i < len; ++i ){
    unsigned int objecttype = frame->unpackInt();

    of = curConnection->createFrame(frame);
    
    Game::getGame()->getObjectTypeManager()->doGetObjectDesc(objecttype, of);
    
    curConnection->sendFrame ( of );
  }
  
}

void PlayerAgent::processGetObjectTypes(Frame * frame){
  Logger::getLogger()->debug("Doing get OrderTypes list");
  
  Frame *of = curConnection->createFrame(frame);
  
  if(frame->getVersion() < fv0_4){
    Logger::getLogger()->debug("Protocol version not high enough");
    of->createFailFrame(fec_FrameError, "Get object type frame isn't supported in this protocol");
    curConnection->sendFrame(of);
    return;
  }

  if((frame->getDataLength() != 12 && frame->getVersion() == fv0_3) || (frame->getDataLength() != 20 && frame->getVersion() >= fv0_4)){
    of->createFailFrame(fec_FrameError, "Invalid frame");
    curConnection->sendFrame(of);
    return;
  }

  Game::getGame()->getObjectTypeManager()->doGetObjectTypes(frame, of);
  
  curConnection->sendFrame(of);
  
}

void PlayerAgent::processGetOrder(Frame * frame){
  Logger::getLogger()->debug("Doing get order frame");
  
  if(frame->getDataLength() < 12){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame");
    curConnection->sendFrame(of);
    return;
  }
 
  uint32_t orderqueueid = frame->unpackInt();
  
  if(frame->getVersion() <= fv0_3){
    IGObject* ob = Game::getGame()->getObjectManager()->getObject(orderqueueid);
    if(ob == NULL){
      Frame *of = curConnection->createFrame(frame);
      of->createFailFrame(fec_NonExistant, "No such Object");
      curConnection->sendFrame(of);
      return;
    }
    OrderQueueObjectParam* oqop = dynamic_cast<OrderQueueObjectParam*>(ob->getParameterByType(obpT_Order_Queue));
    if(oqop == NULL){
      Frame *of = curConnection->createFrame(frame);
      of->createFailFrame(fec_NonExistant, "No such Object OrderQueue");
      curConnection->sendFrame(of);
      return;
    }
    orderqueueid = oqop->getQueueId();
  }
  
  OrderQueue* orderqueue = Game::getGame()->getOrderManager()->getOrderQueue(orderqueueid);
  if(orderqueue == NULL || !orderqueue->isOwner(player->getID())){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_NonExistant, "No such Order Queue");
    curConnection->sendFrame(of);
    return;
  }
  
  int num_orders = frame->unpackInt();
  if(frame->getDataLength() != 8 + 4 * num_orders){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, frame too short");
    curConnection->sendFrame(of);
    return;
  }

  if(num_orders > 1) {
    Logger::getLogger()->debug("Got multiple orders, returning a sequence");
    Frame *seq = curConnection->createFrame(frame);
    seq->setType(ft02_Sequence);
    seq->packInt(num_orders);
    curConnection->sendFrame(seq);
  } else {
    Logger::getLogger()->debug("Got single orders, returning one object");
  }
  
  if(num_orders == 0){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "No orders to get");
    curConnection->sendFrame(of);
    
    return;
  }

  for(int i = 0; i < num_orders; i++){
    Frame *of = curConnection->createFrame(frame);

    int ordpos = frame->unpackInt();
    Order *ord = orderqueue->getOrder(ordpos, player->getID());
    if (ord != NULL) {
      ord->createFrame(of, ordpos);
    } else {
      of->createFailFrame(fec_TempUnavailable, "Could not get Order");
    }
    curConnection->sendFrame(of);
  }
  

}


void PlayerAgent::processAddOrder(Frame * frame){
  Logger::getLogger()->debug("doing add order frame");
  Frame *of = curConnection->createFrame(frame);
  if (frame->getDataLength() >= 8) {

    // See if we have a valid orderqueue id
    
    uint32_t orderqueueid = frame->unpackInt();
    
    if(frame->getVersion() <= fv0_3){
      IGObject* ob = Game::getGame()->getObjectManager()->getObject(orderqueueid);
      if(ob == NULL){
        Frame *of = curConnection->createFrame(frame);
        of->createFailFrame(fec_NonExistant, "No such Object");
        curConnection->sendFrame(of);
        return;
      }
      OrderQueueObjectParam* oqop = dynamic_cast<OrderQueueObjectParam*>(ob->getParameterByType(obpT_Order_Queue));
      if(oqop == NULL){
        Frame *of = curConnection->createFrame(frame);
        of->createFailFrame(fec_NonExistant, "No such Object OrderQueue");
        curConnection->sendFrame(of);
        return;
      }
      orderqueueid = oqop->getQueueId();
    }
    
    OrderQueue* orderqueue = Game::getGame()->getOrderManager()->getOrderQueue(orderqueueid);
    if(orderqueue == NULL || !orderqueue->isOwner(player->getID())){
      Frame *of = curConnection->createFrame(frame);
      of->createFailFrame(fec_NonExistant, "No such Order Queue");
      curConnection->sendFrame(of);
      return;
    }
    
    // Order Slot
    int pos = frame->unpackInt();

    // See if we have a valid order
    Order *ord = Game::getGame()->getOrderManager()->createOrder(frame->unpackInt());
    if (ord == NULL) {
      of->createFailFrame(fec_NonExistant, "No such order type");
    } else {
      ord->setOrderQueueId(orderqueueid);
      Result r = ord->inputFrame(frame, player->getID());
      if (r){
        if(orderqueue->addOrder(ord, pos, player->getID())) {
          if(of->getVersion() >= fv0_4){
            ord->createFrame(of, pos);
          }else{
            of->setType(ft02_OK);
            of->packString("Order Added");
          }
          //update ObjectView
          uint32_t objid = orderqueue->getObjectId();
          if(objid != 0){
            ObjectView* obv = player->getPlayerView()->getObjectView(objid);
            obv->touchModTime();
            player->getPlayerView()->updateObjectView(objid);
          }
        } else {
          of->createFailFrame(fec_TempUnavailable, "Not allowed to add that order type.");
        }
      }else{
                // FIXME: This isn't always a FrameError really...
        of->createFailFrame(fec_FrameError, (std::string("Could not add order, ") + r).c_str());
      }
    }
  } else {
    of->createFailFrame(fec_FrameError, "Invalid frame, Add Order, too short");
  }
  curConnection->sendFrame(of);
}

void PlayerAgent::processRemoveOrder(Frame * frame){
  Logger::getLogger()->debug("doing remove order frame");

  if(frame->getDataLength() < 12){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Remove Order, too short (<12 bytes)");
    curConnection->sendFrame(of);
    return;
  }

  int orderqueueid = frame->unpackInt();
   
  if(frame->getVersion() <= fv0_3){
    IGObject* ob = Game::getGame()->getObjectManager()->getObject(orderqueueid);
    if(ob == NULL){
      Frame *of = curConnection->createFrame(frame);
      of->createFailFrame(fec_NonExistant, "No such Object");
      curConnection->sendFrame(of);
      return;
    }
    OrderQueueObjectParam* oqop = dynamic_cast<OrderQueueObjectParam*>(ob->getParameterByType(obpT_Order_Queue));
    if(oqop == NULL){
      Frame *of = curConnection->createFrame(frame);
      of->createFailFrame(fec_NonExistant, "No such Object OrderQueue");
      curConnection->sendFrame(of);
      return;
    }
    orderqueueid = oqop->getQueueId();
  }
  
  OrderQueue* orderqueue = Game::getGame()->getOrderManager()->getOrderQueue(orderqueueid);
  if(orderqueue == NULL || !orderqueue->isOwner(player->getID())){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_NonExistant, "No such Order Queue");
    curConnection->sendFrame(of);
    return;
  }

  int num_orders = frame->unpackInt();

  if(frame->getDataLength() != 8 + 4 * num_orders){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, too short");
    curConnection->sendFrame(of);
    return;
  }

  if(num_orders > 1){
    Frame *seq = curConnection->createFrame(frame);
    seq->setType(ft02_Sequence);
    seq->packInt(num_orders);
    curConnection->sendFrame(seq);
  }

  for(int i = 0; i < num_orders; i++){
    Frame *of = curConnection->createFrame(frame);
    int ordpos = frame->unpackInt();
    if (orderqueue->removeOrder(ordpos, player->getID())) {
      of->setType(ft02_OK);
      of->packString("Order removed");
       //update ObjectView
      uint32_t objid = orderqueue->getObjectId();
      if(objid != 0){
        ObjectView* obv = player->getPlayerView()->getObjectView(objid);
        obv->touchModTime();
        player->getPlayerView()->updateObjectView(objid);
      }
    } else {
      of->createFailFrame(fec_TempUnavailable, "Could not remove Order");
    }

    curConnection->sendFrame(of);
    
  }

}


void PlayerAgent::processDescribeOrder(Frame * frame)
{
	Logger::getLogger()->debug("doing describe order frame");

        if(frame->getDataLength() < 4){
          Frame *of = curConnection->createFrame(frame);
          of->createFailFrame(fec_FrameError, "Invalid frame, Describe Order, Frame too short (<4 bytes)");
          curConnection->sendFrame(of);
          return;
        }
        
	int numdesc = frame->unpackInt();
        
        if(frame->getDataLength() < 4 + 4 * numdesc){
          Frame *of = curConnection->createFrame(frame);
          of->createFailFrame(fec_FrameError, "Invalid frame, Describe Order, Frame too short");
          curConnection->sendFrame(of);
          return;
        }
        
	if(numdesc > 1){
	  Frame *seq = curConnection->createFrame(frame);
	  seq->setType(ft02_Sequence);
	  seq->packInt(numdesc);
	  curConnection->sendFrame(seq);
	}
	
	if(numdesc == 0){
	  Frame *of = curConnection->createFrame(frame);
	  Logger::getLogger()->debug("asked for no orders to describe, silly client...");
	  of->createFailFrame(fec_NonExistant, "You didn't ask for any order descriptions, try again");
	  curConnection->sendFrame(of);
	}

	for(int i = 0; i < numdesc; i++){
	  Frame *of = curConnection->createFrame(frame);
	  int ordertype = frame->unpackInt();
	  Game::getGame()->getOrderManager()->describeOrder(ordertype, of);
	  curConnection->sendFrame(of);
	}
}

void PlayerAgent::processGetOrderTypes(Frame * frame){
  Logger::getLogger()->debug("doing get order types frame");
  
  Frame *of = curConnection->createFrame(frame);
  
  if(frame->getVersion() < fv0_3){
    Logger::getLogger()->debug("protocol version not high enough");
    of->createFailFrame(fec_FrameError, "Get order type frame isn't supported in this protocol");
    curConnection->sendFrame(of);
    return;
  }

  if(frame->getDataLength() != 12 && frame->getVersion() == fv0_3) {
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Order Types (TP03), Frame too short (<12 bytes)");
  } else if (frame->getDataLength() != 20 && frame->getVersion() >= fv0_4) {
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Order Types (TP04), Frame too short (<20 bytes)");
  } else {
    Game::getGame()->getOrderManager()->doGetOrderTypes(frame, of);
  }
  curConnection->sendFrame(of);
}

void PlayerAgent::processProbeOrder(Frame * frame){
  Logger::getLogger()->debug("doing probe order frame");
  
  if(frame->getVersion() < fv0_3){
    Logger::getLogger()->debug("protocol version not high enough");
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Probe order isn't supported in this protocol");
    curConnection->sendFrame(of);
    return;
  }

  if(frame->getDataLength() < 8){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Probe Order, Frame too short (<8 bytes)");
    curConnection->sendFrame(of);
    return;
  }
  
  int orderqueueid = frame->unpackInt();
  
  if(frame->getVersion() <= fv0_3){
  IGObject* ob = Game::getGame()->getObjectManager()->getObject(orderqueueid);
    if(ob == NULL){
      Frame *of = curConnection->createFrame(frame);
      of->createFailFrame(fec_NonExistant, "No such Object");
      curConnection->sendFrame(of);
      return;
    }
    OrderQueueObjectParam* oqop = dynamic_cast<OrderQueueObjectParam*>(ob->getParameterByType(obpT_Order_Queue));
    if(oqop == NULL){
      Frame *of = curConnection->createFrame(frame);
      of->createFailFrame(fec_NonExistant, "No such Object OrderQueue");
      curConnection->sendFrame(of);
      return;
    }
    orderqueueid = oqop->getQueueId();
  }
  
  OrderQueue* orderqueue = Game::getGame()->getOrderManager()->getOrderQueue(orderqueueid);
  if(orderqueue == NULL || !orderqueue->isOwner(player->getID())){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_NonExistant, "No such Order Queue");
    curConnection->sendFrame(of);
    return;
  }

  int pos = frame->unpackInt();
  
  Frame *of = curConnection->createFrame(frame);
  // See if we have a valid order
  Order *ord = Game::getGame()->getOrderManager()->createOrder(frame->unpackInt());
  if (ord == NULL) {
    of->createFailFrame(fec_NonExistant, "No such order type");
  }else if(orderqueue->checkOrderType(ord->getType(), player->getID())){
    ord->setOrderQueueId(orderqueueid);
    if(ord->inputFrame(frame, player->getID())){
      ord->createFrame(of, pos);
    }else{
      of->createFailFrame(fec_FrameError, "Order could not be unpacked correctly, invalid order");
      Logger::getLogger()->debug("Probe Order, could not unpack order");
    }
    
  }else{
    
    Logger::getLogger()->debug("The order to be probed is not allowed on this object");
    of->createFailFrame(fec_PermUnavailable, "The order to be probed is not allowed on this object, try again");
    
  }
  delete ord;
  curConnection->sendFrame(of);
  
}

void PlayerAgent::processGetBoards(Frame * frame){
  Logger::getLogger()->debug("doing Get Boards frame");
  
  if(frame->getDataLength() < 4){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Boards, Frame too short (<4 bytes)");
    curConnection->sendFrame(of);
    return;
  }
  
  int numboards = frame->unpackInt();
  
  if(frame->getDataLength() < 4 + 4*numboards){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Boards, Frame too short");
    curConnection->sendFrame(of);
    return;
  }
  
  if(numboards > 1){
    Frame *seq = curConnection->createFrame(frame);
    seq->setType(ft02_Sequence);
    seq->packInt(numboards);
    curConnection->sendFrame(seq);
  }
  
  if(numboards == 0){
    Frame *of = curConnection->createFrame(frame);
    Logger::getLogger()->debug("asked for no boards, silly client...");
    of->createFailFrame(fec_NonExistant, "You didn't ask for any boards, try again");
    curConnection->sendFrame(of);
  }

  for(int i = 0; i < numboards; i++){
    Frame *of = curConnection->createFrame(frame);
    uint32_t boardnum = frame->unpackInt();
    if(boardnum == 0 || boardnum == player->getBoardId()){
      Board* board = Game::getGame()->getBoardManager()->getBoard(player->getBoardId());
      board->packBoard(of);
    }else{
      //boards in the game object
      of->createFailFrame(fec_PermUnavailable, "No non-player boards yet");
    }
    curConnection->sendFrame(of);
  }
}

void PlayerAgent::processGetBoardIds(Frame* frame){
  Logger::getLogger()->debug("Doing get board ids frame");
   
  if(frame->getVersion() < fv0_3){
    Logger::getLogger()->debug("protocol version not high enough");
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Get Board Ids isn't supported in this protocol");
    curConnection->sendFrame(of);
    return;
  }

  if((frame->getDataLength() != 12 && frame->getVersion() == fv0_3) || (frame->getDataLength() != 20 && frame->getVersion() >= fv0_4)){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Board Ids, Frame too short");
    curConnection->sendFrame(of);
    return;
  }

  unsigned int seqkey = frame->unpackInt();
  if(seqkey == UINT32_NEG_ONE){
    //start new seqkey
    seqkey = 0;
  }
  uint32_t snum = frame->unpackInt(); // starting number
  uint32_t numtoget = frame->unpackInt();
  uint64_t fromtime = UINT64_NEG_ONE;
  if(frame->getVersion() >= fv0_4){
    fromtime = frame->unpackInt64();
  }

  Frame *of = curConnection->createFrame(frame);
  of->setType(ft03_BoardIds_List);
  of->packInt(seqkey);

  if(numtoget == 0){
    of->packInt(1); // On ID left to get
    of->packInt(0); // Zero sized list
  } else {
    of->packInt(0);
    of->packInt(1);
    of->packInt(0); //personal board
    of->packInt64(Game::getGame()->getBoardManager()->getBoard(player->getBoardId())->getModTime());
  }

  if(frame->getVersion() >= fv0_4){
    of->packInt64(fromtime);
  }
  curConnection->sendFrame(of);
}

void PlayerAgent::processGetMessages(Frame * frame){
  Logger::getLogger()->debug("doing Get Messages frame");

  if(frame->getDataLength() < 8){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Messages, Frame too short (<8 bytes)");
    curConnection->sendFrame(of);
    return;
  }
  
  int lboardid = frame->unpackInt();
  int nummsg = frame->unpackInt();

  if(frame->getDataLength() < 8 + 4 * nummsg){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Messages, Frame too short");
    curConnection->sendFrame(of);
    return;
  }
  
  if(nummsg > 1){
    Frame *seq = curConnection->createFrame(frame);
    seq->setType(ft02_Sequence);
    seq->packInt(nummsg);
    curConnection->sendFrame(seq);
  }

  if(nummsg == 0){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "No messages to get");
    curConnection->sendFrame(of);
    
    return;
  }

  Board * currboard;
    //HACK
    // should depend on what the player should be allowed to see
    if(lboardid == 0)
        lboardid = player->getBoardId();
    currboard = Game::getGame()->getBoardManager()->getBoard(lboardid);

  if(currboard != NULL){
    for(int i = 0; i < nummsg; i++){
      Frame *of = curConnection->createFrame(frame);
      int msgnum = frame->unpackInt();

      currboard->packMessage(of, msgnum);

      curConnection->sendFrame(of);
    
    }

  }else{
     Frame *of = curConnection->createFrame(frame);
     of->createFailFrame(fec_NonExistant, "Board does not exist");
     curConnection->sendFrame(of);
  }

}

void PlayerAgent::processPostMessage(Frame * frame){
  Logger::getLogger()->debug("doing Post Messages frame");

  Frame *of = curConnection->createFrame(frame);

  if(frame->getDataLength() < 28){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Post Message, Frame too short (<28 bytes)");
    curConnection->sendFrame(of);
    return;
  }
  
  int lboardid = frame->unpackInt();
  int pos = frame->unpackInt();

  Board * currboard;
    //HACK
    // should depend on what the player should be allowed to see
    if(lboardid == 0)
        lboardid = player->getBoardId();
    currboard = Game::getGame()->getBoardManager()->getBoard(lboardid);

  if(currboard != NULL){
    Message* msg = new Message();
        int msgtypes = frame->unpackInt(); // message type, no longer used
        for(int i = 0; i < msgtypes; i++){
            //have to clear the list of msgtypes
            // only filled under TP02
            frame->unpackInt();
        }
    msg->setSubject(frame->unpackStdString());
    msg->setBody(frame->unpackStdString());
        if(frame->getVersion() >= fv0_3){
            frame->unpackInt(); // turn created - overwritten by current turn number
            uint32_t numrefs = frame->unpackInt();
            for(uint32_t i = 0; i < numrefs; i++){
                int32_t rtype = frame->unpackInt();
                uint32_t rid = frame->unpackInt();
                msg->addReference(rtype, rid);
            }
        }
    currboard->addMessage(msg, pos);

    of->setType(ft02_OK);
    of->packString("Message posted");
    
  }else{
    of->createFailFrame(fec_NonExistant, "Board does not exist");
  }

  curConnection->sendFrame(of);
  
}

void PlayerAgent::processRemoveMessages(Frame * frame){
  Logger::getLogger()->debug("doing Remove Messages frame");

  if(frame->getDataLength() < 8){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Remove Messages, Frame too short (<8 bytes)");
    curConnection->sendFrame(of);
    return;
  }
  
  int lboardid = frame->unpackInt();
  int nummsg = frame->unpackInt();

  if(frame->getDataLength() < 8 + 4 * nummsg){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Remove Message, Frame too short");
    curConnection->sendFrame(of);
    return;
  }
  if(nummsg > 1){
    Frame *seq = curConnection->createFrame(frame);
    seq->setType(ft02_Sequence);
    seq->packInt(nummsg);
    curConnection->sendFrame(seq);
  }

  Board * currboard;
    //HACK
    // should depend on what the player should be allowed to see
    if(lboardid == 0)
        lboardid = player->getBoardId();
    currboard = Game::getGame()->getBoardManager()->getBoard(lboardid);

  if(currboard != NULL){
    for(int i = 0; i < nummsg; i++){
      Frame *of = curConnection->createFrame(frame);
      int msgnum = frame->unpackInt();

      if(currboard->removeMessage(msgnum)){
	of->setType(ft02_OK);
	of->packString("Message removed");
      }else{
	of->createFailFrame(fec_NonExistant, "Message not removed, does exist");
      }

      curConnection->sendFrame(of);
    
    }

  }else{
     Frame *of = curConnection->createFrame(frame);
     of->createFailFrame(fec_NonExistant, "Board does not exist");
     curConnection->sendFrame(of);
  }

}

void PlayerAgent::processGetResourceDescription(Frame * frame){
  Logger::getLogger()->debug("doing Get Resource Description frame");
  
  if(frame->getDataLength() < 4){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Resource Description, Frame too short (<4 bytes)");
    curConnection->sendFrame(of);
    return;
  }
  
  int numress = frame->unpackInt();
  
  if(frame->getDataLength() < 4 + 4* numress){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Resource Description, Frame too short");
    curConnection->sendFrame(of);
    return;
  }
  
  if(numress > 1){
    Frame *seq = curConnection->createFrame(frame);
    seq->setType(ft02_Sequence);
    seq->packInt(numress);
    curConnection->sendFrame(seq);
  }
  
  if(numress == 0){
    Frame *of = curConnection->createFrame(frame);
    Logger::getLogger()->debug("asked for no resource descriptions, silly client...");
    of->createFailFrame(fec_NonExistant, "You didn't ask for any resource descriptions, try again");
    curConnection->sendFrame(of);
  }

  for(int i = 0; i < numress; i++){
    Frame *of = curConnection->createFrame(frame);
    int rnum = frame->unpackInt();
    
        const ResourceDescription * res = Game::getGame()->getResourceManager()->getResourceDescription(rnum);
        if(res != NULL){
            res->packFrame(of);
        }else{
            of->createFailFrame(fec_NonExistant, "No Resource Descriptions available");
        }

    curConnection->sendFrame(of);
  }
}

void PlayerAgent::processGetResourceTypes(Frame* frame){
  Logger::getLogger()->debug("doing Get Resource Types frame");
  
  if(frame->getVersion() < fv0_3){
    Logger::getLogger()->debug("protocol version not high enough");
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Get Resource Types isn't supported in this protocol");
    curConnection->sendFrame(of);
    return;
  }

   if((frame->getDataLength() != 12 && frame->getVersion() == fv0_3) || (frame->getDataLength() != 20 && frame->getVersion() >= fv0_4)){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Resource Types, Frame too short");
    curConnection->sendFrame(of);
    return;
  }

  unsigned int seqkey = frame->unpackInt();
  if(seqkey == UINT32_NEG_ONE){
    //start new seqkey
    seqkey = 0;
  }

  uint32_t snum = frame->unpackInt();
  uint32_t numtoget = frame->unpackInt();
  uint64_t fromtime = UINT64_NEG_ONE;
  if(frame->getVersion() >= fv0_4){
    fromtime = frame->unpackInt64();
  }
  
  ResourceManager* rm = Game::getGame()->getResourceManager();
  std::set<uint32_t> idset = rm->getAllIds();
  
  std::map<uint32_t, uint64_t> modlist;
  for(std::set<uint32_t>::iterator itcurr = idset.begin();
      itcurr != idset.end(); ++itcurr){
    const ResourceDescription * res = rm->getResourceDescription(*itcurr);
    if(fromtime == UINT64_NEG_ONE || res->getModTime() > fromtime){
      modlist[*itcurr] = res->getModTime();
    }
  }
  
  if(snum > modlist.size()){
    Logger::getLogger()->debug("Starting number too high, snum = %d, size = %d", snum, modlist.size());
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_NonExistant, "Starting number too high");
    curConnection->sendFrame(of);
    return;
  }
  if(numtoget > modlist.size() - snum){
    numtoget = modlist.size() - snum;
  }
  
  if(numtoget > MAX_ID_LIST_SIZE + ((frame->getVersion() < fv0_4)? 1 : 0)){
    Logger::getLogger()->debug("Number of items to get too high, numtoget = %d", numtoget);
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Too many items to get, frame too big");
    curConnection->sendFrame(of);
    return;
  }
  
  Frame *of = curConnection->createFrame(frame);
  of->setType(ft03_ResType_List);
  of->packInt(seqkey);
  of->packInt(modlist.size() - snum - numtoget);
  of->packInt(numtoget);
  std::map<uint32_t, uint64_t>::iterator itcurr = modlist.begin();
  std::advance(itcurr, snum);
  for(uint32_t i = 0; i < numtoget; i++, ++itcurr){
    of->packInt(itcurr->first);
    of->packInt64(itcurr->second);
  }
  
  if(of->getVersion() >= fv0_4){
    of->packInt64(fromtime);
  }
  
  curConnection->sendFrame(of);
}

void PlayerAgent::processGetPlayer(Frame* frame){
  Logger::getLogger()->debug("doing Get Player frame");
  
  if(frame->getDataLength() < 4){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Player, Frame too short (<4 bytes)");
    curConnection->sendFrame(of);
    return;
  }
  
  int numplayers = frame->unpackInt();
  
  if(frame->getDataLength() < 4 + 4 * numplayers){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Player, Frame too short");
    curConnection->sendFrame(of);
    return;
  }
  
  if(numplayers > 1){
    Frame *seq = curConnection->createFrame(frame);
    seq->setType(ft02_Sequence);
    seq->packInt(numplayers);
    curConnection->sendFrame(seq);
  }
  
  if(numplayers == 0){
    Frame *of = curConnection->createFrame(frame);
    Logger::getLogger()->debug("asked for no players, silly client...");
    of->createFailFrame(fec_NonExistant, "You didn't ask for any players, try again");
    curConnection->sendFrame(of);
  }

  for(int i = 0; i < numplayers; i++){
    Frame *of = curConnection->createFrame(frame);
    int pnum = frame->unpackInt();
    if(pnum == 0){
      player->packFrame(of);
    }else{
      if(pnum != -1){
        Player* p = Game::getGame()->getPlayerManager()->getPlayer(pnum);
        if(p != NULL){
          p->packFrame(of);
        }else{
          of->createFailFrame(fec_NonExistant, "Player doesn't exist");
        }
      }else{
        of->createFailFrame(fec_NonExistant, "Player -1 doesn't exist, invalid player id");
      }
    }
    curConnection->sendFrame(of);
  }
}

void PlayerAgent::processGetPlayerIds(Frame* frame){
    Logger::getLogger()->debug("doing Get Resource Types frame");
  
    if(frame->getVersion() < fv0_4){
        Logger::getLogger()->debug("protocol version not high enough");
        Frame *of = curConnection->createFrame(frame);
        of->createFailFrame(fec_FrameError, "Get Playerids isn't supported in this protocol");
        curConnection->sendFrame(of);
        return;
    }
    
    if(frame->getDataLength() != 20){
        Frame *of = curConnection->createFrame(frame);
        of->createFailFrame(fec_FrameError, "Invalid frame, Get Player Ids, Frame not 20 bytes");
        curConnection->sendFrame(of);
        return;
    }
    
    unsigned int seqkey = frame->unpackInt();
    if(seqkey == UINT32_NEG_ONE){
        //start new seqkey
        seqkey = 0;
    }
    
    uint32_t snum = frame->unpackInt();
    uint32_t numtoget = frame->unpackInt();
    uint64_t fromtime = UINT64_NEG_ONE;
    
    fromtime = frame->unpackInt64();
    
    PlayerManager* pm = Game::getGame()->getPlayerManager();
    std::set<playerid_t> idset = pm->getAllIds();
    
    std::map<uint32_t, uint64_t> modlist;
    for(std::set<playerid_t>::iterator itcurr = idset.begin();
        itcurr != idset.end(); ++itcurr){
        const Player * pl = pm->getPlayer(*itcurr);
        if(fromtime == UINT64_NEG_ONE || pl->getModTime() > fromtime){
            modlist[*itcurr] = pl->getModTime();
        }
    }
    
    if(snum > modlist.size()){
        Logger::getLogger()->debug("Starting number too high, snum = %d, size = %d", snum, modlist.size());
        Frame *of = curConnection->createFrame(frame);
        of->createFailFrame(fec_NonExistant, "Starting number too high");
        curConnection->sendFrame(of);
        return;
    }
    if(numtoget > modlist.size() - snum){
        numtoget = modlist.size() - snum;
    }
    
    if(numtoget > MAX_ID_LIST_SIZE){
        Logger::getLogger()->debug("Number of items to get too high, numtoget = %d", numtoget);
        Frame *of = curConnection->createFrame(frame);
        of->createFailFrame(fec_FrameError, "Too many items to get, frame too big");
        curConnection->sendFrame(of);
        return;
    }
    
    Frame *of = curConnection->createFrame(frame);
    of->setType(ft04_PlayerIds_List);
    of->packInt(seqkey);
    of->packInt(modlist.size() - snum - numtoget);
    of->packInt(numtoget);
    std::map<uint32_t, uint64_t>::iterator itcurr = modlist.begin();
    std::advance(itcurr, snum);
    for(uint32_t i = 0; i < numtoget; i++, ++itcurr){
        of->packInt(itcurr->first);
        of->packInt64(itcurr->second);
    }
    
    of->packInt64(fromtime);
    
    curConnection->sendFrame(of);
}

void PlayerAgent::processGetCategory(Frame* frame){
  Logger::getLogger()->debug("doing Get Category frame");

  if(frame->getDataLength() < 4){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Category, Frame too short (<4 bytes)");
    curConnection->sendFrame(of);
    return;
  }
  
  int numcats = frame->unpackInt();
  
  if(frame->getDataLength() < 4 + 4 * numcats){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Category, Frame too short");
    curConnection->sendFrame(of);
    return;
  }
  
  if(numcats > 1){
    Frame *seq = curConnection->createFrame(frame);
    seq->setType(ft02_Sequence);
    seq->packInt(numcats);
    curConnection->sendFrame(seq);
  }
  
  if(numcats == 0){
    Frame *of = curConnection->createFrame(frame);
    Logger::getLogger()->debug("asked for no categories, silly client...");
    of->createFailFrame(fec_NonExistant, "You didn't ask for any categories, try again");
    curConnection->sendFrame(of);
  }

  for(int i = 0; i < numcats; i++){
    Frame *of = curConnection->createFrame(frame);
    int catnum = frame->unpackInt();
    Category* cat = Game::getGame()->getDesignStore()->getCategory(catnum);
    if(cat == NULL){
      of->createFailFrame(fec_NonExistant, "No Such Category");
    }else{
      cat->packFrame(of);
    }
    curConnection->sendFrame(of);
  }

}

void PlayerAgent::processGetCategoryIds(Frame* frame){
  Logger::getLogger()->debug("doing Get Category Ids frame");
  
  if(frame->getVersion() < fv0_3){
    Logger::getLogger()->debug("protocol version not high enough");
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Get Category ids isn't supported in this protocol");
    curConnection->sendFrame(of);
    return;
  }
  
  if((frame->getDataLength() != 12 && frame->getVersion() == fv0_3) || (frame->getDataLength() != 20 && frame->getVersion() >= fv0_4)){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Categories Ids, Frame too short");
    curConnection->sendFrame(of);
    return;
  }
  
  frame->unpackInt(); //seqnum
  uint32_t snum = frame->unpackInt();
  uint32_t numtoget = frame->unpackInt();
  uint64_t fromtime = UINT64_NEG_ONE;
  if(frame->getVersion() >= fv0_4){
    fromtime = frame->unpackInt64();
  }

  
  DesignStore *ds = Game::getGame()->getDesignStore();
  std::set<unsigned int> cids = ds->getCategoryIds();
  
  std::map<uint32_t, uint64_t> modlist;
  for(std::set<uint32_t>::iterator itcurr = cids.begin();
      itcurr != cids.end(); ++itcurr){
    Category * cat = ds->getCategory(*itcurr);
    if(fromtime == UINT64_NEG_ONE || cat->getModTime() > fromtime){
      modlist[*itcurr] = cat->getModTime();
    }
  }
  
  if(snum > modlist.size()){
    Logger::getLogger()->debug("Starting number too high, snum = %d, size = %d", snum, modlist.size());
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_NonExistant, "Starting number too high");
    curConnection->sendFrame(of);
    return;
  }
  
  if(numtoget > modlist.size() - snum){
      numtoget = modlist.size() - snum;
  }
  
  if(numtoget > MAX_ID_LIST_SIZE + ((frame->getVersion() < fv0_4)? 1 : 0)){
    Logger::getLogger()->debug("Number of items to get too high, numtoget = %d", numtoget);
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Too many items to get, frame too big");
    curConnection->sendFrame(of);
    return;
  }
  
  Frame *of = curConnection->createFrame(frame);
  of->setType(ft03_CategoryIds_List);
  of->packInt(0); // seqkey
  of->packInt(modlist.size() - snum - numtoget);
  of->packInt(numtoget);
  std::map<uint32_t, uint64_t>::iterator itcurr = modlist.begin();
  std::advance(itcurr, snum);
  for(uint32_t i = 0; i < numtoget; i++, ++itcurr){
    of->packInt(itcurr->first);
    of->packInt64(itcurr->second);
  }
  
  if(of->getVersion() >= fv0_4){
    of->packInt64(fromtime);
  }
  
  curConnection->sendFrame(of);
}

void PlayerAgent::processGetDesign(Frame* frame){
  Logger::getLogger()->debug("doing Get Design frame");

  if(frame->getDataLength() < 4){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Design, Frame too short (<4 bytes)");
    curConnection->sendFrame(of);
    return;
  }
  
  int numdesigns = frame->unpackInt();
  
  if(frame->getDataLength() < 4 + 4 * numdesigns){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Design, Frame too short");
    curConnection->sendFrame(of);
    return;
  }
  
  if(numdesigns > 1){
    Frame *seq = curConnection->createFrame(frame);
    seq->setType(ft02_Sequence);
    seq->packInt(numdesigns);
    curConnection->sendFrame(seq);
  }
  
  if(numdesigns == 0){
    Frame *of = curConnection->createFrame(frame);
    Logger::getLogger()->debug("asked for no designs, silly client...");
    of->createFailFrame(fec_NonExistant, "You didn't ask for any designs, try again");
    curConnection->sendFrame(of);
  }

  for(int i = 0; i < numdesigns; i++){
    Frame *of = curConnection->createFrame(frame);
    int designnum = frame->unpackInt();
    player->getPlayerView()->processGetDesign(designnum, of);
    curConnection->sendFrame(of);
  }
}

void PlayerAgent::processAddDesign(Frame* frame){
  Logger::getLogger()->debug("doing Add Design frame");

  if(frame->getDataLength() < 40){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Add Design, Frame too short (<40 bytes)");
    curConnection->sendFrame(of);
    return;
  }
  
  Design* design = new Design();
  frame->unpackInt(); //designid, don't take, overwrite
  frame->unpackInt64(); //timestamp, discard
  int numcats = frame->unpackInt(); //num of categories, had better be 1
  if(numcats >= 1){
    design->setCategoryId(frame->unpackInt());
    numcats--;
  }else{
    design->setCategoryId(1); //TODO should check against components
  }
  for(int i = 0; i < numcats; i++){
    frame->unpackInt();
  }
  design->setName(frame->unpackStdString());
  design->setDescription(frame->unpackStdString());
  frame->unpackInt(); //number in use, (client) read only
  design->setOwner(player->getID());
  unsigned int tpid = frame->unpackInt();
  if(player->getID() != tpid){
    Logger::getLogger()->debug("Odd, client sent wrong player id %d", tpid);
  }
  unsigned int numcomp = frame->unpackInt();
  std::map<unsigned int, unsigned int> comps;
  for(unsigned int i = 0; i < numcomp; i++){
        unsigned int compid = frame->unpackInt();
        comps[compid] = (frame->unpackInt());
  }
    design->setComponents(comps);
  //discard rest of frame

  DesignStore* ds = Game::getGame()->getDesignStore();

  Frame *of = curConnection->createFrame(frame);
  if(ds->addDesign(design)){
    of->setType(ft02_OK);
    of->packString("Design added");
  }else{
    of->createFailFrame(fec_FrameError, "Could not add design");
  }
  curConnection->sendFrame(of);
}

void PlayerAgent::processModifyDesign(Frame* frame){
  Logger::getLogger()->debug("doing Modify Design frame");

  if(frame->getDataLength() < 40){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Modify Design, Frame too short (<40 bytes)");
    curConnection->sendFrame(of);
    return;
  }
  
  Design* design = new Design();
  design->setDesignId(frame->unpackInt());
  frame->unpackInt64(); //timestamp, discard
  int numcats = frame->unpackInt(); //num of categories, had better be 1
  if(numcats >= 1){
    design->setCategoryId(frame->unpackInt());
    numcats--;
  }else{
    design->setCategoryId(1); //TODO should check against components
  }
  for(int i = 0; i < numcats; i++){
    frame->unpackInt();
  }
  design->setName(frame->unpackStdString());
  design->setDescription(frame->unpackStdString());
  frame->unpackInt(); //number in use, (client) read only
  design->setOwner(player->getID());
  unsigned int tpid = frame->unpackInt();
  if(player->getID() != tpid){
    Logger::getLogger()->debug("Odd, client sent wrong player id %d", tpid);
  }
  unsigned int numcomp = frame->unpackInt();
  std::map<unsigned int, unsigned int> comps;
  for(unsigned int i = 0; i < numcomp; i++){
        unsigned int compid = frame->unpackInt();
        comps[compid] = (frame->unpackInt());
  }
    design->setComponents(comps);
  //discard rest of frame

  DesignStore* ds = Game::getGame()->getDesignStore();

  Frame *of = curConnection->createFrame(frame);
  if(ds->modifyDesign(design)){
    of->setType(ft02_OK);
    of->packString("Design modified");
  }else{
    of->createFailFrame(fec_FrameError, "Could not modify design");
  }
  curConnection->sendFrame(of);
}

void PlayerAgent::processGetDesignIds(Frame* frame){
  Frame *of = curConnection->createFrame(frame);
  player->getPlayerView()->processGetDesignIds(frame, of);
  curConnection->sendFrame(of);
}

void PlayerAgent::processGetComponent(Frame* frame){
  Logger::getLogger()->debug("doing Get Component frame");

  if(frame->getDataLength() < 4){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Component, Frame too short (<4 bytes)");
    curConnection->sendFrame(of);
    return;
  }
  
  int numcomps = frame->unpackInt();
  
  if(frame->getDataLength() < 4 + 4 * numcomps){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Component, Frame too short");
    curConnection->sendFrame(of);
    return;
  }
  
  if(numcomps > 1){
    Frame *seq = curConnection->createFrame(frame);
    seq->setType(ft02_Sequence);
    seq->packInt(numcomps);
    curConnection->sendFrame(seq);
  }
  
  if(numcomps == 0){
    Frame *of = curConnection->createFrame(frame);
    Logger::getLogger()->debug("asked for no components, silly client...");
    of->createFailFrame(fec_NonExistant, "You didn't ask for any components, try again");
    curConnection->sendFrame(of);
  }

  for(int i = 0; i < numcomps; i++){
    Frame *of = curConnection->createFrame(frame);
    int compnum = frame->unpackInt();
    player->getPlayerView()->processGetComponent(compnum, of);
    curConnection->sendFrame(of);
  }
}

void PlayerAgent::processGetComponentIds(Frame* frame){
  Frame *of = curConnection->createFrame(frame);
  player->getPlayerView()->processGetComponentIds(frame, of);
 
  curConnection->sendFrame(of);
}

void PlayerAgent::processGetProperty(Frame* frame){
  Logger::getLogger()->debug("doing Get Property frame");

  DesignStore* ds = Game::getGame()->getDesignStore();

  if(frame->getDataLength() < 4){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Property, Frame too short (<4 bytes)");
    curConnection->sendFrame(of);
    return;
  }
  
  int numprops = frame->unpackInt();
  
    if(frame->getDataLength() < 4 + 4 * numprops){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Property, Frame too short");
    curConnection->sendFrame(of);
    return;
  }
  
  if(numprops > 1){
    Frame *seq = curConnection->createFrame(frame);
    seq->setType(ft02_Sequence);
    seq->packInt(numprops);
    curConnection->sendFrame(seq);
  }
  
  if(numprops == 0){
    Frame *of = curConnection->createFrame(frame);
    Logger::getLogger()->debug("asked for no properties, silly client...");
    of->createFailFrame(fec_NonExistant, "You didn't ask for any properties, try again");
    curConnection->sendFrame(of);
  }

  for(int i = 0; i < numprops; i++){
    Frame *of = curConnection->createFrame(frame);
    int propnum = frame->unpackInt();
    Property* property = ds->getProperty(propnum);
    if(property == NULL){
      of->createFailFrame(fec_NonExistant, "No Such Property");
    }else{
      property->packFrame(of);
    }
    curConnection->sendFrame(of);
  }
}

void PlayerAgent::processGetPropertyIds(Frame* frame){
 Logger::getLogger()->debug("doing Get Property Ids frame");
  
  if(frame->getVersion() < fv0_3){
    Logger::getLogger()->debug("protocol version not high enough");
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Get Design ids isn't supported in this protocol");
    curConnection->sendFrame(of);
    return;
  }
  
  if((frame->getDataLength() != 12 && frame->getVersion() == fv0_3) || (frame->getDataLength() != 20 && frame->getVersion() >= fv0_4)){
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Invalid frame, Get Property Ids, Frame too short");
    curConnection->sendFrame(of);
    return;
  }

  std::set<uint32_t> propids = Game::getGame()->getDesignStore()->getPropertyIds();
  frame->unpackInt(); //seqnum
  uint32_t snum = frame->unpackInt();
  uint32_t numtoget = frame->unpackInt();
  uint64_t fromtime = UINT64_NEG_ONE;
  if(frame->getVersion() >= fv0_4){
    fromtime = frame->unpackInt64();
  }
  
  DesignStore *ds = Game::getGame()->getDesignStore();
  
  std::map<uint32_t, uint64_t> modlist;
  for(std::set<uint32_t>::iterator itcurr = propids.begin();
      itcurr != propids.end(); ++itcurr){
    Property * prop = ds->getProperty(*itcurr);
    if(fromtime == UINT64_NEG_ONE || prop->getModTime() > fromtime){
      modlist[*itcurr] = prop->getModTime();
    }
  }
  
  if(snum > modlist.size()){
    Logger::getLogger()->debug("Starting number too high, snum = %d, size = %d", snum, modlist.size());
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_NonExistant, "Starting number too high");
    curConnection->sendFrame(of);
    return;
  }
  if(numtoget > modlist.size() - snum){
    numtoget = modlist.size() - snum;
  }
  
  if(numtoget > MAX_ID_LIST_SIZE + ((frame->getVersion() < fv0_4)? 1 : 0)){
    Logger::getLogger()->debug("Number of items to get too high, numtoget = %d", numtoget);
    Frame *of = curConnection->createFrame(frame);
    of->createFailFrame(fec_FrameError, "Too many items to get, frame too big");
    curConnection->sendFrame(of);
    return;
  }
    
  Frame *of = curConnection->createFrame(frame);
  of->setType(ft03_PropertyIds_List);
  of->packInt(0);
  of->packInt(modlist.size() - snum - numtoget);
  of->packInt(numtoget);
  std::map<uint32_t, uint64_t>::iterator itcurr = modlist.begin();
  std::advance(itcurr, snum);
  for(uint32_t i = 0; i < numtoget; i++, ++itcurr){
    of->packInt(itcurr->first);
    of->packInt64(itcurr->second);
  }
  
  if(of->getVersion() >= fv0_4){
    of->packInt64(fromtime);
  }
 
  curConnection->sendFrame(of);
}

void PlayerAgent::processTurnFinished(Frame* frame){
 Logger::getLogger()->debug("doing Done Turn frame");
  
  if(frame->getVersion() < fv0_4){
    Logger::getLogger()->debug("Turn Finished Frame: protocol version not high enough, but continuing anyway");
//     Frame *of = curConnection->createFrame(frame);
//     of->createFailFrame(fec_FrameError, "Finished Turn frame isn't supported in this protocol");
//     curConnection->sendFrame(of);
//     return;
  }
  
  Game::getGame()->getTurnTimer()->playerFinishedTurn(player->getID());
  
  Frame *of = curConnection->createFrame(frame);
  of->setType(ft02_OK);
  of->packString("Thanks for letting me know you have finished your turn.");
  curConnection->sendFrame(of);
  
}
