#include "aquila/global.h"
#include "aquila/source/WaveFile.h"
#include "aquila/tools/TextPlot.h"
#include "aquila/transform/AquilaFft.h"
#include "aquila/transform/OouraFft.h"
#include "aquila/source/generator/SineGenerator.h"
#include "aquila/transform/FftFactory.h"
#include <algorithm>
#include <exception>
#include <utility>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <time.h>
#include <chrono>
#include <limits>
#include <SFML/Audio.hpp>
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
using namespace std::chrono;

float randfloat(float l, float h) {
  float r = rand() / (float)RAND_MAX;
  return (1.0f - r) * l + r * h;
}

GLFWwindow* window;        // Main application window
string RESOURCE_DIR = "";  // Where the resources are loaded from
shared_ptr<Program> prog;
shared_ptr<Program> progP;
shared_ptr<Program> progPh;
shared_ptr<Shape> sphere;
shared_ptr<Shape> cube;
float lightPos[3] = {-0.5f, 0.0f, 1.0f};
float lightColor[3] = {1.0f, 1.0f, 1.0f};
float particleThreshold = 0.7f;
bool drawParts = false;

Eigen::Vector3f eyePos(0, 0, 0);
Eigen::Vector3f lookAtPos(0, 0, -5);
Eigen::Vector3f up(0, 1, 0);
std::pair<double, double> scaleTo(.5, 2);

// information for a each audio file
std::vector<Aquila::WaveFile> wavs;
std::vector<Eigen::Vector3f> paths;
std::vector<std::pair<double, double>> ranges;
std::pair<double, double> fftRange;
std::vector<milliseconds> lastTime;
std::vector<double> interval;
std::vector<int> lastSample;
std::vector<Eigen::Vector3f> particleColor;

int g_width, g_height;
float camRot;
double pathTime;
double speed = .01;
double avgSpeed = .01;
std::pair<double, double> speeds(0, .2);
std::pair<double, double> pointSize(0.0f, 250.0f);
Eigen::Vector3f bounds(4, 3, 10);

// Aquila::SampleType maxValue = 0, minValue = 0, average = 0, aboveLimit = 0;
vector<shared_ptr<Particle>> particles;
int numP = 300;
static GLfloat points[900];
static GLfloat pointColors[1200];
GLuint pointsbuffer;
GLuint colorbuffer;

// data associated with storing the geometry in a vertex array object
GLuint VertexArrayID;

// OpenGL handle to texture data
Texture texture;
GLint h_texture0;

bool keyToggles[256] = {false};
// Display time to control fps
float t0_disp = 0.0f;
float t_disp = 0.0f;

float t = 0.0f;  // reset in init
float h = 0.0f;
Eigen::Vector3f g(0.0f, -0.1f, 0.0f);

static void error_callback(int error, const char* description) {
  cerr << description << endl;
}

static void key_callback(GLFWwindow* window,
                         int key,
                         int scancode,
                         int action,
                         int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GL_TRUE);
  }
}

static void resize_callback(GLFWwindow* window, int width, int height) {
  g_width = width;
  g_height = height;
  glViewport(0, 0, width, height);
}

static void init() {
  GLSL::checkVersion();

  t = 0.0f;
  h = 0.01f;
  g = Eigen::Vector3f(0.0f, -0.01f, 0.0f);
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
  progP->setShaderNames(RESOURCE_DIR + "particle_vert.glsl",
                        RESOURCE_DIR + "particle_frag.glsl");
  progP->init();
  progP->addUniform("P");
  progP->addUniform("MV");
  progP->addAttribute("vertPos");
  progP->addAttribute("Pcolor");
  progP->addTexture(&texture);
  glEnable(GL_DEPTH_TEST);

  // Initialize meshes.
  sphere = make_shared<Shape>();
  sphere->loadMesh(RESOURCE_DIR + "round.obj");
  sphere->resize();
  sphere->init();

  cube = make_shared<Shape>();
  cube->loadMesh(RESOURCE_DIR + "cube.obj");
  cube->resize();
  cube->init();

  prog = make_shared<Program>();
  prog->setVerbose(true);
  prog->setShaderNames(RESOURCE_DIR + "simple_vert.glsl",
                       RESOURCE_DIR + "simple_frag.glsl");
  prog->init();
  prog->addUniform("P");
  prog->addUniform("MV");
  prog->addAttribute("vertPos");
  prog->addAttribute("vertNor");
  prog->addAttribute("vertTex");

  progPh = make_shared<Program>();
  progPh->setVerbose(true);
  progPh->setShaderNames(RESOURCE_DIR + "phong_vert.glsl",
                         RESOURCE_DIR + "phong_frag.glsl");
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

  // set up the paths and colors for the balls
  for (size_t i = 0; i < wavs.size(); i++) {
    particleColor.push_back(
        Eigen::Vector3f(randfloat(0, 1), randfloat(0, 1), randfloat(0, 1)));
    paths.push_back(Eigen::Vector3f(std::rand() % 2 ? -speed : speed,
                                    std::rand() % 2 ? -speed : speed,
                                    std::rand() % 2 ? -speed : speed));
  }

  //////////////////////////////////////////////////////
  // Initialize the particles
  //////////////////////////////////////////////////////
  // set up particles
  int n = numP;
  for (int i = 0; i < n; ++i) {
    auto particle = make_shared<Particle>();
    particles.push_back(particle);
    particle->load();
  }
}

