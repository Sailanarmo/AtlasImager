//
// Created by Jared Gibson on 6/13/25.
//

#include "atlasslider.hpp"

AtlasSlider::AtlasSlider(Qt::Orientation ori, int floor, int ceiling)
{
    this->setStyle(new MyStyle(this->style()));
    this->setOrientation(ori);
    this->setRange(floor, ceiling);
}