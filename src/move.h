#ifndef MOVE_H
#define MOVE_H

#include "order.h"
#include "vector3d.h"

class Move : public Order{
      public:
	Move();
	virtual ~Move();

	Vector3d getDest() const;
	void setDest(const Vector3d & ndest);

	int getETA(IGObject* ob) const;

	void createFrame(Frame * f, int objID, int pos);
	bool inputFrame(Frame * f);

	bool doOrder(IGObject * ob);

	void describeOrder(Frame * f) const;
	Order* clone() const;

      private:
	Vector3d dest;


};

#endif
