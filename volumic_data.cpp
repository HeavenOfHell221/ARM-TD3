#include "volumic_data.h"

#include <stdexcept>

VolumicData::VolumicData()
    : width(-1), height(-1), depth(-1), pixel_width(-1), pixel_height(-1),
      slice_spacing(0) {}

VolumicData::VolumicData(int W, int H, int D)
    : data(W * H * D), width(W), height(H), depth(D) {}

VolumicData::VolumicData(const VolumicData &other)
    : data(other.data), width(other.width), height(other.height),
      depth(other.depth), pixel_width(other.pixel_width),
      pixel_height(other.pixel_height), slice_spacing(other.slice_spacing) {}

VolumicData::~VolumicData() {}

unsigned char VolumicData::getValue(int col, int row, int layer) {
  return data[col + row * width + layer * width * height];
}

void VolumicData::setLayer(uint16_t *layer_data, int layer, double win_min, double win_max, double intercept) {
  if (layer >= depth)
    throw std::out_of_range(
        "Layer " + std::to_string(layer) +
        " is outside of volume (depth=" + std::to_string(depth) + ")");
  int offset = width * height * layer;
  manualWindowHandling(layer_data, width * height, win_min, win_max, intercept);
  for (int i = 0; i < width * height; i++) {
    data[i + offset] = layer_data[i];
  }
}

void VolumicData::manualWindowHandling(uint16_t *layer_data, int size, double win_min, double win_max, double intercept) {

	double win_range = win_max - win_min;
	double offset = pow(2, 15) - intercept;

	for(int i = 0; i < size; ++i) {
		double v = layer_data[i] - offset;
    double c;
  
    if(v < win_min)
      c = 0;
    else if(v > win_max)
      c = 1;
    else
      c = (v - win_min) / win_range;
      
    layer_data[i] = c;
	} 
}