// set up the particles - note at first they have no location - set
// when reborn
void initGeom() {
  // generate the VAO
  glGenVertexArrays(1, &VertexArrayID);
  glBindVertexArray(VertexArrayID);

  // generate vertex buffer to hand off to OGL - using instancing
  glGenBuffers(1, &pointsbuffer);
  // set the current state to focus on our vertex buffer
  glBindBuffer(GL_ARRAY_BUFFER, pointsbuffer);
  // actually memcopy the data - only do this once
  glBufferData(GL_ARRAY_BUFFER, sizeof(points), NULL, GL_STREAM_DRAW);

  glGenBuffers(1, &colorbuffer);
  // set the current state to focus on our vertex buffer
  glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
  // actually memcopy the data - only do this once
  glBufferData(GL_ARRAY_BUFFER, sizeof(pointColors), NULL, GL_STREAM_DRAW);

  assert(glGetError() == GL_NO_ERROR);
}

void updateGeom() {
  Eigen::Vector3f pos;
  Eigen::Vector4f col;

  // go through all the particles and update the CPU buffer
  for (int i = 0; i < numP; i++) {
    pos = particles[i]->getPosition();
    col = particles[i]->getColor();
    points[i * 3 + 0] = pos(0);
    points[i * 3 + 1] = pos(1);
    points[i * 3 + 2] = pos(2);
    pointColors[i * 4 + 0] = col(0) + col(0) / 10;
    pointColors[i * 4 + 1] = col(1) + col(1) / 10;
    pointColors[i * 4 + 2] = col(2) + col(2) / 10;
    pointColors[i * 4 + 3] = col(3);
  }

  // update the GPU data
  glBindBuffer(GL_ARRAY_BUFFER, pointsbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(points), NULL, GL_STREAM_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * numP * 3, points);
  glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(pointColors), NULL, GL_STREAM_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * numP * 4, pointColors);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Sort particles by their z values in camera space
class ParticleSorter {
 public:
  bool operator()(const shared_ptr<Particle> p0,
                  const shared_ptr<Particle> p1) const {
    // Particle positions in world space
    const Eigen::Vector3f& x0 = p0->getPosition();
    const Eigen::Vector3f& x1 = p1->getPosition();
    // Particle positions in camera space
    Eigen::Vector4f x0w = C * Eigen::Vector4f(x0(0), x0(1), x0(2), 1.0f);
    Eigen::Vector4f x1w = C * Eigen::Vector4f(x1(0), x1(1), x1(2), 1.0f);
    return x0w.z() < x1w.z();
  }

  Eigen::Matrix4f C;  // current camera matrix
};
ParticleSorter sorter;

/* note for first update all particles should be "reborn"
   which will initialize their positions */
void updateParticles() {
  // update the particles
  h = speed;
  Eigen::Vector3f clr(0, 0, 0);
  for (size_t i = 0; i < wavs.size(); i++) {
    clr.x() += particleColor.at(i).x();
    clr.y() += particleColor.at(i).y();
    clr.z() += particleColor.at(i).z();
  }
  clr.x() /= wavs.size();
  clr.y() /= wavs.size();
  clr.z() /= wavs.size();
  for (auto particle : particles) {
    particle->update(t, h, g, keyToggles, clr,
                     avgSpeed + (.1 * (avgSpeed - .01)));
  }
  t += h;

  // Sort the particles by Z
  auto temp = make_shared<MatrixStack>();
  temp->rotate(camRot, Eigen::Vector3f(0, 1, 0));
  sorter.C = temp->topMatrix();
  sort(particles.begin(), particles.end(), sorter);
}

