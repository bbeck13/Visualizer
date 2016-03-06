#include "aquila/global.h"
#include "aquila/source/WaveFile.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <thread>         // std::thread
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GLSL.h"

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

int main(int argc, char *argv[])
{
   if (argc < 2)
   {
      std::cout << "Usage: wave_iteration <FILENAME>" << std::endl;
      return 1;
   }

   Aquila::WaveFile wav(argv[1]);
   std::cout << "Loaded file: " << wav.getFilename()
      << " (" << wav.getBitsPerSample() << "b)" << std::endl;
   Aquila::SampleType maxValue = 0, minValue = 0, average = 0;
   std::thread sound (PlayMusic, wav);

   // simple index-based iteration
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
   sound.join();
   std::cout << "Song done" << std::endl;

   return 0;
}
