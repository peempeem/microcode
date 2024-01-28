#pragma once

#include "color.h"
#include <TFT_eSPI.h>

template <class T> class Point2D
{
    public:
        T x;
        T y;

        Point2D();
        Point2D(T x, T y);

        void set(T x, T y);
        float distance(const Point2D<T> other);
};

template <class T> class Dimension2D
{
    public:
        T width;
        T height;

        Dimension2D();
        Dimension2D(T x, T y);

        void set(T width, T height);
        T area();
};

#include "drawable.hpp"
