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
   rebirth(0.0f);
}

/* all particles born at the origin */
void Particle::rebirth(float t)
{
   charge = randFloat(0.0f, 1.0f) < 0.5 ? -1.0f : 1.0f;
   m = 1.0f;
   d = randFloat(0.0f, 0.02f);
   x(0) = 0;
   x(1) = .48;
   x(2) = -2;
   v(0) = randFloat(-0.3f, 0.3f);
   v(1) = randFloat(-0.3f, 0.3f);
   v(2) = randFloat(-0.3f, 0.3f);
   lifespan = randFloat(3.0f, 6.0f);
   tEnd = t + lifespan;
   scale = randFloat(0.2, 1.0f);
   color(0) = randFloat(0.2929f, 0.35f);
   color(1) = randFloat(0.20f, 0.24f);
   color(2) = randFloat(0.023f, 0.030f);
   color(3) = 1.0f;
}

void Particle::update(float t, float h, const Eigen::Vector3f &g, const bool *keyToggles)
{
   if(t > tEnd) {
      rebirth(t);
   }

   //very simple update
   x += h*v;
   //v(1) = v(1) - .003;
   color(3) = (tEnd-t)/lifespan;
}
