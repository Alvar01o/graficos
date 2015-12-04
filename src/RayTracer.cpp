// The main ray tracer.

#pragma warning (disable: 4786)

#include "RayTracer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"

#include "parser/Tokenizer.h"
#include "parser/Parser.h"

#include "ui/TraceUI.h"
#include <cmath>
#include <algorithm>

extern TraceUI* traceUI;

#include <iostream>
#include <fstream>

using namespace std;

// Use this variable to decide if you want to print out
// debugging messages.  Gets set in the "trace single ray" mode
// in TraceGLWindow, for example.
#define PI 3.141592653589793238462643383279502f
bool debugMode = false;
double indexOfAir = 1.0003;

// Trace a top-level ray through normalized window coordinates (x,y)
// through the projection plane, and out into the scene.  All we do is
// enter the main ray-tracing method, getting things started by plugging
// in an initial ray weight of (0.0,0.0,0.0) and an initial recursion depth of 0.
Vec3d RayTracer::trace( double x, double y )
{
	// Clear out the ray cache in the scene for debugging purposes,
	if (!traceUI->isMultithreading())
		scene->intersectCache.clear();

    ray r( Vec3d(0,0,0), Vec3d(0,0,0), ray::VISIBILITY );

    scene->getCamera().rayThrough( x,y,r );
	Vec3d ret = traceRay( r, Vec3d(1.0,1.0,1.0), 0 );
	ret.clamp();
	return ret;
}

Vec3d reflectDirection( isect i, const ray& r){
    Vec3d _d = -r.getDirection();
    double theta = _d*i.N;
    Vec3d reflectDir = -_d+2*i.N*theta;
    reflectDir.normalize();
    return reflectDir;
}

Vec3d refractDirection( isect i, const ray& ray, double n_i, double n_t){
    Vec3d n = i.N;
    Vec3d l = ray.getDirection();
    double r = n_i / n_t;
    double c = -n * l;
    //T = (I / n21) + ( ( (-I � N) / n21) - SQRT(1 + ( ( (-I � N)^2 - 1) / n21^2) ) ) * N
    return  r*l + (r*c - sqrt(1 - r*r*(1-c*c)))*n;
}

bool rayIsEnteringObject(isect i, const ray& r){
    Vec3d d = r.getDirection();
    if (-d*i.N > 0)
        return true; //entering
    if (d*i.N > 0)
        return false; //leaving
    return true;
}

bool notTIR(isect i, const ray& ray, double n_i, double n_t ){
    Vec3d n = i.N;
    Vec3d l = ray.getDirection();
    double r = n_i / n_t;
    double c = -n * l;
    double discriminante = 1 - r*r*(1-c*c);
    if (discriminante < 0){
         return false;
    } else {
        return true;
    }
}

// Do recursive ray tracing!  You'll want to insert a lot of code here
// (or places called from here) to handle reflection, refraction, etc etc.
Vec3d RayTracer::traceRay( const ray& r, 
	const Vec3d& thresh, int depth )
{
	isect i;
    Vec3d I = Vec3d( 0.0, 0.0, 0.0 );
    if( depth <= traceUI->getDepth() && scene->intersect( r, i ) ) {
		// YOUR CODE HERE

		// An intersection occured!  We've got work to do.  For now,
		// this code gets the material for the surface that was intersected,
		// and asks that material to provide a color for the ray.  

		// This is a great place to insert code for recursive ray tracing.
		// Instead of just returning the result of shade(), add some
		// more steps: add in the contributions from reflected and refracted
		// rays.
		
		const Material& m = i.getMaterial();
        I = m.shade(scene, r, i);
	Vec3d D_ = Vec3d(0,0,0);
	//shadow
	for ( vector<Light*>::const_iterator litr = scene->beginLights(); 
        litr != scene->endLights(); 
        ++litr )
    {
        Light* pLight = *litr;
		Vec3d R_ = -pLight->getDirection(r.at(i.t) );
		ray newRayShaDow( r.at(i.t), R_, ray::SHADOW );
		D_ += traceRay( newRayShaDow, Vec3d(1.0,1.0,1.0), depth+1);
	}
	if(D_!=Vec3d(0,0,0)){
		return 		Vec3d(0,0,0);
	}

		//reflection
        Vec3d R = reflectDirection(i,r);
        ray newRayReflect( r.at(i.t), R, ray::REFLECTION );
        I += prod(i.getMaterial().kr(i), traceRay( newRayReflect, Vec3d(1.0,1.0,1.0), depth+1)); //I=I + mtrl.kr*traceRay(scene, Q, R)   

		//refraction
        double n_i = 0;
        double n_t = 0;
        if (rayIsEnteringObject(i, r)){
            n_i = indexOfAir;
            n_t = i.getMaterial().index(i);
        } else {
            n_i = i.getMaterial().index(i);
            n_t = indexOfAir;
        }
        if (notTIR(i, r, n_i, n_t)){
            Vec3d T = refractDirection(i, r, n_i, n_t);
            ray newRayRefract( r.at(i.t), T, ray::REFRACTION );
            Vec3d temporal = traceRay(newRayRefract, Vec3d(1.0,1.0,1.0), depth+1);
            I += prod(i.getMaterial().kt(i), temporal);
        }
        return I;
	
	} else {
		// No intersection.  This ray travels to infinity, so we color
		// it according to the background color, which in this (simple) case
		// is just black.

		return Vec3d( 0.0, 0.0, 0.0 );
	}
}

