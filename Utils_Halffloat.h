#ifndef UTILS_HALFFLOAT
#define UTILS_HALFFLOAT

#include <iostream>


union Float
{
    unsigned __int32 u32;
    float f32;
    struct // single precision floating point (binary32) format IEEE 754-2008
    {
        unsigned __int32 frac:23;
        unsigned __int32 exp:8;
        unsigned __int32 sign:1;
    };
};

union Half
{
    unsigned short u16;
    struct // half (binary16) format IEEE 754-2008
    {
        unsigned short frac:10;
        unsigned short exp:5;
        unsigned short sign:1;
    };
    Float toFloat()
    {
        Float f;
        f.sign=sign;
        switch(exp)
        {
            case 0: // subnormal : (-1)^sign * 2^-14 * 0.frac
            if(frac) // subnormals but non-zeros -> normals in float32
            {
                f.exp=-15+127;
                unsigned __int32 _frac(frac);
                while(!(_frac & 0x200)) { _frac<<=1; f.exp--; }
                f.frac=(_frac & 0x1FF)<<14;
            }
            else { f.frac=0; f.exp=0; } // ± 0 -> ± 0
            break;
            case 31: // infinity or NaNs : frac ? NaN : (-1)^sign * infinity
                f.exp=255;
                f.frac= frac ? 0x200000 : 0; // signaling Nan or zero
                break;
            default: // normal : (-1)^sign * 2^(exp-15) * 1.frac
                f.exp=exp-15+127;
                f.frac=((unsigned __int32)frac)<<13;
        }
        return f;
    }
};

inline float halfToFloat(unsigned short val)
{
    Half h;
    h.u16 = val;

    return h.toFloat().f32;
}

#endif // UTILS_HALFFLOAT

