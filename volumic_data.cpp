#include "volumic_data.h"

#include <stdexcept>

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
	//double offset = pow(2, 15) - intercept;
  double v = value;

  if(v < win_min) 
    return 0;
  if(v > win_max) 
    return 1;

  return (v - win_min) / (win_max - win_min);
}

int VolumicData::threshold(double value, double min, double max) {
  //double offset = pow(2, 15) - intercept;
  double v = value;

  if(v >= min && v <= max)
    return 1;
  
  return 0;
}