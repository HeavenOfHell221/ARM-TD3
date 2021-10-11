#ifndef CHECKBOX_H
#define CHECKBOX_H

#include <QLabel>
#include <QCheckBox>
#include <QWidget>

class CheckBox: public QWidget {
  Q_OBJECT
public:
    CheckBox(const QString &box_name, const QString &box_text, QWidget *parent = 0);
    ~CheckBox();
    bool value();

public slots:

    void setValue(bool new_value);
    void onValueChange(bool new_value);

signals:
  void valueChanged(bool new_value);

private:
  QLayout *layout;
  QLabel *name_label;
  QCheckBox *checkBox;

  void updateValueLabel();
};

#endif // CHECKBOX_H