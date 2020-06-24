////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#define restrict __restrict__

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
typedef short s16;
typedef unsigned int u32;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
struct Vec3
{
	s16 x, y, z;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
struct Mat33
{
	s16 m00, m01, m02;
	s16 m10, m11, m12;
	s16 m20, m21, m22;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void VecMath_RotateVecs(Vec3* restrict results, int numVecs, const Vec3* restrict vecs, const Mat33& rot)
{
	// Because of the range for the sin and cos table entries this rotation ends up shrinking the vertices
	// to one quarter of their original size, it's faster to live with this than add a few extra shifts.

	#if 1

	s16 vx, vy, vz;
	s16 t0, t1;
	u32 m00_m10, m01_m11, m02_m12, m20, m21, m22;

	asm volatile(
	"		move.w	6(%[rot]),%[t0]					\n"
	"		swap	%[t0]							\n"
	"		move.w	(%[rot]),%[t0]					\n"
	"		move.l	%[t0],%[m00_m10]				\n"

	"		move.w	8(%[rot]),%[t0]					\n"
	"		swap	%[t0]							\n"
	"		move.w	2(%[rot]),%[t0]					\n"
	"		move.l	%[t0],%[m01_m11]				\n"

	"		move.w	10(%[rot]),%[t0]				\n"
	"		swap	%[t0]							\n"
	"		move.w	4(%[rot]),%[t0]					\n"
	"		movea.l	%[t0],%[m02_m12]				\n"

	"		movea.w	12(%[rot]),%[m20]				\n"
	"		movea.w	14(%[rot]),%[m21]				\n"
	"		movea.w	16(%[rot]),%[m22]				\n"

	"		subq.w	#1,%[num]						\n"
	"loop%=:										\n"

	"		movem.w	(%[vecs])+,%[vx]/%[vy]/%[vz]	\n"

	"		move.w	%[m00_m10],%[t0]				\n"
	"		muls	%[vx],%[t0]						\n"

	"		move.w	%[m01_m11],%[t1]				\n"
	"		muls	%[vy],%[t1]						\n"
	"		add.l	%[t1],%[t0]						\n"

	"		move.w	%[m02_m12],%[t1]				\n"
	"		muls	%[vz],%[t1]						\n"
	"		add.l	%[t1],%[t0]						\n"

	"		swap	%[t0]							\n"
	"		move.w	%[t0],(%[results])+				\n"

	"		move.l	%[m00_m10],%[t0]				\n"
	"		swap	%[t0]							\n"
	"		muls	%[vx],%[t0]						\n"
	"		move.l	%[m01_m11],%[t1]				\n"
	"		swap	%[t1]							\n"
	"		muls	%[vy],%[t1]						\n"
	"		add.l	%[t1],%[t0]						\n"
	"		move.l	%[m02_m12],%[t1]				\n"
	"		swap	%[t1]							\n"
	"		muls	%[vz],%[t1]						\n"
	"		add.l	%[t1],%[t0]						\n"

	"		swap	%[t0]							\n"
	"		move.w	%[t0],(%[results])+				\n"

	"		move.w	%[m20],%[t0]					\n"
	"		muls	%[vx],%[t0]						\n"
	"		move.w	%[m21],%[t1]					\n"
	"		muls	%[vy],%[t1]						\n"
	"		add.l	%[t1],%[t0]						\n"
	"		move.w	%[m22],%[t1]					\n"
	"		muls	%[vz],%[t1]						\n"
	"		add.l	%[t1],%[t0]						\n"

	"		swap	%[t0]							\n"
	"		move.w	%[t0],(%[results])+				\n"

	"		dbra	%[num],loop%=					\n"
	:
	[results]	"+a"	(results),	// "result" pointer gets incremented.
	[num]		"+d"	(numVecs),	// "numVecs" counts get decremented.
	[vecs]		"+a"	(vecs),		// "vecs" pointers gets incremented.
	[vx]		"=d"	(vx),		// "vx, vy, vz" holds current vector.
	[vy]		"=d"	(vy),
	[vz]		"=d"	(vz),
	[t0]		"=d"	(t0),		// "t0, t1" are used as temporaries.
	[t1]		"=d"	(t1),
	[m00_m10]	"=d"	(m00_m10),	// "m00..m22" holds the rotation matrix.
	[m01_m11]	"=d"	(m01_m11),
	[m02_m12]	"=&a"	(m02_m12),	// Marked "&" as early clobbers.
	[m20]		"=&a"	(m20),		// This is to prevent the compiler from overwriting
	[m21]		"=&a"	(m21),		// any input registers of the same type before we've
	[m22]		"=&a"	(m22)		// finished with them, in our case the "rot" pointer.
	:
	[rot]		"a"		(&rot)		// "rot" points to the rotation matrix.
	:
	"cc",
	"memory"
	);

	#else

	for (int i = 0; i < numVecs; i++)
	{
		results[i].x = (rot.m00 * vecs[i].x + rot.m01 * vecs[i].y + rot.m02 * vecs[i].z) >> 16;
		results[i].y = (rot.m10 * vecs[i].x + rot.m11 * vecs[i].y + rot.m12 * vecs[i].z) >> 16;
		results[i].z = (rot.m20 * vecs[i].x + rot.m21 * vecs[i].y + rot.m22 * vecs[i].z) >> 16;
	}

	#endif
}
