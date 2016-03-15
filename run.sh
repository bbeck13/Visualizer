if [ "$#" -eq 1 ]; then
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
            visualizer ../$1 ../resources
         fi
      fi
   else
      cd build
      make
      if [ $? -ne 0 ]; then
         echo "Compilation errors!!"
      else
         visualizer ../$1 ../resources
      fi
   fi
else
   echo "Usage run <name of .wav file>"
fi
