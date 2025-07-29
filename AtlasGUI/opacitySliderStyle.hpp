//
// Created by Jared Gibson on 6/13/25.
//

#ifndef ATLASIMAGER_OPACITYSLIDERSTYLE_H
#define ATLASIMAGER_OPACITYSLIDERSTYLE_H

#include <QProxyStyle>

// This class enables us to have a freely moving slider. Implementation found
// and adopted from: https:a//stackoverflow.com/a/26281608/5824979

class MyStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;

    int styleHint(QStyle::StyleHint hint, const QStyleOption* option = nullptr,
                  const QWidget* widget = nullptr, QStyleHintReturn* returnData = nullptr) const
    {
        return QProxyStyle::styleHint(hint,option,widget,returnData);
    }
};

#endif //ATLASIMAGER_OPACITYSLIDERSTYLE_H
