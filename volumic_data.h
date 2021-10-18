#ifndef VOLUMIC_DATA_H
#define VOLUMIC_DATA_H

#include <vector>
#include <cstdint>
#include <memory>
#include <iostream>
#include <cmath>

class VolumicData {
public:
  // The data from the volume stored:
  // - column by column
  // - line by line
  // - slice by slice
  std::vector<uint16_t> data;

  int width;
  int height;
  int depth;

  double pixel_width;
  double pixel_height;
  double slice_spacing;

  // TODO: add pixel_width, pixel_height and pixel_depth

  // The data provided
  VolumicData();
  VolumicData(int width, int height, int depth);
  VolumicData(const VolumicData &other);
  ~VolumicData();

  unsigned char getValue(int col, int row, int layer);

  void setLayer(uint16_t *layer_data, int layer, double win_min, double win_max, double intercept);
  void manualWindowHandling(uint16_t *layer_data, int size, double win_min, double win_max, double intercept);

};

#endif // VOLUMIC_DATA_H
