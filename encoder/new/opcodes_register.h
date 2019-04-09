
/*
 * Source file: new/xptr_opcodes.xml
 * Stylesheet:  registermap v0.3             (c) 2010-2015 section5.ch
 *
 */


// #include <stdlib.h>

// Allow to define an external offset
#ifndef REGISTERMAP_OFFSET
#define REGISTERMAP_OFFSET(x) (x)
#endif

#ifndef _BITMASK_
#define _BITMASK_(msb, lsb) ( ~(~0 << (msb - lsb + 1)) << lsb )
#endif
#ifndef _BIT_
#define _BIT_(pos) (1 << pos)
#endif


/******************************************************
 *  DEVICE NHWEngine
 ******************************************************
 * NHW opcode engine
 *
 * Device description version:
 * v0.0develop
 **************************************************************************/



/* Device description revision tags */
#define HWREV_opcodes_MAJOR 0
#define HWREV_opcodes_MINOR 0
#define HWREV_opcodes_EXT   "develop"


/*********************************************************
 * Address segment 'Cmd'
 *********************************************************/
		
#ifdef Unit_Offset_
#define Cmd_Offset Unit_Offset_
#else
#define Cmd_Offset 0
#endif
#define Reg_OpcodeGeneric                        (Cmd_Offset + 0x00)
#	define OPCODE                      _BIT_(15)
#	define OPCODE_SHFT  15
#	define OPCODE_RES                  _BIT_(14)
#	define OPCODE_RES_SHFT  14     /* When set, it is a residual opcode */
#	define VALUE                       _BITMASK_(9, 0)
#	define VALUE_SHFT  0
#define Reg_OpcodeRes                            (Cmd_Offset + 0x00)
#	define RES5                        _BIT_(13)
#	define RES5_SHFT  13
#	define RES3                        _BIT_(12)
#	define RES3_SHFT  12
#	define RES1                        _BIT_(11)
#	define RES1_SHFT  11
#	define ADD_W0                      _BITMASK_(10, 7)
#	define ADD_W0_SHFT  7     /* Signed value to add to W[0] */
#	define ADD_W1                      _BITMASK_(6, 4)
#	define ADD_W1_SHFT  4     /* Value to add to W[1], sign extended */
#	define ADD_W2                      _BITMASK_(3, 2)
#	define ADD_W2_SHFT  2     /* 0: 0, 1: 2, 2: -2 */
#	define RI                          _BITMASK_(1, 0)
#	define RI_SHFT  0
