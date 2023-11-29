#pragma once

template <class T> static void log(const char* header, T data, bool nl=true);
template <class T> static void logc(T data, bool nl=true);
template <class T> static void logx(T data, bool nl=true);
static void logf();
static void logs();

#include "log.hpp"
