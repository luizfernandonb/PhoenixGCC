/* Description of builtins used by the ARM backend.
   Copyright (C) 2014 Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "rtl.h"
#include "tree.h"
#include "stor-layout.h"
#include "expr.h"
#include "tm_p.h"
#include "recog.h"
#include "langhooks.h"
#include "diagnostic-core.h"
#include "optabs.h"
#include "gimple-expr.h"
#include "target.h"
#include "ggc.h"
#include "arm-protos.h"

typedef enum {
  T_V8QI,
  T_V4HI,
  T_V4HF,
  T_V2SI,
  T_V2SF,
  T_DI,
  T_V16QI,
  T_V8HI,
  T_V4SI,
  T_V4SF,
  T_V2DI,
  T_TI,
  T_EI,
  T_OI,
  T_MAX		/* Size of enum.  Keep last.  */
} neon_builtin_type_mode;

#define TYPE_MODE_BIT(X) (1 << (X))

#define TB_DREG (TYPE_MODE_BIT (T_V8QI) | TYPE_MODE_BIT (T_V4HI)	\
		 | TYPE_MODE_BIT (T_V4HF) | TYPE_MODE_BIT (T_V2SI)	\
		 | TYPE_MODE_BIT (T_V2SF) | TYPE_MODE_BIT (T_DI))
#define TB_QREG (TYPE_MODE_BIT (T_V16QI) | TYPE_MODE_BIT (T_V8HI)	\
		 | TYPE_MODE_BIT (T_V4SI) | TYPE_MODE_BIT (T_V4SF)	\
		 | TYPE_MODE_BIT (T_V2DI) | TYPE_MODE_BIT (T_TI))

#define v8qi_UP  T_V8QI
#define v4hi_UP  T_V4HI
#define v4hf_UP  T_V4HF
#define v2si_UP  T_V2SI
#define v2sf_UP  T_V2SF
#define di_UP    T_DI
#define v16qi_UP T_V16QI
#define v8hi_UP  T_V8HI
#define v4si_UP  T_V4SI
#define v4sf_UP  T_V4SF
#define v2di_UP  T_V2DI
#define ti_UP	 T_TI
#define ei_UP	 T_EI
#define oi_UP	 T_OI

#define UP(X) X##_UP

typedef enum {
  NEON_BINOP,
  NEON_TERNOP,
  NEON_UNOP,
  NEON_BSWAP,
  NEON_GETLANE,
  NEON_SETLANE,
  NEON_CREATE,
  NEON_RINT,
  NEON_COPYSIGNF,
  NEON_DUP,
  NEON_DUPLANE,
  NEON_COMBINE,
  NEON_SPLIT,
  NEON_LANEMUL,
  NEON_LANEMULL,
  NEON_LANEMULH,
  NEON_LANEMAC,
  NEON_SCALARMUL,
  NEON_SCALARMULL,
  NEON_SCALARMULH,
  NEON_SCALARMAC,
  NEON_CONVERT,
  NEON_FLOAT_WIDEN,
  NEON_FLOAT_NARROW,
  NEON_FIXCONV,
  NEON_SELECT,
  NEON_REINTERP,
  NEON_VTBL,
  NEON_VTBX,
  NEON_LOAD1,
  NEON_LOAD1LANE,
  NEON_STORE1,
  NEON_STORE1LANE,
  NEON_LOADSTRUCT,
  NEON_LOADSTRUCTLANE,
  NEON_STORESTRUCT,
  NEON_STORESTRUCTLANE,
  NEON_LOGICBINOP,
  NEON_SHIFTINSERT,
  NEON_SHIFTIMM,
  NEON_SHIFTACC
} neon_itype;

typedef struct {
  const char *name;
  const neon_itype itype;
  const neon_builtin_type_mode mode;
  const enum insn_code code;
  unsigned int fcode;
} neon_builtin_datum;

#define CF(N,X) CODE_FOR_neon_##N##X

