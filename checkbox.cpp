#include "checkbox.h"

#include <QHBoxLayout>

CheckBox::CheckBox(const QString &box_name, const QString &box_text, QWidget *parent) 
: QWidget(parent) {
    layout = new QHBoxLayout(this);
    name_label = new QLabel(box_name);
    checkBox = new QCheckBox(box_text, parent);
    layout->addWidget(checkBox);
    connect(checkBox, SIGNAL(valueChanged(bool)), this, SLOT(onValueChange(bool)));
}

CheckBox::~CheckBox() {}

bool CheckBox::value() {
    return checkBox->isTristate(); 
}

void CheckBox::setValue(bool new_value) {
    
    checkBox->setTristate(new_value); 
}

void CheckBox::onValueChange(bool new_value) {
    emit valueChanged(new_value);
}