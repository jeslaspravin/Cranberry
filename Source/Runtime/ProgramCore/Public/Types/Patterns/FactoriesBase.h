#pragma once

template<typename TargetBase, typename ...ConstructParams>
class FactoriesBase
{
public:
    virtual TargetBase create(ConstructParams ...params) const = 0;
};