template <typename tVal>
tVal mapRange(std::pair<tVal, tVal> a, std::pair<tVal, tVal> b, tVal inVal) {
  tVal inValNorm = inVal - a.first;
  tVal aUpperNorm = a.second - a.first;
  tVal normPosition = inValNorm / aUpperNorm;

  tVal bUpperNorm = b.second - b.first;
  tVal bValNorm = normPosition * bUpperNorm;
  tVal outVal = b.first + bValNorm;

  return outVal;
}

template <typename Iterator>
void drawGraph(std::vector<double> spectrum,
               Iterator begin,
               Iterator end,
               std::shared_ptr<MatrixStack> P,
               std::shared_ptr<MatrixStack> M,
               std::shared_ptr<MatrixStack> V,
               int baseY,
               int baseX) {
  const double max = *std::max_element(begin, end);
  const double min = *std::min_element(begin, end);
  const double range = max - min;
  const double size = spectrum.size() / 2.0f, m_width = 32, m_height = 16;
  std::vector<std::vector<bool>> matrix(size);
  std::vector<std::vector<bool>> matrix2(size);

  begin += 2;
  for (size_t xPos = 0; xPos < size; xPos++) {
    matrix[xPos].resize(m_width, false);
    double normalVal = (*begin++ - min + 1) / range;
    std::size_t yPos =
        m_height - static_cast<size_t>(std::ceil(m_height * normalVal));

    if (yPos >= m_height) {
      yPos = m_height - 1;
    }
    matrix[xPos][yPos] = true;
  }
  // begin += 2;
  // for (size_t xPos = 0; xPos < size; xPos++) {
  // matrix2[xPos].resize(m_width, false);
  // double normalVal = (*begin++ - min + 1) / range;
  // std::size_t yPos = m_height
  //- static_cast<size_t>(std::ceil(m_height * normalVal));

  // if (yPos >= m_height) {
  // yPos = m_height -1;
  //}
  // if (matrix[xPos][yPos] == true && yPos < 3) {
  // std::cout << yPos << std::endl;
  // matrix[xPos][yPos] = false;
  //}
  // matrix2[xPos][yPos] = true;
  //}
  // std::cout << "len = " << matrix.size() << std::endl;
  size_t yPos = 0, xPos = 0;
  for (float y = baseY; y < (baseY + 4); y += 0.25f) {
    for (float x = baseX; x < (baseX + 8); x += 0.25f) {
      if (matrix[xPos][yPos] == true) {
        M->pushMatrix();
        M->loadIdentity();
        M->translate(Eigen::Vector3f(x, -y, -15));
        M->scale(Eigen::Vector3f(.1, .1, .00000001));
        float r, g, b;
        b = 0;
        g = mapRange(std::pair<float, float>(baseY, baseY + 4),
                     std::pair<float, float>(0, 1), y);
        r = 1 - g;
        // std::cout << "r " << r <<" g " << g << " b" << b <<std::endl;
        glUniform3f(progPh->getUniform("MatAmb"), r, g, b);
        glUniform3f(progPh->getUniform("MatDif"), r, g, b);
        glUniform3f(progPh->getUniform("specular"), r, g, b);
        glUniform1f(progPh->getUniform("shine"), 27.9);
        glUniformMatrix4fv(progPh->getUniform("ModelMatrix"), 1, GL_FALSE,
                           M->topMatrix().data());
        glUniform1f(progPh->getUniform("NColor"), 0);
        glUniform3fv(progPh->getUniform("lightPos"), 1, lightPos);
        glUniform3fv(progPh->getUniform("lightColor"), 1, lightColor);
        cube->draw(progPh);
        M->popMatrix();
        for (size_t tempY = yPos; tempY < m_height; tempY++) {
          matrix[xPos][tempY] = true;
        }
      }
      xPos++;
    }
    yPos++;
    xPos = 0;
  }
}

