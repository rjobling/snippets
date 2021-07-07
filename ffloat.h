////////////////////////////////////////////////////////////////////////////////
// ffloat.h
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <proto/mathffp.h>

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
struct ffloat
{
	float value;

	ffloat() {}
	ffloat(const ffloat& f) { value = f.value; }
	explicit ffloat(float f) { value = f; }
	explicit ffloat(int i) { value = SPFlt(i); }

	explicit operator float() const { return value; }
	explicit operator int() const { return SPFix(value); }
	explicit operator char() const { return ((char) SPFix(value)); }
	explicit operator short() const { return ((short) SPFix(value)); }
	explicit operator unsigned int() const { return ((unsigned int) SPFix(value)); }
	explicit operator unsigned char() const { return ((unsigned char) SPFix(value)); }
	explicit operator unsigned short() const { return ((unsigned short) SPFix(value)); }

	ffloat& operator=(ffloat f) { value = f.value; return *this; }
	ffloat& operator+=(ffloat f) { value = SPAdd(f.value, value); return *this; }
	ffloat& operator-=(ffloat f) { value = SPSub(f.value, value); return *this; }
	ffloat& operator*=(ffloat f) { value = SPMul(f.value, value); return *this; }
	ffloat& operator/=(ffloat f) { value = SPDiv(f.value, value); return *this; }

	ffloat operator-() const { ffloat f = *this; asm("eor.b #0x80,%[f]" : [f] "+d" (f.value) : : "cc"); return f; }
	ffloat operator+(const ffloat& f) const { return ffloat(SPAdd(f.value, value)); }
	ffloat operator-(const ffloat& f) const { return ffloat(SPSub(f.value, value)); }
	ffloat operator*(const ffloat& f) const { return ffloat(SPMul(f.value, value)); }
	ffloat operator/(const ffloat& f) const { return ffloat(SPDiv(f.value, value)); }

	bool operator==(ffloat f) const { bool r; asm("cmp.l %[a],%[b]\n seq.b %[r]\n neg.b %[r]" : [r] "=d" (r) : [a] "d" (value), [b] "d" (f.value) : "cc"); return r; }
	bool operator!=(ffloat f) const { bool r; asm("cmp.l %[a],%[b]\n sne.b %[r]\n neg.b %[r]" : [r] "=d" (r) : [a] "d" (value), [b] "d" (f.value) : "cc"); return r; }
	bool operator>(ffloat f) const { return (SPCmp(value, f.value) > 0); }
	bool operator<(ffloat f) const { return (SPCmp(value, f.value) < 0); }
	bool operator>=(ffloat f) const { return (SPCmp(value, f.value) >= 0); }
	bool operator<=(ffloat f) const { return (SPCmp(value, f.value) <= 0); }
};

////////////////////////////////////////////////////////////////////////////////
// Constants.
////////////////////////////////////////////////////////////////////////////////
extern const ffloat ff_zero;
extern const ffloat ff_one;
extern const ffloat ff_two;
extern const ffloat ff_half;
extern const ffloat ff_pi;
extern const ffloat ff_one_over_pi;
extern const ffloat ff_two_pi;
extern const ffloat ff_two_over_pi;
extern const ffloat ff_half_pi;
extern const ffloat ff_three_half_pi;
extern const ffloat ff_four_over_pi;
extern const ffloat ff_pi_over_oneeighty;
extern const ffloat ff_oneeighty_over_pi;

////////////////////////////////////////////////////////////////////////////////
// Conversions.
////////////////////////////////////////////////////////////////////////////////
ffloat ffloat_from_ieee(float f);
float ieee_from_ffloat(ffloat f);

////////////////////////////////////////////////////////////////////////////////
// Math.
////////////////////////////////////////////////////////////////////////////////
inline ffloat abs(ffloat f) { asm("and.b #0x7f,%[f]" : [f] "+d" (f.value) : : "cc"); return f; }
inline ffloat sign(ffloat f) { ffloat r; asm("move.l #0x80000041,%[r]\n and.b #0x80,%[f]\n or.b %[f],%[r]" : [r] "=d" (r.value), [f] "+d" (f.value) : : "cc"); return r; }
inline ffloat floor(ffloat f) { return ffloat(SPFloor(f.value)); }
inline ffloat ceil(ffloat f) { return ffloat(SPCeil(f.value)); }
inline ffloat round(ffloat f) { return ((SPTst(f.value) < 0) ? ceil(f - ff_half) : floor(f + ff_half)); }

////////////////////////////////////////////////////////////////////////////////
// Trig.
////////////////////////////////////////////////////////////////////////////////
inline ffloat degrees_to_radians(ffloat degrees) { return (degrees * ff_pi_over_oneeighty); }
inline ffloat radians_to_degrees(ffloat radians) { return (radians * ff_oneeighty_over_pi); }
ffloat cos(ffloat x);
ffloat sin(ffloat x);
ffloat tan(ffloat x);
ffloat log2(ffloat x);
ffloat exp2(ffloat x);
