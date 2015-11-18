#include <cmath>
#include <stdio.h>
#include "Sphere.h"

using namespace std;


bool Sphere::intersectLocal( const ray& r, isect& i ) const
{
	double x0 = r.getPosition()[0];
	double y0 = r.getPosition()[1];
	double z0 = r.getPosition()[2];  

	double x1 = r.getDirection()[0];
	double y1 = r.getDirection()[1];
	double z1 = r.getDirection()[2];

	double a = x1*x1+y1*y1 + z1*z1;
	double b = 2.0*(x0*x1 + y0*y1 + z0*z1);
	double c = x0*x0 + y0*y0 + z0*z0 - 1.0;

	double discriminant = b*b - 4.0*a*c;

	if( discriminant < 0.0 ) {
		return false;
	}
	
	discriminant = sqrt( discriminant );

	double t2 = (-b + discriminant) / (2.0 * a);

	if( t2 <= RAY_EPSILON ) {
		return false;
	}

	double t1 = (-b - discriminant) / (2.0 * a);

	if( t1 > RAY_EPSILON ) {
		// Two intersections.
		Vec3d P = r.at( t1 );
		double z = P[2];
//		if( z >= 0.0 && z <= 1.0 ) {
			if(P[0] == 0 && P[1]==0 ){
				return false;
			}
			Vec3d normal( P[0], P[1],  z);
			i.obj = this;
			i.setUVCoordinates( Vec2d(1.0, 1.0) );		
			i.t = t1;
			i.N = normal;
			return true;
//		}
	}

	Vec3d P = r.at( t2 );
	if(P[0] == 0 && P[1]==0){
		return false;
	}
	double z = P[2];

	i.t = t2;//-(sqrt((P[0]*(P[0])) + (P[1]*(P[1]))))
	Vec3d normal( P[0], P[1], z );
	i.N = normal;
	i.obj = this;
	i.setUVCoordinates( Vec2d(1.0, 1.0) );	
	return true;
}

