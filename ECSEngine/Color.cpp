#include "PreHeader.hpp"
#include "Color.hpp"

using namespace ECSEngine;

ColorR8G8B8A8::ColorR8G8B8A8(colort r, colort g, colort b, colort a)
{
    r = std::min(r, rMax);
    g = std::min(g, gMax);
    b = std::min(b, bMax);
    a = std::min(a, aMax);
    value = (r << (bitsSum - rBits)) | (g << (bBits + aBits)) | (b << aBits) | a;
}

ColorR8G8B8A8::ColorR8G8B8A8(const ColorR8G8B8 &source) : TColor((source.value << 8) | aMax)
{}

ColorR8G8B8A8::ColorR8G8B8A8(const ColorR5G6B5 &source) : TColor(source)
{}

ColorR8G8B8A8::ColorR8G8B8A8(const ColorR4G4B4A4 &source) : TColor(source)
{}

ColorR8G8B8A8::ColorR8G8B8A8(const ColorA8 &source) : TColor(source.value)
{}

ColorR8G8B8A8::ColorR8G8B8A8(colort source) : TColor(source)
{
}

ColorR5G6B5::ColorR5G6B5(colort r, colort g, colort b)
{
    r = std::min(r, rMax);
    g = std::min(g, gMax);
    b = std::min(b, bMax);
    value = (r << (bitsSum - rBits)) | (g << (bBits + aBits)) | (b << aBits);
}

ColorR5G6B5::ColorR5G6B5(const ColorR8G8B8A8 &source) : TColor(source)
{}

ColorR5G6B5::ColorR5G6B5(const ColorR8G8B8 &source) : TColor(source)
{}

ColorR5G6B5::ColorR5G6B5(const ColorR4G4B4A4 &source) : TColor(source)
{}

ColorR5G6B5::ColorR5G6B5(const ColorA8 &source)
{
    /* alpha is dropped, everything else is 0 */
}

ColorR5G6B5::ColorR5G6B5(colort source) : TColor(source)
{
}

ColorR8G8B8::ColorR8G8B8(colort r, colort g, colort b)
{
    r = std::min(r, rMax);
    g = std::min(g, gMax);
    b = std::min(b, bMax);
    value = (r << (bitsSum - rBits)) | (g << (bBits + aBits)) | (b << aBits);
}

ColorR8G8B8::ColorR8G8B8(const ColorR8G8B8A8 &source) : TColor(source.value >> 8)
{}

ColorR8G8B8::ColorR8G8B8(const ColorR5G6B5 &source) : TColor(source)
{}

ColorR8G8B8::ColorR8G8B8(const ColorR4G4B4A4 &source) : TColor(source)
{}

ColorR8G8B8::ColorR8G8B8(const ColorA8 &source)
{
    /* alpha is dropped, everything else is 0 */
}

ColorR8G8B8::ColorR8G8B8(colort source) : TColor(source)
{
}

ColorR4G4B4A4::ColorR4G4B4A4(colort r, colort g, colort b, colort a)
{
    r = std::min(r, rMax);
    g = std::min(g, gMax);
    b = std::min(b, bMax);
    a = std::min(a, aMax);
    value = (r << (bitsSum - rBits)) | (g << (bBits + aBits)) | (b << aBits) | a;
}

ColorR4G4B4A4::ColorR4G4B4A4(const ColorR8G8B8A8 &source) : TColor(source)
{}

ColorR4G4B4A4::ColorR4G4B4A4(const ColorR8G8B8 &source) : TColor(source)
{}

ColorR4G4B4A4::ColorR4G4B4A4(const ColorR5G6B5 &source) : TColor(source)
{}

ColorR4G4B4A4::ColorR4G4B4A4(const ColorA8 &source)
{
    A(source.A() >> 4);
}

ColorR4G4B4A4::ColorR4G4B4A4(colort source) : TColor(source)
{
}

ColorA8::ColorA8(colort a)
{
    A(a);
}

ColorA8::ColorA8(const ColorR8G8B8A8 &source)
{
    A(source.A());
}

ColorA8::ColorA8(const ColorR8G8B8 &source)
{}

ColorA8::ColorA8(const ColorR5G6B5 &source)
{}

ColorA8::ColorA8(const ColorR4G4B4A4 &source)
{
    A(source.A() << 4);
}