static void render() {
  glfwGetFramebufferSize(window, &g_width, &g_height);
  glViewport(0, 0, g_width, g_height);

  // Clear framebuffer.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  double diffTime;  // scale_temp;
  int sampleNum = 0;
  // std::pair<double,double> from(0, maxValue), to(1, 2.5);
  double scale;
  static double x[4] = {randfloat(-2.0f, 2.0f), randfloat(-2.0f, 2.0f),
                        randfloat(-2.0f, 2.0f), randfloat(-2.0f, 2.0f)},
                y[4] = {randfloat(-2.0f, 2.0f), randfloat(-2.0f, 2.0f),
                        randfloat(-2.0f, 2.0f), randfloat(-2.0f, 2.0f)},
                z[4] = {-5, -5, -5, -5};

  float aspect = g_width / (float)g_height;
  double avgpower = 0, power;
  avgSpeed = 0;
  auto P = make_shared<MatrixStack>();
  auto MV = make_shared<MatrixStack>();

  // Apply perspective projection.
  P->pushMatrix();
  P->perspective(45.0f, aspect, 0.01f, 100.0f);
  auto M = make_shared<MatrixStack>();
  auto V = make_shared<MatrixStack>();
  V->pushMatrix();
  V->lookAt(eyePos, lookAtPos, up);
  size_t i = 0, done = 0;

  while (i < wavs.size()) {
    progPh->bind();
    glUniformMatrix4fv(progPh->getUniform("P"), 1, GL_FALSE,
                       P->topMatrix().data());
    glUniformMatrix4fv(progPh->getUniform("ViewMatrix"), 1, GL_FALSE,
                       V->topMatrix().data());
    milliseconds ms =
        duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    diffTime = ms.count() - lastTime.at(i).count();
    sampleNum = diffTime * interval.at(i);
    if ((size_t)sampleNum >= wavs.at(i).getSamplesCount()) {
      done++;
      if (done == wavs.size()) {
        glfwSetWindowShouldClose(window, GL_TRUE);
        return;
      } else {
        i++;
        continue;
      }
    }
    std::vector<Aquila::SampleType> v;
    for (int j = lastSample.at(i); j <= sampleNum; j++) {
      v.push_back(wavs.at(i).sample(j));
    }
    Aquila::SignalSource src(v, wavs.at(i).getSampleFrequency());
    power = Aquila::power(src);
    avgpower += power;
    scale = mapRange(ranges.at(i), scaleTo, power);
    // draw!!
    M->pushMatrix();
    M->loadIdentity();
    M->translate(Eigen::Vector3f(x[i], y[i], z[i]));
    M->scale(Eigen::Vector3f(scale, scale, scale));

    glUniform3f(progPh->getUniform("MatAmb"), particleColor.at(i).x(),
                particleColor.at(i).y(), particleColor.at(i).z());
    glUniform3f(progPh->getUniform("MatDif"), particleColor.at(i).x(),
                particleColor.at(i).y(), particleColor.at(i).z());
    glUniform3f(progPh->getUniform("specular"), particleColor.at(i).x() - .2,
                particleColor.at(i).y() + .3, particleColor.at(i).z() + .4);
    glUniform1f(progPh->getUniform("shine"), 27.9);
    glUniformMatrix4fv(progPh->getUniform("ModelMatrix"), 1, GL_FALSE,
                       M->topMatrix().data());
    glUniform1f(progPh->getUniform("NColor"), 0);
    glUniform3fv(progPh->getUniform("lightPos"), 1, lightPos);
    glUniform3fv(progPh->getUniform("lightColor"), 1, lightColor);
    sphere->draw(progPh);
    M->popMatrix();

    if (scale > scaleTo.first + .0001) {
      Aquila::AquilaFft coldTurkey(256);
      Aquila::SpectrumType spectrum = coldTurkey.fft(src.toArray());

      double len = spectrum.size();
      std::vector<double> absSpectrum(len / 4);
      for (size_t j = 0; j < len / 4; j++) {
        absSpectrum[j] =
            (std::abs(spectrum[j * 2]) + std::abs(spectrum[(j + 1) * 2]) +
             std::abs(spectrum[(j + 2) * 2]) +
             std::abs(spectrum[(j + 3) * 2])) /
            4;
      }
      // std::cout << "fft Vector (" << spectrum.size() << ") " << std::endl;
      switch (i) {
        case 0:
          if (wavs.size() == 1)
            drawGraph(absSpectrum, absSpectrum.begin(), absSpectrum.end(), P, M,
                      V, -4, -4);
          else
            drawGraph(absSpectrum, absSpectrum.begin(), absSpectrum.end(), P, M,
                      V, -6, -8);
          break;
        case 1:
          drawGraph(absSpectrum, absSpectrum.begin(), absSpectrum.end(), P, M,
                    V, -6, 2);
          break;
        case 2:
          drawGraph(absSpectrum, absSpectrum.begin(), absSpectrum.end(), P, M,
                    V, 0, -8);
          break;
        case 3:
          drawGraph(absSpectrum, absSpectrum.begin(), absSpectrum.end(), P, M,
                    V, 0, 2);
          break;
        default:
          drawGraph(absSpectrum, absSpectrum.begin(), absSpectrum.end(), P, M,
                    V, -4, -4);
          break;
      }
    }

    bool changed = false;
    progPh->unbind();
    speed = mapRange(ranges.at(i), speeds, power);
    if (speed <= 0.0001) {
      particleColor.at(i).x() = 0;
      particleColor.at(i).y() = 0;
      particleColor.at(i).z() = 0;
    } else if (particleColor.at(i).x() == 0 && particleColor.at(i).y() == 0 &&
               particleColor.at(i).z() == 0) {
      particleColor.at(i).x() = randfloat(0, 1);
      particleColor.at(i).y() = randfloat(0, 1);
      particleColor.at(i).z() = randfloat(0, 1);
    }
    avgSpeed += speed;

    if (x[i] + paths.at(i).x() >= bounds.x()) {
      changed = true;
      paths.at(i).x() = -speed;
    } else if (x[i] + paths.at(i).x() <= -bounds.x()) {
      changed = true;
      paths.at(i).x() = speed;
    } else if (std::abs(paths.at(i).x()) != speed) {
      paths.at(i).x() = paths.at(i).x() < 0 ? -speed : speed;
    }
    x[i] += paths.at(i).x();

    if (y[i] + paths.at(i).y() >= bounds.y()) {
      changed = true;
      paths.at(i).y() = -speed;
    } else if (y[i] + paths.at(i).y() <= -bounds.y()) {
      changed = true;
      paths.at(i).y() = speed;
    } else if (std::abs(paths.at(i).y()) != speed) {
      paths.at(i).y() = paths.at(i).y() < 0 ? -speed : speed;
    }

    y[i] += paths.at(i).y();

    if (z[i] + paths.at(i).z() >= -5) {
      changed = true;
      paths.at(i).z() = -speed;
    } else if (z[i] + paths.at(i).z() <= -bounds.z()) {
      changed = true;
      paths.at(i).z() = speed;
    } else if (std::abs(paths.at(i).z()) != speed) {
      paths.at(i).z() = paths.at(i).z() < 0 ? -speed : speed;
    }
    z[i] += paths.at(i).z();

    if (changed) {
      particleColor.at(i).x() = randfloat(0, 1);
      particleColor.at(i).y() = randfloat(0, 1);
      particleColor.at(i).z() = randfloat(0, 1);
    }

    lastSample.at(i) = sampleNum;
    i++;
  }
  avgSpeed /= i;
  avgpower /= i;
  // Pop matrix stacks.
  P->popMatrix();
  V->popMatrix();

  // Apply perspective projection.
  P->pushMatrix();
  P->perspective(45.0f, aspect, 0.01f, 100.0f);
  MV->loadIdentity();
  // camera rotate
  MV->rotate(camRot, Eigen::Vector3f(0, 1, 0));
  MV->translate(Eigen::Vector3f(0, 0, -10));

  // Draw
  progP->bind();
  std::pair<double, double> oneZero(-.2f, .5f);
  // cpp typechecking is awful don't try to be something you're not !!
  double pos = mapRange(*std::max_element(ranges.begin(), ranges.end()),
                        oneZero, avgpower);
  lightPos[0] = pos;

  glPointSize(mapRange(*std::max_element(ranges.begin(), ranges.end()),
                       pointSize, avgpower));
  updateParticles();
  updateGeom();
  glUniformMatrix4fv(progP->getUniform("P"), 1, GL_FALSE,
                     P->topMatrix().data());

  glUniformMatrix4fv(progP->getUniform("MV"), 1, GL_FALSE,
                     MV->topMatrix().data());

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, pointsbuffer);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

  glVertexAttribDivisor(0, 1);
  glVertexAttribDivisor(1, 1);
  // Draw the points !
  glDrawArraysInstanced(GL_POINTS, 0, 1, numP);

  glVertexAttribDivisor(0, 0);
  glVertexAttribDivisor(1, 0);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  progP->unbind();
  // Pop matrix stacks.
  P->popMatrix();
}

