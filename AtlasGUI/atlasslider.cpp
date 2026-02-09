//
// Created by Jared Gibson on 6/13/25.
//

#include "atlasslider.hpp"

AtlasSlider::AtlasSlider(const Qt::Orientation ori, const int floor, const int ceiling)
{
    this->setStyle(new MyStyle(this->style()));
    this->setOrientation(ori);
    this->setRange(floor, ceiling);
}