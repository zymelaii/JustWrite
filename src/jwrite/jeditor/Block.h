#pragma once

#include <QString>

namespace jwrite::jeditor {

class AbstractBlockItem {
public:
    virtual int  height() const = 0;
    virtual bool dirty() const  = 0;
    virtual void render()       = 0;

    virtual QString type() const = 0;
};

} // namespace jwrite::jeditor
