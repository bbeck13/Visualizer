//
// sueda - geometry edits Z. Wood
// 3/16
//

#include <iostream>
#include "Particle.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Texture.h"

using namespace std;
using namespace Eigen;

float randFloat(float l, float h)
{
   float r = rand() / (float)RAND_MAX;
   return (1.0f - r) * l + r * h;
}

Particle::Particle() :
   charge(1.0f),
   m(1.0f),
   d(0.0f),
   x(0.0f, 0.0f, 0.0f),
   v(0.0f, 0.0f, 0.0f),
   lifespan(1.0f),
   tEnd(0.0f),
   scale(1.0f),
   color(1.0f, 1.0f, 1.0f, 1.0f)
{
}

Particle::~Particle()
{
}

void Particle::load()
{
   // Random initialization
   Eigen::Vector3f clr(0, 0, 0);
   rebirth(0.0f, clr, 4);
}

/* all particles born at the origin */
void Particle::rebirth(float t, Eigen::Vector3f newColor, float speed) {
   charge = randFloat(0.0f, 1.0f) < 0.5 ? -1.0f : 1.0f;
   m = 1.0f;
   d = randFloat(0.0f, 0.02f);
   x(0) = 0;
   x(1) = 0;
   x(2) = 0;
   v(0) = randFloat(-0.3f, 0.3f);
   v(1) = randFloat(-0.3f, 0.3f);
   v(2) = randFloat(-0.3f, 0.3f);
   lifespan = randFloat(8.0f, 9.0f);
   tEnd = t + lifespan;
   scale = randFloat(0.2, 1.0f);
   color(0) = randFloat(newColor.x() -.1, newColor.x() + .1);
   color(1) = randFloat(newColor.y() -.1, newColor.y() + .1);
   color(2) = randFloat(newColor.z() -.1, newColor.z() + .1);
   color(3) = 1.0f;
}

void Particle::update(float t, float h, const Eigen::Vector3f &g, const bool *keyToggles, Eigen::Vector3f clr, float speed) {
   if(t > tEnd) {
      rebirth(t, clr, speed);
   }

   //very simple update
   x += h*v;
   color(3) = (tEnd-t+4)/lifespan;
}
