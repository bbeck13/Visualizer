//
// sueda
// November, 2014
//

#pragma once
#ifndef _PARTICLE_H_
#define _PARTICLE_H_

#include <vector>

#define EIGEN_DONT_ALIGN_STATICALLY
#include <Eigen/Dense>

#define GLEW_STATIC
#include <GL/glew.h>

class MatrixStack;
class Program;
class Texture;

class Particle
{
public:
	Particle();
	virtual ~Particle();
	void load();
	void rebirth(float t, Eigen::Vector3f color, float speed);
	void update(float t, float h, const Eigen::Vector3f &g, const bool *keyToggles, Eigen::Vector3f color, float speed);
	const Eigen::Vector3f &getPosition() const { return x; };
	const Eigen::Vector3f &getVelocity() const { return v; };
	const Eigen::Vector4f &getColor() const { return color; };

private:
	float charge; // +1 or -1
	float m; // mass
	float d; // viscous damping
	Eigen::Vector3f x; // position
	Eigen::Vector3f v; // velocity
	float lifespan; // how long this particle lives
	float tEnd;     // time this particle dies
	float scale;
	Eigen::Vector4f color;
};

#endif
