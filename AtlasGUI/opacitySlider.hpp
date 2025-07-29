//
// Created by Jared Gibson on 6/13/25.
//

#ifndef ATLASIMAGER_OPACITYSLIDER_H
#define ATLASIMAGER_OPACITYSLIDER_H

#include <QWidget>
#include <QSlider>

#include "opacitySliderStyle.hpp"

class OpacitySlider : public QSlider
{
    Q_OBJECT

public:
    OpacitySlider(Qt::Orientation ori, int floor, int ceiling);
    void updateValue(int value) { this->setValue(value); }
    int get() const { return this->value(); }
};

#endif //ATLASIMAGER_OPACITYSLIDER_H