#define VAR1(T, N, A) \
  {#N, NEON_##T, UP (A), CF (N, A), 0}
#define VAR2(T, N, A, B) \
  VAR1 (T, N, A), \
  {#N, NEON_##T, UP (B), CF (N, B), 0}
#define VAR3(T, N, A, B, C) \
  VAR2 (T, N, A, B), \
  {#N, NEON_##T, UP (C), CF (N, C), 0}
#define VAR4(T, N, A, B, C, D) \
  VAR3 (T, N, A, B, C), \
  {#N, NEON_##T, UP (D), CF (N, D), 0}
#define VAR5(T, N, A, B, C, D, E) \
  VAR4 (T, N, A, B, C, D), \
  {#N, NEON_##T, UP (E), CF (N, E), 0}
#define VAR6(T, N, A, B, C, D, E, F) \
  VAR5 (T, N, A, B, C, D, E), \
  {#N, NEON_##T, UP (F), CF (N, F), 0}
#define VAR7(T, N, A, B, C, D, E, F, G) \
  VAR6 (T, N, A, B, C, D, E, F), \
  {#N, NEON_##T, UP (G), CF (N, G), 0}
#define VAR8(T, N, A, B, C, D, E, F, G, H) \
  VAR7 (T, N, A, B, C, D, E, F, G), \
  {#N, NEON_##T, UP (H), CF (N, H), 0}
#define VAR9(T, N, A, B, C, D, E, F, G, H, I) \
  VAR8 (T, N, A, B, C, D, E, F, G, H), \
  {#N, NEON_##T, UP (I), CF (N, I), 0}
#define VAR10(T, N, A, B, C, D, E, F, G, H, I, J) \
  VAR9 (T, N, A, B, C, D, E, F, G, H, I), \
  {#N, NEON_##T, UP (J), CF (N, J), 0}

/* The NEON builtin data can be found in arm_neon_builtins.def.
   The mode entries in the following table correspond to the "key" type of the
   instruction variant, i.e. equivalent to that which would be specified after
   the assembler mnemonic, which usually refers to the last vector operand.
   (Signed/unsigned/polynomial types are not differentiated between though, and
   are all mapped onto the same mode for a given element size.) The modes
   listed per instruction should be the same as those defined for that
   instruction's pattern in neon.md.  */

static neon_builtin_datum neon_builtin_data[] =
{
#include "arm_neon_builtins.def"
};

#undef CF
#undef VAR1
#undef VAR2
#undef VAR3
#undef VAR4
#undef VAR5
#undef VAR6
#undef VAR7
#undef VAR8
#undef VAR9
#undef VAR10

#define CF(N,X) ARM_BUILTIN_NEON_##N##X
#define VAR1(T, N, A) \
  CF (N, A)
#define VAR2(T, N, A, B) \
  VAR1 (T, N, A), \
  CF (N, B)
#define VAR3(T, N, A, B, C) \
  VAR2 (T, N, A, B), \
  CF (N, C)
#define VAR4(T, N, A, B, C, D) \
  VAR3 (T, N, A, B, C), \
  CF (N, D)
#define VAR5(T, N, A, B, C, D, E) \
  VAR4 (T, N, A, B, C, D), \
  CF (N, E)
#define VAR6(T, N, A, B, C, D, E, F) \
  VAR5 (T, N, A, B, C, D, E), \
  CF (N, F)
#define VAR7(T, N, A, B, C, D, E, F, G) \
  VAR6 (T, N, A, B, C, D, E, F), \
  CF (N, G)
#define VAR8(T, N, A, B, C, D, E, F, G, H) \
  VAR7 (T, N, A, B, C, D, E, F, G), \
  CF (N, H)
#define VAR9(T, N, A, B, C, D, E, F, G, H, I) \
  VAR8 (T, N, A, B, C, D, E, F, G, H), \
  CF (N, I)
#define VAR10(T, N, A, B, C, D, E, F, G, H, I, J) \
  VAR9 (T, N, A, B, C, D, E, F, G, H, I), \
  CF (N, J)
enum arm_builtins
{
  ARM_BUILTIN_GETWCGR0,
  ARM_BUILTIN_GETWCGR1,
  ARM_BUILTIN_GETWCGR2,
  ARM_BUILTIN_GETWCGR3,

  ARM_BUILTIN_SETWCGR0,
  ARM_BUILTIN_SETWCGR1,
  ARM_BUILTIN_SETWCGR2,
  ARM_BUILTIN_SETWCGR3,

  ARM_BUILTIN_WZERO,

  ARM_BUILTIN_WAVG2BR,
  ARM_BUILTIN_WAVG2HR,
  ARM_BUILTIN_WAVG2B,
  ARM_BUILTIN_WAVG2H,

  ARM_BUILTIN_WACCB,
  ARM_BUILTIN_WACCH,
  ARM_BUILTIN_WACCW,

  ARM_BUILTIN_WMACS,
  ARM_BUILTIN_WMACSZ,
  ARM_BUILTIN_WMACU,
  ARM_BUILTIN_WMACUZ,

  ARM_BUILTIN_WSADB,
  ARM_BUILTIN_WSADBZ,
  ARM_BUILTIN_WSADH,
  ARM_BUILTIN_WSADHZ,

  ARM_BUILTIN_WALIGNI,
  ARM_BUILTIN_WALIGNR0,
  ARM_BUILTIN_WALIGNR1,
  ARM_BUILTIN_WALIGNR2,
  ARM_BUILTIN_WALIGNR3,

  ARM_BUILTIN_TMIA,
  ARM_BUILTIN_TMIAPH,
  ARM_BUILTIN_TMIABB,
  ARM_BUILTIN_TMIABT,
  ARM_BUILTIN_TMIATB,
  ARM_BUILTIN_TMIATT,

  ARM_BUILTIN_TMOVMSKB,
  ARM_BUILTIN_TMOVMSKH,
  ARM_BUILTIN_TMOVMSKW,

  ARM_BUILTIN_TBCSTB,
  ARM_BUILTIN_TBCSTH,
  ARM_BUILTIN_TBCSTW,

  ARM_BUILTIN_WMADDS,
  ARM_BUILTIN_WMADDU,

  ARM_BUILTIN_WPACKHSS,
  ARM_BUILTIN_WPACKWSS,
  ARM_BUILTIN_WPACKDSS,
  ARM_BUILTIN_WPACKHUS,
  ARM_BUILTIN_WPACKWUS,
  ARM_BUILTIN_WPACKDUS,

  ARM_BUILTIN_WADDB,
  ARM_BUILTIN_WADDH,
  ARM_BUILTIN_WADDW,
  ARM_BUILTIN_WADDSSB,
  ARM_BUILTIN_WADDSSH,
  ARM_BUILTIN_WADDSSW,
  ARM_BUILTIN_WADDUSB,
  ARM_BUILTIN_WADDUSH,
  ARM_BUILTIN_WADDUSW,
  ARM_BUILTIN_WSUBB,
  ARM_BUILTIN_WSUBH,
  ARM_BUILTIN_WSUBW,
  ARM_BUILTIN_WSUBSSB,
  ARM_BUILTIN_WSUBSSH,
  ARM_BUILTIN_WSUBSSW,
  ARM_BUILTIN_WSUBUSB,
  ARM_BUILTIN_WSUBUSH,
  ARM_BUILTIN_WSUBUSW,

  ARM_BUILTIN_WAND,
  ARM_BUILTIN_WANDN,
  ARM_BUILTIN_WOR,
  ARM_BUILTIN_WXOR,

  ARM_BUILTIN_WCMPEQB,
  ARM_BUILTIN_WCMPEQH,
  ARM_BUILTIN_WCMPEQW,
  ARM_BUILTIN_WCMPGTUB,
  ARM_BUILTIN_WCMPGTUH,
  ARM_BUILTIN_WCMPGTUW,
  ARM_BUILTIN_WCMPGTSB,
  ARM_BUILTIN_WCMPGTSH,
  ARM_BUILTIN_WCMPGTSW,

  ARM_BUILTIN_TEXTRMSB,
  ARM_BUILTIN_TEXTRMSH,
  ARM_BUILTIN_TEXTRMSW,
  ARM_BUILTIN_TEXTRMUB,
  ARM_BUILTIN_TEXTRMUH,
  ARM_BUILTIN_TEXTRMUW,
  ARM_BUILTIN_TINSRB,
  ARM_BUILTIN_TINSRH,
  ARM_BUILTIN_TINSRW,

  ARM_BUILTIN_WMAXSW,
  ARM_BUILTIN_WMAXSH,
  ARM_BUILTIN_WMAXSB,
  ARM_BUILTIN_WMAXUW,
  ARM_BUILTIN_WMAXUH,
  ARM_BUILTIN_WMAXUB,
  ARM_BUILTIN_WMINSW,
  ARM_BUILTIN_WMINSH,
  ARM_BUILTIN_WMINSB,
  ARM_BUILTIN_WMINUW,
  ARM_BUILTIN_WMINUH,
  ARM_BUILTIN_WMINUB,

  ARM_BUILTIN_WMULUM,
  ARM_BUILTIN_WMULSM,
  ARM_BUILTIN_WMULUL,

  ARM_BUILTIN_PSADBH,
  ARM_BUILTIN_WSHUFH,

  ARM_BUILTIN_WSLLH,
  ARM_BUILTIN_WSLLW,
  ARM_BUILTIN_WSLLD,
  ARM_BUILTIN_WSRAH,
  ARM_BUILTIN_WSRAW,
  ARM_BUILTIN_WSRAD,
  ARM_BUILTIN_WSRLH,
  ARM_BUILTIN_WSRLW,
  ARM_BUILTIN_WSRLD,
  ARM_BUILTIN_WRORH,
  ARM_BUILTIN_WRORW,
  ARM_BUILTIN_WRORD,
  ARM_BUILTIN_WSLLHI,
  ARM_BUILTIN_WSLLWI,
  ARM_BUILTIN_WSLLDI,
  ARM_BUILTIN_WSRAHI,
  ARM_BUILTIN_WSRAWI,
  ARM_BUILTIN_WSRADI,
  ARM_BUILTIN_WSRLHI,
  ARM_BUILTIN_WSRLWI,
  ARM_BUILTIN_WSRLDI,
  ARM_BUILTIN_WRORHI,
  ARM_BUILTIN_WRORWI,
  ARM_BUILTIN_WRORDI,

  ARM_BUILTIN_WUNPCKIHB,
  ARM_BUILTIN_WUNPCKIHH,
  ARM_BUILTIN_WUNPCKIHW,
  ARM_BUILTIN_WUNPCKILB,
  ARM_BUILTIN_WUNPCKILH,
  ARM_BUILTIN_WUNPCKILW,

  ARM_BUILTIN_WUNPCKEHSB,
  ARM_BUILTIN_WUNPCKEHSH,
  ARM_BUILTIN_WUNPCKEHSW,
  ARM_BUILTIN_WUNPCKEHUB,
  ARM_BUILTIN_WUNPCKEHUH,
  ARM_BUILTIN_WUNPCKEHUW,
  ARM_BUILTIN_WUNPCKELSB,
  ARM_BUILTIN_WUNPCKELSH,
  ARM_BUILTIN_WUNPCKELSW,
  ARM_BUILTIN_WUNPCKELUB,
  ARM_BUILTIN_WUNPCKELUH,
  ARM_BUILTIN_WUNPCKELUW,

  ARM_BUILTIN_WABSB,
  ARM_BUILTIN_WABSH,
  ARM_BUILTIN_WABSW,

  ARM_BUILTIN_WADDSUBHX,
  ARM_BUILTIN_WSUBADDHX,

  ARM_BUILTIN_WABSDIFFB,
  ARM_BUILTIN_WABSDIFFH,
  ARM_BUILTIN_WABSDIFFW,

  ARM_BUILTIN_WADDCH,
  ARM_BUILTIN_WADDCW,

  ARM_BUILTIN_WAVG4,
  ARM_BUILTIN_WAVG4R,

  ARM_BUILTIN_WMADDSX,
  ARM_BUILTIN_WMADDUX,

  ARM_BUILTIN_WMADDSN,
  ARM_BUILTIN_WMADDUN,

  ARM_BUILTIN_WMULWSM,
  ARM_BUILTIN_WMULWUM,

  ARM_BUILTIN_WMULWSMR,
  ARM_BUILTIN_WMULWUMR,

  ARM_BUILTIN_WMULWL,

  ARM_BUILTIN_WMULSMR,
  ARM_BUILTIN_WMULUMR,

  ARM_BUILTIN_WQMULM,
  ARM_BUILTIN_WQMULMR,

  ARM_BUILTIN_WQMULWM,
  ARM_BUILTIN_WQMULWMR,

  ARM_BUILTIN_WADDBHUSM,
  ARM_BUILTIN_WADDBHUSL,

  ARM_BUILTIN_WQMIABB,
  ARM_BUILTIN_WQMIABT,
  ARM_BUILTIN_WQMIATB,
  ARM_BUILTIN_WQMIATT,

  ARM_BUILTIN_WQMIABBN,
  ARM_BUILTIN_WQMIABTN,
  ARM_BUILTIN_WQMIATBN,
  ARM_BUILTIN_WQMIATTN,

  ARM_BUILTIN_WMIABB,
  ARM_BUILTIN_WMIABT,
  ARM_BUILTIN_WMIATB,
  ARM_BUILTIN_WMIATT,

  ARM_BUILTIN_WMIABBN,
  ARM_BUILTIN_WMIABTN,
  ARM_BUILTIN_WMIATBN,
  ARM_BUILTIN_WMIATTN,

  ARM_BUILTIN_WMIAWBB,
  ARM_BUILTIN_WMIAWBT,
  ARM_BUILTIN_WMIAWTB,
  ARM_BUILTIN_WMIAWTT,

  ARM_BUILTIN_WMIAWBBN,
  ARM_BUILTIN_WMIAWBTN,
  ARM_BUILTIN_WMIAWTBN,
  ARM_BUILTIN_WMIAWTTN,

  ARM_BUILTIN_WMERGE,

  ARM_BUILTIN_CRC32B,
  ARM_BUILTIN_CRC32H,
  ARM_BUILTIN_CRC32W,
  ARM_BUILTIN_CRC32CB,
  ARM_BUILTIN_CRC32CH,
  ARM_BUILTIN_CRC32CW,

  ARM_BUILTIN_GET_FPSCR,
  ARM_BUILTIN_SET_FPSCR,

#undef CRYPTO1
#undef CRYPTO2
#undef CRYPTO3

#define CRYPTO1(L, U, M1, M2) \
  ARM_BUILTIN_CRYPTO_##U,
#define CRYPTO2(L, U, M1, M2, M3) \
  ARM_BUILTIN_CRYPTO_##U,
#define CRYPTO3(L, U, M1, M2, M3, M4) \
  ARM_BUILTIN_CRYPTO_##U,

#include "crypto.def"

#undef CRYPTO1
#undef CRYPTO2
#undef CRYPTO3

#include "arm_neon_builtins.def"

  ,ARM_BUILTIN_MAX
};

#define ARM_BUILTIN_NEON_BASE (ARM_BUILTIN_MAX - ARRAY_SIZE (neon_builtin_data))

#undef CF
#undef VAR1
#undef VAR2
#undef VAR3
#undef VAR4
#undef VAR5
#undef VAR6
#undef VAR7
#undef VAR8
#undef VAR9
#undef VAR10

static GTY(()) tree arm_builtin_decls[ARM_BUILTIN_MAX];

#define NUM_DREG_TYPES 5
#define NUM_QREG_TYPES 6

static void
arm_init_neon_builtins (void)
{
  unsigned int i, fcode;
  tree decl;

  tree neon_intQI_type_node;
  tree neon_intHI_type_node;
  tree neon_floatHF_type_node;
  tree neon_polyQI_type_node;
  tree neon_polyHI_type_node;
  tree neon_intSI_type_node;
  tree neon_intDI_type_node;
  tree neon_intUTI_type_node;
  tree neon_float_type_node;

  tree intQI_pointer_node;
  tree intHI_pointer_node;
  tree intSI_pointer_node;
  tree intDI_pointer_node;
  tree float_pointer_node;

  tree const_intQI_node;
  tree const_intHI_node;
  tree const_intSI_node;
  tree const_intDI_node;
  tree const_float_node;

  tree const_intQI_pointer_node;
  tree const_intHI_pointer_node;
  tree const_intSI_pointer_node;
  tree const_intDI_pointer_node;
  tree const_float_pointer_node;

  tree V8QI_type_node;
  tree V4HI_type_node;
  tree V4UHI_type_node;
  tree V4HF_type_node;
  tree V2SI_type_node;
  tree V2USI_type_node;
  tree V2SF_type_node;
  tree V16QI_type_node;
  tree V8HI_type_node;
  tree V8UHI_type_node;
  tree V4SI_type_node;
  tree V4USI_type_node;
  tree V4SF_type_node;
  tree V2DI_type_node;
  tree V2UDI_type_node;

  tree intUQI_type_node;
  tree intUHI_type_node;
  tree intUSI_type_node;
  tree intUDI_type_node;

  tree intEI_type_node;
  tree intOI_type_node;
  tree intCI_type_node;
  tree intXI_type_node;

  tree reinterp_ftype_dreg[NUM_DREG_TYPES][NUM_DREG_TYPES];
  tree reinterp_ftype_qreg[NUM_QREG_TYPES][NUM_QREG_TYPES];
  tree dreg_types[NUM_DREG_TYPES], qreg_types[NUM_QREG_TYPES];

  /* Create distinguished type nodes for NEON vector element types,
     and pointers to values of such types, so we can detect them later.  */
  neon_intQI_type_node = make_signed_type (GET_MODE_PRECISION (QImode));
  neon_intHI_type_node = make_signed_type (GET_MODE_PRECISION (HImode));
  neon_polyQI_type_node = make_signed_type (GET_MODE_PRECISION (QImode));
  neon_polyHI_type_node = make_signed_type (GET_MODE_PRECISION (HImode));
  neon_intSI_type_node = make_signed_type (GET_MODE_PRECISION (SImode));
  neon_intDI_type_node = make_signed_type (GET_MODE_PRECISION (DImode));
  neon_float_type_node = make_node (REAL_TYPE);
  TYPE_PRECISION (neon_float_type_node) = FLOAT_TYPE_SIZE;
  layout_type (neon_float_type_node);
  neon_floatHF_type_node = make_node (REAL_TYPE);
  TYPE_PRECISION (neon_floatHF_type_node) = GET_MODE_PRECISION (HFmode);
  layout_type (neon_floatHF_type_node);

  /* Define typedefs which exactly correspond to the modes we are basing vector
     types on.  If you change these names you'll need to change
     the table used by arm_mangle_type too.  */
  (*lang_hooks.types.register_builtin_type) (neon_intQI_type_node,
					     "__builtin_neon_qi");
  (*lang_hooks.types.register_builtin_type) (neon_intHI_type_node,
					     "__builtin_neon_hi");
  (*lang_hooks.types.register_builtin_type) (neon_floatHF_type_node,
					     "__builtin_neon_hf");
  (*lang_hooks.types.register_builtin_type) (neon_intSI_type_node,
					     "__builtin_neon_si");
  (*lang_hooks.types.register_builtin_type) (neon_float_type_node,
					     "__builtin_neon_sf");
  (*lang_hooks.types.register_builtin_type) (neon_intDI_type_node,
					     "__builtin_neon_di");
  (*lang_hooks.types.register_builtin_type) (neon_polyQI_type_node,
					     "__builtin_neon_poly8");
  (*lang_hooks.types.register_builtin_type) (neon_polyHI_type_node,
					     "__builtin_neon_poly16");

  intQI_pointer_node = build_pointer_type (neon_intQI_type_node);
  intHI_pointer_node = build_pointer_type (neon_intHI_type_node);
  intSI_pointer_node = build_pointer_type (neon_intSI_type_node);
  intDI_pointer_node = build_pointer_type (neon_intDI_type_node);
  float_pointer_node = build_pointer_type (neon_float_type_node);

  /* Next create constant-qualified versions of the above types.  */
  const_intQI_node = build_qualified_type (neon_intQI_type_node,
					   TYPE_QUAL_CONST);
  const_intHI_node = build_qualified_type (neon_intHI_type_node,
					   TYPE_QUAL_CONST);
  const_intSI_node = build_qualified_type (neon_intSI_type_node,
					   TYPE_QUAL_CONST);
  const_intDI_node = build_qualified_type (neon_intDI_type_node,
					   TYPE_QUAL_CONST);
  const_float_node = build_qualified_type (neon_float_type_node,
					   TYPE_QUAL_CONST);

  const_intQI_pointer_node = build_pointer_type (const_intQI_node);
  const_intHI_pointer_node = build_pointer_type (const_intHI_node);
  const_intSI_pointer_node = build_pointer_type (const_intSI_node);
  const_intDI_pointer_node = build_pointer_type (const_intDI_node);
  const_float_pointer_node = build_pointer_type (const_float_node);

  /* Unsigned integer types for various mode sizes.  */
  intUQI_type_node = make_unsigned_type (GET_MODE_PRECISION (QImode));
  intUHI_type_node = make_unsigned_type (GET_MODE_PRECISION (HImode));
  intUSI_type_node = make_unsigned_type (GET_MODE_PRECISION (SImode));
  intUDI_type_node = make_unsigned_type (GET_MODE_PRECISION (DImode));
  neon_intUTI_type_node = make_unsigned_type (GET_MODE_PRECISION (TImode));
  /* Now create vector types based on our NEON element types.  */
  /* 64-bit vectors.  */
  V8QI_type_node =
    build_vector_type_for_mode (neon_intQI_type_node, V8QImode);
  V4HI_type_node =
    build_vector_type_for_mode (neon_intHI_type_node, V4HImode);
  V4UHI_type_node =
    build_vector_type_for_mode (intUHI_type_node, V4HImode);
  V4HF_type_node =
    build_vector_type_for_mode (neon_floatHF_type_node, V4HFmode);
  V2SI_type_node =
    build_vector_type_for_mode (neon_intSI_type_node, V2SImode);
  V2USI_type_node =
    build_vector_type_for_mode (intUSI_type_node, V2SImode);
  V2SF_type_node =
    build_vector_type_for_mode (neon_float_type_node, V2SFmode);
  /* 128-bit vectors.  */
  V16QI_type_node =
    build_vector_type_for_mode (neon_intQI_type_node, V16QImode);
  V8HI_type_node =
    build_vector_type_for_mode (neon_intHI_type_node, V8HImode);
  V8UHI_type_node =
    build_vector_type_for_mode (intUHI_type_node, V8HImode);
  V4SI_type_node =
    build_vector_type_for_mode (neon_intSI_type_node, V4SImode);
  V4USI_type_node =
    build_vector_type_for_mode (intUSI_type_node, V4SImode);
  V4SF_type_node =
    build_vector_type_for_mode (neon_float_type_node, V4SFmode);
  V2DI_type_node =
    build_vector_type_for_mode (neon_intDI_type_node, V2DImode);
  V2UDI_type_node =
    build_vector_type_for_mode (intUDI_type_node, V2DImode);


  (*lang_hooks.types.register_builtin_type) (intUQI_type_node,
					     "__builtin_neon_uqi");
  (*lang_hooks.types.register_builtin_type) (intUHI_type_node,
					     "__builtin_neon_uhi");
  (*lang_hooks.types.register_builtin_type) (intUSI_type_node,
					     "__builtin_neon_usi");
  (*lang_hooks.types.register_builtin_type) (intUDI_type_node,
					     "__builtin_neon_udi");
  (*lang_hooks.types.register_builtin_type) (intUDI_type_node,
					     "__builtin_neon_poly64");
  (*lang_hooks.types.register_builtin_type) (neon_intUTI_type_node,
					     "__builtin_neon_poly128");

  /* Opaque integer types for structures of vectors.  */
  intEI_type_node = make_signed_type (GET_MODE_PRECISION (EImode));
  intOI_type_node = make_signed_type (GET_MODE_PRECISION (OImode));
  intCI_type_node = make_signed_type (GET_MODE_PRECISION (CImode));
  intXI_type_node = make_signed_type (GET_MODE_PRECISION (XImode));

  (*lang_hooks.types.register_builtin_type) (intTI_type_node,
					     "__builtin_neon_ti");
  (*lang_hooks.types.register_builtin_type) (intEI_type_node,
					     "__builtin_neon_ei");
  (*lang_hooks.types.register_builtin_type) (intOI_type_node,
					     "__builtin_neon_oi");
  (*lang_hooks.types.register_builtin_type) (intCI_type_node,
					     "__builtin_neon_ci");
  (*lang_hooks.types.register_builtin_type) (intXI_type_node,
					     "__builtin_neon_xi");

  if (TARGET_CRYPTO && TARGET_HARD_FLOAT)
  {

    tree V16UQI_type_node =
      build_vector_type_for_mode (intUQI_type_node, V16QImode);

    tree v16uqi_ftype_v16uqi
      = build_function_type_list (V16UQI_type_node, V16UQI_type_node, NULL_TREE);

    tree v16uqi_ftype_v16uqi_v16uqi
      = build_function_type_list (V16UQI_type_node, V16UQI_type_node,
                                  V16UQI_type_node, NULL_TREE);

    tree v4usi_ftype_v4usi
      = build_function_type_list (V4USI_type_node, V4USI_type_node, NULL_TREE);

    tree v4usi_ftype_v4usi_v4usi
      = build_function_type_list (V4USI_type_node, V4USI_type_node,
                                  V4USI_type_node, NULL_TREE);

    tree v4usi_ftype_v4usi_v4usi_v4usi
      = build_function_type_list (V4USI_type_node, V4USI_type_node,
                                  V4USI_type_node, V4USI_type_node, NULL_TREE);

    tree uti_ftype_udi_udi
      = build_function_type_list (neon_intUTI_type_node, intUDI_type_node,
                                  intUDI_type_node, NULL_TREE);

    #undef CRYPTO1
    #undef CRYPTO2
    #undef CRYPTO3
    #undef C
    #undef N
    #undef CF
    #undef FT1
    #undef FT2
    #undef FT3

    #define C(U) \
      ARM_BUILTIN_CRYPTO_##U
    #define N(L) \
      "__builtin_arm_crypto_"#L
    #define FT1(R, A) \
      R##_ftype_##A
    #define FT2(R, A1, A2) \
      R##_ftype_##A1##_##A2
    #define FT3(R, A1, A2, A3) \
      R##_ftype_##A1##_##A2##_##A3
    #define CRYPTO1(L, U, R, A) \
      arm_builtin_decls[C (U)] = add_builtin_function (N (L), FT1 (R, A), \
                                                       C (U), BUILT_IN_MD, \
                                                       NULL, NULL_TREE);
    #define CRYPTO2(L, U, R, A1, A2) \
      arm_builtin_decls[C (U)] = add_builtin_function (N (L), FT2 (R, A1, A2), \
                                                       C (U), BUILT_IN_MD, \
                                                       NULL, NULL_TREE);

    #define CRYPTO3(L, U, R, A1, A2, A3) \
      arm_builtin_decls[C (U)] = add_builtin_function (N (L), FT3 (R, A1, A2, A3), \
                                                       C (U), BUILT_IN_MD, \
                                                       NULL, NULL_TREE);
    #include "crypto.def"

    #undef CRYPTO1
    #undef CRYPTO2
    #undef CRYPTO3
    #undef C
    #undef N
    #undef FT1
    #undef FT2
    #undef FT3
  }
  dreg_types[0] = V8QI_type_node;
  dreg_types[1] = V4HI_type_node;
  dreg_types[2] = V2SI_type_node;
  dreg_types[3] = V2SF_type_node;
  dreg_types[4] = neon_intDI_type_node;

  qreg_types[0] = V16QI_type_node;
  qreg_types[1] = V8HI_type_node;
  qreg_types[2] = V4SI_type_node;
  qreg_types[3] = V4SF_type_node;
  qreg_types[4] = V2DI_type_node;
  qreg_types[5] = neon_intUTI_type_node;

  for (i = 0; i < NUM_QREG_TYPES; i++)
    {
      int j;
      for (j = 0; j < NUM_QREG_TYPES; j++)
        {
          if (i < NUM_DREG_TYPES && j < NUM_DREG_TYPES)
            reinterp_ftype_dreg[i][j]
              = build_function_type_list (dreg_types[i], dreg_types[j], NULL);

          reinterp_ftype_qreg[i][j]
            = build_function_type_list (qreg_types[i], qreg_types[j], NULL);
        }
    }

  for (i = 0, fcode = ARM_BUILTIN_NEON_BASE;
       i < ARRAY_SIZE (neon_builtin_data);
       i++, fcode++)
    {
      neon_builtin_datum *d = &neon_builtin_data[i];

      const char* const modenames[] = {
	"v8qi", "v4hi", "v4hf", "v2si", "v2sf", "di",
	"v16qi", "v8hi", "v4si", "v4sf", "v2di",
	"ti", "ei", "oi"
      };
      char namebuf[60];
      tree ftype = NULL;
      int is_load = 0, is_store = 0;

      gcc_assert (ARRAY_SIZE (modenames) == T_MAX);

      d->fcode = fcode;

      switch (d->itype)
	{
	case NEON_LOAD1:
	case NEON_LOAD1LANE:
	case NEON_LOADSTRUCT:
	case NEON_LOADSTRUCTLANE:
	  is_load = 1;
	  /* Fall through.  */
	case NEON_STORE1:
	case NEON_STORE1LANE:
	case NEON_STORESTRUCT:
	case NEON_STORESTRUCTLANE:
	  if (!is_load)
	    is_store = 1;
	  /* Fall through.  */
	case NEON_UNOP:
	case NEON_RINT:
	case NEON_BINOP:
	case NEON_LOGICBINOP:
	case NEON_SHIFTINSERT:
	case NEON_TERNOP:
	case NEON_GETLANE:
	case NEON_SETLANE:
	case NEON_CREATE:
	case NEON_DUP:
	case NEON_DUPLANE:
	case NEON_SHIFTIMM:
	case NEON_SHIFTACC:
	case NEON_COMBINE:
	case NEON_SPLIT:
	case NEON_CONVERT:
	case NEON_FIXCONV:
	case NEON_LANEMUL:
	case NEON_LANEMULL:
	case NEON_LANEMULH:
	case NEON_LANEMAC:
	case NEON_SCALARMUL:
	case NEON_SCALARMULL:
	case NEON_SCALARMULH:
	case NEON_SCALARMAC:
	case NEON_SELECT:
	case NEON_VTBL:
	case NEON_VTBX:
	  {
	    int k;
	    tree return_type = void_type_node, args = void_list_node;

	    /* Build a function type directly from the insn_data for
	       this builtin.  The build_function_type() function takes
	       care of removing duplicates for us.  */
	    for (k = insn_data[d->code].n_generator_args - 1; k >= 0; k--)
	      {
		tree eltype;

		if (is_load && k == 1)
		  {
		    /* Neon load patterns always have the memory
		       operand in the operand 1 position.  */
		    gcc_assert (insn_data[d->code].operand[k].predicate
				== neon_struct_operand);

		    switch (d->mode)
		      {
		      case T_V8QI:
		      case T_V16QI:
			eltype = const_intQI_pointer_node;
			break;

		      case T_V4HI:
		      case T_V8HI:
			eltype = const_intHI_pointer_node;
			break;

		      case T_V2SI:
		      case T_V4SI:
			eltype = const_intSI_pointer_node;
			break;

		      case T_V2SF:
		      case T_V4SF:
			eltype = const_float_pointer_node;
			break;

		      case T_DI:
		      case T_V2DI:
			eltype = const_intDI_pointer_node;
			break;

		      default: gcc_unreachable ();
		      }
		  }
		else if (is_store && k == 0)
		  {
		    /* Similarly, Neon store patterns use operand 0 as
		       the memory location to store to.  */
		    gcc_assert (insn_data[d->code].operand[k].predicate
				== neon_struct_operand);

		    switch (d->mode)
		      {
		      case T_V8QI:
		      case T_V16QI:
			eltype = intQI_pointer_node;
			break;

		      case T_V4HI:
		      case T_V8HI:
			eltype = intHI_pointer_node;
			break;

		      case T_V2SI:
		      case T_V4SI:
			eltype = intSI_pointer_node;
			break;

		      case T_V2SF:
		      case T_V4SF:
			eltype = float_pointer_node;
			break;

		      case T_DI:
		      case T_V2DI:
			eltype = intDI_pointer_node;
			break;

		      default: gcc_unreachable ();
		      }
		  }
		else
		  {
		    switch (insn_data[d->code].operand[k].mode)
		      {
		      case VOIDmode: eltype = void_type_node; break;
			/* Scalars.  */
		      case QImode: eltype = neon_intQI_type_node; break;
		      case HImode: eltype = neon_intHI_type_node; break;
		      case SImode: eltype = neon_intSI_type_node; break;
		      case SFmode: eltype = neon_float_type_node; break;
		      case DImode: eltype = neon_intDI_type_node; break;
		      case TImode: eltype = intTI_type_node; break;
		      case EImode: eltype = intEI_type_node; break;
		      case OImode: eltype = intOI_type_node; break;
		      case CImode: eltype = intCI_type_node; break;
		      case XImode: eltype = intXI_type_node; break;
			/* 64-bit vectors.  */
		      case V8QImode: eltype = V8QI_type_node; break;
		      case V4HImode: eltype = V4HI_type_node; break;
		      case V2SImode: eltype = V2SI_type_node; break;
		      case V2SFmode: eltype = V2SF_type_node; break;
			/* 128-bit vectors.  */
		      case V16QImode: eltype = V16QI_type_node; break;
		      case V8HImode: eltype = V8HI_type_node; break;
		      case V4SImode: eltype = V4SI_type_node; break;
		      case V4SFmode: eltype = V4SF_type_node; break;
		      case V2DImode: eltype = V2DI_type_node; break;
		      default: gcc_unreachable ();
		      }
		  }

		if (k == 0 && !is_store)
		  return_type = eltype;
		else
		  args = tree_cons (NULL_TREE, eltype, args);
	      }

	    ftype = build_function_type (return_type, args);
	  }
	  break;

	case NEON_REINTERP:
	  {
	    /* We iterate over NUM_DREG_TYPES doubleword types,
	       then NUM_QREG_TYPES quadword  types.
	       V4HF is not a type used in reinterpret, so we translate
	       d->mode to the correct index in reinterp_ftype_dreg.  */
	    bool qreg_p
	      = GET_MODE_SIZE (insn_data[d->code].operand[0].mode) > 8;
	    int rhs = (d->mode - ((!qreg_p && (d->mode > T_V4HF)) ? 1 : 0))
	              % NUM_QREG_TYPES;
	    switch (insn_data[d->code].operand[0].mode)
	      {
	      case V8QImode: ftype = reinterp_ftype_dreg[0][rhs]; break;
	      case V4HImode: ftype = reinterp_ftype_dreg[1][rhs]; break;
	      case V2SImode: ftype = reinterp_ftype_dreg[2][rhs]; break;
	      case V2SFmode: ftype = reinterp_ftype_dreg[3][rhs]; break;
	      case DImode: ftype = reinterp_ftype_dreg[4][rhs]; break;
	      case V16QImode: ftype = reinterp_ftype_qreg[0][rhs]; break;
	      case V8HImode: ftype = reinterp_ftype_qreg[1][rhs]; break;
	      case V4SImode: ftype = reinterp_ftype_qreg[2][rhs]; break;
	      case V4SFmode: ftype = reinterp_ftype_qreg[3][rhs]; break;
	      case V2DImode: ftype = reinterp_ftype_qreg[4][rhs]; break;
	      case TImode: ftype = reinterp_ftype_qreg[5][rhs]; break;
	      default: gcc_unreachable ();
	      }
	  }
	  break;
	case NEON_FLOAT_WIDEN:
	  {
	    tree eltype = NULL_TREE;
	    tree return_type = NULL_TREE;

	    switch (insn_data[d->code].operand[1].mode)
	    {
	      case V4HFmode:
	        eltype = V4HF_type_node;
	        return_type = V4SF_type_node;
	        break;
	      default: gcc_unreachable ();
	    }
	    ftype = build_function_type_list (return_type, eltype, NULL);
	    break;
	  }
	case NEON_FLOAT_NARROW:
	  {
	    tree eltype = NULL_TREE;
	    tree return_type = NULL_TREE;

	    switch (insn_data[d->code].operand[1].mode)
	    {
	      case V4SFmode:
	        eltype = V4SF_type_node;
	        return_type = V4HF_type_node;
	        break;
	      default: gcc_unreachable ();
	    }
	    ftype = build_function_type_list (return_type, eltype, NULL);
	    break;
	  }
	case NEON_BSWAP:
	{
	    tree eltype = NULL_TREE;
	    switch (insn_data[d->code].operand[1].mode)
	    {
	      case V4HImode:
	        eltype = V4UHI_type_node;
	        break;
	      case V8HImode:
	        eltype = V8UHI_type_node;
	        break;
	      case V2SImode:
	        eltype = V2USI_type_node;
	        break;
	      case V4SImode:
	        eltype = V4USI_type_node;
	        break;
	      case V2DImode:
	        eltype = V2UDI_type_node;
	        break;
	      default: gcc_unreachable ();
	    }
	    ftype = build_function_type_list (eltype, eltype, NULL);
	    break;
	}
	case NEON_COPYSIGNF:
	  {
	    tree eltype = NULL_TREE;
	    switch (insn_data[d->code].operand[1].mode)
	      {
	      case V2SFmode:
		eltype = V2SF_type_node;
		break;
	      case V4SFmode:
		eltype = V4SF_type_node;
		break;
	      default: gcc_unreachable ();
	      }
	    ftype = build_function_type_list (eltype, eltype, NULL);
	    break;
	  }
	default:
	  gcc_unreachable ();
	}

      gcc_assert (ftype != NULL);

      sprintf (namebuf, "__builtin_neon_%s%s", d->name, modenames[d->mode]);

      decl = add_builtin_function (namebuf, ftype, fcode, BUILT_IN_MD, NULL,
				   NULL_TREE);
      arm_builtin_decls[fcode] = decl;
    }
}

#undef NUM_DREG_TYPES
#undef NUM_QREG_TYPES

#define def_mbuiltin(MASK, NAME, TYPE, CODE)				\
  do									\
    {									\
      if ((MASK) & insn_flags)						\
	{								\
	  tree bdecl;							\
	  bdecl = add_builtin_function ((NAME), (TYPE), (CODE),		\
					BUILT_IN_MD, NULL, NULL_TREE);	\
	  arm_builtin_decls[CODE] = bdecl;				\
	}								\
    }									\
  while (0)

struct builtin_description
{
  const unsigned int       mask;
  const enum insn_code     icode;
  const char * const       name;
  const enum arm_builtins  code;
  const enum rtx_code      comparison;
  const unsigned int       flag;
};

static const struct builtin_description bdesc_2arg[] =
{
#define IWMMXT_BUILTIN(code, string, builtin) \
  { FL_IWMMXT, CODE_FOR_##code, "__builtin_arm_" string, \
    ARM_BUILTIN_##builtin, UNKNOWN, 0 },

#define IWMMXT2_BUILTIN(code, string, builtin) \
  { FL_IWMMXT2, CODE_FOR_##code, "__builtin_arm_" string, \
    ARM_BUILTIN_##builtin, UNKNOWN, 0 },

  IWMMXT_BUILTIN (addv8qi3, "waddb", WADDB)
  IWMMXT_BUILTIN (addv4hi3, "waddh", WADDH)
  IWMMXT_BUILTIN (addv2si3, "waddw", WADDW)
  IWMMXT_BUILTIN (subv8qi3, "wsubb", WSUBB)
  IWMMXT_BUILTIN (subv4hi3, "wsubh", WSUBH)
  IWMMXT_BUILTIN (subv2si3, "wsubw", WSUBW)
  IWMMXT_BUILTIN (ssaddv8qi3, "waddbss", WADDSSB)
  IWMMXT_BUILTIN (ssaddv4hi3, "waddhss", WADDSSH)
  IWMMXT_BUILTIN (ssaddv2si3, "waddwss", WADDSSW)
  IWMMXT_BUILTIN (sssubv8qi3, "wsubbss", WSUBSSB)
  IWMMXT_BUILTIN (sssubv4hi3, "wsubhss", WSUBSSH)
  IWMMXT_BUILTIN (sssubv2si3, "wsubwss", WSUBSSW)
  IWMMXT_BUILTIN (usaddv8qi3, "waddbus", WADDUSB)
  IWMMXT_BUILTIN (usaddv4hi3, "waddhus", WADDUSH)
  IWMMXT_BUILTIN (usaddv2si3, "waddwus", WADDUSW)
  IWMMXT_BUILTIN (ussubv8qi3, "wsubbus", WSUBUSB)
  IWMMXT_BUILTIN (ussubv4hi3, "wsubhus", WSUBUSH)
  IWMMXT_BUILTIN (ussubv2si3, "wsubwus", WSUBUSW)
  IWMMXT_BUILTIN (mulv4hi3, "wmulul", WMULUL)
  IWMMXT_BUILTIN (smulv4hi3_highpart, "wmulsm", WMULSM)
  IWMMXT_BUILTIN (umulv4hi3_highpart, "wmulum", WMULUM)
  IWMMXT_BUILTIN (eqv8qi3, "wcmpeqb", WCMPEQB)
  IWMMXT_BUILTIN (eqv4hi3, "wcmpeqh", WCMPEQH)
  IWMMXT_BUILTIN (eqv2si3, "wcmpeqw", WCMPEQW)
  IWMMXT_BUILTIN (gtuv8qi3, "wcmpgtub", WCMPGTUB)
  IWMMXT_BUILTIN (gtuv4hi3, "wcmpgtuh", WCMPGTUH)
  IWMMXT_BUILTIN (gtuv2si3, "wcmpgtuw", WCMPGTUW)
  IWMMXT_BUILTIN (gtv8qi3, "wcmpgtsb", WCMPGTSB)
  IWMMXT_BUILTIN (gtv4hi3, "wcmpgtsh", WCMPGTSH)
  IWMMXT_BUILTIN (gtv2si3, "wcmpgtsw", WCMPGTSW)
  IWMMXT_BUILTIN (umaxv8qi3, "wmaxub", WMAXUB)
  IWMMXT_BUILTIN (smaxv8qi3, "wmaxsb", WMAXSB)
  IWMMXT_BUILTIN (umaxv4hi3, "wmaxuh", WMAXUH)
  IWMMXT_BUILTIN (smaxv4hi3, "wmaxsh", WMAXSH)
  IWMMXT_BUILTIN (umaxv2si3, "wmaxuw", WMAXUW)
  IWMMXT_BUILTIN (smaxv2si3, "wmaxsw", WMAXSW)
  IWMMXT_BUILTIN (uminv8qi3, "wminub", WMINUB)
  IWMMXT_BUILTIN (sminv8qi3, "wminsb", WMINSB)
  IWMMXT_BUILTIN (uminv4hi3, "wminuh", WMINUH)
  IWMMXT_BUILTIN (sminv4hi3, "wminsh", WMINSH)
  IWMMXT_BUILTIN (uminv2si3, "wminuw", WMINUW)
  IWMMXT_BUILTIN (sminv2si3, "wminsw", WMINSW)
  IWMMXT_BUILTIN (iwmmxt_anddi3, "wand", WAND)
  IWMMXT_BUILTIN (iwmmxt_nanddi3, "wandn", WANDN)
  IWMMXT_BUILTIN (iwmmxt_iordi3, "wor", WOR)
  IWMMXT_BUILTIN (iwmmxt_xordi3, "wxor", WXOR)
  IWMMXT_BUILTIN (iwmmxt_uavgv8qi3, "wavg2b", WAVG2B)
  IWMMXT_BUILTIN (iwmmxt_uavgv4hi3, "wavg2h", WAVG2H)
  IWMMXT_BUILTIN (iwmmxt_uavgrndv8qi3, "wavg2br", WAVG2BR)
  IWMMXT_BUILTIN (iwmmxt_uavgrndv4hi3, "wavg2hr", WAVG2HR)
  IWMMXT_BUILTIN (iwmmxt_wunpckilb, "wunpckilb", WUNPCKILB)
  IWMMXT_BUILTIN (iwmmxt_wunpckilh, "wunpckilh", WUNPCKILH)
  IWMMXT_BUILTIN (iwmmxt_wunpckilw, "wunpckilw", WUNPCKILW)
  IWMMXT_BUILTIN (iwmmxt_wunpckihb, "wunpckihb", WUNPCKIHB)
  IWMMXT_BUILTIN (iwmmxt_wunpckihh, "wunpckihh", WUNPCKIHH)
  IWMMXT_BUILTIN (iwmmxt_wunpckihw, "wunpckihw", WUNPCKIHW)
  IWMMXT2_BUILTIN (iwmmxt_waddsubhx, "waddsubhx", WADDSUBHX)
  IWMMXT2_BUILTIN (iwmmxt_wsubaddhx, "wsubaddhx", WSUBADDHX)
  IWMMXT2_BUILTIN (iwmmxt_wabsdiffb, "wabsdiffb", WABSDIFFB)
  IWMMXT2_BUILTIN (iwmmxt_wabsdiffh, "wabsdiffh", WABSDIFFH)
  IWMMXT2_BUILTIN (iwmmxt_wabsdiffw, "wabsdiffw", WABSDIFFW)
  IWMMXT2_BUILTIN (iwmmxt_avg4, "wavg4", WAVG4)
  IWMMXT2_BUILTIN (iwmmxt_avg4r, "wavg4r", WAVG4R)
  IWMMXT2_BUILTIN (iwmmxt_wmulwsm, "wmulwsm", WMULWSM)
  IWMMXT2_BUILTIN (iwmmxt_wmulwum, "wmulwum", WMULWUM)
  IWMMXT2_BUILTIN (iwmmxt_wmulwsmr, "wmulwsmr", WMULWSMR)
  IWMMXT2_BUILTIN (iwmmxt_wmulwumr, "wmulwumr", WMULWUMR)
  IWMMXT2_BUILTIN (iwmmxt_wmulwl, "wmulwl", WMULWL)
  IWMMXT2_BUILTIN (iwmmxt_wmulsmr, "wmulsmr", WMULSMR)
  IWMMXT2_BUILTIN (iwmmxt_wmulumr, "wmulumr", WMULUMR)
  IWMMXT2_BUILTIN (iwmmxt_wqmulm, "wqmulm", WQMULM)
  IWMMXT2_BUILTIN (iwmmxt_wqmulmr, "wqmulmr", WQMULMR)
  IWMMXT2_BUILTIN (iwmmxt_wqmulwm, "wqmulwm", WQMULWM)
  IWMMXT2_BUILTIN (iwmmxt_wqmulwmr, "wqmulwmr", WQMULWMR)
  IWMMXT_BUILTIN (iwmmxt_walignr0, "walignr0", WALIGNR0)
  IWMMXT_BUILTIN (iwmmxt_walignr1, "walignr1", WALIGNR1)
  IWMMXT_BUILTIN (iwmmxt_walignr2, "walignr2", WALIGNR2)
  IWMMXT_BUILTIN (iwmmxt_walignr3, "walignr3", WALIGNR3)

#define IWMMXT_BUILTIN2(code, builtin) \
  { FL_IWMMXT, CODE_FOR_##code, NULL, ARM_BUILTIN_##builtin, UNKNOWN, 0 },

#define IWMMXT2_BUILTIN2(code, builtin) \
  { FL_IWMMXT2, CODE_FOR_##code, NULL, ARM_BUILTIN_##builtin, UNKNOWN, 0 },

  IWMMXT2_BUILTIN2 (iwmmxt_waddbhusm, WADDBHUSM)
  IWMMXT2_BUILTIN2 (iwmmxt_waddbhusl, WADDBHUSL)
  IWMMXT_BUILTIN2 (iwmmxt_wpackhss, WPACKHSS)
  IWMMXT_BUILTIN2 (iwmmxt_wpackwss, WPACKWSS)
  IWMMXT_BUILTIN2 (iwmmxt_wpackdss, WPACKDSS)
  IWMMXT_BUILTIN2 (iwmmxt_wpackhus, WPACKHUS)
  IWMMXT_BUILTIN2 (iwmmxt_wpackwus, WPACKWUS)
  IWMMXT_BUILTIN2 (iwmmxt_wpackdus, WPACKDUS)
  IWMMXT_BUILTIN2 (iwmmxt_wmacuz, WMACUZ)
  IWMMXT_BUILTIN2 (iwmmxt_wmacsz, WMACSZ)


#define FP_BUILTIN(L, U) \
  {0, CODE_FOR_##L, "__builtin_arm_"#L, ARM_BUILTIN_##U, \
   UNKNOWN, 0},

  FP_BUILTIN (get_fpscr, GET_FPSCR)
  FP_BUILTIN (set_fpscr, SET_FPSCR)
#undef FP_BUILTIN

#define CRC32_BUILTIN(L, U) \
  {0, CODE_FOR_##L, "__builtin_arm_"#L, ARM_BUILTIN_##U, \
   UNKNOWN, 0},
   CRC32_BUILTIN (crc32b, CRC32B)
   CRC32_BUILTIN (crc32h, CRC32H)
   CRC32_BUILTIN (crc32w, CRC32W)
   CRC32_BUILTIN (crc32cb, CRC32CB)
   CRC32_BUILTIN (crc32ch, CRC32CH)
   CRC32_BUILTIN (crc32cw, CRC32CW)
#undef CRC32_BUILTIN


#define CRYPTO_BUILTIN(L, U) \
  {0, CODE_FOR_crypto_##L, "__builtin_arm_crypto_"#L, ARM_BUILTIN_CRYPTO_##U, \
   UNKNOWN, 0},
#undef CRYPTO1
#undef CRYPTO2
#undef CRYPTO3
#define CRYPTO2(L, U, R, A1, A2) CRYPTO_BUILTIN (L, U)
#define CRYPTO1(L, U, R, A)
#define CRYPTO3(L, U, R, A1, A2, A3)
#include "crypto.def"
#undef CRYPTO1
#undef CRYPTO2
#undef CRYPTO3

};

static const struct builtin_description bdesc_1arg[] =
{
  IWMMXT_BUILTIN (iwmmxt_tmovmskb, "tmovmskb", TMOVMSKB)
  IWMMXT_BUILTIN (iwmmxt_tmovmskh, "tmovmskh", TMOVMSKH)
  IWMMXT_BUILTIN (iwmmxt_tmovmskw, "tmovmskw", TMOVMSKW)
  IWMMXT_BUILTIN (iwmmxt_waccb, "waccb", WACCB)
  IWMMXT_BUILTIN (iwmmxt_wacch, "wacch", WACCH)
  IWMMXT_BUILTIN (iwmmxt_waccw, "waccw", WACCW)
  IWMMXT_BUILTIN (iwmmxt_wunpckehub, "wunpckehub", WUNPCKEHUB)
  IWMMXT_BUILTIN (iwmmxt_wunpckehuh, "wunpckehuh", WUNPCKEHUH)
  IWMMXT_BUILTIN (iwmmxt_wunpckehuw, "wunpckehuw", WUNPCKEHUW)
  IWMMXT_BUILTIN (iwmmxt_wunpckehsb, "wunpckehsb", WUNPCKEHSB)
  IWMMXT_BUILTIN (iwmmxt_wunpckehsh, "wunpckehsh", WUNPCKEHSH)
  IWMMXT_BUILTIN (iwmmxt_wunpckehsw, "wunpckehsw", WUNPCKEHSW)
  IWMMXT_BUILTIN (iwmmxt_wunpckelub, "wunpckelub", WUNPCKELUB)
  IWMMXT_BUILTIN (iwmmxt_wunpckeluh, "wunpckeluh", WUNPCKELUH)
  IWMMXT_BUILTIN (iwmmxt_wunpckeluw, "wunpckeluw", WUNPCKELUW)
  IWMMXT_BUILTIN (iwmmxt_wunpckelsb, "wunpckelsb", WUNPCKELSB)
  IWMMXT_BUILTIN (iwmmxt_wunpckelsh, "wunpckelsh", WUNPCKELSH)
  IWMMXT_BUILTIN (iwmmxt_wunpckelsw, "wunpckelsw", WUNPCKELSW)
  IWMMXT2_BUILTIN (iwmmxt_wabsv8qi3, "wabsb", WABSB)
  IWMMXT2_BUILTIN (iwmmxt_wabsv4hi3, "wabsh", WABSH)
  IWMMXT2_BUILTIN (iwmmxt_wabsv2si3, "wabsw", WABSW)
  IWMMXT_BUILTIN (tbcstv8qi, "tbcstb", TBCSTB)
  IWMMXT_BUILTIN (tbcstv4hi, "tbcsth", TBCSTH)
  IWMMXT_BUILTIN (tbcstv2si, "tbcstw", TBCSTW)

#define CRYPTO1(L, U, R, A) CRYPTO_BUILTIN (L, U)
#define CRYPTO2(L, U, R, A1, A2)
#define CRYPTO3(L, U, R, A1, A2, A3)
#include "crypto.def"
#undef CRYPTO1
#undef CRYPTO2
#undef CRYPTO3
};

static const struct builtin_description bdesc_3arg[] =
{
#define CRYPTO3(L, U, R, A1, A2, A3) CRYPTO_BUILTIN (L, U)
#define CRYPTO1(L, U, R, A)
#define CRYPTO2(L, U, R, A1, A2)
#include "crypto.def"
#undef CRYPTO1
#undef CRYPTO2
#undef CRYPTO3
 };
#undef CRYPTO_BUILTIN

/* Set up all the iWMMXt builtins.  This is not called if
   TARGET_IWMMXT is zero.  */

static void
arm_init_iwmmxt_builtins (void)
{
  const struct builtin_description * d;
  size_t i;

  tree V2SI_type_node = build_vector_type_for_mode (intSI_type_node, V2SImode);
  tree V4HI_type_node = build_vector_type_for_mode (intHI_type_node, V4HImode);
  tree V8QI_type_node = build_vector_type_for_mode (intQI_type_node, V8QImode);

  tree v8qi_ftype_v8qi_v8qi_int
    = build_function_type_list (V8QI_type_node,
				V8QI_type_node, V8QI_type_node,
				integer_type_node, NULL_TREE);
  tree v4hi_ftype_v4hi_int
    = build_function_type_list (V4HI_type_node,
				V4HI_type_node, integer_type_node, NULL_TREE);
  tree v2si_ftype_v2si_int
    = build_function_type_list (V2SI_type_node,
				V2SI_type_node, integer_type_node, NULL_TREE);
  tree v2si_ftype_di_di
    = build_function_type_list (V2SI_type_node,
				long_long_integer_type_node,
				long_long_integer_type_node,
				NULL_TREE);
  tree di_ftype_di_int
    = build_function_type_list (long_long_integer_type_node,
				long_long_integer_type_node,
				integer_type_node, NULL_TREE);
  tree di_ftype_di_int_int
    = build_function_type_list (long_long_integer_type_node,
				long_long_integer_type_node,
				integer_type_node,
				integer_type_node, NULL_TREE);
  tree int_ftype_v8qi
    = build_function_type_list (integer_type_node,
				V8QI_type_node, NULL_TREE);
  tree int_ftype_v4hi
    = build_function_type_list (integer_type_node,
				V4HI_type_node, NULL_TREE);
  tree int_ftype_v2si
    = build_function_type_list (integer_type_node,
				V2SI_type_node, NULL_TREE);
  tree int_ftype_v8qi_int
    = build_function_type_list (integer_type_node,
				V8QI_type_node, integer_type_node, NULL_TREE);
  tree int_ftype_v4hi_int
    = build_function_type_list (integer_type_node,
				V4HI_type_node, integer_type_node, NULL_TREE);
  tree int_ftype_v2si_int
    = build_function_type_list (integer_type_node,
				V2SI_type_node, integer_type_node, NULL_TREE);
  tree v8qi_ftype_v8qi_int_int
    = build_function_type_list (V8QI_type_node,
				V8QI_type_node, integer_type_node,
				integer_type_node, NULL_TREE);
  tree v4hi_ftype_v4hi_int_int
    = build_function_type_list (V4HI_type_node,
				V4HI_type_node, integer_type_node,
				integer_type_node, NULL_TREE);
  tree v2si_ftype_v2si_int_int
    = build_function_type_list (V2SI_type_node,
				V2SI_type_node, integer_type_node,
				integer_type_node, NULL_TREE);
  /* Miscellaneous.  */
  tree v8qi_ftype_v4hi_v4hi
    = build_function_type_list (V8QI_type_node,
				V4HI_type_node, V4HI_type_node, NULL_TREE);
  tree v4hi_ftype_v2si_v2si
    = build_function_type_list (V4HI_type_node,
				V2SI_type_node, V2SI_type_node, NULL_TREE);
  tree v8qi_ftype_v4hi_v8qi
    = build_function_type_list (V8QI_type_node,
	                        V4HI_type_node, V8QI_type_node, NULL_TREE);
  tree v2si_ftype_v4hi_v4hi
    = build_function_type_list (V2SI_type_node,
				V4HI_type_node, V4HI_type_node, NULL_TREE);
  tree v2si_ftype_v8qi_v8qi
    = build_function_type_list (V2SI_type_node,
				V8QI_type_node, V8QI_type_node, NULL_TREE);
  tree v4hi_ftype_v4hi_di
    = build_function_type_list (V4HI_type_node,
				V4HI_type_node, long_long_integer_type_node,
				NULL_TREE);
  tree v2si_ftype_v2si_di
    = build_function_type_list (V2SI_type_node,
				V2SI_type_node, long_long_integer_type_node,
				NULL_TREE);
  tree di_ftype_void
    = build_function_type_list (long_long_unsigned_type_node, NULL_TREE);
  tree int_ftype_void
    = build_function_type_list (integer_type_node, NULL_TREE);
  tree di_ftype_v8qi
    = build_function_type_list (long_long_integer_type_node,
				V8QI_type_node, NULL_TREE);
  tree di_ftype_v4hi
    = build_function_type_list (long_long_integer_type_node,
				V4HI_type_node, NULL_TREE);
  tree di_ftype_v2si
    = build_function_type_list (long_long_integer_type_node,
				V2SI_type_node, NULL_TREE);
  tree v2si_ftype_v4hi
    = build_function_type_list (V2SI_type_node,
				V4HI_type_node, NULL_TREE);
  tree v4hi_ftype_v8qi
    = build_function_type_list (V4HI_type_node,
				V8QI_type_node, NULL_TREE);
  tree v8qi_ftype_v8qi
    = build_function_type_list (V8QI_type_node,
	                        V8QI_type_node, NULL_TREE);
  tree v4hi_ftype_v4hi
    = build_function_type_list (V4HI_type_node,
	                        V4HI_type_node, NULL_TREE);
  tree v2si_ftype_v2si
    = build_function_type_list (V2SI_type_node,
	                        V2SI_type_node, NULL_TREE);

  tree di_ftype_di_v4hi_v4hi
    = build_function_type_list (long_long_unsigned_type_node,
				long_long_unsigned_type_node,
				V4HI_type_node, V4HI_type_node,
				NULL_TREE);

  tree di_ftype_v4hi_v4hi
    = build_function_type_list (long_long_unsigned_type_node,
				V4HI_type_node,V4HI_type_node,
				NULL_TREE);

  tree v2si_ftype_v2si_v4hi_v4hi
    = build_function_type_list (V2SI_type_node,
                                V2SI_type_node, V4HI_type_node,
                                V4HI_type_node, NULL_TREE);

  tree v2si_ftype_v2si_v8qi_v8qi
    = build_function_type_list (V2SI_type_node,
                                V2SI_type_node, V8QI_type_node,
                                V8QI_type_node, NULL_TREE);

  tree di_ftype_di_v2si_v2si
     = build_function_type_list (long_long_unsigned_type_node,
                                 long_long_unsigned_type_node,
                                 V2SI_type_node, V2SI_type_node,
                                 NULL_TREE);

   tree di_ftype_di_di_int
     = build_function_type_list (long_long_unsigned_type_node,
                                 long_long_unsigned_type_node,
                                 long_long_unsigned_type_node,
                                 integer_type_node, NULL_TREE);

   tree void_ftype_int
     = build_function_type_list (void_type_node,
                                 integer_type_node, NULL_TREE);

   tree v8qi_ftype_char
     = build_function_type_list (V8QI_type_node,
                                 signed_char_type_node, NULL_TREE);

   tree v4hi_ftype_short
     = build_function_type_list (V4HI_type_node,
                                 short_integer_type_node, NULL_TREE);

   tree v2si_ftype_int
     = build_function_type_list (V2SI_type_node,
                                 integer_type_node, NULL_TREE);

  /* Normal vector binops.  */
  tree v8qi_ftype_v8qi_v8qi
    = build_function_type_list (V8QI_type_node,
				V8QI_type_node, V8QI_type_node, NULL_TREE);
  tree v4hi_ftype_v4hi_v4hi
    = build_function_type_list (V4HI_type_node,
				V4HI_type_node,V4HI_type_node, NULL_TREE);
  tree v2si_ftype_v2si_v2si
    = build_function_type_list (V2SI_type_node,
				V2SI_type_node, V2SI_type_node, NULL_TREE);
  tree di_ftype_di_di
    = build_function_type_list (long_long_unsigned_type_node,
				long_long_unsigned_type_node,
				long_long_unsigned_type_node,
				NULL_TREE);

  /* Add all builtins that are more or less simple operations on two
     operands.  */
  for (i = 0, d = bdesc_2arg; i < ARRAY_SIZE (bdesc_2arg); i++, d++)
    {
      /* Use one of the operands; the target can have a different mode for
	 mask-generating compares.  */
      machine_mode mode;
      tree type;

      if (d->name == 0 || !(d->mask == FL_IWMMXT || d->mask == FL_IWMMXT2))
	continue;

      mode = insn_data[d->icode].operand[1].mode;

      switch (mode)
	{
	case V8QImode:
	  type = v8qi_ftype_v8qi_v8qi;
	  break;
	case V4HImode:
	  type = v4hi_ftype_v4hi_v4hi;
	  break;
	case V2SImode:
	  type = v2si_ftype_v2si_v2si;
	  break;
	case DImode:
	  type = di_ftype_di_di;
	  break;

	default:
	  gcc_unreachable ();
	}

      def_mbuiltin (d->mask, d->name, type, d->code);
    }

  /* Add the remaining MMX insns with somewhat more complicated types.  */
#define iwmmx_mbuiltin(NAME, TYPE, CODE)			\
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_" NAME, (TYPE),	\
		ARM_BUILTIN_ ## CODE)

#define iwmmx2_mbuiltin(NAME, TYPE, CODE)                      \
  def_mbuiltin (FL_IWMMXT2, "__builtin_arm_" NAME, (TYPE),     \
               ARM_BUILTIN_ ## CODE)

  iwmmx_mbuiltin ("wzero", di_ftype_void, WZERO);
  iwmmx_mbuiltin ("setwcgr0", void_ftype_int, SETWCGR0);
  iwmmx_mbuiltin ("setwcgr1", void_ftype_int, SETWCGR1);
  iwmmx_mbuiltin ("setwcgr2", void_ftype_int, SETWCGR2);
  iwmmx_mbuiltin ("setwcgr3", void_ftype_int, SETWCGR3);
  iwmmx_mbuiltin ("getwcgr0", int_ftype_void, GETWCGR0);
  iwmmx_mbuiltin ("getwcgr1", int_ftype_void, GETWCGR1);
  iwmmx_mbuiltin ("getwcgr2", int_ftype_void, GETWCGR2);
  iwmmx_mbuiltin ("getwcgr3", int_ftype_void, GETWCGR3);

  iwmmx_mbuiltin ("wsllh", v4hi_ftype_v4hi_di, WSLLH);
  iwmmx_mbuiltin ("wsllw", v2si_ftype_v2si_di, WSLLW);
  iwmmx_mbuiltin ("wslld", di_ftype_di_di, WSLLD);
  iwmmx_mbuiltin ("wsllhi", v4hi_ftype_v4hi_int, WSLLHI);
  iwmmx_mbuiltin ("wsllwi", v2si_ftype_v2si_int, WSLLWI);
  iwmmx_mbuiltin ("wslldi", di_ftype_di_int, WSLLDI);

  iwmmx_mbuiltin ("wsrlh", v4hi_ftype_v4hi_di, WSRLH);
  iwmmx_mbuiltin ("wsrlw", v2si_ftype_v2si_di, WSRLW);
  iwmmx_mbuiltin ("wsrld", di_ftype_di_di, WSRLD);
  iwmmx_mbuiltin ("wsrlhi", v4hi_ftype_v4hi_int, WSRLHI);
  iwmmx_mbuiltin ("wsrlwi", v2si_ftype_v2si_int, WSRLWI);
  iwmmx_mbuiltin ("wsrldi", di_ftype_di_int, WSRLDI);

  iwmmx_mbuiltin ("wsrah", v4hi_ftype_v4hi_di, WSRAH);
  iwmmx_mbuiltin ("wsraw", v2si_ftype_v2si_di, WSRAW);
  iwmmx_mbuiltin ("wsrad", di_ftype_di_di, WSRAD);
  iwmmx_mbuiltin ("wsrahi", v4hi_ftype_v4hi_int, WSRAHI);
  iwmmx_mbuiltin ("wsrawi", v2si_ftype_v2si_int, WSRAWI);
  iwmmx_mbuiltin ("wsradi", di_ftype_di_int, WSRADI);

  iwmmx_mbuiltin ("wrorh", v4hi_ftype_v4hi_di, WRORH);
  iwmmx_mbuiltin ("wrorw", v2si_ftype_v2si_di, WRORW);
  iwmmx_mbuiltin ("wrord", di_ftype_di_di, WRORD);
  iwmmx_mbuiltin ("wrorhi", v4hi_ftype_v4hi_int, WRORHI);
  iwmmx_mbuiltin ("wrorwi", v2si_ftype_v2si_int, WRORWI);
  iwmmx_mbuiltin ("wrordi", di_ftype_di_int, WRORDI);

  iwmmx_mbuiltin ("wshufh", v4hi_ftype_v4hi_int, WSHUFH);

  iwmmx_mbuiltin ("wsadb", v2si_ftype_v2si_v8qi_v8qi, WSADB);
  iwmmx_mbuiltin ("wsadh", v2si_ftype_v2si_v4hi_v4hi, WSADH);
  iwmmx_mbuiltin ("wmadds", v2si_ftype_v4hi_v4hi, WMADDS);
  iwmmx2_mbuiltin ("wmaddsx", v2si_ftype_v4hi_v4hi, WMADDSX);
  iwmmx2_mbuiltin ("wmaddsn", v2si_ftype_v4hi_v4hi, WMADDSN);
  iwmmx_mbuiltin ("wmaddu", v2si_ftype_v4hi_v4hi, WMADDU);
  iwmmx2_mbuiltin ("wmaddux", v2si_ftype_v4hi_v4hi, WMADDUX);
  iwmmx2_mbuiltin ("wmaddun", v2si_ftype_v4hi_v4hi, WMADDUN);
  iwmmx_mbuiltin ("wsadbz", v2si_ftype_v8qi_v8qi, WSADBZ);
  iwmmx_mbuiltin ("wsadhz", v2si_ftype_v4hi_v4hi, WSADHZ);

  iwmmx_mbuiltin ("textrmsb", int_ftype_v8qi_int, TEXTRMSB);
  iwmmx_mbuiltin ("textrmsh", int_ftype_v4hi_int, TEXTRMSH);
  iwmmx_mbuiltin ("textrmsw", int_ftype_v2si_int, TEXTRMSW);
  iwmmx_mbuiltin ("textrmub", int_ftype_v8qi_int, TEXTRMUB);
  iwmmx_mbuiltin ("textrmuh", int_ftype_v4hi_int, TEXTRMUH);
  iwmmx_mbuiltin ("textrmuw", int_ftype_v2si_int, TEXTRMUW);
  iwmmx_mbuiltin ("tinsrb", v8qi_ftype_v8qi_int_int, TINSRB);
  iwmmx_mbuiltin ("tinsrh", v4hi_ftype_v4hi_int_int, TINSRH);
  iwmmx_mbuiltin ("tinsrw", v2si_ftype_v2si_int_int, TINSRW);

  iwmmx_mbuiltin ("waccb", di_ftype_v8qi, WACCB);
  iwmmx_mbuiltin ("wacch", di_ftype_v4hi, WACCH);
  iwmmx_mbuiltin ("waccw", di_ftype_v2si, WACCW);

  iwmmx_mbuiltin ("tmovmskb", int_ftype_v8qi, TMOVMSKB);
  iwmmx_mbuiltin ("tmovmskh", int_ftype_v4hi, TMOVMSKH);
  iwmmx_mbuiltin ("tmovmskw", int_ftype_v2si, TMOVMSKW);

  iwmmx2_mbuiltin ("waddbhusm", v8qi_ftype_v4hi_v8qi, WADDBHUSM);
  iwmmx2_mbuiltin ("waddbhusl", v8qi_ftype_v4hi_v8qi, WADDBHUSL);

  iwmmx_mbuiltin ("wpackhss", v8qi_ftype_v4hi_v4hi, WPACKHSS);
  iwmmx_mbuiltin ("wpackhus", v8qi_ftype_v4hi_v4hi, WPACKHUS);
  iwmmx_mbuiltin ("wpackwus", v4hi_ftype_v2si_v2si, WPACKWUS);
  iwmmx_mbuiltin ("wpackwss", v4hi_ftype_v2si_v2si, WPACKWSS);
  iwmmx_mbuiltin ("wpackdus", v2si_ftype_di_di, WPACKDUS);
  iwmmx_mbuiltin ("wpackdss", v2si_ftype_di_di, WPACKDSS);

  iwmmx_mbuiltin ("wunpckehub", v4hi_ftype_v8qi, WUNPCKEHUB);
  iwmmx_mbuiltin ("wunpckehuh", v2si_ftype_v4hi, WUNPCKEHUH);
  iwmmx_mbuiltin ("wunpckehuw", di_ftype_v2si, WUNPCKEHUW);
  iwmmx_mbuiltin ("wunpckehsb", v4hi_ftype_v8qi, WUNPCKEHSB);
  iwmmx_mbuiltin ("wunpckehsh", v2si_ftype_v4hi, WUNPCKEHSH);
  iwmmx_mbuiltin ("wunpckehsw", di_ftype_v2si, WUNPCKEHSW);
  iwmmx_mbuiltin ("wunpckelub", v4hi_ftype_v8qi, WUNPCKELUB);
  iwmmx_mbuiltin ("wunpckeluh", v2si_ftype_v4hi, WUNPCKELUH);
  iwmmx_mbuiltin ("wunpckeluw", di_ftype_v2si, WUNPCKELUW);
  iwmmx_mbuiltin ("wunpckelsb", v4hi_ftype_v8qi, WUNPCKELSB);
  iwmmx_mbuiltin ("wunpckelsh", v2si_ftype_v4hi, WUNPCKELSH);
  iwmmx_mbuiltin ("wunpckelsw", di_ftype_v2si, WUNPCKELSW);

  iwmmx_mbuiltin ("wmacs", di_ftype_di_v4hi_v4hi, WMACS);
  iwmmx_mbuiltin ("wmacsz", di_ftype_v4hi_v4hi, WMACSZ);
  iwmmx_mbuiltin ("wmacu", di_ftype_di_v4hi_v4hi, WMACU);
  iwmmx_mbuiltin ("wmacuz", di_ftype_v4hi_v4hi, WMACUZ);

  iwmmx_mbuiltin ("walign", v8qi_ftype_v8qi_v8qi_int, WALIGNI);
  iwmmx_mbuiltin ("tmia", di_ftype_di_int_int, TMIA);
  iwmmx_mbuiltin ("tmiaph", di_ftype_di_int_int, TMIAPH);
  iwmmx_mbuiltin ("tmiabb", di_ftype_di_int_int, TMIABB);
  iwmmx_mbuiltin ("tmiabt", di_ftype_di_int_int, TMIABT);
  iwmmx_mbuiltin ("tmiatb", di_ftype_di_int_int, TMIATB);
  iwmmx_mbuiltin ("tmiatt", di_ftype_di_int_int, TMIATT);

  iwmmx2_mbuiltin ("wabsb", v8qi_ftype_v8qi, WABSB);
  iwmmx2_mbuiltin ("wabsh", v4hi_ftype_v4hi, WABSH);
  iwmmx2_mbuiltin ("wabsw", v2si_ftype_v2si, WABSW);

  iwmmx2_mbuiltin ("wqmiabb", v2si_ftype_v2si_v4hi_v4hi, WQMIABB);
  iwmmx2_mbuiltin ("wqmiabt", v2si_ftype_v2si_v4hi_v4hi, WQMIABT);
  iwmmx2_mbuiltin ("wqmiatb", v2si_ftype_v2si_v4hi_v4hi, WQMIATB);
  iwmmx2_mbuiltin ("wqmiatt", v2si_ftype_v2si_v4hi_v4hi, WQMIATT);

  iwmmx2_mbuiltin ("wqmiabbn", v2si_ftype_v2si_v4hi_v4hi, WQMIABBN);
  iwmmx2_mbuiltin ("wqmiabtn", v2si_ftype_v2si_v4hi_v4hi, WQMIABTN);
  iwmmx2_mbuiltin ("wqmiatbn", v2si_ftype_v2si_v4hi_v4hi, WQMIATBN);
  iwmmx2_mbuiltin ("wqmiattn", v2si_ftype_v2si_v4hi_v4hi, WQMIATTN);

  iwmmx2_mbuiltin ("wmiabb", di_ftype_di_v4hi_v4hi, WMIABB);
  iwmmx2_mbuiltin ("wmiabt", di_ftype_di_v4hi_v4hi, WMIABT);
  iwmmx2_mbuiltin ("wmiatb", di_ftype_di_v4hi_v4hi, WMIATB);
  iwmmx2_mbuiltin ("wmiatt", di_ftype_di_v4hi_v4hi, WMIATT);

  iwmmx2_mbuiltin ("wmiabbn", di_ftype_di_v4hi_v4hi, WMIABBN);
  iwmmx2_mbuiltin ("wmiabtn", di_ftype_di_v4hi_v4hi, WMIABTN);
  iwmmx2_mbuiltin ("wmiatbn", di_ftype_di_v4hi_v4hi, WMIATBN);
  iwmmx2_mbuiltin ("wmiattn", di_ftype_di_v4hi_v4hi, WMIATTN);

  iwmmx2_mbuiltin ("wmiawbb", di_ftype_di_v2si_v2si, WMIAWBB);
  iwmmx2_mbuiltin ("wmiawbt", di_ftype_di_v2si_v2si, WMIAWBT);
  iwmmx2_mbuiltin ("wmiawtb", di_ftype_di_v2si_v2si, WMIAWTB);
  iwmmx2_mbuiltin ("wmiawtt", di_ftype_di_v2si_v2si, WMIAWTT);

  iwmmx2_mbuiltin ("wmiawbbn", di_ftype_di_v2si_v2si, WMIAWBBN);
  iwmmx2_mbuiltin ("wmiawbtn", di_ftype_di_v2si_v2si, WMIAWBTN);
  iwmmx2_mbuiltin ("wmiawtbn", di_ftype_di_v2si_v2si, WMIAWTBN);
  iwmmx2_mbuiltin ("wmiawttn", di_ftype_di_v2si_v2si, WMIAWTTN);

  iwmmx2_mbuiltin ("wmerge", di_ftype_di_di_int, WMERGE);

  iwmmx_mbuiltin ("tbcstb", v8qi_ftype_char, TBCSTB);
  iwmmx_mbuiltin ("tbcsth", v4hi_ftype_short, TBCSTH);
  iwmmx_mbuiltin ("tbcstw", v2si_ftype_int, TBCSTW);

#undef iwmmx_mbuiltin
#undef iwmmx2_mbuiltin
}

static void
arm_init_fp16_builtins (void)
{
  tree fp16_type = make_node (REAL_TYPE);
  TYPE_PRECISION (fp16_type) = 16;
  layout_type (fp16_type);
  (*lang_hooks.types.register_builtin_type) (fp16_type, "__fp16");
}

static void
arm_init_crc32_builtins ()
{
  tree si_ftype_si_qi
    = build_function_type_list (unsigned_intSI_type_node,
                                unsigned_intSI_type_node,
                                unsigned_intQI_type_node, NULL_TREE);
  tree si_ftype_si_hi
    = build_function_type_list (unsigned_intSI_type_node,
                                unsigned_intSI_type_node,
                                unsigned_intHI_type_node, NULL_TREE);
  tree si_ftype_si_si
    = build_function_type_list (unsigned_intSI_type_node,
                                unsigned_intSI_type_node,
                                unsigned_intSI_type_node, NULL_TREE);

  arm_builtin_decls[ARM_BUILTIN_CRC32B]
    = add_builtin_function ("__builtin_arm_crc32b", si_ftype_si_qi,
                            ARM_BUILTIN_CRC32B, BUILT_IN_MD, NULL, NULL_TREE);
  arm_builtin_decls[ARM_BUILTIN_CRC32H]
    = add_builtin_function ("__builtin_arm_crc32h", si_ftype_si_hi,
                            ARM_BUILTIN_CRC32H, BUILT_IN_MD, NULL, NULL_TREE);
  arm_builtin_decls[ARM_BUILTIN_CRC32W]
    = add_builtin_function ("__builtin_arm_crc32w", si_ftype_si_si,
                            ARM_BUILTIN_CRC32W, BUILT_IN_MD, NULL, NULL_TREE);
  arm_builtin_decls[ARM_BUILTIN_CRC32CB]
    = add_builtin_function ("__builtin_arm_crc32cb", si_ftype_si_qi,
                            ARM_BUILTIN_CRC32CB, BUILT_IN_MD, NULL, NULL_TREE);
  arm_builtin_decls[ARM_BUILTIN_CRC32CH]
    = add_builtin_function ("__builtin_arm_crc32ch", si_ftype_si_hi,
                            ARM_BUILTIN_CRC32CH, BUILT_IN_MD, NULL, NULL_TREE);
  arm_builtin_decls[ARM_BUILTIN_CRC32CW]
    = add_builtin_function ("__builtin_arm_crc32cw", si_ftype_si_si,
                            ARM_BUILTIN_CRC32CW, BUILT_IN_MD, NULL, NULL_TREE);
}

void
arm_init_builtins (void)
{
  if (TARGET_REALLY_IWMMXT)
    arm_init_iwmmxt_builtins ();

  if (TARGET_NEON)
    arm_init_neon_builtins ();

  if (arm_fp16_format)
    arm_init_fp16_builtins ();

  if (TARGET_CRC32)
    arm_init_crc32_builtins ();

  if (TARGET_VFP && TARGET_HARD_FLOAT)
    {
      tree ftype_set_fpscr
	= build_function_type_list (void_type_node, unsigned_type_node, NULL);
      tree ftype_get_fpscr
	= build_function_type_list (unsigned_type_node, NULL);

      arm_builtin_decls[ARM_BUILTIN_GET_FPSCR]
	= add_builtin_function ("__builtin_arm_ldfscr", ftype_get_fpscr,
				ARM_BUILTIN_GET_FPSCR, BUILT_IN_MD, NULL, NULL_TREE);
      arm_builtin_decls[ARM_BUILTIN_SET_FPSCR]
	= add_builtin_function ("__builtin_arm_stfscr", ftype_set_fpscr,
				ARM_BUILTIN_SET_FPSCR, BUILT_IN_MD, NULL, NULL_TREE);
    }
}

/* Return the ARM builtin for CODE.  */

tree
arm_builtin_decl (unsigned code, bool initialize_p ATTRIBUTE_UNUSED)
{
  if (code >= ARM_BUILTIN_MAX)
    return error_mark_node;

  return arm_builtin_decls[code];
}

/* Errors in the source file can cause expand_expr to return const0_rtx
   where we expect a vector.  To avoid crashing, use one of the vector
   clear instructions.  */

static rtx
safe_vector_operand (rtx x, machine_mode mode)
{
  if (x != const0_rtx)
    return x;
  x = gen_reg_rtx (mode);

  emit_insn (gen_iwmmxt_clrdi (mode == DImode ? x
			       : gen_rtx_SUBREG (DImode, x, 0)));
  return x;
}

/* Function to expand ternary builtins.  */
static rtx
arm_expand_ternop_builtin (enum insn_code icode,
                           tree exp, rtx target)
{
  rtx pat;
  tree arg0 = CALL_EXPR_ARG (exp, 0);
  tree arg1 = CALL_EXPR_ARG (exp, 1);
  tree arg2 = CALL_EXPR_ARG (exp, 2);

  rtx op0 = expand_normal (arg0);
  rtx op1 = expand_normal (arg1);
  rtx op2 = expand_normal (arg2);
  rtx op3 = NULL_RTX;

  /* The sha1c, sha1p, sha1m crypto builtins require a different vec_select
     lane operand depending on endianness.  */
  bool builtin_sha1cpm_p = false;

  if (insn_data[icode].n_operands == 5)
    {
      gcc_assert (icode == CODE_FOR_crypto_sha1c
                  || icode == CODE_FOR_crypto_sha1p
                  || icode == CODE_FOR_crypto_sha1m);
      builtin_sha1cpm_p = true;
    }
  machine_mode tmode = insn_data[icode].operand[0].mode;
  machine_mode mode0 = insn_data[icode].operand[1].mode;
  machine_mode mode1 = insn_data[icode].operand[2].mode;
  machine_mode mode2 = insn_data[icode].operand[3].mode;


  if (VECTOR_MODE_P (mode0))
    op0 = safe_vector_operand (op0, mode0);
  if (VECTOR_MODE_P (mode1))
    op1 = safe_vector_operand (op1, mode1);
  if (VECTOR_MODE_P (mode2))
    op2 = safe_vector_operand (op2, mode2);

  if (! target
      || GET_MODE (target) != tmode
      || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
    target = gen_reg_rtx (tmode);

  gcc_assert ((GET_MODE (op0) == mode0 || GET_MODE (op0) == VOIDmode)
	      && (GET_MODE (op1) == mode1 || GET_MODE (op1) == VOIDmode)
	      && (GET_MODE (op2) == mode2 || GET_MODE (op2) == VOIDmode));

  if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
    op0 = copy_to_mode_reg (mode0, op0);
  if (! (*insn_data[icode].operand[2].predicate) (op1, mode1))
    op1 = copy_to_mode_reg (mode1, op1);
  if (! (*insn_data[icode].operand[3].predicate) (op2, mode2))
    op2 = copy_to_mode_reg (mode2, op2);
  if (builtin_sha1cpm_p)
    op3 = GEN_INT (TARGET_BIG_END ? 1 : 0);

  if (builtin_sha1cpm_p)
    pat = GEN_FCN (icode) (target, op0, op1, op2, op3);
  else
    pat = GEN_FCN (icode) (target, op0, op1, op2);
  if (! pat)
    return 0;
  emit_insn (pat);
  return target;
}

/* Subroutine of arm_expand_builtin to take care of binop insns.  */

static rtx
arm_expand_binop_builtin (enum insn_code icode,
			  tree exp, rtx target)
{
  rtx pat;
  tree arg0 = CALL_EXPR_ARG (exp, 0);
  tree arg1 = CALL_EXPR_ARG (exp, 1);
  rtx op0 = expand_normal (arg0);
  rtx op1 = expand_normal (arg1);
  machine_mode tmode = insn_data[icode].operand[0].mode;
  machine_mode mode0 = insn_data[icode].operand[1].mode;
  machine_mode mode1 = insn_data[icode].operand[2].mode;

  if (VECTOR_MODE_P (mode0))
    op0 = safe_vector_operand (op0, mode0);
  if (VECTOR_MODE_P (mode1))
    op1 = safe_vector_operand (op1, mode1);

  if (! target
      || GET_MODE (target) != tmode
      || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
    target = gen_reg_rtx (tmode);

  gcc_assert ((GET_MODE (op0) == mode0 || GET_MODE (op0) == VOIDmode)
	      && (GET_MODE (op1) == mode1 || GET_MODE (op1) == VOIDmode));

  if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
    op0 = copy_to_mode_reg (mode0, op0);
  if (! (*insn_data[icode].operand[2].predicate) (op1, mode1))
    op1 = copy_to_mode_reg (mode1, op1);

  pat = GEN_FCN (icode) (target, op0, op1);
  if (! pat)
    return 0;
  emit_insn (pat);
  return target;
}

/* Subroutine of arm_expand_builtin to take care of unop insns.  */

static rtx
arm_expand_unop_builtin (enum insn_code icode,
			 tree exp, rtx target, int do_load)
{
  rtx pat;
  tree arg0 = CALL_EXPR_ARG (exp, 0);
  rtx op0 = expand_normal (arg0);
  rtx op1 = NULL_RTX;
  machine_mode tmode = insn_data[icode].operand[0].mode;
  machine_mode mode0 = insn_data[icode].operand[1].mode;
  bool builtin_sha1h_p = false;

  if (insn_data[icode].n_operands == 3)
    {
      gcc_assert (icode == CODE_FOR_crypto_sha1h);
      builtin_sha1h_p = true;
    }

  if (! target
      || GET_MODE (target) != tmode
      || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
    target = gen_reg_rtx (tmode);
  if (do_load)
    op0 = gen_rtx_MEM (mode0, copy_to_mode_reg (Pmode, op0));
  else
    {
      if (VECTOR_MODE_P (mode0))
	op0 = safe_vector_operand (op0, mode0);

      if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
	op0 = copy_to_mode_reg (mode0, op0);
    }
  if (builtin_sha1h_p)
    op1 = GEN_INT (TARGET_BIG_END ? 1 : 0);

  if (builtin_sha1h_p)
    pat = GEN_FCN (icode) (target, op0, op1);
  else
    pat = GEN_FCN (icode) (target, op0);
  if (! pat)
    return 0;
  emit_insn (pat);
  return target;
}

typedef enum {
  NEON_ARG_COPY_TO_REG,
  NEON_ARG_CONSTANT,
  NEON_ARG_MEMORY,
  NEON_ARG_STOP
} builtin_arg;

#define NEON_MAX_BUILTIN_ARGS 5

/* EXP is a pointer argument to a Neon load or store intrinsic.  Derive
   and return an expression for the accessed memory.

   The intrinsic function operates on a block of registers that has
   mode REG_MODE.  This block contains vectors of type TYPE_MODE.  The
   function references the memory at EXP of type TYPE and in mode
   MEM_MODE; this mode may be BLKmode if no more suitable mode is
   available.  */

static tree
neon_dereference_pointer (tree exp, tree type, machine_mode mem_mode,
			  machine_mode reg_mode,
			  neon_builtin_type_mode type_mode)
{
  HOST_WIDE_INT reg_size, vector_size, nvectors, nelems;
  tree elem_type, upper_bound, array_type;

  /* Work out the size of the register block in bytes.  */
  reg_size = GET_MODE_SIZE (reg_mode);

  /* Work out the size of each vector in bytes.  */
  gcc_assert (TYPE_MODE_BIT (type_mode) & (TB_DREG | TB_QREG));
  vector_size = (TYPE_MODE_BIT (type_mode) & TB_QREG ? 16 : 8);

  /* Work out how many vectors there are.  */
  gcc_assert (reg_size % vector_size == 0);
  nvectors = reg_size / vector_size;

  /* Work out the type of each element.  */
  gcc_assert (POINTER_TYPE_P (type));
  elem_type = TREE_TYPE (type);

  /* Work out how many elements are being loaded or stored.
     MEM_MODE == REG_MODE implies a one-to-one mapping between register
     and memory elements; anything else implies a lane load or store.  */
  if (mem_mode == reg_mode)
    nelems = vector_size * nvectors / int_size_in_bytes (elem_type);
  else
    nelems = nvectors;

  /* Create a type that describes the full access.  */
  upper_bound = build_int_cst (size_type_node, nelems - 1);
  array_type = build_array_type (elem_type, build_index_type (upper_bound));

  /* Dereference EXP using that type.  */
  return fold_build2 (MEM_REF, array_type, exp,
		      build_int_cst (build_pointer_type (array_type), 0));
}

/* Expand a Neon builtin.  */
static rtx
arm_expand_neon_args (rtx target, int icode, int have_retval,
		      neon_builtin_type_mode type_mode,
		      tree exp, int fcode, ...)
{
  va_list ap;
  rtx pat;
  tree arg[NEON_MAX_BUILTIN_ARGS];
  rtx op[NEON_MAX_BUILTIN_ARGS];
  tree arg_type;
  tree formals;
  machine_mode tmode = insn_data[icode].operand[0].mode;
  machine_mode mode[NEON_MAX_BUILTIN_ARGS];
  machine_mode other_mode;
  int argc = 0;
  int opno;

  if (have_retval
      && (!target
	  || GET_MODE (target) != tmode
	  || !(*insn_data[icode].operand[0].predicate) (target, tmode)))
    target = gen_reg_rtx (tmode);

  va_start (ap, fcode);

  formals = TYPE_ARG_TYPES (TREE_TYPE (arm_builtin_decls[fcode]));

  for (;;)
    {
      builtin_arg thisarg = (builtin_arg) va_arg (ap, int);

      if (thisarg == NEON_ARG_STOP)
        break;
      else
        {
          opno = argc + have_retval;
          mode[argc] = insn_data[icode].operand[opno].mode;
          arg[argc] = CALL_EXPR_ARG (exp, argc);
	  arg_type = TREE_VALUE (formals);
          if (thisarg == NEON_ARG_MEMORY)
            {
              other_mode = insn_data[icode].operand[1 - opno].mode;
              arg[argc] = neon_dereference_pointer (arg[argc], arg_type,
						    mode[argc], other_mode,
						    type_mode);
            }

	  /* Use EXPAND_MEMORY for NEON_ARG_MEMORY to ensure a MEM_P
	     be returned.  */
	  op[argc] = expand_expr (arg[argc], NULL_RTX, VOIDmode,
				  (thisarg == NEON_ARG_MEMORY
				   ? EXPAND_MEMORY : EXPAND_NORMAL));

          switch (thisarg)
            {
            case NEON_ARG_COPY_TO_REG:
              /*gcc_assert (GET_MODE (op[argc]) == mode[argc]);*/
              if (!(*insn_data[icode].operand[opno].predicate)
                     (op[argc], mode[argc]))
                op[argc] = copy_to_mode_reg (mode[argc], op[argc]);
              break;

            case NEON_ARG_CONSTANT:
              /* FIXME: This error message is somewhat unhelpful.  */
              if (!(*insn_data[icode].operand[opno].predicate)
                    (op[argc], mode[argc]))
		error ("argument must be a constant");
              break;

            case NEON_ARG_MEMORY:
	      /* Check if expand failed.  */
	      if (op[argc] == const0_rtx)
		return 0;
	      gcc_assert (MEM_P (op[argc]));
	      PUT_MODE (op[argc], mode[argc]);
	      /* ??? arm_neon.h uses the same built-in functions for signed
		 and unsigned accesses, casting where necessary.  This isn't
		 alias safe.  */
	      set_mem_alias_set (op[argc], 0);
	      if (!(*insn_data[icode].operand[opno].predicate)
                    (op[argc], mode[argc]))
		op[argc] = (replace_equiv_address
			    (op[argc], force_reg (Pmode, XEXP (op[argc], 0))));
              break;

            case NEON_ARG_STOP:
              gcc_unreachable ();
            }

          argc++;
	  formals = TREE_CHAIN (formals);
        }
    }

  va_end (ap);

  if (have_retval)
    switch (argc)
      {
      case 1:
	pat = GEN_FCN (icode) (target, op[0]);
	break;

      case 2:
	pat = GEN_FCN (icode) (target, op[0], op[1]);
	break;

      case 3:
	pat = GEN_FCN (icode) (target, op[0], op[1], op[2]);
	break;

      case 4:
	pat = GEN_FCN (icode) (target, op[0], op[1], op[2], op[3]);
	break;

      case 5:
	pat = GEN_FCN (icode) (target, op[0], op[1], op[2], op[3], op[4]);
	break;

      default:
	gcc_unreachable ();
      }
  else
    switch (argc)
      {
      case 1:
	pat = GEN_FCN (icode) (op[0]);
	break;

      case 2:
	pat = GEN_FCN (icode) (op[0], op[1]);
	break;

      case 3:
	pat = GEN_FCN (icode) (op[0], op[1], op[2]);
	break;

      case 4:
	pat = GEN_FCN (icode) (op[0], op[1], op[2], op[3]);
	break;

      case 5:
	pat = GEN_FCN (icode) (op[0], op[1], op[2], op[3], op[4]);
        break;

      default:
	gcc_unreachable ();
      }

  if (!pat)
    return 0;

  emit_insn (pat);

  return target;
}

/* Expand a Neon builtin. These are "special" because they don't have symbolic
   constants defined per-instruction or per instruction-variant. Instead, the
   required info is looked up in the table neon_builtin_data.  */
static rtx
arm_expand_neon_builtin (int fcode, tree exp, rtx target)
{
  neon_builtin_datum *d = &neon_builtin_data[fcode - ARM_BUILTIN_NEON_BASE];
  neon_itype itype = d->itype;
  enum insn_code icode = d->code;
  neon_builtin_type_mode type_mode = d->mode;

  switch (itype)
    {
    case NEON_UNOP:
    case NEON_CONVERT:
    case NEON_DUPLANE:
      return arm_expand_neon_args (target, icode, 1, type_mode, exp, fcode,
        NEON_ARG_COPY_TO_REG, NEON_ARG_STOP);

    case NEON_BINOP:
    case NEON_LOGICBINOP:
    case NEON_SCALARMUL:
    case NEON_SCALARMULL:
    case NEON_SCALARMULH:
      return arm_expand_neon_args (target, icode, 1, type_mode, exp, fcode,
        NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG, NEON_ARG_STOP);

    case NEON_TERNOP:
      return arm_expand_neon_args (target, icode, 1, type_mode, exp, fcode,
        NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG,
        NEON_ARG_STOP);

    case NEON_GETLANE:
    case NEON_FIXCONV:
    case NEON_SHIFTIMM:
      return arm_expand_neon_args (target, icode, 1, type_mode, exp, fcode,
        NEON_ARG_COPY_TO_REG, NEON_ARG_CONSTANT,
        NEON_ARG_STOP);

    case NEON_CREATE:
      return arm_expand_neon_args (target, icode, 1, type_mode, exp, fcode,
        NEON_ARG_COPY_TO_REG, NEON_ARG_STOP);

    case NEON_DUP:
    case NEON_RINT:
    case NEON_SPLIT:
    case NEON_FLOAT_WIDEN:
    case NEON_FLOAT_NARROW:
    case NEON_BSWAP:
    case NEON_REINTERP:
      return arm_expand_neon_args (target, icode, 1, type_mode, exp, fcode,
        NEON_ARG_COPY_TO_REG, NEON_ARG_STOP);

    case NEON_COPYSIGNF:
    case NEON_COMBINE:
    case NEON_VTBL:
      return arm_expand_neon_args (target, icode, 1, type_mode, exp, fcode,
        NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG, NEON_ARG_STOP);

    case NEON_LANEMUL:
    case NEON_LANEMULL:
    case NEON_LANEMULH:
    case NEON_SETLANE:
    case NEON_SHIFTINSERT:
      return arm_expand_neon_args (target, icode, 1, type_mode, exp, fcode,
        NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG, NEON_ARG_CONSTANT,
        NEON_ARG_STOP);

    case NEON_LANEMAC:
      return arm_expand_neon_args (target, icode, 1, type_mode, exp, fcode,
        NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG,
        NEON_ARG_CONSTANT, NEON_ARG_STOP);

    case NEON_SHIFTACC:
      return arm_expand_neon_args (target, icode, 1, type_mode, exp, fcode,
        NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG, NEON_ARG_CONSTANT,
        NEON_ARG_STOP);

    case NEON_SCALARMAC:
      return arm_expand_neon_args (target, icode, 1, type_mode, exp, fcode,
	NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG,
        NEON_ARG_STOP);

    case NEON_SELECT:
    case NEON_VTBX:
      return arm_expand_neon_args (target, icode, 1, type_mode, exp, fcode,
	NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG,
        NEON_ARG_STOP);

    case NEON_LOAD1:
    case NEON_LOADSTRUCT:
      return arm_expand_neon_args (target, icode, 1, type_mode, exp, fcode,
	NEON_ARG_MEMORY, NEON_ARG_STOP);

    case NEON_LOAD1LANE:
    case NEON_LOADSTRUCTLANE:
      return arm_expand_neon_args (target, icode, 1, type_mode, exp, fcode,
	NEON_ARG_MEMORY, NEON_ARG_COPY_TO_REG, NEON_ARG_CONSTANT,
	NEON_ARG_STOP);

    case NEON_STORE1:
    case NEON_STORESTRUCT:
      return arm_expand_neon_args (target, icode, 0, type_mode, exp, fcode,
	NEON_ARG_MEMORY, NEON_ARG_COPY_TO_REG, NEON_ARG_STOP);

    case NEON_STORE1LANE:
    case NEON_STORESTRUCTLANE:
      return arm_expand_neon_args (target, icode, 0, type_mode, exp, fcode,
	NEON_ARG_MEMORY, NEON_ARG_COPY_TO_REG, NEON_ARG_CONSTANT,
	NEON_ARG_STOP);
    }

  gcc_unreachable ();
}

/* Expand an expression EXP that calls a built-in function,
   with result going to TARGET if that's convenient
   (and in mode MODE if that's convenient).
   SUBTARGET may be used as the target for computing one of EXP's operands.
   IGNORE is nonzero if the value is to be ignored.  */

rtx
arm_expand_builtin (tree exp,
		    rtx target,
		    rtx subtarget ATTRIBUTE_UNUSED,
		    machine_mode mode ATTRIBUTE_UNUSED,
		    int ignore ATTRIBUTE_UNUSED)
{
  const struct builtin_description * d;
  enum insn_code    icode;
  tree              fndecl = TREE_OPERAND (CALL_EXPR_FN (exp), 0);
  tree              arg0;
  tree              arg1;
  tree              arg2;
  rtx               op0;
  rtx               op1;
  rtx               op2;
  rtx               pat;
  unsigned int      fcode = DECL_FUNCTION_CODE (fndecl);
  size_t            i;
  machine_mode tmode;
  machine_mode mode0;
  machine_mode mode1;
  machine_mode mode2;
  int opint;
  int selector;
  int mask;
  int imm;

  if (fcode >= ARM_BUILTIN_NEON_BASE)
    return arm_expand_neon_builtin (fcode, exp, target);

  switch (fcode)
    {
    case ARM_BUILTIN_GET_FPSCR:
    case ARM_BUILTIN_SET_FPSCR:
      if (fcode == ARM_BUILTIN_GET_FPSCR)
	{
	  icode = CODE_FOR_get_fpscr;
	  target = gen_reg_rtx (SImode);
	  pat = GEN_FCN (icode) (target);
	}
      else
	{
	  target = NULL_RTX;
	  icode = CODE_FOR_set_fpscr;
	  arg0 = CALL_EXPR_ARG (exp, 0);
	  op0 = expand_normal (arg0);
	  pat = GEN_FCN (icode) (op0);
	}
      emit_insn (pat);
      return target;

    case ARM_BUILTIN_TEXTRMSB:
    case ARM_BUILTIN_TEXTRMUB:
    case ARM_BUILTIN_TEXTRMSH:
    case ARM_BUILTIN_TEXTRMUH:
    case ARM_BUILTIN_TEXTRMSW:
    case ARM_BUILTIN_TEXTRMUW:
      icode = (fcode == ARM_BUILTIN_TEXTRMSB ? CODE_FOR_iwmmxt_textrmsb
	       : fcode == ARM_BUILTIN_TEXTRMUB ? CODE_FOR_iwmmxt_textrmub
	       : fcode == ARM_BUILTIN_TEXTRMSH ? CODE_FOR_iwmmxt_textrmsh
	       : fcode == ARM_BUILTIN_TEXTRMUH ? CODE_FOR_iwmmxt_textrmuh
	       : CODE_FOR_iwmmxt_textrmw);

      arg0 = CALL_EXPR_ARG (exp, 0);
      arg1 = CALL_EXPR_ARG (exp, 1);
      op0 = expand_normal (arg0);
      op1 = expand_normal (arg1);
      tmode = insn_data[icode].operand[0].mode;
      mode0 = insn_data[icode].operand[1].mode;
      mode1 = insn_data[icode].operand[2].mode;

      if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
	op0 = copy_to_mode_reg (mode0, op0);
      if (! (*insn_data[icode].operand[2].predicate) (op1, mode1))
	{
	  /* @@@ better error message */
	  error ("selector must be an immediate");
	  return gen_reg_rtx (tmode);
	}

      opint = INTVAL (op1);
      if (fcode == ARM_BUILTIN_TEXTRMSB || fcode == ARM_BUILTIN_TEXTRMUB)
	{
	  if (opint > 7 || opint < 0)
	    error ("the range of selector should be in 0 to 7");
	}
      else if (fcode == ARM_BUILTIN_TEXTRMSH || fcode == ARM_BUILTIN_TEXTRMUH)
	{
	  if (opint > 3 || opint < 0)
	    error ("the range of selector should be in 0 to 3");
	}
      else /* ARM_BUILTIN_TEXTRMSW || ARM_BUILTIN_TEXTRMUW.  */
	{
	  if (opint > 1 || opint < 0)
	    error ("the range of selector should be in 0 to 1");
	}

      if (target == 0
	  || GET_MODE (target) != tmode
	  || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
	target = gen_reg_rtx (tmode);
      pat = GEN_FCN (icode) (target, op0, op1);
      if (! pat)
	return 0;
      emit_insn (pat);
      return target;

    case ARM_BUILTIN_WALIGNI:
      /* If op2 is immediate, call walighi, else call walighr.  */
      arg0 = CALL_EXPR_ARG (exp, 0);
      arg1 = CALL_EXPR_ARG (exp, 1);
      arg2 = CALL_EXPR_ARG (exp, 2);
      op0 = expand_normal (arg0);
      op1 = expand_normal (arg1);
      op2 = expand_normal (arg2);
      if (CONST_INT_P (op2))
        {
	  icode = CODE_FOR_iwmmxt_waligni;
          tmode = insn_data[icode].operand[0].mode;
	  mode0 = insn_data[icode].operand[1].mode;
	  mode1 = insn_data[icode].operand[2].mode;
	  mode2 = insn_data[icode].operand[3].mode;
          if (!(*insn_data[icode].operand[1].predicate) (op0, mode0))
	    op0 = copy_to_mode_reg (mode0, op0);
          if (!(*insn_data[icode].operand[2].predicate) (op1, mode1))
	    op1 = copy_to_mode_reg (mode1, op1);
          gcc_assert ((*insn_data[icode].operand[3].predicate) (op2, mode2));
	  selector = INTVAL (op2);
	  if (selector > 7 || selector < 0)
	    error ("the range of selector should be in 0 to 7");
	}
      else
        {
	  icode = CODE_FOR_iwmmxt_walignr;
          tmode = insn_data[icode].operand[0].mode;
	  mode0 = insn_data[icode].operand[1].mode;
	  mode1 = insn_data[icode].operand[2].mode;
	  mode2 = insn_data[icode].operand[3].mode;
          if (!(*insn_data[icode].operand[1].predicate) (op0, mode0))
	    op0 = copy_to_mode_reg (mode0, op0);
          if (!(*insn_data[icode].operand[2].predicate) (op1, mode1))
	    op1 = copy_to_mode_reg (mode1, op1);
          if (!(*insn_data[icode].operand[3].predicate) (op2, mode2))
	    op2 = copy_to_mode_reg (mode2, op2);
	}
      if (target == 0
	  || GET_MODE (target) != tmode
	  || !(*insn_data[icode].operand[0].predicate) (target, tmode))
	target = gen_reg_rtx (tmode);
      pat = GEN_FCN (icode) (target, op0, op1, op2);
      if (!pat)
	return 0;
      emit_insn (pat);
      return target;

    case ARM_BUILTIN_TINSRB:
    case ARM_BUILTIN_TINSRH:
    case ARM_BUILTIN_TINSRW:
    case ARM_BUILTIN_WMERGE:
      icode = (fcode == ARM_BUILTIN_TINSRB ? CODE_FOR_iwmmxt_tinsrb
	       : fcode == ARM_BUILTIN_TINSRH ? CODE_FOR_iwmmxt_tinsrh
	       : fcode == ARM_BUILTIN_WMERGE ? CODE_FOR_iwmmxt_wmerge
	       : CODE_FOR_iwmmxt_tinsrw);
      arg0 = CALL_EXPR_ARG (exp, 0);
      arg1 = CALL_EXPR_ARG (exp, 1);
      arg2 = CALL_EXPR_ARG (exp, 2);
      op0 = expand_normal (arg0);
      op1 = expand_normal (arg1);
      op2 = expand_normal (arg2);
      tmode = insn_data[icode].operand[0].mode;
      mode0 = insn_data[icode].operand[1].mode;
      mode1 = insn_data[icode].operand[2].mode;
      mode2 = insn_data[icode].operand[3].mode;

      if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
	op0 = copy_to_mode_reg (mode0, op0);
      if (! (*insn_data[icode].operand[2].predicate) (op1, mode1))
	op1 = copy_to_mode_reg (mode1, op1);
      if (! (*insn_data[icode].operand[3].predicate) (op2, mode2))
	{
	  error ("selector must be an immediate");
	  return const0_rtx;
	}
      if (icode == CODE_FOR_iwmmxt_wmerge)
	{
	  selector = INTVAL (op2);
	  if (selector > 7 || selector < 0)
	    error ("the range of selector should be in 0 to 7");
	}
      if ((icode == CODE_FOR_iwmmxt_tinsrb)
	  || (icode == CODE_FOR_iwmmxt_tinsrh)
	  || (icode == CODE_FOR_iwmmxt_tinsrw))
        {
	  mask = 0x01;
	  selector= INTVAL (op2);
	  if (icode == CODE_FOR_iwmmxt_tinsrb && (selector < 0 || selector > 7))
	    error ("the range of selector should be in 0 to 7");
	  else if (icode == CODE_FOR_iwmmxt_tinsrh && (selector < 0 ||selector > 3))
	    error ("the range of selector should be in 0 to 3");
	  else if (icode == CODE_FOR_iwmmxt_tinsrw && (selector < 0 ||selector > 1))
	    error ("the range of selector should be in 0 to 1");
	  mask <<= selector;
	  op2 = GEN_INT (mask);
	}
      if (target == 0
	  || GET_MODE (target) != tmode
	  || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
	target = gen_reg_rtx (tmode);
      pat = GEN_FCN (icode) (target, op0, op1, op2);
      if (! pat)
	return 0;
      emit_insn (pat);
      return target;

    case ARM_BUILTIN_SETWCGR0:
    case ARM_BUILTIN_SETWCGR1:
    case ARM_BUILTIN_SETWCGR2:
    case ARM_BUILTIN_SETWCGR3:
      icode = (fcode == ARM_BUILTIN_SETWCGR0 ? CODE_FOR_iwmmxt_setwcgr0
	       : fcode == ARM_BUILTIN_SETWCGR1 ? CODE_FOR_iwmmxt_setwcgr1
	       : fcode == ARM_BUILTIN_SETWCGR2 ? CODE_FOR_iwmmxt_setwcgr2
	       : CODE_FOR_iwmmxt_setwcgr3);
      arg0 = CALL_EXPR_ARG (exp, 0);
      op0 = expand_normal (arg0);
      mode0 = insn_data[icode].operand[0].mode;
      if (!(*insn_data[icode].operand[0].predicate) (op0, mode0))
        op0 = copy_to_mode_reg (mode0, op0);
      pat = GEN_FCN (icode) (op0);
      if (!pat)
	return 0;
      emit_insn (pat);
      return 0;

    case ARM_BUILTIN_GETWCGR0:
    case ARM_BUILTIN_GETWCGR1:
    case ARM_BUILTIN_GETWCGR2:
    case ARM_BUILTIN_GETWCGR3:
      icode = (fcode == ARM_BUILTIN_GETWCGR0 ? CODE_FOR_iwmmxt_getwcgr0
	       : fcode == ARM_BUILTIN_GETWCGR1 ? CODE_FOR_iwmmxt_getwcgr1
	       : fcode == ARM_BUILTIN_GETWCGR2 ? CODE_FOR_iwmmxt_getwcgr2
	       : CODE_FOR_iwmmxt_getwcgr3);
      tmode = insn_data[icode].operand[0].mode;
      if (target == 0
	  || GET_MODE (target) != tmode
	  || !(*insn_data[icode].operand[0].predicate) (target, tmode))
        target = gen_reg_rtx (tmode);
      pat = GEN_FCN (icode) (target);
      if (!pat)
        return 0;
      emit_insn (pat);
      return target;

    case ARM_BUILTIN_WSHUFH:
      icode = CODE_FOR_iwmmxt_wshufh;
      arg0 = CALL_EXPR_ARG (exp, 0);
      arg1 = CALL_EXPR_ARG (exp, 1);
      op0 = expand_normal (arg0);
      op1 = expand_normal (arg1);
      tmode = insn_data[icode].operand[0].mode;
      mode1 = insn_data[icode].operand[1].mode;
      mode2 = insn_data[icode].operand[2].mode;

      if (! (*insn_data[icode].operand[1].predicate) (op0, mode1))
	op0 = copy_to_mode_reg (mode1, op0);
      if (! (*insn_data[icode].operand[2].predicate) (op1, mode2))
	{
	  error ("mask must be an immediate");
	  return const0_rtx;
	}
      selector = INTVAL (op1);
      if (selector < 0 || selector > 255)
	error ("the range of mask should be in 0 to 255");
      if (target == 0
	  || GET_MODE (target) != tmode
	  || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
	target = gen_reg_rtx (tmode);
      pat = GEN_FCN (icode) (target, op0, op1);
      if (! pat)
	return 0;
      emit_insn (pat);
      return target;

    case ARM_BUILTIN_WMADDS:
      return arm_expand_binop_builtin (CODE_FOR_iwmmxt_wmadds, exp, target);
    case ARM_BUILTIN_WMADDSX:
      return arm_expand_binop_builtin (CODE_FOR_iwmmxt_wmaddsx, exp, target);
    case ARM_BUILTIN_WMADDSN:
      return arm_expand_binop_builtin (CODE_FOR_iwmmxt_wmaddsn, exp, target);
    case ARM_BUILTIN_WMADDU:
      return arm_expand_binop_builtin (CODE_FOR_iwmmxt_wmaddu, exp, target);
    case ARM_BUILTIN_WMADDUX:
      return arm_expand_binop_builtin (CODE_FOR_iwmmxt_wmaddux, exp, target);
    case ARM_BUILTIN_WMADDUN:
      return arm_expand_binop_builtin (CODE_FOR_iwmmxt_wmaddun, exp, target);
    case ARM_BUILTIN_WSADBZ:
      return arm_expand_binop_builtin (CODE_FOR_iwmmxt_wsadbz, exp, target);
    case ARM_BUILTIN_WSADHZ:
      return arm_expand_binop_builtin (CODE_FOR_iwmmxt_wsadhz, exp, target);

      /* Several three-argument builtins.  */
    case ARM_BUILTIN_WMACS:
    case ARM_BUILTIN_WMACU:
    case ARM_BUILTIN_TMIA:
    case ARM_BUILTIN_TMIAPH:
    case ARM_BUILTIN_TMIATT:
    case ARM_BUILTIN_TMIATB:
    case ARM_BUILTIN_TMIABT:
    case ARM_BUILTIN_TMIABB:
    case ARM_BUILTIN_WQMIABB:
    case ARM_BUILTIN_WQMIABT:
    case ARM_BUILTIN_WQMIATB:
    case ARM_BUILTIN_WQMIATT:
    case ARM_BUILTIN_WQMIABBN:
    case ARM_BUILTIN_WQMIABTN:
    case ARM_BUILTIN_WQMIATBN:
    case ARM_BUILTIN_WQMIATTN:
    case ARM_BUILTIN_WMIABB:
    case ARM_BUILTIN_WMIABT:
    case ARM_BUILTIN_WMIATB:
    case ARM_BUILTIN_WMIATT:
    case ARM_BUILTIN_WMIABBN:
    case ARM_BUILTIN_WMIABTN:
    case ARM_BUILTIN_WMIATBN:
    case ARM_BUILTIN_WMIATTN:
    case ARM_BUILTIN_WMIAWBB:
    case ARM_BUILTIN_WMIAWBT:
    case ARM_BUILTIN_WMIAWTB:
    case ARM_BUILTIN_WMIAWTT:
    case ARM_BUILTIN_WMIAWBBN:
    case ARM_BUILTIN_WMIAWBTN:
    case ARM_BUILTIN_WMIAWTBN:
    case ARM_BUILTIN_WMIAWTTN:
    case ARM_BUILTIN_WSADB:
    case ARM_BUILTIN_WSADH:
      icode = (fcode == ARM_BUILTIN_WMACS ? CODE_FOR_iwmmxt_wmacs
	       : fcode == ARM_BUILTIN_WMACU ? CODE_FOR_iwmmxt_wmacu
	       : fcode == ARM_BUILTIN_TMIA ? CODE_FOR_iwmmxt_tmia
	       : fcode == ARM_BUILTIN_TMIAPH ? CODE_FOR_iwmmxt_tmiaph
	       : fcode == ARM_BUILTIN_TMIABB ? CODE_FOR_iwmmxt_tmiabb
	       : fcode == ARM_BUILTIN_TMIABT ? CODE_FOR_iwmmxt_tmiabt
	       : fcode == ARM_BUILTIN_TMIATB ? CODE_FOR_iwmmxt_tmiatb
	       : fcode == ARM_BUILTIN_TMIATT ? CODE_FOR_iwmmxt_tmiatt
	       : fcode == ARM_BUILTIN_WQMIABB ? CODE_FOR_iwmmxt_wqmiabb
	       : fcode == ARM_BUILTIN_WQMIABT ? CODE_FOR_iwmmxt_wqmiabt
	       : fcode == ARM_BUILTIN_WQMIATB ? CODE_FOR_iwmmxt_wqmiatb
	       : fcode == ARM_BUILTIN_WQMIATT ? CODE_FOR_iwmmxt_wqmiatt
	       : fcode == ARM_BUILTIN_WQMIABBN ? CODE_FOR_iwmmxt_wqmiabbn
	       : fcode == ARM_BUILTIN_WQMIABTN ? CODE_FOR_iwmmxt_wqmiabtn
	       : fcode == ARM_BUILTIN_WQMIATBN ? CODE_FOR_iwmmxt_wqmiatbn
	       : fcode == ARM_BUILTIN_WQMIATTN ? CODE_FOR_iwmmxt_wqmiattn
	       : fcode == ARM_BUILTIN_WMIABB ? CODE_FOR_iwmmxt_wmiabb
	       : fcode == ARM_BUILTIN_WMIABT ? CODE_FOR_iwmmxt_wmiabt
	       : fcode == ARM_BUILTIN_WMIATB ? CODE_FOR_iwmmxt_wmiatb
	       : fcode == ARM_BUILTIN_WMIATT ? CODE_FOR_iwmmxt_wmiatt
	       : fcode == ARM_BUILTIN_WMIABBN ? CODE_FOR_iwmmxt_wmiabbn
	       : fcode == ARM_BUILTIN_WMIABTN ? CODE_FOR_iwmmxt_wmiabtn
	       : fcode == ARM_BUILTIN_WMIATBN ? CODE_FOR_iwmmxt_wmiatbn
	       : fcode == ARM_BUILTIN_WMIATTN ? CODE_FOR_iwmmxt_wmiattn
	       : fcode == ARM_BUILTIN_WMIAWBB ? CODE_FOR_iwmmxt_wmiawbb
	       : fcode == ARM_BUILTIN_WMIAWBT ? CODE_FOR_iwmmxt_wmiawbt
	       : fcode == ARM_BUILTIN_WMIAWTB ? CODE_FOR_iwmmxt_wmiawtb
	       : fcode == ARM_BUILTIN_WMIAWTT ? CODE_FOR_iwmmxt_wmiawtt
	       : fcode == ARM_BUILTIN_WMIAWBBN ? CODE_FOR_iwmmxt_wmiawbbn
	       : fcode == ARM_BUILTIN_WMIAWBTN ? CODE_FOR_iwmmxt_wmiawbtn
	       : fcode == ARM_BUILTIN_WMIAWTBN ? CODE_FOR_iwmmxt_wmiawtbn
	       : fcode == ARM_BUILTIN_WMIAWTTN ? CODE_FOR_iwmmxt_wmiawttn
	       : fcode == ARM_BUILTIN_WSADB ? CODE_FOR_iwmmxt_wsadb
	       : CODE_FOR_iwmmxt_wsadh);
      arg0 = CALL_EXPR_ARG (exp, 0);
      arg1 = CALL_EXPR_ARG (exp, 1);
      arg2 = CALL_EXPR_ARG (exp, 2);
      op0 = expand_normal (arg0);
      op1 = expand_normal (arg1);
      op2 = expand_normal (arg2);
      tmode = insn_data[icode].operand[0].mode;
      mode0 = insn_data[icode].operand[1].mode;
      mode1 = insn_data[icode].operand[2].mode;
      mode2 = insn_data[icode].operand[3].mode;

      if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
	op0 = copy_to_mode_reg (mode0, op0);
      if (! (*insn_data[icode].operand[2].predicate) (op1, mode1))
	op1 = copy_to_mode_reg (mode1, op1);
      if (! (*insn_data[icode].operand[3].predicate) (op2, mode2))
	op2 = copy_to_mode_reg (mode2, op2);
      if (target == 0
	  || GET_MODE (target) != tmode
	  || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
	target = gen_reg_rtx (tmode);
      pat = GEN_FCN (icode) (target, op0, op1, op2);
      if (! pat)
	return 0;
      emit_insn (pat);
      return target;

    case ARM_BUILTIN_WZERO:
      target = gen_reg_rtx (DImode);
      emit_insn (gen_iwmmxt_clrdi (target));
      return target;

    case ARM_BUILTIN_WSRLHI:
    case ARM_BUILTIN_WSRLWI:
    case ARM_BUILTIN_WSRLDI:
    case ARM_BUILTIN_WSLLHI:
    case ARM_BUILTIN_WSLLWI:
    case ARM_BUILTIN_WSLLDI:
    case ARM_BUILTIN_WSRAHI:
    case ARM_BUILTIN_WSRAWI:
    case ARM_BUILTIN_WSRADI:
    case ARM_BUILTIN_WRORHI:
    case ARM_BUILTIN_WRORWI:
    case ARM_BUILTIN_WRORDI:
    case ARM_BUILTIN_WSRLH:
    case ARM_BUILTIN_WSRLW:
    case ARM_BUILTIN_WSRLD:
    case ARM_BUILTIN_WSLLH:
    case ARM_BUILTIN_WSLLW:
    case ARM_BUILTIN_WSLLD:
    case ARM_BUILTIN_WSRAH:
    case ARM_BUILTIN_WSRAW:
    case ARM_BUILTIN_WSRAD:
    case ARM_BUILTIN_WRORH:
    case ARM_BUILTIN_WRORW:
    case ARM_BUILTIN_WRORD:
      icode = (fcode == ARM_BUILTIN_WSRLHI ? CODE_FOR_lshrv4hi3_iwmmxt
	       : fcode == ARM_BUILTIN_WSRLWI ? CODE_FOR_lshrv2si3_iwmmxt
	       : fcode == ARM_BUILTIN_WSRLDI ? CODE_FOR_lshrdi3_iwmmxt
	       : fcode == ARM_BUILTIN_WSLLHI ? CODE_FOR_ashlv4hi3_iwmmxt
	       : fcode == ARM_BUILTIN_WSLLWI ? CODE_FOR_ashlv2si3_iwmmxt
	       : fcode == ARM_BUILTIN_WSLLDI ? CODE_FOR_ashldi3_iwmmxt
	       : fcode == ARM_BUILTIN_WSRAHI ? CODE_FOR_ashrv4hi3_iwmmxt
	       : fcode == ARM_BUILTIN_WSRAWI ? CODE_FOR_ashrv2si3_iwmmxt
	       : fcode == ARM_BUILTIN_WSRADI ? CODE_FOR_ashrdi3_iwmmxt
	       : fcode == ARM_BUILTIN_WRORHI ? CODE_FOR_rorv4hi3
	       : fcode == ARM_BUILTIN_WRORWI ? CODE_FOR_rorv2si3
	       : fcode == ARM_BUILTIN_WRORDI ? CODE_FOR_rordi3
	       : fcode == ARM_BUILTIN_WSRLH  ? CODE_FOR_lshrv4hi3_di
	       : fcode == ARM_BUILTIN_WSRLW  ? CODE_FOR_lshrv2si3_di
	       : fcode == ARM_BUILTIN_WSRLD  ? CODE_FOR_lshrdi3_di
	       : fcode == ARM_BUILTIN_WSLLH  ? CODE_FOR_ashlv4hi3_di
	       : fcode == ARM_BUILTIN_WSLLW  ? CODE_FOR_ashlv2si3_di
	       : fcode == ARM_BUILTIN_WSLLD  ? CODE_FOR_ashldi3_di
	       : fcode == ARM_BUILTIN_WSRAH  ? CODE_FOR_ashrv4hi3_di
	       : fcode == ARM_BUILTIN_WSRAW  ? CODE_FOR_ashrv2si3_di
	       : fcode == ARM_BUILTIN_WSRAD  ? CODE_FOR_ashrdi3_di
	       : fcode == ARM_BUILTIN_WRORH  ? CODE_FOR_rorv4hi3_di
	       : fcode == ARM_BUILTIN_WRORW  ? CODE_FOR_rorv2si3_di
	       : fcode == ARM_BUILTIN_WRORD  ? CODE_FOR_rordi3_di
	       : CODE_FOR_nothing);
      arg1 = CALL_EXPR_ARG (exp, 1);
      op1 = expand_normal (arg1);
      if (GET_MODE (op1) == VOIDmode)
	{
	  imm = INTVAL (op1);
	  if ((fcode == ARM_BUILTIN_WRORHI || fcode == ARM_BUILTIN_WRORWI
	       || fcode == ARM_BUILTIN_WRORH || fcode == ARM_BUILTIN_WRORW)
	      && (imm < 0 || imm > 32))
	    {
	      if (fcode == ARM_BUILTIN_WRORHI)
		error ("the range of count should be in 0 to 32.  please check the intrinsic _mm_rori_pi16 in code.");
	      else if (fcode == ARM_BUILTIN_WRORWI)
		error ("the range of count should be in 0 to 32.  please check the intrinsic _mm_rori_pi32 in code.");
	      else if (fcode == ARM_BUILTIN_WRORH)
		error ("the range of count should be in 0 to 32.  please check the intrinsic _mm_ror_pi16 in code.");
	      else
		error ("the range of count should be in 0 to 32.  please check the intrinsic _mm_ror_pi32 in code.");
	    }
	  else if ((fcode == ARM_BUILTIN_WRORDI || fcode == ARM_BUILTIN_WRORD)
		   && (imm < 0 || imm > 64))
	    {
	      if (fcode == ARM_BUILTIN_WRORDI)
		error ("the range of count should be in 0 to 64.  please check the intrinsic _mm_rori_si64 in code.");
	      else
		error ("the range of count should be in 0 to 64.  please check the intrinsic _mm_ror_si64 in code.");
	    }
	  else if (imm < 0)
	    {
	      if (fcode == ARM_BUILTIN_WSRLHI)
		error ("the count should be no less than 0.  please check the intrinsic _mm_srli_pi16 in code.");
	      else if (fcode == ARM_BUILTIN_WSRLWI)
		error ("the count should be no less than 0.  please check the intrinsic _mm_srli_pi32 in code.");
	      else if (fcode == ARM_BUILTIN_WSRLDI)
		error ("the count should be no less than 0.  please check the intrinsic _mm_srli_si64 in code.");
	      else if (fcode == ARM_BUILTIN_WSLLHI)
		error ("the count should be no less than 0.  please check the intrinsic _mm_slli_pi16 in code.");
	      else if (fcode == ARM_BUILTIN_WSLLWI)
		error ("the count should be no less than 0.  please check the intrinsic _mm_slli_pi32 in code.");
	      else if (fcode == ARM_BUILTIN_WSLLDI)
		error ("the count should be no less than 0.  please check the intrinsic _mm_slli_si64 in code.");
	      else if (fcode == ARM_BUILTIN_WSRAHI)
		error ("the count should be no less than 0.  please check the intrinsic _mm_srai_pi16 in code.");
	      else if (fcode == ARM_BUILTIN_WSRAWI)
		error ("the count should be no less than 0.  please check the intrinsic _mm_srai_pi32 in code.");
	      else if (fcode == ARM_BUILTIN_WSRADI)
		error ("the count should be no less than 0.  please check the intrinsic _mm_srai_si64 in code.");
	      else if (fcode == ARM_BUILTIN_WSRLH)
		error ("the count should be no less than 0.  please check the intrinsic _mm_srl_pi16 in code.");
	      else if (fcode == ARM_BUILTIN_WSRLW)
		error ("the count should be no less than 0.  please check the intrinsic _mm_srl_pi32 in code.");
	      else if (fcode == ARM_BUILTIN_WSRLD)
		error ("the count should be no less than 0.  please check the intrinsic _mm_srl_si64 in code.");
	      else if (fcode == ARM_BUILTIN_WSLLH)
		error ("the count should be no less than 0.  please check the intrinsic _mm_sll_pi16 in code.");
	      else if (fcode == ARM_BUILTIN_WSLLW)
		error ("the count should be no less than 0.  please check the intrinsic _mm_sll_pi32 in code.");
	      else if (fcode == ARM_BUILTIN_WSLLD)
		error ("the count should be no less than 0.  please check the intrinsic _mm_sll_si64 in code.");
	      else if (fcode == ARM_BUILTIN_WSRAH)
		error ("the count should be no less than 0.  please check the intrinsic _mm_sra_pi16 in code.");
	      else if (fcode == ARM_BUILTIN_WSRAW)
		error ("the count should be no less than 0.  please check the intrinsic _mm_sra_pi32 in code.");
	      else
		error ("the count should be no less than 0.  please check the intrinsic _mm_sra_si64 in code.");
	    }
	}
      return arm_expand_binop_builtin (icode, exp, target);

    default:
      break;
    }

  for (i = 0, d = bdesc_2arg; i < ARRAY_SIZE (bdesc_2arg); i++, d++)
    if (d->code == (const enum arm_builtins) fcode)
      return arm_expand_binop_builtin (d->icode, exp, target);

  for (i = 0, d = bdesc_1arg; i < ARRAY_SIZE (bdesc_1arg); i++, d++)
    if (d->code == (const enum arm_builtins) fcode)
      return arm_expand_unop_builtin (d->icode, exp, target, 0);

  for (i = 0, d = bdesc_3arg; i < ARRAY_SIZE (bdesc_3arg); i++, d++)
    if (d->code == (const enum arm_builtins) fcode)
      return arm_expand_ternop_builtin (d->icode, exp, target);

  /* @@@ Should really do something sensible here.  */
  return NULL_RTX;
}

tree
arm_builtin_vectorized_function (tree fndecl, tree type_out, tree type_in)
{
  machine_mode in_mode, out_mode;
  int in_n, out_n;
  bool out_unsigned_p = TYPE_UNSIGNED (type_out);

  if (TREE_CODE (type_out) != VECTOR_TYPE
      || TREE_CODE (type_in) != VECTOR_TYPE)
    return NULL_TREE;

  out_mode = TYPE_MODE (TREE_TYPE (type_out));
  out_n = TYPE_VECTOR_SUBPARTS (type_out);
  in_mode = TYPE_MODE (TREE_TYPE (type_in));
  in_n = TYPE_VECTOR_SUBPARTS (type_in);

/* ARM_CHECK_BUILTIN_MODE and ARM_FIND_VRINT_VARIANT are used to find the
   decl of the vectorized builtin for the appropriate vector mode.
   NULL_TREE is returned if no such builtin is available.  */
#undef ARM_CHECK_BUILTIN_MODE
#define ARM_CHECK_BUILTIN_MODE(C)    \
  (TARGET_NEON && TARGET_FPU_ARMV8   \
   && flag_unsafe_math_optimizations \
   && ARM_CHECK_BUILTIN_MODE_1 (C))

#undef ARM_CHECK_BUILTIN_MODE_1
#define ARM_CHECK_BUILTIN_MODE_1(C) \
  (out_mode == SFmode && out_n == C \
   && in_mode == SFmode && in_n == C)

#undef ARM_FIND_VRINT_VARIANT
#define ARM_FIND_VRINT_VARIANT(N) \
  (ARM_CHECK_BUILTIN_MODE (2) \
    ? arm_builtin_decl(ARM_BUILTIN_NEON_##N##v2sf, false) \
    : (ARM_CHECK_BUILTIN_MODE (4) \
      ? arm_builtin_decl(ARM_BUILTIN_NEON_##N##v4sf, false) \
      : NULL_TREE))

  if (DECL_BUILT_IN_CLASS (fndecl) == BUILT_IN_NORMAL)
    {
      enum built_in_function fn = DECL_FUNCTION_CODE (fndecl);
      switch (fn)
        {
          case BUILT_IN_FLOORF:
            return ARM_FIND_VRINT_VARIANT (vrintm);
          case BUILT_IN_CEILF:
            return ARM_FIND_VRINT_VARIANT (vrintp);
          case BUILT_IN_TRUNCF:
            return ARM_FIND_VRINT_VARIANT (vrintz);
          case BUILT_IN_ROUNDF:
            return ARM_FIND_VRINT_VARIANT (vrinta);
#undef ARM_CHECK_BUILTIN_MODE_1
#define ARM_CHECK_BUILTIN_MODE_1(C) \
  (out_mode == SImode && out_n == C \
   && in_mode == SFmode && in_n == C)

#define ARM_FIND_VCVT_VARIANT(N) \
  (ARM_CHECK_BUILTIN_MODE (2) \
   ? arm_builtin_decl(ARM_BUILTIN_NEON_##N##v2sfv2si, false) \
   : (ARM_CHECK_BUILTIN_MODE (4) \
     ? arm_builtin_decl(ARM_BUILTIN_NEON_##N##v4sfv4si, false) \
     : NULL_TREE))

#define ARM_FIND_VCVTU_VARIANT(N) \
  (ARM_CHECK_BUILTIN_MODE (2) \
   ? arm_builtin_decl(ARM_BUILTIN_NEON_##N##uv2sfv2si, false) \
   : (ARM_CHECK_BUILTIN_MODE (4) \
     ? arm_builtin_decl(ARM_BUILTIN_NEON_##N##uv4sfv4si, false) \
     : NULL_TREE))
          case BUILT_IN_LROUNDF:
            return out_unsigned_p
                     ? ARM_FIND_VCVTU_VARIANT (vcvta)
                     : ARM_FIND_VCVT_VARIANT (vcvta);
          case BUILT_IN_LCEILF:
            return out_unsigned_p
                     ? ARM_FIND_VCVTU_VARIANT (vcvtp)
                     : ARM_FIND_VCVT_VARIANT (vcvtp);
          case BUILT_IN_LFLOORF:
            return out_unsigned_p
                     ? ARM_FIND_VCVTU_VARIANT (vcvtm)
                     : ARM_FIND_VCVT_VARIANT (vcvtm);
#undef ARM_CHECK_BUILTIN_MODE
#define ARM_CHECK_BUILTIN_MODE(C, N) \
  (out_mode == N##mode && out_n == C \
   && in_mode == N##mode && in_n == C)
          case BUILT_IN_BSWAP16:
            if (ARM_CHECK_BUILTIN_MODE (4, HI))
              return arm_builtin_decl (ARM_BUILTIN_NEON_bswapv4hi, false);
            else if (ARM_CHECK_BUILTIN_MODE (8, HI))
              return arm_builtin_decl (ARM_BUILTIN_NEON_bswapv8hi, false);
            else
              return NULL_TREE;
          case BUILT_IN_BSWAP32:
            if (ARM_CHECK_BUILTIN_MODE (2, SI))
              return arm_builtin_decl (ARM_BUILTIN_NEON_bswapv2si, false);
            else if (ARM_CHECK_BUILTIN_MODE (4, SI))
              return arm_builtin_decl (ARM_BUILTIN_NEON_bswapv4si, false);
            else
              return NULL_TREE;
          case BUILT_IN_BSWAP64:
            if (ARM_CHECK_BUILTIN_MODE (2, DI))
              return arm_builtin_decl (ARM_BUILTIN_NEON_bswapv2di, false);
            else
              return NULL_TREE;
	  case BUILT_IN_COPYSIGNF:
	    if (ARM_CHECK_BUILTIN_MODE (2, SF))
              return arm_builtin_decl (ARM_BUILTIN_NEON_copysignfv2sf, false);
	    else if (ARM_CHECK_BUILTIN_MODE (4, SF))
              return arm_builtin_decl (ARM_BUILTIN_NEON_copysignfv4sf, false);
	    else
	      return NULL_TREE;

          default:
            return NULL_TREE;
        }
    }
  return NULL_TREE;
}
#undef ARM_FIND_VCVT_VARIANT
#undef ARM_FIND_VCVTU_VARIANT
#undef ARM_CHECK_BUILTIN_MODE
#undef ARM_FIND_VRINT_VARIANT

void
arm_atomic_assign_expand_fenv (tree *hold, tree *clear, tree *update)
{
  const unsigned ARM_FE_INVALID = 1;
  const unsigned ARM_FE_DIVBYZERO = 2;
  const unsigned ARM_FE_OVERFLOW = 4;
  const unsigned ARM_FE_UNDERFLOW = 8;
  const unsigned ARM_FE_INEXACT = 16;
  const unsigned HOST_WIDE_INT ARM_FE_ALL_EXCEPT = (ARM_FE_INVALID
						    | ARM_FE_DIVBYZERO
						    | ARM_FE_OVERFLOW
						    | ARM_FE_UNDERFLOW
						    | ARM_FE_INEXACT);
  const unsigned HOST_WIDE_INT ARM_FE_EXCEPT_SHIFT = 8;
  tree fenv_var, get_fpscr, set_fpscr, mask, ld_fenv, masked_fenv;
  tree new_fenv_var, reload_fenv, restore_fnenv;
  tree update_call, atomic_feraiseexcept, hold_fnclex;

  if (!TARGET_VFP || !TARGET_HARD_FLOAT)
    return;

  /* Generate the equivalent of :
       unsigned int fenv_var;
       fenv_var = __builtin_arm_get_fpscr ();

       unsigned int masked_fenv;
       masked_fenv = fenv_var & mask;

       __builtin_arm_set_fpscr (masked_fenv);  */

  fenv_var = create_tmp_var (unsigned_type_node, NULL);
  get_fpscr = arm_builtin_decls[ARM_BUILTIN_GET_FPSCR];
  set_fpscr = arm_builtin_decls[ARM_BUILTIN_SET_FPSCR];
  mask = build_int_cst (unsigned_type_node,
			~((ARM_FE_ALL_EXCEPT << ARM_FE_EXCEPT_SHIFT)
			  | ARM_FE_ALL_EXCEPT));
  ld_fenv = build2 (MODIFY_EXPR, unsigned_type_node,
		    fenv_var, build_call_expr (get_fpscr, 0));
  masked_fenv = build2 (BIT_AND_EXPR, unsigned_type_node, fenv_var, mask);
  hold_fnclex = build_call_expr (set_fpscr, 1, masked_fenv);
  *hold = build2 (COMPOUND_EXPR, void_type_node,
		  build2 (COMPOUND_EXPR, void_type_node, masked_fenv, ld_fenv),
		  hold_fnclex);

  /* Store the value of masked_fenv to clear the exceptions:
     __builtin_arm_set_fpscr (masked_fenv);  */

  *clear = build_call_expr (set_fpscr, 1, masked_fenv);

  /* Generate the equivalent of :
       unsigned int new_fenv_var;
       new_fenv_var = __builtin_arm_get_fpscr ();

       __builtin_arm_set_fpscr (fenv_var);

       __atomic_feraiseexcept (new_fenv_var);  */

  new_fenv_var = create_tmp_var (unsigned_type_node, NULL);
  reload_fenv = build2 (MODIFY_EXPR, unsigned_type_node, new_fenv_var,
			build_call_expr (get_fpscr, 0));
  restore_fnenv = build_call_expr (set_fpscr, 1, fenv_var);
  atomic_feraiseexcept = builtin_decl_implicit (BUILT_IN_ATOMIC_FERAISEEXCEPT);
  update_call = build_call_expr (atomic_feraiseexcept, 1,
				 fold_convert (integer_type_node, new_fenv_var));
  *update = build2 (COMPOUND_EXPR, void_type_node,
		    build2 (COMPOUND_EXPR, void_type_node,
			    reload_fenv, restore_fnenv), update_call);
}

#include "gt-arm-builtins.h"
