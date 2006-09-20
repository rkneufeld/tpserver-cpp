#ifndef FLEET_H
#define FLEET_H
/*  Fleet Object class
 *
 *  Copyright (C) 2004-2005  Lee Begg and the Thousand Parsec Project
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

#include <tpserver/ownedobject.h>

class Fleet:public OwnedObject {
      public:
	Fleet();
	virtual ~Fleet();

	void addShips(int type, int number);
	bool removeShips(int type, int number);
	int numShips(int type);
	std::map<int, int> getShips() const;
	int totalShips() const;

	long long maxSpeed();
	unsigned int firepower( bool draw);
	bool hit( unsigned int firepower);

        int getDamage() const;
        void setDamage(int nd);

	void packExtraData(Frame * frame);

	void doOnceATurn(IGObject * obj);

	void packAllowedOrders(Frame * frame, int playerid);

	bool checkAllowedOrder(int ot, int playerid);

	int getContainerType();

	ObjectData* clone();

 protected:
    unsigned int getLargestShipType();
    void shipDestroyed( unsigned int type);

      private:
	std::map<int, int> ships;
	unsigned int damage;

};

#endif