RayTracer::RayTracer()
	: scene( 0 ), buffer( 0 ), buffer_width( 0 ), buffer_height( 0 ), m_bBufferReady( false )
{
}


RayTracer::~RayTracer()
{
	delete scene;
	delete [] buffer;
}

void RayTracer::getBuffer( unsigned char *&buf, int &w, int &h )
{
	buf = buffer;
	w = buffer_width;
	h = buffer_height;
}

double RayTracer::aspectRatio()
{
	return sceneLoaded() ? scene->getCamera().getAspectRatio() : 1;
}

bool RayTracer::loadScene( const char* fn )
{
	ifstream ifs( fn );
	if( !ifs ) {
		string msg( "Error: couldn't read scene file " );
		msg.append( fn );
		traceUI->alert( msg );
		return false;
	}
	
	// Strip off filename, leaving only the path:
	string path( fn );
	if( path.find_last_of( "\\/" ) == string::npos )
		path = ".";
	else
		path = path.substr(0, path.find_last_of( "\\/" ));

	// Call this with 'true' for debug output from the tokenizer
	Tokenizer tokenizer( ifs, false );
    Parser parser( tokenizer, path );
	try {
		delete scene;
		scene = 0;
		scene = parser.parseScene();
	} 
	catch( SyntaxErrorException& pe ) {
		traceUI->alert( pe.formattedMessage() );
		return false;
	}
	catch( ParserException& pe ) {
		string msg( "Parser: fatal exception " );
		msg.append( pe.message() );
		traceUI->alert( msg );
		return false;
	}
	catch( TextureMapException e ) {
		string msg( "Texture mapping exception: " );
		msg.append( e.message() );
		traceUI->alert( msg );
		return false;
	}


	if( ! sceneLoaded() )
		return false;


	// Initialize the scene's BSP tree
	scene->initBSPTree();

	
	return true;
}

void RayTracer::traceSetup( int w, int h )
{
	if( buffer_width != w || buffer_height != h )
	{
		buffer_width = w;
		buffer_height = h;

		bufferSize = buffer_width * buffer_height * 3;
		delete [] buffer;
		buffer = new unsigned char[ bufferSize ];

	}
	memset( buffer, 0, w*h*3 );
	m_bBufferReady = true;
}

void RayTracer::tracePixel( int i, int j )
{
	Vec3d col;

	if( ! sceneLoaded() )
		return;

	double x = double(i)/double(buffer_width);
	double y = double(j)/double(buffer_height);

	col = trace( x,y );
	unsigned char *pixel = buffer + ( i + j * buffer_width ) * 3;

	pixel[0] = (int)( 255.0 * col[0]);
	pixel[1] = (int)( 255.0 * col[1]);
	pixel[2] = (int)( 255.0 * col[2]);
}

