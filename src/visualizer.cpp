#include "aquila/global.h"
#include "aquila/source/WaveFile.h"
#include <algorithm>
#include <exception>
#include <utility>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <time.h>
#include <chrono>
#include <limits>
#include <float.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "Shape.h"
#include "Texture.h"
#include "Particle.h"

using namespace std;
using namespace Eigen;
using namespace std::chrono;

float randfloat(float l, float h)
{
   float r = rand() / (float)RAND_MAX;
   return (1.0f - r) * l + r * h;
}

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the resources are loaded from
shared_ptr<Program> prog;
shared_ptr<Program> progP;
shared_ptr<Program> progPh;
shared_ptr<Shape> shape;
bool play = false;
float lightPos[3] = {-0.5f, 0.0f, 1.0f};
float lightColor[3] = {0.4f, 0.1f, 0.6f};
Eigen::Vector3f eyePos(0,0,0);
Eigen::Vector3f lookAtPos(0,0,-5);
Eigen::Vector3f up(0, 1 ,0);

//information for a each audio file
std::vector<Aquila::WaveFile> wavs;
std::vector<std::pair<double,double>> ranges;
std::vector<milliseconds> lastTime;
std::vector<double> interval;

int g_width, g_height;
float camRot;

//Aquila::SampleType maxValue = 0, minValue = 0, average = 0, aboveLimit = 0;
vector<shared_ptr<Particle>> particles;
int numP = 300;
static GLfloat points[900];
static GLfloat pointColors[1200];
GLuint pointsbuffer;
GLuint colorbuffer;

//data associated with storing the geometry in a vertex array object
GLuint VertexArrayID;

// OpenGL handle to texture data
Texture texture;
GLint h_texture0;

bool keyToggles[256] = {false};
// Display time to control fps
float t0_disp = 0.0f;
float t_disp = 0.0f;

float t = 0.0f; //reset in init
float h = 0.0f;
Vector3f g(0.0f, -0.1f, 0.0f);

static void error_callback(int error, const char *description) {
   cerr << description << endl;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
   if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, GL_TRUE);
   }
}

static void resize_callback(GLFWwindow *window, int width, int height) {
   g_width = width;
   g_height = height;
   glViewport(0, 0, width, height);
}

static void init() {
   GLSL::checkVersion();

   t = 0.0f;
   h = 0.01f;
   g = Vector3f(0.0f, -0.01f, 0.0f);
   // Set background color.
   glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
   // Enable z-buffer test.
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glPointSize(14.0f);

   texture.setFilename(RESOURCE_DIR + "alpha.bmp");
   //////////////////////////////////////////////////////
   // Intialize textures
   //////////////////////////////////////////////////////
   texture.setUnit(0);
   texture.setName("alphaTexture");
   texture.init();

   // Initialize the GLSL program.
   progP = make_shared<Program>();
   progP->setVerbose(true);
   progP->setShaderNames(RESOURCE_DIR + "particle_vert.glsl", RESOURCE_DIR + "particle_frag.glsl");
   progP->init();
   progP->addUniform("P");
   progP->addUniform("MV");
   progP->addAttribute("vertPos");
   progP->addAttribute("Pcolor");
   progP->addTexture(&texture);
   glEnable(GL_DEPTH_TEST);

   // Initialize mesh.
   shape = make_shared<Shape>();
   shape->loadMesh(RESOURCE_DIR + "round.obj");
   shape->resize();
   shape->init();

   prog = make_shared<Program>();
   prog->setVerbose(true);
   prog->setShaderNames(RESOURCE_DIR + "simple_vert.glsl", RESOURCE_DIR + "simple_frag.glsl");
   prog->init();
   prog->addUniform("P");
   prog->addUniform("MV");
   prog->addAttribute("vertPos");
   prog->addAttribute("vertNor");
   prog->addAttribute("vertTex");

   progPh = make_shared<Program>();
   progPh->setVerbose(true);
   progPh->setShaderNames(RESOURCE_DIR + "phong_vert.glsl", RESOURCE_DIR + "phong_frag.glsl");
   progPh->init();
   progPh->addUniform("ViewMatrix");
   progPh->addUniform("ModelMatrix");
   progPh->addUniform("P");
   progPh->addUniform("MatAmb");
   progPh->addUniform("MatDif");
   progPh->addUniform("NColor");
   progPh->addUniform("ShadingOp");
   progPh->addUniform("lightPos");
   progPh->addUniform("lightColor");
   progPh->addUniform("specular");
   progPh->addUniform("shine");
   progPh->addAttribute("vertPos");
   progPh->addAttribute("vertNor");
   progPh->addAttribute("vertTex");
   progPh->addAttribute("vertPos");
   progPh->addAttribute("vertNor");
   progPh->addAttribute("vertTex");

   //////////////////////////////////////////////////////
   // Initialize the particles
   //////////////////////////////////////////////////////
   // set up particles
   int n = numP;
   for(int i = 0; i < n; ++i) {
      auto particle = make_shared<Particle>();
      particles.push_back(particle);
      particle->load();
   }
}

