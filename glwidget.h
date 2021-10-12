#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QMatrix4x4>
#include <QOpenGLWidget>
#include <QString>

#include <memory>

#include "volumic_data.h"

class GLWidget : public QOpenGLWidget {
public:
  Q_OBJECT
public:
  enum ViewType { ORTHO, FRUSTUM };

  GLWidget(QWidget *parent = 0);
  ~GLWidget();
  QSize sizeHint() const { return QSize(200, 200); }

  float getAlpha() const;

  void updateVolumicData(std::unique_ptr<VolumicData> new_data);

public slots:
  void setAlpha(double new_alpha);

protected:
  struct DrawablePoint {
    QVector3D pos;
    double c;
  };

  void initializeGL() override;
  void paintGL() override;

  void updateDisplayPoints();

  void wheelEvent(QWheelEvent *event) override;

  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;

  /**
   * If 'shift' modifier is pressed, multiplies value by 10
   */
  double modifiedDelta(double delta);

  QPoint lastPos;
  float alpha;
  /**
   * Zoom value using a log scale
   * - positive is zoom in
   * - negative is zoom out
   */
  float log2_zoom;
  QMatrix4x4 transform;

  ViewType view_type;

  /// When enabled, all points with a drawing color = 0 are hidden
  bool hide_empty_points;

  /// The data of all the slices stored in a single object
  std::unique_ptr<VolumicData> volumic_data;

  /// The points to be drawn
  std::vector<DrawablePoint> display_points;
};

#endif // GLWIDGET_H