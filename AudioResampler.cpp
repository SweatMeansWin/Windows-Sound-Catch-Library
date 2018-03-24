#include "AudioResampler.h"

#define MAX_HWORD (32767)
#define MIN_HWORD (-32768)

/* Conversion constants */
#define Nhc       8
#define Na        7
#define Np       (Nhc+Na)
#define Npc      (1<<Nhc)
#define Amask    ((1<<Na)-1)
#define Pmask    ((1<<Np)-1)
#define Nh       16
#define Nb       16
#define Nhxn     14
#define Nhg      (Nh-Nhxn)
#define NLpScl   13

typedef float          HWORD;
typedef unsigned short UHWORD;
typedef int            X_WORD;
typedef unsigned int   UWORD;

#define HWORD_TO_MUS_SAMPLE_TYPE(x) ((mus_sample_t)((x)<<(MUS_SAMPLE_BITS-16)))
#define HWORD_TO_MUS_SAMPLE_TYPE(x) ((mus_sample_t)((x)<<(MUS_SAMPLE_BITS-16)))

static int  SrcLinear(HWORD X[], HWORD Y[], double factor, UWORD *Time, UHWORD Nx);


int CAudioResampler::GetOutLen(int dataLen, int inRate, int outRate)
{
	double factor = (double)outRate / inRate;
	dataLen /= sizeof(HWORD);
	int outSize = (int)(((double)dataLen)*factor + 2.0);
	return outSize * sizeof(HWORD);
}

void CAudioResampler::ProcessBuffer(unsigned char* pData, int dataLen, int inRate,
	int outRate, unsigned char* out, size_t* outCount)
{
	double factor = (double)outRate / inRate;
	dataLen /= sizeof(HWORD);
	int outSize = (int)(((double)dataLen)*factor + 2.0);

	UHWORD Xoff = 0;//10;
	UWORD Time = (Xoff << Np);
	UHWORD Nx = dataLen - 2 * Xoff;
	*outCount = SrcLinear((HWORD*)pData, (HWORD*)out, factor, &Time, Nx);
	*outCount *= sizeof(HWORD);
}

static HWORD WordToHword(X_WORD v, int scl)
{
	HWORD out;
	X_WORD llsb = (1 << (scl - 1));
	v += llsb;          /* round */
	v >>= scl;
	if (v > MAX_HWORD) {
		v = MAX_HWORD;
	}
	else if (v < MIN_HWORD) {
		v = MIN_HWORD;
	}
	out = (HWORD)v;
	return out;
}


static int SrcLinear(HWORD X[], HWORD Y[], double factor, UWORD *Time, UHWORD Nx)
{
	HWORD iconst;
	HWORD *Xp, *Ystart;
	X_WORD v = 0, x1 = 0, x2 = 0;

	double dt;                  /* Step through input signal */
	UWORD dtb;                  /* Fixed-point version of Dt */
	UWORD endTime;              /* When Time reaches EndTime, return to user */

	dt = 1.0 / factor;            /* Output sampling period */
	dtb = dt * (1 << Np) + 0.5;     /* Fixed-point representation */

	Ystart = Y;
	endTime = *Time + (1 << Np)*(X_WORD)Nx;
	while (*Time < endTime)
	{
		iconst = (*Time) & Pmask;
		Xp = &X[(*Time) >> Np];      /* Ptr to current input sample */
		*Y++ = *Xp;//WordToHword(v,Np);   /* Deposit output */
		*Time += dtb;               /* Move to next sample by time increment */
	}
	return static_cast<int>(Y - Ystart);            /* Return number of output samples */
}