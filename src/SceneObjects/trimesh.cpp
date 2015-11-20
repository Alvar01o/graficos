#include <cmath>
#include <float.h>
#include "trimesh.h"

using namespace std;

Trimesh::~Trimesh()
{
	for( Materials::iterator i = materials.begin(); i != materials.end(); ++i )
		delete *i;
}

// must add vertices, normals, and materials IN ORDER
void Trimesh::addVertex( const Vec3d &v )
{
    vertices.push_back( v );
}

void Trimesh::addMaterial( Material *m )
{
    materials.push_back( m );
}

void Trimesh::addNormal( const Vec3d &n )
{
    normals.push_back( n );
}

// Returns false if the vertices a,b,c don't all exist
bool Trimesh::addFace( int a, int b, int c )
{
    int vcnt = vertices.size();

    if( a >= vcnt || b >= vcnt || c >= vcnt )
        return false;

    TrimeshFace *newFace = new TrimeshFace( scene, new Material(*this->material), this, a, b, c );
    newFace->setTransform(this->transform);
    faces.push_back( newFace );
    scene->add(newFace);
    return true;
}

char *
Trimesh::doubleCheck()
// Check to make sure that if we have per-vertex materials or normals
// they are the right number.
{
    if( !materials.empty() && materials.size() != vertices.size() )
        return "Bad Trimesh: Wrong number of materials.";
    if( !normals.empty() && normals.size() != vertices.size() )
        return "Bad Trimesh: Wrong number of normals.";

    return 0;
}

// Calculates and returns the normal of the triangle too.
bool TrimeshFace::intersectLocal( const ray& r, isect& i ) const
{
    // YOUR CODE HERE:
    // Add triangle intersection code here.
    // it currently ignores all triangles and just return false.
    //
    // Note that you are only intersecting a single triangle, and the vertices
    // of the triangle are supplied to you by the trimesh class.
    //
    // You should retrieve the vertices using code like this:
    //
	/*
		Based on http://www.inmensia.com/articulos/raytracing/planotrianguloycubo.html?pag=2
	*/
	//vertices
	const Vec3d& v1 = parent->vertices[ids[0]];
	const Vec3d& v2 = parent->vertices[ids[1]];
	const Vec3d& v3 = parent->vertices[ids[2]];

	//rayo proyectado
	Vec3d p = r.getPosition();
	Vec3d d = r.getDirection();

	Vec3d v2_v1 = (v2 - v1);
	Vec3d v3_v1 = (v3 - v1);

	Vec3d v2_v1_x_v3_v1 = v2_v1^v3_v1; //normal 

	Vec3d n = v2_v1_x_v3_v1;

	v2_v1_x_v3_v1.normalize();

	double d_ = - ((n[0]*v2[0]) + (n[1]*v2[1]) +(n[2]*v2[2]) );

	double denom = ((n[0]*d[0]) + (n[1]*d[1]) + (n[2]*d[2]));

	if(denom == 0 )return false;

	double t = - ((n[0] * p[0]) +(n[1] * p[1]) + (n[2] * p[2]) + d_) / denom;

	double x=p[0] + t*d[0];
	double y=p[1] + t*d[1];
	double z=p[2] + t*d[2];
	Vec3d intersect = Vec3d(x,y,z);

	//s1 = ( (V2 - V1) x (I - V1) ) · N
	Vec3d vc1 =((v2_v1)^(intersect-v1));
	double s1 = vc1*n;
	//s2 = ( (V3 - V2) x (I - V2) ) · N
	Vec3d vc2 =((v3-v2)^(intersect-v2));
	double s2 = vc2*n;
	//s3 = ( (V1 - V3) x (I - V3) ) · N
	Vec3d vc3 =((v1-v3)^(intersect-v3));
	double s3 = vc3*n;

	if((s1 > 0 && s2 > 0 && s3 > 0) || (s1 < 0 && s2 < 0 && s3 < 0)) {
		i.obj = this;
		i.N = v2_v1_x_v3_v1;
		i.t = t;
		return true;
	}
	return false;
}


void
Trimesh::generateNormals()
// Once you've loaded all the verts and faces, we can generate per
// vertex normals by averaging the normals of the neighboring faces.
{
    int cnt = vertices.size();
    normals.resize( cnt );
    int *numFaces = new int[ cnt ]; // the number of faces assoc. with each vertex
    memset( numFaces, 0, sizeof(int)*cnt );
    
    for( Faces::iterator fi = faces.begin(); fi != faces.end(); ++fi )
    {
        Vec3d a = vertices[(**fi)[0]];
        Vec3d b = vertices[(**fi)[1]];
        Vec3d c = vertices[(**fi)[2]];
        
        Vec3d faceNormal = ((b-a) ^ (c-a));
		faceNormal.normalize();
        
        for( int i = 0; i < 3; ++i )
        {
            normals[(**fi)[i]] += faceNormal;
            ++numFaces[(**fi)[i]];
        }
    }

    for( int i = 0; i < cnt; ++i )
    {
        if( numFaces[i] )
            normals[i]  /= numFaces[i];
    }

    delete [] numFaces;
}

