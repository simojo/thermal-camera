/*
 * fonts.h
 *
 * @brief Defines fonts in bitmap form.
 *
 * @copyright Copyright (C) 2025 Simon J. Jones <github@simonjjones.com>
 * Licensed under the Apache License, Version 2.0.
 *
 * NOTE: Structure was inspired from Daniel Hepper <daniel@hepper.net>.
 * NOTE: Font was used from Daniel Hepper.
 * Link: https://github.com/dhepper/font8x8
 */

#ifndef _FONTS_H
#define _FONTS_H

#include <stdint.h>

#define FONT_W 8
#define FONT_H 16

// Constant: font8x8_basic
// Contains an 8x8 font map for unicode points U+0000 - U+007F (basic latin)
extern const unsigned char thefont[];

#endif
