#include <QMessageBox>
#include <QString>
#include <QTransform>
#include <QtGui>

#include "glwidget.h"

#include <iostream>

GLWidget::GLWidget(QWidget *parent)
	: QOpenGLWidget(parent), alpha(0.05), log2_zoom(0),
	  view_type(ViewType::ORTHO), hide_empty_points(true)
{
	QSizePolicy size_policy;
	size_policy.setVerticalPolicy(QSizePolicy::MinimumExpanding);
	size_policy.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
	setSizePolicy(size_policy);
	contours_mode = false;
}

GLWidget::~GLWidget() {}

float GLWidget::getAlpha() const { return alpha; }

void GLWidget::setAlpha(double new_alpha)
{
	alpha = new_alpha;
	update();
}

void GLWidget::onContoursModeChange(int state)
{
	if (state >= 1)
	{
		contours_mode = true;
	}
	else
	{
		contours_mode = false;
	}
	updateDisplayPoints();
	update();
}

void GLWidget::updateVolumicData(std::unique_ptr<VolumicData> new_data)
{
	volumic_data = std::move(new_data);
	updateDisplayPoints();
	update();
}

void GLWidget::updateDisplayPoints()
{
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

	bool useMultipleSegment = true;

	double cur_win_min = win_center - (win_width / 2);
	double cur_win_max = win_center + (win_width / 2);

	// Importing points
	for (int idx = idx_start; idx < idx_end; idx++)
	{
		double raw_color = volumic_data->data[idx];
		double c = volumic_data->manualWindowHandling(raw_color); // c [0;1]

		if (c > 0 || !hide_empty_points)
		{
			int segment = volumic_data->threshold(raw_color, cur_win_min, cur_win_max, useMultipleSegment);

			if (segment != 0)
			{
				if (!contours_mode || (contours_mode && connectivity(0, idx, segment, cur_win_min, cur_win_max, useMultipleSegment)))
				{
					DrawablePoint p;

					p.color = volumic_data->getColorSegment(segment, c);

					p.pos = QVector3D((col - W / 2.) * x_factor, (row - H / 2.) * y_factor, (depth - D / 2.) * z_factor);
					display_points.push_back(p);
				}
			}
		}
		col++;
		if (col == W)
		{
			row++;
			col = 0;
		}
		if (row == H)
		{
			depth++;
			row = 0;
		}
	}
	std::cout << "Nb points: " << display_points.size() << std::endl;
}

void GLWidget::initializeGL()
{
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_NEVER);
}

void GLWidget::paintGL()
{
	QSize viewport_size = size();
	int width = viewport_size.width();
	int height = viewport_size.height();
	double aspect_ratio = width / (float)height;
	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	switch (view_type)
	{
	case ViewType::ORTHO:
	{
		double view_half_size = std::pow(2, -log2_zoom);
		glScalef(1.0, aspect_ratio, 1.0);
		QVector3D center(0, 0, 0);
		glOrtho(center.x() - view_half_size, center.x() + view_half_size,
				center.y() - view_half_size, center.y() + view_half_size,
				center.z() - view_half_size, center.z() + view_half_size);
		glMultMatrixf(transform.constData());
		break;
	}
	case ViewType::FRUSTUM:
	{
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
	for (const DrawablePoint &p : display_points)
	{
		glColor4f(p.color.x(), p.color.y(), p.color.z(), alpha);
		glVertex3d(p.pos.x(), p.pos.y(), p.pos.z());
	}
	glEnd();
}

void GLWidget::mousePressEvent(QMouseEvent *event) { lastPos = event->pos(); }

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
	double dx = modifiedDelta(event->x() - lastPos.x());
	double dy = modifiedDelta(event->y() - lastPos.y());

	if (event->buttons() & Qt::LeftButton)
	{
		QQuaternion local_rotation =
			QQuaternion::fromEulerAngles(0.5 * dy, 0.5 * dx, 0);
		QMatrix4x4 local_matrix;
		local_matrix.rotate(local_rotation);
		transform = local_matrix * transform;
	}
	else if (event->buttons() & Qt::RightButton)
	{
		QMatrix4x4 local_matrix;
		local_matrix.translate(QVector3D(dx, -dy, 0) * 0.001);
		transform = local_matrix * transform;
	}

	lastPos = event->pos();
	update();
}

void GLWidget::wheelEvent(QWheelEvent *event)
{
	double delta = modifiedDelta(event->delta() / 1000.0);
	log2_zoom += delta;
	update();
}

double GLWidget::modifiedDelta(double delta)
{
	if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier)
	{
		return 10 * delta;
	}
	return delta;
}

void GLWidget::setWinCenter(double new_value)
{
	win_center = new_value;
	updateDisplayPoints();
	update();
}

void GLWidget::setWinWidth(double new_value)
{
	win_width = new_value;
	updateDisplayPoints();
	update();
}

bool GLWidget::connectivity(const int mode, const int idx, const int curr_segment, const int min, const int max, const bool colorMode)
{
	int W = volumic_data->width;
	int H = volumic_data->height;
	int D = volumic_data->depth;

	QVector3D pos = volumic_data->getCoordinate(idx);
	
	int x = pos.x();
	int y = pos.y();
	int z = pos.z();

	switch (mode)
	{
	case 0: // 6-connectivity
	{
		for (int dz = -1; dz <= 1; ++dz)
			for (int dy = -1; dy <= 1; ++dy)
				for (int dx = -1; dx <= 1; ++dx)
				{
					if(abs(dx+dy+dz) == 1 && (dx == 0 || dy == 0 || dz == 0))
					{
						int new_x = x + dx;
						int new_y = y + dy;
						int new_z = z + dz;

						if(new_x < 0 || new_y < 0 || new_z < 0 || new_x >= W || new_y >= H || new_z >= D)
							continue;
			
						double raw_color = volumic_data->getValue(new_x, new_y, new_z);
						int neighbors_segment = volumic_data->threshold(raw_color, min, max, colorMode);
						if (curr_segment != neighbors_segment) {
							return true;
						}
					}
					
				}
		break;
	}

	case 1: // 18-connectivity
	{
		for (int dz = -1; dz <= 1; ++dz)
			for (int dy = -1; dy <= 1; ++dy)
				for (int dx = -1; dx <= 1; ++dx)
				{
					if(dx != 0 && dy != 0 && dz != 0)
						continue;
							
					int new_x = x + dx;
					int new_y = y + dy;
					int new_z = z + dz;

					if(new_x < 0 || new_y < 0 || new_z < 0 || new_x >= W || new_y >= H || new_z >= D)
						continue;
		
					double raw_color = volumic_data->getValue(new_x, new_y, new_z);
					int neighbors_segment = volumic_data->threshold(raw_color, min, max, colorMode);
					if (curr_segment != neighbors_segment) {
						return true;
					}
				}
		break;
	}

	case 2: // 26-connectivity
	{
		for (int dz = z - 1; dz <= z + 1; ++dz)
			for (int dy = y - 1; dy <= y + 1; ++dy)
				for (int dx = x - 1; dx <= x + 1; ++dx)
				{
					if(dx < 0 || dy < 0 || dz < 0 || dx >= W || dy >= H || dz >= D)
						continue;
		
					double raw_color = volumic_data->getValue(dx, dy, dz);
					int neighbors_segment = volumic_data->threshold(raw_color, min, max, colorMode);
					if (curr_segment != neighbors_segment) {
						return true;
					}
				}
		break;
	}

	default:
	{
		break;
	}
	}

	return false;
}