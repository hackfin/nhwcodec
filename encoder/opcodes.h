#include "new/opcodes_register.h"
#include "new/opcodes_register_modes.h"

#define OP_R(x)   (OPCODE_RES | x)

#define _R(x)   (x)

#define _W0(x) _BFM_(x, ADD_W0)
#define _W1(x) _BFM_(x, ADD_W1)

#define OP_R11_N  OP_R(RES1        | _R(1)  | _W0(0)                            )
#define OP_R10_P  OP_R(RES1        | _R(0)  | _W0(1)                            )
#define OP_R51_N  OP_R(RES5        | _R(1)  | _W0(2)                            )
#define OP_R50_P  OP_R(RES5        | _R(0)  | _W0(3)                            )
#define OP_R31_N  OP_R(RES3        | _R(1)  | _W0(4)  | _W1(-3)                 )
#define OP_R30_P  OP_R(RES3        | _R(0)  | _W0(5)  | _W1( 3)                 )
#define OP_R33_N  OP_R(RES3        | _R(3)  | _W0(6)  | _W1(-2) | ADD_W2_MINUS2 )
#define OP_R32_P  OP_R(RES3        | _R(2)  | _W0(7)  | _W1( 2) | ADD_W2_PLUS2  )
#define OP_R3R1N  OP_R(RES3 | RES1 | _R(1)  | _W0(8)  | _W1(-3)                 )
#define OP_R3R0P  OP_R(RES3 | RES1 | _R(0)  | _W0(9)  | _W1( 3)                 )
#define OP_R5R1N  OP_R(RES5 | RES1 | _R(1)  | _W0(10)                           )
#define OP_R5R1P  OP_R(RES5 | RES1 | _R(0)  | _W0(11)                           )