sf::Music* GetMusic(Aquila::WaveFile wav) {
  sf::Music* music = new sf::Music;
  if (!music->openFromFile(wav.getFilename())) {
    cerr << "Couldn't play " << wav.getFilename() << endl;
    exit(EXIT_FAILURE);
  }
  std::cout << "Loaded: " << wav.getFilename() << ":" << std::endl;
  std::cout << " " << music->getDuration().asSeconds() << " seconds"
            << std::endl;
  std::cout << " " << music->getSampleRate() << " samples / sec" << std::endl;
  std::cout << " " << music->getChannelCount() << " channels" << std::endl;
  return music;
}

int main(int argc, char* argv[]) {
  g_width = 640;
  g_height = 480;
  if (argc > 6) {
    std::cout << "maximum 4 wav files allowed!!" << std::endl;
    std::cout << "Usage: visualizer <FILENAME> ... <RESOURCEDIR>" << std::endl;
    return 1;
  }
  if (argc < 3) {
    std::cout << "Usage: visualizer <FILENAME> ... <RESOURCEDIR>" << std::endl;
    if (argc < 2) {
      std::cout << "Going in with " << std::endl;
      RESOURCE_DIR = argv[2] + string("/");
    }
    return 1;
  } else {
    RESOURCE_DIR = argv[argc - 1] + string("/");
  }
  g_width = 640;
  g_height = 480;
  for (int i = 0; argc > 2; i++, argc--) {
    wavs.push_back(Aquila::WaveFile(argv[i + 1]));
  }
  int i = 0;
  // set up audio information
  for (Aquila::WaveFile wav : wavs) {
    std::cout << "Loaded file: " << wav.getFilename() << " ("
              << wav.getBitsPerSample() << "b)" << std::endl;
    std::cout << wav.getSamplesCount() << " samples at " << wav.getAudioLength()
              << " ms" << std::endl;

    Aquila::SampleType maxValue = 0, minValue = 0;

    interval.push_back((double)wav.getSamplesCount() /
                       (double)wav.getAudioLength());

    for (size_t j = 0; j < wav.getSamplesCount(); ++j) {
      std::vector<Aquila::SampleType> v = {wav.sample(j)};
      Aquila::SignalSource src(v, wav.getSampleFrequency());
      if (Aquila::power(src) > maxValue)
        maxValue = Aquila::power(src);
      if (Aquila::power(src) < minValue)
        minValue = Aquila::power(src);
    }
    ranges.push_back(std::pair<double, double>(minValue, maxValue));
    i++;
  }
  /* your main will always include a similar set up to establish your window
     and GL context, etc. */

  // Set error callback as openGL will report errors but they need a call back
  glfwSetErrorCallback(error_callback);
  // Initialize the library.
  if (!glfwInit()) {
    return -1;
  }
  // request the highest possible version of OGL - important for mac
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

  // Create a windowed mode window and its OpenGL context.
  window = glfwCreateWindow(g_width, g_height, "textures", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }
  // Make the window's context current.
  glfwMakeContextCurrent(window);
  // Initialize GLEW.
  glewExperimental = true;
  if (glewInit() != GLEW_OK) {
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
  // set the window resize call back
  glfwSetFramebufferSizeCallback(window, resize_callback);

  init();
  initGeom();
  std::vector<sf::Music*> musics;
  for (Aquila::WaveFile wav : wavs) {
    musics.push_back(GetMusic(wav));
  }
#pragma omp parallel for
  for (sf::Music* music : musics) {
    music->play();
  }
  // make these two events happen as close as possible
  // starting the music / starting the clock for analyzing said music
  lastTime.push_back(
      duration_cast<milliseconds>(system_clock::now().time_since_epoch()));
  lastSample.push_back(0);
  for (size_t i = 1; i < wavs.size(); i++) {
    lastTime.push_back(lastTime.at(0));
    lastSample.push_back(lastSample.at(0));
  }
  // Loop until the user closes the window.
  while (!glfwWindowShouldClose(window)) {
    render();
    glfwSwapBuffers(window);
    // Poll for and process events.
    glfwPollEvents();
  }
  // Quit program.
  glfwDestroyWindow(window);
  glfwTerminate();
  for (sf::Music* music : musics) {
    delete music;
  }
  return 0;
}