//set up the particles - note at first they have no location - set
//when reborn
void initGeom() {

   //generate the VAO
   glGenVertexArrays(1, &VertexArrayID);
   glBindVertexArray(VertexArrayID);

   //generate vertex buffer to hand off to OGL - using instancing
   glGenBuffers(1, &pointsbuffer);
   //set the current state to focus on our vertex buffer
   glBindBuffer(GL_ARRAY_BUFFER, pointsbuffer);
   //actually memcopy the data - only do this once
   glBufferData(GL_ARRAY_BUFFER, sizeof(points), NULL, GL_STREAM_DRAW);

   glGenBuffers(1, &colorbuffer);
   //set the current state to focus on our vertex buffer
   glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
   //actually memcopy the data - only do this once
   glBufferData(GL_ARRAY_BUFFER, sizeof(pointColors), NULL, GL_STREAM_DRAW);

   assert(glGetError() == GL_NO_ERROR);

}

//Note you could add scale later for each particle - not implemented
void updateGeom() {
   Eigen::Vector3f pos;
   Eigen::Vector4f col;

   //go through all the particles and update the CPU buffer
   for (int i = 0; i < numP; i++) {
      pos = particles[i]->getPosition();
      col = particles[i]->getColor();
      points[i*3+0] =pos(0);
      points[i*3+1] =pos(1);
      points[i*3+2] =pos(2);
      pointColors[i*4+0] =col(0) + col(0)/10;
      pointColors[i*4+1] =col(1) + col(1)/10;
      pointColors[i*4+2] =col(2) + col(2)/10;
      pointColors[i*4+3] =col(3);
   }

   //update the GPU data
   glBindBuffer(GL_ARRAY_BUFFER, pointsbuffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(points), NULL, GL_STREAM_DRAW);
   glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*numP*3, points);
   glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(pointColors), NULL, GL_STREAM_DRAW);
   glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*numP*4, pointColors);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Sort particles by their z values in camera space
class ParticleSorter {
   public:
      bool operator()(const shared_ptr<Particle> p0, const shared_ptr<Particle> p1) const
      {
         // Particle positions in world space
         const Vector3f &x0 = p0->getPosition();
         const Vector3f &x1 = p1->getPosition();
         // Particle positions in camera space
         Vector4f x0w = C * Vector4f(x0(0), x0(1), x0(2), 1.0f);
         Vector4f x1w = C * Vector4f(x1(0), x1(1), x1(2), 1.0f);
         return x0w.z() < x1w.z();
      }

      Eigen::Matrix4f C; // current camera matrix
};
ParticleSorter sorter;

/* note for first update all particles should be "reborn"
   which will initialize their positions */
