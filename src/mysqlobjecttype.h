#ifndef MYSQLOBJECTTYPE_H
#define MYSQLOBJECTTYPE_H
/*  MysqlObjectType class
 *
 *  Copyright (C) 2005  Lee Begg and the Thousand Parsec Project
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

#include <stdint.h>

typedef struct st_mysql MYSQL;
class MysqlPersistence;
class IGObject;

class MysqlObjectType{
public:
    virtual ~MysqlObjectType();
    
    void setType(uint32_t nt);
    uint32_t getType() const;

    virtual bool save(MysqlPersistence* persistence, MYSQL* conn, IGObject* ob) = 0;
    virtual bool update(MysqlPersistence* persistence, MYSQL* conn, IGObject* ob) = 0;
    virtual bool retrieve(MYSQL* conn, IGObject* ob) = 0;
    virtual bool remove(MYSQL* conn, uint32_t obid) = 0;
    
    virtual void initialise(MysqlPersistence* persistence, MYSQL* conn) = 0;

protected:
    uint32_t type;
};

#endif