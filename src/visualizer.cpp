#include "aquila/global.h"
#include "aquila/source/WaveFile.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <time.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "Shape.h"

using namespace std;
using namespace Eigen;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the resources are loaded from
int g_width, g_height;

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
   if (argc < 3)
   {
      std::cout << "Usage: wave_iteration <FILENAME> <RESOURCEDIR>" << std::endl;
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
   Aquila::SampleType maxValue = 0, minValue = 0, average = 0;
   std::thread sound (PlayMusic, wav);

   // simple index-based iteration
   double length = wav.getAudioLength();
   printf("Ms per sample %lf\n samples %d\n ms %d\n", length / wav.getSamplesCount(),
         wav.getSamplesCount(), length);

   for (std::size_t i = 0; i < wav.getSamplesCount(); ++i)
   {
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
   std::cout << "Minimum samplel value: " << minValue << std::endl;

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

   // Loop until the user closes the window.
   while(!glfwWindowShouldClose(window)) {
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
