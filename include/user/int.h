#pragma once

typedef signed char  I8;
typedef signed short I16;
typedef signed int   I32;

typedef unsigned char  U8;
typedef unsigned short U16;
typedef unsigned int   U32;

typedef decltype(sizeof(0)) Size;
typedef volatile U32 Register;

static_assert(sizeof(I8) == 1, "wrong size");
static_assert(sizeof(I16) == 2, "wrong size");
static_assert(sizeof(I32) == 4, "wrong size");

static_assert(sizeof(U8) == 1, "wrong size");
static_assert(sizeof(U16) == 2, "wrong size");
static_assert(sizeof(U32) == 4, "wrong size");
