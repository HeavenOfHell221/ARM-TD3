#include <QMessageBox>
#include <QString>
#include <QTransform>
#include <QtGui>

#include "glwidget.h"

#include <iostream>

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent), alpha(0.05), log2_zoom(0),
      view_type(ViewType::ORTHO), hide_empty_points(true) {
  QSizePolicy size_policy;
  size_policy.setVerticalPolicy(QSizePolicy::MinimumExpanding);
  size_policy.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
  setSizePolicy(size_policy);
  contours_mode = false;
}

GLWidget::~GLWidget() {}

float GLWidget::getAlpha() const { return alpha; }

void GLWidget::setAlpha(double new_alpha) {
  alpha = new_alpha;
  update();
}

void GLWidget::onContoursModeChange(int state) {
  if (state >= 1){
    contours_mode = true;
  }else{
    contours_mode = false;
  }
  updateDisplayPoints();
}

void GLWidget::updateVolumicData(std::unique_ptr<VolumicData> new_data) {
  volumic_data = std::move(new_data);
  updateDisplayPoints();
  update();
}

void GLWidget::updateDisplayPoints() {
  display_points.clear();
  if (!volumic_data)
    return;
  int W = volumic_data->width;
  int H = volumic_data->height;
  int D = volumic_data->depth;
  int col = 0;
  int row = 0;
  int depth = 0;
  double x_factor = volumic_data->pixel_width;
  double y_factor = volumic_data->pixel_height;
  double z_factor = volumic_data->slice_spacing;
  double max_size =
      std::max(std::max(x_factor * W, y_factor * H), z_factor * D);
  double global_factor = 2.0 / max_size;
  x_factor *= global_factor;
  y_factor *= global_factor;
  z_factor *= global_factor;
  int idx_start = 0;
  int idx_end = W * H * D;
  display_points.reserve(idx_end - idx_start);

  double cur_win_min = win_center - (win_width/2);
  double cur_win_max = win_center + (win_width/2);

  // Importing points
  for (int idx = idx_start; idx < idx_end; idx++) {
    double raw_color = volumic_data->data[idx];
    double c = volumic_data->manualWindowHandling(raw_color); // c entre [0;1]

    int segment = volumic_data->threshold(raw_color, cur_win_min, cur_win_max); // segment {0,1}

    double curr_raw_color;
    int curr_segment, slice, x, y;
    int width = volumic_data->width;
    int height = volumic_data->height;

    if(segment == 1 && contours_mode==true)
    {
      bool drawable = false;
      //6 connectivity
      // for(slice=-1; slice<=1; slice++){
      //   for(y=-1; y<=1; y++){
      //     for(x=-1; x<=1; x++){
      //       if(((x==-1||x==1) && y==0 && slice==0) ||
      //             (x==0 && (y==-1||y==1) && slice==0) ||
      //             (x==0 && y==0 && (slice==-1||slice==1)))
      //       {  
      //         curr_raw_color = volumic_data->data[idx + slice*width*height + y*height + x];
      //         curr_segment = volumic_data->threshold(curr_raw_color, cur_win_min, cur_win_max);

      //         if(curr_segment!=segment)
      //           drawable = true;
      //       }
      //     }
      //   }
      // }

      //18 connectivity
      // for(slice=-1; slice<=1; slice++){
      //   for(y=-1; y<=1; y++){
      //     for(x=-1; x<=1; x++){
      //       if(!(slice!=0 && y!=0 && x!=0))
      //       {  
      //         curr_raw_color = volumic_data->data[idx + slice*width*height + y*height + x];
      //         curr_segment = volumic_data->threshold(curr_raw_color, cur_win_min, cur_win_max);

      //         if(curr_segment!=segment)
      //           drawable = true;
      //       }
      //     }
      //   }
      // }

      //26 connectivity
      for(slice=-1; slice<=1; slice++){
        for(y=-1; y<=1; y++){
          for(x=-1; x<=1; x++){
            curr_raw_color = volumic_data->data[idx + slice*width*height + y*height + x];
            curr_segment = volumic_data->threshold(curr_raw_color, cur_win_min, cur_win_max);

            if(curr_segment!=segment)
              drawable = true;
          }
        }
      }

      if(drawable == false){
        segment = 0;
      }
    }


    if (segment == 1 && (c > 0 || !hide_empty_points)) {
      DrawablePoint p;
    
      p.color.setX(c);
      p.color.setY(c);
      p.color.setZ(c);
      
      p.pos = QVector3D((col - W / 2.) * x_factor, (row - H / 2.) * y_factor,
                        (depth - D / 2.) * z_factor);
      display_points.push_back(p);
    }
    col++;
    if (col == W) {
      row++;
      col = 0;
    }
    if (row == H) {
      depth++;
      row = 0;
    }
  }
  std::cout << "Nb points: " << display_points.size() << std::endl;
}

