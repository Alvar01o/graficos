#include "ray.h"
#include "material.h"
#include "light.h"

#include "../fileio/imageio.h"

using namespace std;
extern bool debugMode;

Vec3d controlColor(Vec3d color){
    if (color[0]<0 ){
        color[0] = 0;
    }
    if (color[1]<0){
        color[1] = 0;
    }
    if (color[2]<0){
        color[2] = 0;
    }
    if (color[0]>1.0 ){
        color[0] = 1;
    }
    if (color[1]>1.0){
        color[1] = 1;
    }
    if (color[2]>1.0){
        color[2] = 1;
    }
    return color;
}

// Apply the Phong model to this point on the surface of the object, returning
// the color of that point.
Vec3d Material::shade( Scene *scene, const ray& r, const isect& i ) const
{
	// YOUR CODE HERE

	// For now, this method just returns the diffuse color of the object.
	// This gives a single matte color for every distinct surface in the
	// scene, and that's it.  Simple, but enough to get you started.
	// (It's also inconsistent with the Phong model...)

	// Your mission is to fill in this method with the rest of the phong
	// shading model, including the contributions of all the light sources.
    // You will need to call both distanceAttenuation() and shadowAttenuation()
    // somewhere in your code in order to compute shadows and light falloff.
	if( debugMode )
		std::cout << "Debugging the Phong code (or lack thereof...)" << std::endl;

	// When you're iterating through the lights,
	// you'll want to use code that looks something
	// like this:
	//
   /* I = mtrl.ke + mtrl.ka * ILa
        for each light source Light do:
        atten = Light -> distanceAttenuation( )
        L = Light -> getDirection ( )
        I = I + atten*(diffuse term + specular term)
        end for
        return I
        Thus, the color on the surface will
vary according to the cosine of the angle between the surface normal and the
light direction. Note that the vector l is typically assumed not to depend on the
location of the object. That assumption is equivalent to assuming the light is
“distant” relative to object size. Such a “distant” light is often called a directional
light, because its position is specified only by a direction.
    */
    

    Vec3d Ie = Vec3d(0,0,0);
    Vec3d Ia = Vec3d(0,0,0);
    Vec3d temp = Vec3d(0,0,0);
    Vec3d I = Vec3d(0,0,0);
    Vec3d result = Vec3d(0,0,0);
    Vec3d Id;
    Vec3d Is;
    double cosD = 0.0;
    double cosS = 0.0;
    double shininess = i.getMaterial().shininess(i);
    Vec3d P = r.at(i.t);
	for ( vector<Light*>::const_iterator litr = scene->beginLights(); 
        litr != scene->endLights(); 
        ++litr )
    {
        Id = Vec3d(0,0,0);
        Is = Vec3d(0,0,0);
        Light* pLight = *litr;
        cosD = pLight->getDirection(P) * i.N;

        Vec3d dirLight = pLight->getDirection(P);
        Vec3d dirViewer = -r.getDirection();
        dirViewer.normalize();
        Vec3d h = dirLight + dirViewer;
        h.normalize();
        cosS = h*i.N;
        if ( cosD >= 0){
            Id = kd(i)*cosD;
        }
        if ( cosS >= 0){
            Is = ks(i)*pow(cosS, shininess);
        }
        double fatt = pLight->distanceAttenuation(P);
        I = prod((Id + Is),pLight->getColor(P)*fatt);
        temp += prod(I,pLight->shadowAttenuation(P)) ;
    }
    Ia =  prod(ka(i), scene->ambient());
    result = ke(i) + Ia + temp;
    result.clamp();
    return result;
}


TextureMap::TextureMap( string filename )
{
    data = load( filename.c_str(), width, height );
    if( 0 == data )
    {
        width = 0;
        height = 0;
        string error( "Unable to load texture map '" );
        error.append( filename );
        error.append( "'." );
        throw TextureMapException( error );
    }
}

Vec3d TextureMap::getMappedValue( const Vec2d& coord ) const
{
	// YOUR CODE HERE

    // In order to add texture mapping support to the 
    // raytracer, you need to implement this function.
    // What this function should do is convert from
    // parametric space which is the unit square
    // [0, 1] x [0, 1] in 2-space to Image coordinates,
    // and use these to perform bilinear interpolation
    // of the values.


    return Vec3d(1.0, 1.0, 1.0);
}


Vec3d TextureMap::getPixelAt( int x, int y ) const
{
    // This keeps it from crashing if it can't load
    // the texture, but the person tries to render anyway.
    if (0 == data)
      return Vec3d(1.0, 1.0, 1.0);

    if( x >= width )
       x = width - 1;
    if( y >= height )
       y = height - 1;

    // Find the position in the big data array...
    int pos = (y * width + x) * 3;
    return Vec3d( double(data[pos]) / 255.0, 
       double(data[pos+1]) / 255.0,
       double(data[pos+2]) / 255.0 );
}

Vec3d MaterialParameter::value( const isect& is ) const
{
    if( 0 != _textureMap )
        return _textureMap->getMappedValue( is.uvCoordinates );
    else
        return _value;
}

double MaterialParameter::intensityValue( const isect& is ) const
{
    if( 0 != _textureMap )
    {
        Vec3d value( _textureMap->getMappedValue( is.uvCoordinates ) );
        return (0.299 * value[0]) + (0.587 * value[1]) + (0.114 * value[2]);
    }
    else
        return (0.299 * _value[0]) + (0.587 * _value[1]) + (0.114 * _value[2]);
}

