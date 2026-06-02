/*
 * AABB.cpp
 *
 *  Created on: 01-06-2017
 *      Author: ignacio
 */

#include "AABB.h"

using namespace std;

namespace clp {

AABB::AABB(long x1, long y1, long z1, long x2, long y2, long z2) : mins(x1,y1,z1), maxs(x2,y2,z2),
	volume((x2-x1) * (y2-y1) * (z2-z1)), block(NULL) {}

AABB::AABB(const Vector3& mins, const Block* b) : mins(mins),
			maxs(mins+*b), block(b), volume(block->getVolume()) {}

AABB::AABB(const Vector3& mins, const Vector3& maxs) : mins(mins), maxs(maxs), block(NULL),
		volume( (maxs.getX() - mins.getX())*(maxs.getY() - mins.getY())*(maxs.getZ() - mins.getZ()) ) { };

long AABB::getOccupiedVolume() const {return block->getOccupiedVolume();}

list<AABB> AABB::subtract(const AABB& b) const{
	const AABB& b1=*this;
	const AABB& b2=b;

	list<AABB> sub;

	long x1 = b1.getXmin();
	long x2 = b1.getXmax();
	long y1 = b1.getYmin();
	long y2 = b1.getYmax();
	long z1 = b1.getZmin();
	long z2 = b1.getZmax();

	long bx1 = b2.getXmin();
	long bx2 = b2.getXmax();
	long by1 = b2.getYmin();
	long by2 = b2.getYmax();
	long bz1 = b2.getZmin();
	long bz2 = b2.getZmax();

	// left slab
	if(bx1 > x1)
		sub.push_back(AABB(x1, y1, z1, bx1, y2, z2));

	// right slab
	if(bx2 < x2)
		sub.push_back(AABB(bx2, y1, z1, x2, y2, z2));

	long rx1 = max(x1, bx1);
	long rx2 = min(x2, bx2);

	// back slab
	if(by1 > y1 && rx2 > rx1)
		sub.push_back(AABB(rx1, y1, z1, rx2, by1, z2));

	// front slab
	if(by2 < y2 && rx2 > rx1)
		sub.push_back(AABB(rx1, by2, z1, rx2, y2, z2));

	long ry1 = max(y1, by1);
	long ry2 = min(y2, by2);

	// bottom slab
	if(bz1 > z1 && rx2 > rx1 && ry2 > ry1)
		sub.push_back(AABB(rx1, ry1, z1, rx2, ry2, bz1));

	// top slab
	if(bz2 < z2 && rx2 > rx1 && ry2 > ry1)
		sub.push_back(AABB(rx1, ry1, bz2, rx2, ry2, z2));

	return sub;
}



bool greater_volume(const AABB& a, const AABB& b) { return a.getVolume() > b.getVolume() ; }
}


