#include "aquila/global.h"
#include "aquila/source/WaveFile.h"
#include <algorithm>
#include <utility>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <time.h>
#include <chrono>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "Shape.h"

using namespace std;
using namespace Eigen;
using namespace std::chrono;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the resources are loaded from
shared_ptr<Program> prog;
shared_ptr<Shape> shape;

milliseconds lastTime;

double interval;

int g_width, g_height;

Aquila::SampleType maxValue = 0, minValue = 0, average = 0, aboveLimit = 0;

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

   // Set background color.
   glClearColor(0.5f, 0.5f, 1.0f, 1.0f);
   // Enable z-buffer test.
   glEnable(GL_DEPTH_TEST);

   // Initialize mesh.
   shape = make_shared<Shape>();
   shape->loadMesh(RESOURCE_DIR + "sphere.obj");
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
   lastTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

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

static void render(Aquila::WaveFile wav) {
   int width, height;
   double diffTime;
   static int sampleNum = 0;
   std::pair<double,double> from(-30, 30), to(0, 3);
   double scale = mapRange(from, to, wav.sample(0));

   milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

   //std::cout << "ms " << ms.count() - lastTime.count() << " interval "  << interval << std::endl;
   diffTime = ms.count() - lastTime.count();
   if (diffTime >= interval) {
      lastTime = ms;
      sampleNum++;
      scale = mapRange(from, to, wav.sample(sampleNum));

      std::cout << "sampleNum " << sampleNum << " value " << wav.sample(sampleNum) << std::endl;
      //std::cout << "max " << maxValue << " min " << minValue << " cur " << wav.sample(sampleNum) << std::endl;
   }
   //std::cout << "scale " << scale << std::endl;

   glfwGetFramebufferSize(window, &width, &height);
   glViewport(0, 0, width, height);

   //Clear framebuffer.
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   //Use the local matrices for lab 5
   float aspect = width/(float)height;
   auto P = make_shared<MatrixStack>();
   auto MV = make_shared<MatrixStack>();
   //Apply perspective projection.
   P->pushMatrix();
   P->perspective(45.0f, aspect, 0.01f, 100.0f);

   //Draw mesh using GLSL.
   MV->pushMatrix();
   MV->loadIdentity();
   MV->translate(Vector3f(0, 0, -10));
   MV->translate(Vector3f(0, scale, 0));
   //MV->scale(Vector3f(scale, scale, scale));
   prog->bind();
   glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, P->topMatrix().data());
   glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, MV->topMatrix().data());
   shape->draw(prog);
   prog->unbind();
}

void PlayMusic(Aquila::WaveFile wav) {
   if(system(NULL)) {
      std::ostringstream stringStream;
      stringStream << "canberra-gtk-play -f " << wav.getFilename();
      const std::string tmp =  stringStream.str();
      std::cerr << "Attempting: " << tmp.c_str();
      system(tmp.c_str());
   } else {
      std::cout << "Cannot Play audio won't continue" << std::endl;
   }
}

int main(int argc, char *argv[]) {
   g_width = 640;
   g_height = 480;
   if (argc < 3) {
      std::cout << "Usage: visualizer <FILENAME> <RESOURCEDIR>" << std::endl;
      return 1;
   }
   RESOURCE_DIR = argv[2] + string("/");
   g_width = 640;
   g_height = 480;

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

   Aquila::WaveFile wav(argv[1]);
   std::cout << "Loaded file: " << wav.getFilename()
      << " (" << wav.getBitsPerSample() << "b)" << std::endl;
   //Aquila::SampleType maxValue = 0, minValue = 0, average = 0;
   std::thread sound (PlayMusic, wav);

   // simple index-based iteration
   double length = wav.getAudioLength();
   interval = wav.getSamplesCount() / length;
   //printf("Ms per sample %lf\n samples %u\n ms %lf\n", wav.getSamplesCount() / length,
   //      wav.getSamplesCount(), length);

   for (std::size_t i = 0; i < wav.getSamplesCount(); ++i)
   {
      //if (i < 300)
      //   std::cout << "Maximum sample value: " << wav.sample(i) << std::endl;
      if (wav.sample(i) > maxValue)
         maxValue = wav.sample(i);
   }
   std::cout << "Maximum sample value: " << maxValue << std::endl;

   // iterator usage
   for (auto it = wav.begin(); it != wav.end(); ++it)
   {
      if (*it < minValue)
         minValue = *it;
   }
   std::cout << "Minimum sample value: " << minValue << std::endl;

   // range-based for loop
   for (auto sample : wav)
   {
      average += sample;
   }
   average /= wav.getSamplesCount();
   std::cout << "Average: " << average << std::endl;

   // STL algorithms work too, so the previous examples could be rewritten
   // using max/min_element.
   int limit = 5000;
   int aboveLimit = std::count_if(
         wav.begin(),
         wav.end(),
         [limit] (Aquila::SampleType sample) {
         return sample > limit;
         }
         );
   std::cout << "There are " << aboveLimit << " samples greater than "
      << limit << std::endl;

   init();
   // Loop until the user closes the window.
   while(!glfwWindowShouldClose(window)) {
      render(wav);
      // Swap front and back buffers.
      glfwSwapBuffers(window);
      // Poll for and process events.
      glfwPollEvents();
   }
   // Quit program.
   glfwDestroyWindow(window);
   glfwTerminate();
   sound.join();
   std::cout << "Song done" << std::endl;
   return 0;
}
