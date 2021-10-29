#include "volumic_data.h"

#include <stdexcept>

#define range(value, min, max) value >= min && value < max 

VolumicData::VolumicData()
    : width(-1), height(-1), depth(-1), pixel_width(-1), pixel_height(-1),
      slice_spacing(0) {}

VolumicData::VolumicData(int W, int H, int D, double min, double max, double I)
    : data(W * H * D), width(W), height(H), depth(D), win_min(min), win_max(max), intercept(I) {}

VolumicData::VolumicData(const VolumicData &other)
    : data(other.data), width(other.width), height(other.height),
      depth(other.depth), pixel_width(other.pixel_width),
      pixel_height(other.pixel_height), slice_spacing(other.slice_spacing) {}

VolumicData::~VolumicData() {}

unsigned char VolumicData::getValue(int col, int row, int layer) {
  return data[col + row * width + layer * width * height];
}

QVector3D VolumicData::getCoordinate(int idx) {
  int x = idx % width;
  int y = (idx/width) % height;
  int z = idx / (width*height);
  return QVector3D(x, y, z); 
}

void VolumicData::setLayer(uint16_t *layer_data, int layer) {
  if (layer >= depth)
    throw std::out_of_range(
        "Layer " + std::to_string(layer) +
        " is outside of volume (depth=" + std::to_string(depth) + ")");
  int offset = width * height * layer;
  double offset_ = pow(2, 15) - intercept;
  for (int i = 0; i < width * height; i++) {
    data[i + offset] = layer_data[i] - offset_;
  }
}

double VolumicData::manualWindowHandling(double value) {
  if(value < win_min)  return 0;
  if(value > win_max)  return 1;

  return (value - win_min) / (win_max - win_min);
}

int VolumicData::threshold(double value, double min, double max) {
  if(value > max || value < min)
    return -1;
  return 0;
}

int VolumicData::threshold(double value) {
  if(range(value, 200, 1024))         return 1; // Bone
  else if(range(value, 100, 200))     return 2; // Structures faiblement calcifiées
  else if(range(value, 37, 45))       return 3; // Matière grise
  else if(range(value, 20, 30))       return 4; // Matière blanche
  else if(range(value, -5, 15))       return 5; // Eau et liquides cérébraux
  else if(range(value, -1024, -10))   return 6; // Graisses, poumons, air

  return -1;
}

QVector3D VolumicData::getColorSegment(int segment, double c) {
  switch (segment)
  {
    case 0: return QVector3D(c, c, c);
    case 1: return QVector3D(1, 1, 1);        // blanc
    case 2: return QVector3D(0.5, 0.5, 0.5);  // gris
    case 3: return QVector3D(0, 1, 0);        // vert
    case 4: return QVector3D(1, 0.7, 0);        // jaune
    case 5: return QVector3D(0.2, 0.2, 1);    // bleu
    case 6: return QVector3D(1, 0, 0);        // rouge

    default: return QVector3D(0, 0, 0);
  }
}