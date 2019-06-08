#pragma once

namespace ECSEngine
{
    struct ColorR8G8B8A8;
    struct ColorR8G8B8;
    struct ColorR5G6B5;
    struct ColorR4G4B4A4;
    struct ColorA8;

    // TODO: HDR colors
	// TODO: other comparison operators

    // for non-HDR colors
    template <ui32 rBitsT, ui32 gBitsT, ui32 bBitsT, ui32 aBitsT = 0> struct TColor
    {
        static constexpr ui32 rBits = rBitsT;
        static constexpr ui32 gBits = gBitsT;
        static constexpr ui32 bBits = bBitsT;
        static constexpr ui32 aBits = aBitsT;

        static constexpr ui32 bitsSum = rBits + gBits + bBits + aBits;

        using colort =
            conditional_t<bitsSum <= 8, ui8,
            conditional_t<bitsSum <= 16, ui16,
            conditional_t<bitsSum <= 32, ui32,
            conditional_t<bitsSum <= 64, ui64, void>>>>;

        static constexpr colort rMax = (colort(1) << rBits) - 1;
        static constexpr colort gMax = (colort(1) << gBits) - 1;
        static constexpr colort bMax = (colort(1) << bBits) - 1;
        static constexpr colort aMax = (colort(1) << aBits) - 1;

        colort value;

        TColor() : value(aMax)
        {}

        TColor(colort value) : value(value)
        {}

        template <ui32... bits> TColor(TColor<bits...> other)
        {
            ConvertFrom(other);
        }

        TColor(array<f32, 4> source)
        {
            ConvertFromHDR(source);
        }

		[[nodiscard]] ui32 R() const
        {
            return value >> (gBits + bBits + aBits);
        }

        void R(colort r)
        {
            r = std::min(r, rMax);
            value <<= rBits;
            value >>= rBits;
            value |= r << (bitsSum - rBits);
        }

		[[nodiscard]] ui32 G() const
        {
            return (value >> (bBits + aBits)) & gMax;
        }

        void G(colort g)
        {
            g = std::min(g, gMax);
            value &= ~(gMax << (bBits + aBits));
            value |= g << (bBits + aBits);
        }

		[[nodiscard]] ui32 B() const
        {
            return (value >> aBits) & bMax;
        }

        void B(colort b)
        {
            b = std::min(b, bMax);
            value &= ~(bMax << aBits);
            value |= b << aBits;
        }

		[[nodiscard]] ui32 A() const
        {
            return value & aMax;
        }

        void A(colort a)
        {
            a = std::min(a, aMax);
            value &= ~aMax;
            value |= a;
        }

        template <ui32... bits> void ConvertFrom(TColor<bits...> other)
        {
            R(ConvertColor<colort, rBits, other.rBits>(other.R()));
            G(ConvertColor<colort, gBits, other.gBits>(other.G()));
            B(ConvertColor<colort, bBits, other.bBits>(other.B()));

            if (other.aBits == 0)
            {
                A(aMax);
            }
            else
            {
                A(ConvertColor<colort, aBits, other.aBits>(other.A()));
            }
        }

        template <typename T> [[nodiscard]] T ConvertTo() const
        {
            return {R(), G(), B(), A()};
        }

		[[nodiscard]] array<f32, 4> ConvertToHDR() const
        {
            constexpr f32 rdiv256 = 0.003921568628f;

            f32 r = rBits == 8 ? R() * rdiv256 : R() / static_cast<f32>(rMax);
            f32 g = gBits == 8 ? G() * rdiv256 : G() / static_cast<f32>(gMax);
            f32 b = bBits == 8 ? B() * rdiv256 : B() / static_cast<f32>(bMax);

            f32 a;
            if constexpr (aBits == 8)
            {
                a = A() * rdiv256;
            }
            else if constexpr (aBits == 0)
            {
                a = 1.0f;
            }
            else
            {
				static_assert(aMax > 0);
                a = A() / static_cast<f32>(aMax);
            }

            return {r, g, b, a};
        }

        void ConvertFromHDR(array<f32, 4> source)
        {
            auto convert = [](f32 value, colort maxValue) -> colort
            {
                if (value <= 0.0f)
                {
                    return 0;
                }
                if (value >= 1.0f)
                {
                    return maxValue;
                }
                return (colort)(value * maxValue);
            };

            R(convert(source[0], rMax));
            G(convert(source[1], gMax));
            B(convert(source[2], bMax));
            A(convert(source[3], aMax));
        }

		[[nodiscard]] auto operator <=> (const TColor &other) const = default;

    protected:
        template <typename T, ui32 targetBits, ui32 sourceBits, typename U> [[nodiscard]]T ConvertColor(U sourceColor)
        {
            if constexpr(sourceBits < targetBits)
            {
                return sourceColor << (targetBits - sourceBits);
            }
            else
            {
                return sourceColor >> (sourceBits - targetBits);
            }
        }
    };

    struct ColorR8G8B8A8 : public TColor<8, 8, 8, 8>
    {
        using TColor<8, 8, 8, 8>::TColor;

        ColorR8G8B8A8() = default;
        ColorR8G8B8A8(colort r, colort g, colort b, colort a = aMax);
        ColorR8G8B8A8(const ColorR8G8B8 &source);
        ColorR8G8B8A8(const ColorR5G6B5 &source);
        ColorR8G8B8A8(const ColorR4G4B4A4 &source);
        ColorR8G8B8A8(const ColorA8 &source);
        ColorR8G8B8A8(colort source);
    };

    struct ColorR8G8B8 : public TColor<8, 8, 8>
    {
        using TColor<8, 8, 8>::TColor;

        ColorR8G8B8() = default;
        ColorR8G8B8(colort r, colort g, colort b);
        ColorR8G8B8(const ColorR8G8B8A8 &source);
        ColorR8G8B8(const ColorR5G6B5 &source);
        ColorR8G8B8(const ColorR4G4B4A4 &source);
        ColorR8G8B8(const ColorA8 &source);
        ColorR8G8B8(colort source);
    };

    struct ColorR5G6B5 : public TColor<5, 6, 5>
    {
        using TColor<5, 6, 5>::TColor;

        ColorR5G6B5() = default;
        ColorR5G6B5(colort r, colort g, colort b);
        ColorR5G6B5(const ColorR8G8B8A8 &source);
        ColorR5G6B5(const ColorR8G8B8 &source);
        ColorR5G6B5(const ColorR4G4B4A4 &source);
        ColorR5G6B5(const ColorA8 &source);
        ColorR5G6B5(colort source);
    };

    struct ColorR4G4B4A4 : public TColor<4, 4, 4, 4>
    {
        using TColor<4, 4, 4, 4>::TColor;

        ColorR4G4B4A4() = default;
        ColorR4G4B4A4(colort r, colort g, colort b, colort a = aMax);
        ColorR4G4B4A4(const ColorR8G8B8A8 &source);
        ColorR4G4B4A4(const ColorR8G8B8 &source);
        ColorR4G4B4A4(const ColorR5G6B5 &source);
        ColorR4G4B4A4(const ColorA8 &source);
        ColorR4G4B4A4(colort source);
    };

    struct ColorA8 : public TColor<0, 0, 0, 8>
    {
        using TColor<0, 0, 0, 8>::TColor;

        ColorA8() = default;
        ColorA8(colort a);
        ColorA8(const ColorR8G8B8A8 &source);
        ColorA8(const ColorR8G8B8 &source);
        ColorA8(const ColorR5G6B5 &source);
        ColorA8(const ColorR4G4B4A4 &source);
    };
}