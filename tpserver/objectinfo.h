#ifndef OBJECTINFO_H
#define OBJECTINFO_H
/*  ObjectInfo class
 *
 *  Copyright (C) 2008  Lee Begg and the Thousand Parsec Project
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
#include <string>
#include <boost/shared_ptr.hpp>

class Frame;

class ObjectInfoData;

typedef boost::shared_ptr<ObjectInfoData> ObjectInfoPtr;

class ObjectInfoData {
  public:
    ObjectInfoData();
    ~ObjectInfoData();
    
    uint32_t getType() const;
    std::string getName() const;
    std::string getDescription() const;

    void setType(uint32_t nt);
    void setName(const std::string& nn);
    void setDescription(const std::string& nd);

    void packFrame(Frame* f);
    void unpackModFrame(Frame* f);
    
    uint64_t getModTime() const;
    bool isDirty() const;
    void setModTime(uint64_t nmt);
    void setIsDirty(bool id);
    
  private:
    void touchModTime();
    uint32_t type;
    std::string name;
    std::string desc;
    uint64_t modtime;
    bool dirty;

};

#endif
