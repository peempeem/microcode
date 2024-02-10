#include "drawable.h"
#include "math.h"

template <class T> Point2D<T>::Point2D()
{

}

template <class T> Point2D<T>::Point2D(T x, T y) : x(x), y(y)
{

}

template <class T> void Point2D<T>::set(T x, T y)
{
    this->x = x;
    this->y = y;
}

template <class T> float Point2D<T>::distance(const Point2D<T> other)
{
    return sqrtf(powf(other.x - x, 2) + powf(other.y - y, 2));
}

template <class T> Dimension2D<T>::Dimension2D()
{

}

template <class T> Dimension2D<T>::Dimension2D(T width, T height) : width(width), height(height)
{

}

template <class T> void Dimension2D<T>::set(T width, T height)
{
    this->width = width;
    this->height = height;
}

template <class T> T Dimension2D<T>::area()
{
    return width * height;
}