void GLWidget::initializeGL() {
  glEnable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthFunc(GL_NEVER);
}

void GLWidget::paintGL() {
  QSize viewport_size = size();
  int width = viewport_size.width();
  int height = viewport_size.height();
  double aspect_ratio = width / (float)height;
  glViewport(0, 0, width, height);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  switch (view_type) {
  case ViewType::ORTHO: {
    double view_half_size = std::pow(2, -log2_zoom);
    glScalef(1.0, aspect_ratio, 1.0);
    QVector3D center(0, 0, 0);
    glOrtho(center.x() - view_half_size, center.x() + view_half_size,
            center.y() - view_half_size, center.y() + view_half_size,
            center.z() - view_half_size, center.z() + view_half_size);
    glMultMatrixf(transform.constData());
    break;
  }
  case ViewType::FRUSTUM: {
    float near_dist = 0.5;
    float far_dist = 5.0;
    QMatrix4x4 projection;
    projection.perspective(90, aspect_ratio, near_dist, far_dist);
    QMatrix4x4 cam_offset;
    cam_offset.translate(0, 0, -2 * (1 - log2_zoom));
    QMatrix4x4 pov = projection * cam_offset * transform;
    glMultMatrixf(pov.constData());
  }
  }

  glMatrixMode(GL_MODELVIEW);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  glBegin(GL_POINTS);
  for (const DrawablePoint &p : display_points) {
    glColor4f(p.color.x(), p.color.y(), p.color.z(), alpha);
    glVertex3d(p.pos.x(), p.pos.y(), p.pos.z());
  }
  glEnd();
}

void GLWidget::mousePressEvent(QMouseEvent *event) { lastPos = event->pos(); }

void GLWidget::mouseMoveEvent(QMouseEvent *event) {
  double dx = modifiedDelta(event->x() - lastPos.x());
  double dy = modifiedDelta(event->y() - lastPos.y());

  if (event->buttons() & Qt::LeftButton) {
    QQuaternion local_rotation =
        QQuaternion::fromEulerAngles(0.5 * dy, 0.5 * dx, 0);
    QMatrix4x4 local_matrix;
    local_matrix.rotate(local_rotation);
    transform = local_matrix * transform;
  } else if (event->buttons() & Qt::RightButton) {
    QMatrix4x4 local_matrix;
    local_matrix.translate(QVector3D(dx, -dy, 0) * 0.001);
    transform = local_matrix * transform;
  }

  lastPos = event->pos();
  update();
}

void GLWidget::wheelEvent(QWheelEvent *event) {
  double delta = modifiedDelta(event->delta() / 1000.0);
  log2_zoom += delta;
  update();
}

double GLWidget::modifiedDelta(double delta) {
  if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier) {
    return 10 * delta;
  }
  return delta;
}

void GLWidget::setWinCenter(double new_value) {
  win_center = new_value;
  updateDisplayPoints();
  update();
}

void GLWidget::setWinWidth(double new_value) {
  win_width = new_value;
  updateDisplayPoints();
  update();
}