void updateParticles() {

   //update the particles
   for(auto particle : particles) {
      particle->update(t, h, g, keyToggles);
   }
   t += h;

   // Sort the particles by Z
   auto temp = make_shared<MatrixStack>();
   temp->rotate(camRot, Vector3f(0, 1, 0));
   sorter.C = temp->topMatrix();
   sort(particles.begin(), particles.end(), sorter);
}

template<typename tVal>
tVal mapRange(std::pair<tVal,tVal> a, std::pair<tVal, tVal> b, tVal inVal) {
   tVal inValNorm = inVal - a.first;
   tVal aUpperNorm = a.second - a.first;
   tVal normPosition = inValNorm / aUpperNorm;

   tVal bUpperNorm = b.second - b.first;
   tVal bValNorm = normPosition * bUpperNorm;
   tVal outVal = b.first + bValNorm;

   return outVal;
}

static void render() {
   glfwGetFramebufferSize(window, &g_width, &g_height);
   glViewport(0, 0, g_width, g_height);

   //Clear framebuffer.
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   double diffTime;// scale_temp;
   int sampleNum = 0;
   //std::pair<double,double> from(0, maxValue), to(1, 2.5);
   double scale;
   static double x[4] = {randfloat(-2.0f, 2.0f), randfloat(-2.0f, 2.0f), randfloat(-2.0f, 2.0f),randfloat(-2.0f, 2.0f)},
                 y[4] = {randfloat(-2.0f, 2.0f), randfloat(-2.0f, 2.0f), randfloat(-2.0f, 2.0f),randfloat(-2.0f, 2.0f)};

   float aspect = g_width/(float)g_height;
   auto P = make_shared<MatrixStack>();
   auto MV = make_shared<MatrixStack>();
   //Apply perspective projection.
   P->pushMatrix();
   P->perspective(45.0f, aspect, 0.01f, 100.0f);
   auto M = make_shared<MatrixStack>();
   auto V = make_shared<MatrixStack>();
   V->pushMatrix();
   V->lookAt(eyePos, lookAtPos, up);

   size_t i = 0;
   size_t done = 0;
   while (i < wavs.size()) {
      milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
      diffTime = ms.count() - lastTime.at(i).count();
      sampleNum = diffTime*interval.at(i);
      if ((size_t)sampleNum >= wavs.at(i).getSamplesCount()) {
         done++;
         if (done == wavs.size()) {
            glfwSetWindowShouldClose(window, GL_TRUE);
            return;
         }

      }
      std::vector<Aquila::SampleType> v = {wavs.at(i).sample(sampleNum)};
      Aquila::SignalSource src(v, wavs.at(i).getSampleFrequency());
      scale = mapRange(ranges.at(i), std::pair<double,double>(1, 1.5), Aquila::power(src));
      std::cout << "interval " << interval.at(i) << " scale " << scale << std::endl;
      //draw!!
      progPh->bind();
      M->pushMatrix();
      M->loadIdentity();
      M->translate(Vector3f(x[i], y[i], -5));
      M->scale(Vector3f(scale, scale, scale));
      glUniformMatrix4fv(progPh->getUniform("P"), 1, GL_FALSE, P->topMatrix().data());
      glUniformMatrix4fv(progPh->getUniform("ViewMatrix"), 1, GL_FALSE, V->topMatrix().data());

      glUniform3f(progPh->getUniform("MatAmb"), 0.3294, 0.2235, 0.02745);
      glUniform3f(progPh->getUniform("MatDif"), 0.7804, 0.5686, 0.11373);
      glUniform3f(progPh->getUniform("specular"), 0.9922, 0.941176, 0.80784);
      glUniform1f(progPh->getUniform("shine"), 27.9);
      glUniformMatrix4fv(progPh->getUniform("ModelMatrix"), 1, GL_FALSE, M->topMatrix().data());
      glUniform1f(progPh->getUniform("NColor"), 0);
      glUniform3fv(progPh->getUniform("lightPos"), 1, lightPos);
      glUniform3fv(progPh->getUniform("lightColor"), 1, lightColor);
      shape->draw(progPh);
      M->popMatrix();
      progPh->unbind();
      i++;
      P->popMatrix();
      V->popMatrix();
   }
   ///diff = scale_temp - scale;

   //if (diff > .3 || diff < -.3) {
   //   scale = scale_temp - (scale_temp *.2);
   //} else {
   //   scale = scale_temp;
   //}

   //std::cout << "scale? " << scale << std::endl;
   //std::cout << " wave " << wav.getFilename() << std::endl;

   //Cukk
   //std::cout << "sampleNum " << sampleNum << " value " << (wav.sample(sampleNum)).getSampleFrequency() << std::endl;
   //std::cout << "max " << maxValue << " min " << minValue << " cur " << wav.sample(sampleNum) << std::endl;



   //Draw mesh using GLSL.
   //MV->pushMatrix();
   //MV->loadIdentity();
   //MV->translate(Vector3f(-2, 2, -10));
   //MV->scale(Vector3f(scale, scale, scale));
   //prog->bind();
   //glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, P->topMatrix().data());
   //glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, MV->topMatrix().data());
   //shape->draw(prog);
   //prog->unbind();

   //scale = mapRange(from, to, Aquila::energy(src));
   //MV->pushMatrix();
   //MV->loadIdentity();
   //MV->translate(Vector3f(2, 2, -10));
   //MV->scale(Vector3f(scale, scale, scale));
   //prog->bind();
   //glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, P->topMatrix().data());
   //glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, MV->topMatrix().data());
   //shape->draw(prog);
   //prog->unbind();

   //scale = mapRange(from, to, Aquila::power(src));
   ////std::cout << "scale " << scale << std::endl;
   //MV->pushMatrix();
   //MV->loadIdentity();
   //MV->translate(Vector3f(-2, -2, -10));
   //MV->scale(Vector3f(scale, scale, scale));
   //prog->bind();
   //glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, P->topMatrix().data());
   //glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, MV->topMatrix().data());
   //shape->draw(prog);
   //prog->unbind();

   //scale = mapRange(from, to, Aquila::energy(src));
   //MV->pushMatrix();
   //MV->loadIdentity();
   //MV->translate(Vector3f(2, -2, -10));
   //MV->scale(Vector3f(scale, scale, scale));
   //prog->bind();
   //glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, P->topMatrix().data());
   //glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, MV->topMatrix().data());
   //shape->draw(prog);
   //prog->unbind();


   //particles!
   updateParticles();
   updateGeom();
   // Apply perspective projection.
   P->pushMatrix();
   P->perspective(45.0f, aspect, 0.01f, 100.0f);
   MV->loadIdentity();
   //camera rotate
   MV->rotate(camRot, Vector3f(0, 1, 0));

   // Draw
   progP->bind();
   glUniformMatrix4fv(progP->getUniform("P"), 1, GL_FALSE, P->topMatrix().data());

   glUniformMatrix4fv(progP->getUniform("MV"), 1, GL_FALSE, MV->topMatrix().data());

   glEnableVertexAttribArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, pointsbuffer);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0,(void*)0);

   glEnableVertexAttribArray(1);
   glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
   glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0,(void*)0);

   glVertexAttribDivisor(0, 1);
   glVertexAttribDivisor(1, 1);
   // Draw the points !
   //std::cout << "sample num " << sampleNum << " Num samples " << wav.getSamplesCount() << std::endl;
   glDrawArraysInstanced(GL_POINTS, 0, 1, numP);
   //std::cout << "sample num " << sampleNum << " Num samples " << wav.getSamplesCount() << std::endl;

   glVertexAttribDivisor(0, 0);
   glVertexAttribDivisor(1, 0);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
   progP->unbind();
   // Pop matrix stacks.
   P->popMatrix();

}

