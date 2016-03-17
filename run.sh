ls build 2>&1 > /dev/null
if [ $? -ne  0 ]; then
   echo "Creating Build dir in ./build"
   mkdir build
   cd build
   cmake ..
   if [ $? -ne 0 ]; then
      echo "CMakeErrors!!"
   else
      make
      if [ $? -ne 0 ]; then
         echo "Compilation errors!!"
      else
         visualizer ../resources/audio/Song3/1.wav ../resources
      fi
   fi
else
   cd build
   make
   if [ $? -ne 0 ]; then
      echo "Compilation errors!!"
   else
      visualizer ../resources/audio/Song3/1.wav ../resources
   fi
fi
