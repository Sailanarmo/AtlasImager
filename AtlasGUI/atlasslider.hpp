//
// Created by Jared Gibson on 6/13/25.
//

#pragma once

#include <QWidget>
#include <QSlider>

#include "atlassliderstyle.hpp"

class AtlasSlider : public QSlider
{
    Q_OBJECT

public:
    AtlasSlider(Qt::Orientation ori, int floor, int ceiling);
    void updateValue(int value) { this->setValue(value); }
    int get() const { return this->value(); }
};