void PlayMusic(Aquila::WaveFile wav) {
   while(!play)
      ;
   if(system(NULL)) {
      std::ostringstream stringStream;
      stringStream << "canberra-gtk-play -f " << wav.getFilename();
      const std::string tmp =  stringStream.str();
      //std::cerr << "Attempting: " << tmp.c_str();
      system(tmp.c_str());
   } else {
      std::cout << "Cannot Play audio won't continue" << std::endl;
   }
}

int main(int argc, char *argv[]) {
   g_width = 640;
   g_height = 480;
   if (argc < 3) {
      std::cout << "Usage: visualizer <FILENAME> ... <RESOURCEDIR>" << std::endl;
      if (argc < 2) {
         std::cout << "Going in with " << std::endl;
         RESOURCE_DIR = argv[2] + string("/");
      }
      return 1;
   } else {
      RESOURCE_DIR = argv[argc-1] + string("/");
   }
   g_width = 640;
   g_height = 480;
   for (int i = 0; argc > 2; i++, argc--) {
      wavs.push_back(Aquila::WaveFile(argv[i+1]));
   }
   int i = 0;
   //set up audio information
   for (Aquila::WaveFile wav : wavs) {
      std::cout << "Loaded file: " << wav.getFilename() << " (" << wav.getBitsPerSample() << "b)" << std::endl;
      std::cout << wav.getSamplesCount() << " samples at " << wav.getAudioLength() << " ms" << std::endl;

      Aquila::SampleType maxValue = 0, minValue = 0;

      interval.push_back((double)wav.getSamplesCount() / (double)wav.getAudioLength());

      for (std::size_t i = 0; i < wav.getSamplesCount(); ++i) {
         std::vector<Aquila::SampleType> v = {wav.sample(i)};
         Aquila::SignalSource src(v, wav.getSampleFrequency());
         if (Aquila::power(src) > maxValue)
            maxValue = Aquila::power(src);
         if (Aquila::power(src) < minValue)
            minValue = Aquila::power(src);
      }
      ranges.push_back(std::pair<double,double>(minValue, maxValue));
      i++;
   }
   /* your main will always include a similar set up to establish your window
      and GL context, etc. */

   // Set error callback as openGL will report errors but they need a call back
   glfwSetErrorCallback(error_callback);
   // Initialize the library.
   if(!glfwInit()) {
      return -1;
   }
   //request the highest possible version of OGL - important for mac
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

   // Create a windowed mode window and its OpenGL context.
   window = glfwCreateWindow(g_width, g_height, "textures", NULL, NULL);
   if(!window) {
      glfwTerminate();
      return -1;
   }
   // Make the window's context current.
   glfwMakeContextCurrent(window);
   // Initialize GLEW.
   glewExperimental = true;
   if(glewInit() != GLEW_OK) {
      cerr << "Failed to initialize GLEW" << endl;
      return -1;
   }

   glGetError();
   cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
   cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

   // Set vsync.
   glfwSwapInterval(1);
   // Set keyboard callback.
   glfwSetKeyCallback(window, key_callback);
   //set the window resize call back
   glfwSetFramebufferSizeCallback(window, resize_callback);


   init();
   initGeom();
   std::vector<std::thread> sounds;
   for (Aquila::WaveFile wav : wavs) {
      sounds.push_back(std::thread(PlayMusic, wav));
   }
   //make these two events happen as close as possible
   lastTime.push_back(duration_cast<milliseconds>(system_clock::now().time_since_epoch()));
   play = true;
   for (size_t i = 1; i < wavs.size(); i++) {
      lastTime.push_back(lastTime.at(0));
   }
   // Loop until the user closes the window.
   while(!glfwWindowShouldClose(window)) {
      render();
      glfwSwapBuffers(window);
      // Poll for and process events.
      glfwPollEvents();
   }
   // Quit program.
   glfwDestroyWindow(window);
   glfwTerminate();
   std::cout << "^C to stop music" << std::endl;
   for (size_t i = 0; i < sounds.size(); i++) {
      sounds.at(i).join();
      std::cout << "song " << i + 1 << " stopped" << std::endl;
   }
   std::cout << "Music done" << std::endl;
   return 0;
}
