#define CLAMP2(x, low, high)  \
		if (x > high) { x = high ; } else if (x < low) { x = low; }

#define CLAMPH(x, high)  \
		if (x > high) { x = high ; }
#define CLAMPL(x, low)  \
		if (x < low) { x = low ; }


#define ROUND_8(x)  ((x) & ~7)
#define ROUND_4(x)  ((x) & ~3)
#define ROUND_2(x)  ((x) & ~1)


////////////////////////////////////////////////////////////////////////////
// Special code markers:

// TODO: Put all functionality into opcodes



// Codes used for res256 buffer:
#define CODE_12100 12100
#define CODE_12200 12200
#define CODE_12300 12300
#define CODE_12400 12400
#define CODE_12500 12500
#define CODE_12600 12600

#define CODE_14000 14000
#define CODE_14100 14100
// #define CODE_14400 14500  // Not used as 'CODE', see TAG
#define CODE_14500 14500
#define CODE_14900 14900

#define IS_CODE(r)  (r > 12000)

// Tags that are the byte equivalents from the above CODEs:
// They mark the residual compensations for the final
// coordinate list of values to be compensated


#ifdef USE_OPCODES
#include "opcodes.h"
#define IS_OPCODE(a)  (a & OPCODE_RES)

#else

#define OP_R30_P 122
#define OP_R31_N 121
#define OP_R32_P 123
#define OP_R33_N 124
#define OP_R3R1N 125
#define OP_R3R0P 126

#define OP_R10_P 140
#define OP_R11_N 141

#define OP_R50_P 145
#define OP_R51_N 144
#define OP_R5R1N 148
#define OP_R5R1P 149

#endif

// Codes used for the im_process buffer:
#define OFFS_C_CODE_12600 12600
#define OFFS_C_CODE_12400 12400
#define OFFS_C_CODE_12900 12900
#define OFFS_C_CODE_13000 13000

// Markers used by image_processing:offsetY_LL_q4()
// and reduce_HL_q4()
#define MARK_121   12100
#define MARK_122   12204
#define MARK_125   10204
#define MARK_126   10300
#define MARK_127   12700
#define MARK_128   10100
#define MARK_129   12900

// These are used in offset reconstruction: offsetY_recons256_q4()
#define MARK_15300 15300
#define MARK_15400 15400
#define MARK_15500 15500
#define MARK_15600 15600
#define MARK_15700 15700
#define MARK_15800 15800

#define MARK_12000 0x2000
#define MARK_16000 0x3000
#define MARK_24000 0x4000

#define TAG_PIXEL(x, which)     (x += (which + 256))
#define UNTAG_PIXEL(x, which)   (x -= (which + 256))

#define IS_TAG(x)      (x > (MARK_16000))
#define IS_TAG_RES(x)  (x > (MARK_24000))


#ifndef __CONCAT1
#define __CONCAT1(x, y) x##y
#endif

void copy_bitplane0(unsigned char *sp, int n, unsigned char *res);
void copy_buffer(short *dst, const short *src, int x, int y);
void copy_buffer8bit(short *dst, const unsigned char *src, int n);
void copy_to_quadrant(short *dst, const short *src, int x, int y);
void copy_from_quadrant(short *dst, const short *src, int x, int y);

void copy_uv_chunks(unsigned char *dst, const short *src, int line);
void extract_bitplane(unsigned char *scan_run,
	unsigned char *tree, int n);

