//
// Created by Jared Gibson on 6/13/25.
//
#pragma once

#include <QWidget>
#include <QSlider>

#include "atlasstyleslider.hpp"

class AtlasSlider : public QSlider
{
    Q_OBJECT

public:
    AtlasSlider(const Qt::Orientation ori, const int floor, const int ceiling);
    inline void updateValue(const int value) { this->setValue(value); }
    inline int get() const { return this->value(); }
};
