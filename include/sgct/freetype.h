/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef FREE__TYPE_H
#define FREE__TYPE_H

#include "Font.h"

//Some STL headers
#include <string>
#include <vector>
#include <wchar.h>
#include <glm/glm.hpp>

//Using the STL exception library increases the
//chances that someone else using our code will corretly
//catch any exceptions that we throw.
#include <stdexcept>

namespace sgct_text
{
void print(sgct_text::Font * ft_font, float x, float y, const char * format, ...);
void print(sgct_text::Font * ft_font, float x, float y, const wchar_t * format, ...);
void print3d(sgct_text::Font * ft_font, glm::mat4 mvp, const char *format, ...);
void print3d(sgct_text::Font * ft_font, glm::mat4 mvp, const wchar_t *format, ...);

//with color
void print(sgct_text::Font * ft_font, float x, float y, const glm::vec4 & color, const char * format, ...);
void print(sgct_text::Font * ft_font, float x, float y, const glm::vec4 & color, const wchar_t *format, ...);
void print3d(sgct_text::Font * ft_font, glm::mat4 mvp, const glm::vec4 & color, const char *format, ...);
void print3d(sgct_text::Font * ft_font, glm::mat4 mvp, const glm::vec4 & color, const wchar_t *format, ...);
}

#endif
