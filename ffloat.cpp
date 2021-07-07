////////////////////////////////////////////////////////////////////////////////
// ffloat.cpp
////////////////////////////////////////////////////////////////////////////////

#include "ffloat.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
const ffloat ff_zero			  = ffloat_from_ieee(0.0f);
const ffloat ff_one				  = ffloat_from_ieee(1.0f);
const ffloat ff_two				  = ffloat_from_ieee(2.0f);
const ffloat ff_half			  = ffloat_from_ieee(0.5f);
const ffloat ff_pi				  = ffloat_from_ieee(3.14159274f);
const ffloat ff_one_over_pi		  = ffloat_from_ieee(0.318309873f);
const ffloat ff_two_pi			  = ffloat_from_ieee(6.28318548f);
const ffloat ff_two_over_pi		  = ffloat_from_ieee(0.636619747f);
const ffloat ff_half_pi			  = ffloat_from_ieee(1.57079637f);
const ffloat ff_three_half_pi	  = ffloat_from_ieee(4.71238899f);
const ffloat ff_four_over_pi	  = ffloat_from_ieee(1.27323949f);
const ffloat ff_pi_over_oneeighty = ffloat_from_ieee(0.017453292519943f);
const ffloat ff_oneeighty_over_pi = ffloat_from_ieee(57.29577951308232f);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static const ffloat c1 = ffloat_from_ieee( 0.9999932946f);
static const ffloat c2 = ffloat_from_ieee(-0.4999124376f);
static const ffloat c3 = ffloat_from_ieee( 0.0414877472f);
static const ffloat c4 = ffloat_from_ieee(-0.0012712095f);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static const ffloat t1 = ffloat_from_ieee(-3.167830270f);
static const ffloat t2 = ffloat_from_ieee( 0.134516124f);
static const ffloat t3 = ffloat_from_ieee(-4.033321984f);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static const ffloat range_over_two_pi = ffloat_from_ieee(2670176.75f);
static const ffloat two_pi_over_range = ffloat_from_ieee(3.74507039e-07f);
static const int range_mask = 0xffffff;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static const ffloat lp00 = ffloat_from_ieee(-1.45326486f);
static const ffloat lp01 = ffloat_from_ieee(0.951366714f);
static const ffloat lp02 = ffloat_from_ieee(0.501994886f);
static const ffloat lq00 = ffloat_from_ieee(0.352143751f);
static const ffloat lq01 = ff_one;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static const ffloat sqrt2 = ffloat_from_ieee(1.4142135623730950488f);
static const ffloat ep00 = ffloat_from_ieee(7.2152891521493f);
static const ffloat ep01 = ffloat_from_ieee(0.0576900723731f);
static const ffloat eq00 = ffloat_from_ieee(20.8189237930062f);
static const ffloat eq01 = ff_one;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static ffloat cos_52(ffloat x)
{
	ffloat x2 = x * x;

	return (c1 + x2 * (c2 + x2 * (c3 + c4 * x2)));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static ffloat tan_56(ffloat x)
{
	ffloat x2 = x * x;

	return (x * (t1 + t2 * x2) / (t3 + x2));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ffloat ffloat_from_ieee(float f)
{
	float t;

	asm(
	"				swap.w	%[f]				\n"	// swap word halves
	"				ror.l	#7,%[f]				\n"	// exponent to low byte
	"				move.l	#0x80,%[t]			\n"	// load $80 mask in work register
	"				eor.b	%[t],%[f]			\n"	// convert from excess 127 to two's-complement
	"				add.b	%[f],%[f]			\n"	// from 8 to 7 bit exponent
	"				bvs.b	ffpovf%=			\n"	// branch will not fit
	"				add.b	#2<<1+1,%[f]		\n"	// adjust excess 127 to 64 and set mantissa high bit
	"				bvs.b	exphi%=				\n"	// branch exponent too large (overflow)
	"				eor.b	%[t],%[f]			\n"	// back to excess 64
 	"				ror.l	#1,%[f]				\n"	// to fast float representation ("v" cleared)
	"				bra.b	done%=				\n"
	"											\n"	// overflow detected - caused by one of the following:
	"											\n"	// - false overflow due to difference between excess 127 and 64 format
	"											\n"	// - exponent too high or low to fit in 7 bits (exponent over/underflow)
	"											\n"	// - an exponent of $ff representing an infinity
	"											\n"	// - an exponent of $00 representing a zero, nan, or denormalized value
	"ffpovf%=:		bcc.b	ffpovlw%=			\n"	// branch if overflow (exponent $ff or too large)
	"											\n"	// overflow - check for possible false overflow due to different excess formats
	"				cmp.b	#0x7c,%[f]			\n"	// ? was original argument really in range
	"				beq.b	ffpovfls%=			\n"	// yes, branch false overflow
	"				cmp.b	#0x7e,%[f]			\n"	// ? was original argument really in range
	"				bne.b	ffptovf%=			\n"	// no, branch true overflow
	"ffpovfls%=:	add.b	#0x80+2<<1+1,%[f]	\n" // excess 64 adjustment and mantissa high bit
	"				ror.l	#1,%[f]				\n"	// finalize to fast floating point format
	"				tst.b	%[f]				\n"	// insure no illegal zero sign+exponent byte
	"				bne.b	done%=				\n"	// done if does not set s+exp all zeroes
	"				bra.b	explo%=				\n"	// treat as underflowed exponent otherwise
	"											\n"	// exponent low - check for zero, denormalized value, or too small an exponent
	"ffptovf%=:		and.w	#0xfeff,%[f]		\n" // clear sign bit out
	"				tst.l	%[f]				\n"	// ? entire value now zero
	"				beq.b	done%=				\n"	// branch if value is zero
	"				tst.b	%[f]				\n"	// ? denormalized number (significant#0, exp=0)
	"				beq.b	denor%=				\n"	// branch if denormalized
	"											\n" // explo - exponent too small for ffp format
	"											\n" // the sign bit will be bit 8.
	"explo%=:		moveq	#0,%[f]				\n" // default zero for this case ("v" cleared)
	"				bra.b	done%=				\n" // return to mainline
	"											\n" // denor - denormalized number
	"denor%=:		moveq	#0,%[f]				\n"	// default is to return a zero ("v" cleared)
	"				bra.b	done%=				\n" // return to mainline
	"											\n" // exponent high - check for exponent too high, infinity, or nan
	"ffpovlw%=:		cmp.b	#0xfe,%[f]			\n" // ? was original exponent $ff
 	"				bne.b	exphi%=				\n" // no, branch exponent too large
 	"				lsr.l	#8,%[f]				\n" // shift out exponent
 	"				lsr.l	#1,%[f]				\n" // shift out sign
 	"				bne.b	nan%=				\n" // branch not-a-number
	"											\n" // inf - infinity
	"											\n" // the carry and x bit represent the sign
	"inf%=:			moveq	#-1,%[f]			\n" // setup maximum ffp value
	"				roxr.b	#1,%[f]				\n" // shift in sign
	"				or.b	#0x02,%%ccr			\n" // show overflow occured
	"				bra.b	done%=				\n" // return with maximum same sign to mainline
	"											\n" // exphi - exponent to large for ffp format
	"											\n" // the sign bit will be bit 8.
	"exphi%=:		lsl.w	#8,%[f]				\n"	// set x bit to sign
	"				moveq	#-1,%[f]			\n" // setup maximum number
	"				roxr.b	#1,%[f]				\n" // shift in sign
	"				or.b	#0x02,%%ccr			\n" // show overflow ocurred
	"				bra.b	done%=				\n" // return maximum same sign to mainline
	"											\n"	// nan - not-a-number
	"											\n"	// bits 0 thru 22 contain the nan data field
	"nan%=:			moveq	#0,%[f]				\n" // default to a zero ("v" bit cleared)
	"done%=:									\n"
	:
	[f] "+d" (f),
	[t] "=d" (t)
	:
	:
	"cc"
	);

	return ffloat(f);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
float ieee_from_ffloat(ffloat f)
{
	asm(
	"		add.l	%[f],%[f]	\n"	// delete mantissa high bit
	"		beq.b	done%=		\n"	// branch zero as finished
	"		eor.b	#0x80,%[f]	\n"	// to twos complement exponent
	"		asr.b	#1,%[f]		\n"	// form 8-bit exponent
	"		sub.b	#0x82,%[f]	\n"	// adjust 64 to 127 and excessize
	"		swap.w	%[f]		\n"	// swap for high byte placement
	"		rol.l	#7,%[f]		\n"	// set sign+exp in high byte
	"done%=:					\n"
	:
	[f] "+d" (f.value)
	:
	:
	"cc"
	);

	return f.value;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ffloat cos(ffloat x)
{
	x = abs(x);

	int i = (int) (x * range_over_two_pi);

	i &= range_mask;

	x = ffloat(i) * two_pi_over_range;

	int quadrant = (int) (x * ff_two_over_pi);

	switch (quadrant)
	{
		case 0: x =  cos_52(			x); break;
		case 1: x = -cos_52(	ff_pi - x); break;
		case 2: x = -cos_52(	x - ff_pi); break;
		case 3: x =  cos_52(ff_two_pi - x); break;
	}

	return x;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ffloat sin(ffloat x)
{
	return cos(ff_half_pi - x);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ffloat tan(ffloat x)
{
	int i = (int) (x * range_over_two_pi);

	i &= range_mask;

	x = ffloat(i) * two_pi_over_range;

	int octant = (int) (x * ff_four_over_pi);

	switch (octant)
	{
		case 0: x =			  tan_56(x						* ff_four_over_pi); break;
		case 1: x = ff_one /  tan_56((ff_half_pi - x)		* ff_four_over_pi); break;
		case 2: x = ff_one / -tan_56((x - ff_half_pi)		* ff_four_over_pi); break;
		case 3: x =			 -tan_56((ff_pi - x)			* ff_four_over_pi); break;
		case 4: x =			  tan_56((x - ff_pi)			* ff_four_over_pi); break;
		case 5: x = ff_one /  tan_56((ff_three_half_pi - x)	* ff_four_over_pi); break;
		case 6: x = ff_one / -tan_56((x - ff_three_half_pi)	* ff_four_over_pi); break;
		case 7: x =			 -tan_56((ff_two_pi - x)		* ff_four_over_pi); break;
	}

	return x;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ffloat log2(ffloat x)
{
	bool adjustment = false;
	if (x < ff_half)
	{
		x = ff_one / x;
		adjustment = true;
	}

	int i = (int) x;
	int two_to_n = 1;
	ffloat n = ff_zero;
	while (i != 0)
	{
		i >>= 1;
		two_to_n <<= 1;
		n += ff_one;
	}

	x /= ffloat(two_to_n);

	ffloat poly = (lp00 + x * (lp01 + x * lp02)) / (lq00 + lq01 * x);

	return (adjustment ? (-n - poly) : (poly + n));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ffloat exp2(ffloat x)
{
	bool negative = false;
	if (x < ff_zero)
	{
		x = -x;
		negative = true;
	}

	bool adjustment = false;
	int i = (int) x;
	if ((x - ffloat(i)) > ff_half)
	{
		adjustment = true;
		x -= ff_half;
	}
	x -= ffloat(i);

	ffloat two_int_a = ff_one;
	while (i != 0)
	{
		two_int_a *= ff_two;
		i--;
	}

	ffloat x2 = x * x;
	ffloat Q = eq00 + eq01 * x2;
	ffloat x_P = x * (ep00 + ep01 * x2);

	ffloat answer = ((Q + x_P) / (Q - x_P)) * two_int_a;

	if (adjustment)
	{
		answer *= sqrt2;
	}

	return (negative ? (ff_one / answer) : answer);
}