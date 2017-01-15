CPE 471 Final Project
Braden Beck (bnbeck@calpoly.edu)
version 3 - 15 - 2016

Usage:
   The handy run.sh file will setup the build dir and run the example wav file.
   if you end the visualization early you must press control-c to stop the program

   You are given some sample audio files to run heres what you can type to run
   them if you ran the run.sh and are in the build directory
   ./visualizer  ../resources/audio/Song1/*.wav ../resources/
   ./visualizer  ../resources/audio/Song2/*.wav ../resources/
   ./visualizer  ../resources/audio/Song3/*.wav ../resources/

   If *.wav doesn't expand for you please get a new shell but first run the
   command for each .wav file in the directory like so
      ./visualizer  ../resources/audio/Song1/1.wav ../resources/audio/Song1/2.wav ../resources/audio/Song1/3.wav ../resources/audio/Song1/4.wav ../resources/

   If you want to run with your own music files they must be less than 3
   minutes (roughly I don't know the exact file size limit) for Libcanberra
   canberra-gtk-play to be able to play them, (if you don't want sound you can
   use big files they will be visualized but the sound won't work).
   Additionally you must also encode all your music files to  WAV (Microsoft)
   signed 16-bit PCM I used audacity(http://www.audacityteam.org/) to do this
   it can convert almost any audio format to WAV (Microsoft) signed 16-bit PCM

Features:
   For full usage you can supply up to 4 wav files via the command line and the
   specify the resource dir (default ../resources if you ran the script and are
   in the build dir) as the last argument

   Complete with a Fast Fourier Transform visualization, particle effects and
   bright bouncing balls.

   You can also change the range of size of the particles and the balls by
   editing the global variable pointSize:
      std::pair<double,double> pointSize(min.f, max.f);
   for the range in size of the particles and scaleTo:
      std::pair<double,double> scaleTo(min.f, max.f);
   and changing then min and max fields of the pairs making the ranges bigger
   will cause the particles and balls to react more violently to the audio
   (e.g. get bigger and smaller as it animates)
   you can also change the range of the speeds of the particles and balls by
   editing:
      std::pair<double,double> speeds(min.f, max.f);
   where the min and the max are strictly >= 0.0f for all of these pairs

   The default settings for the ranges of sizes are ok but I recommending at
   least changing the pointSize to be a larger range because the particles
   look cooler when they have a large range of sizes they can be.

Requirements:
   To run this program you will need the Aquila cpp library available at:
      http://aquila-dsp.org/
      see
         http://aquila-dsp.org/download/
      for installation information
   you will also need the glut, glew, glfw, and eigen cpp libraries
      see:
      http://users.csc.calpoly.edu/~ssueda/teaching/CSC474/2016W/labs/L00/
      for installation information
   finally you will also need sfml
      see:
      http://www.sfml-dev.org/index.php


   Speaking of operating systems this software is only confirmed to work on
   linux based operating systems so if your running osx or windows now would
   be a good time to reformat your hardrive and install a real os, (Arch Linux
   is recommended as its the environment I developed this in)
