/*******************************************************************************
Copyright (C) 2016 Marvell International Ltd.

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the three
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
Marvell GPL License Option

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation, either version 2 of the License, or any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************************
Marvell GNU General Public License FreeRTOS Exception

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the Lesser
General Public License Version 2.1 plus the following FreeRTOS exception.
An independent module is a module which is not derived from or based on
FreeRTOS.
Clause 1:
Linking FreeRTOS statically or dynamically with other modules is making a
combined work based on FreeRTOS. Thus, the terms and conditions of the GNU
General Public License cover the whole combination.
As a special exception, the copyright holder of FreeRTOS gives you permission
to link FreeRTOS with independent modules that communicate with FreeRTOS solely
through the FreeRTOS API interface, regardless of the license terms of these
independent modules, and to copy and distribute the resulting combined work
under terms of your choice, provided that:
1. Every copy of the combined work is accompanied by a written statement that
details to the recipient the version of FreeRTOS used and an offer by yourself
to provide the FreeRTOS source code (including any modifications you may have
made) should the recipient request it.
2. The combined work is not itself an RTOS, scheduler, kernel or related
product.
3. The independent modules add significant and primary functionality to
FreeRTOS and do not merely extend the existing functionality already present in
FreeRTOS.
Clause 2:
FreeRTOS may not be used for any competitive or comparative purpose, including
the publication of any form of run time or compile time metric, without the
express permission of Real Time Engineers Ltd. (this is the norm within the
industry and is intended to ensure information accuracy).

********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

	* Redistributions of source code must retain the above copyright notice,
	  this list of conditions and the following disclaimer.

	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.

	* Neither the name of Marvell nor the names of its contributors may be
	  used to endorse or promote products derived from this software without
	  specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include "mv_ddr_mc6_drv.h"
#include "ddr3_init.h"
#include "mv_ddr_common.h"

/* extern */
extern struct page_element page_param[];	/* FIXME: this data base should have get, set functions */

void mv_ddr_mc6_timing_regs_cfg(unsigned int freq_mhz)
{
	struct mv_ddr_mc6_timing mc6_timing;
	unsigned int page_size;
	struct mv_ddr_topology_map *tm = mv_ddr_topology_map_get();

	/* get the spped bin index */
	enum hws_speed_bin speed_bin_index = tm->interface_params[IF_ID_0].speed_bin_index;

	/* calculate memory size */
	enum mv_ddr_die_capacity memory_size = tm->interface_params[IF_ID_0].memory_size;

	/* calculate page size */
	page_size = tm->interface_params[IF_ID_0].bus_width == MV_DDR_DEV_WIDTH_8BIT ?
		page_param[memory_size].page_size_8bit : page_param[memory_size].page_size_16bit;
	/* printf("page_size = %d\n", page_size); */

	/* calculate t_clck */
	mc6_timing.t_ckclk = MEGA / freq_mhz;
	/* printf("t_ckclk = %d\n", mc6_timing.t_ckclk); */

	/* calculate t_refi  */
	mc6_timing.t_refi = (tm->interface_params[IF_ID_0].interface_temp == MV_DDR_TEMP_HIGH) ? TREFI_HIGH : TREFI_LOW;

	/* the t_refi is in nsec */
	mc6_timing.t_refi = mc6_timing.t_refi / (MEGA / MV_DDR_MC6_FCLK_200MHZ_IN_KILO);
	/* printf("t_refi = %d\n", mc6_timing.t_refi); */
	mc6_timing.t_wr = speed_bin_table(speed_bin_index, SPEED_BIN_TWR);
	/* printf("t_wr = %d\n", mc6_timing.t_wr); */
	mc6_timing.t_wr = ceil_div(mc6_timing.t_wr, mc6_timing.t_ckclk);
	/* printf("t_wr = %d\n", mc6_timing.t_wr); */

	/* calculate t_rrd */
	mc6_timing.t_rrd = (page_size == 1) ? speed_bin_table(speed_bin_index, SPEED_BIN_TRRD1K) :
		speed_bin_table(speed_bin_index, SPEED_BIN_TRRD2K);
	/* printf("t_rrd = %d\n", mc6_timing.t_rrd); */
	mc6_timing.t_rrd = GET_MAX_VALUE(mc6_timing.t_ckclk * 4, mc6_timing.t_rrd);
	/* printf("t_rrd = %d\n", mc6_timing.t_rrd); */
	mc6_timing.t_rrd = ceil_div(mc6_timing.t_rrd, mc6_timing.t_ckclk);
	/* printf("t_rrd = %d\n", mc6_timing.t_rrd); */

	/* calculate t_faw */
	mc6_timing.t_faw = (page_size == 1) ? speed_bin_table(speed_bin_index, SPEED_BIN_TFAW1K):
		speed_bin_table(speed_bin_index, SPEED_BIN_TFAW2K);
	/* printf("t_faw = %d\n", mc6_timing.t_faw); */
	mc6_timing.t_faw = ceil_div(mc6_timing.t_faw, mc6_timing.t_ckclk);
	/* printf("t_faw = %d\n", mc6_timing. t_faw); */

	/* calculate t_rtp */
	mc6_timing.t_rtp = speed_bin_table(speed_bin_index, SPEED_BIN_TRTP);
	/* printf("t_rtp = %d\n", mc6_timing.t_rtp); */
	mc6_timing.t_rtp = GET_MAX_VALUE(mc6_timing.t_ckclk * 4, mc6_timing.t_rtp);
	/* printf("t_rtp = %d\n", mc6_timing.t_rtp); */
	mc6_timing.t_rtp = ceil_div(mc6_timing.t_rtp, mc6_timing.t_ckclk);
	/* printf("t_rtp = %d\n", mc6_timing.t_rtp); */

	/* calculate t_mode */
	mc6_timing.t_mod = speed_bin_table(speed_bin_index, SPEED_BIN_TMOD);
	/* printf("t_mod = %d\n", mc6_timing.t_mod); */
	mc6_timing.t_mod = GET_MAX_VALUE(mc6_timing.t_ckclk * 24, mc6_timing.t_mod);
	/* printf("t_mod = %d\n", mc6_timing.t_mod); */
	mc6_timing.t_mod = ceil_div(mc6_timing.t_mod, mc6_timing.t_ckclk);
	/* printf("t_mod = %d\n",mc6_timing. t_mod); */

	/* calculate t_wtr */
	mc6_timing.t_wtr = speed_bin_table(speed_bin_index, SPEED_BIN_TWTR);
	/* printf("t_wtr = %d\n", mc6_timing.t_wtr); */
	mc6_timing.t_wtr = GET_MAX_VALUE(mc6_timing.t_ckclk * 2, mc6_timing.t_wtr);
	/* printf("t_wtr = %d\n", mc6_timing.t_wtr); */
	mc6_timing.t_wtr = ceil_div(mc6_timing.t_wtr, mc6_timing.t_ckclk);
	/* printf("t_wtr = %d\n", mc6_timing.t_wtr); */

	/* calculate t_wtr_l */
	mc6_timing.t_wtr_l = speed_bin_table(speed_bin_index, SPEED_BIN_TWTRL);
	/* printf("t_wtr_l = %d\n", mc6_timing.t_wtr_l); */
	mc6_timing.t_wtr_l = GET_MAX_VALUE(mc6_timing.t_ckclk * 4, mc6_timing.t_wtr_l);
	/* printf("t_wtr_l = %d\n", mc6_timing.t_wtr_l); */
	mc6_timing.t_wtr_l = ceil_div(mc6_timing.t_wtr_l, mc6_timing.t_ckclk);
	/* printf("t_wtr_l = %d\n", mc6_timing.t_wtr_l); */

	/* calculate t_xp */
	mc6_timing.t_xp = MV_DDR_MC6_TIMING_T_XP;
	/* printf("t_xp = %d\n", mc6_timing.t_xp); */
	mc6_timing.t_xp = GET_MAX_VALUE(mc6_timing.t_ckclk * 4, mc6_timing.t_xp);
	/* printf("t_xp = %d\n", mc6_timing.t_xp); */
	mc6_timing.t_xp = ceil_div(mc6_timing.t_xp, mc6_timing.t_ckclk);
	/* printf("t_xp = %d\n", mc6_timing.t_xp); */

	/* calculate t_xs_fast */
	/* printf("t_xs_fast = %d\n", MV_DDR_MC6_TIMING_T_XS_FAST); */
	mc6_timing.t_xs_fast = ceil_div(MV_DDR_MC6_TIMING_T_XS_FAST, mc6_timing.t_ckclk);
	/* printf("t_xs_fast = %d\n", mc6_timing.t_xs_fast); */

	/* calculate t_xs */
	/* printf("t_xs = %d\n", MV_DDR_MC6_TIMING_T_XS); */
	mc6_timing.t_xs = ceil_div(MV_DDR_MC6_TIMING_T_XS, mc6_timing.t_ckclk);
	/* printf("t_xs = %d\n", mc6_timing.t_xs); */

	/* calculate t_cke */
	mc6_timing.t_cke = MV_DDR_MC6_TIMING_T_CKE;
	/* printf("t_cke = %d\n", mc6_timing.t_cke); */
	mc6_timing.t_cke = GET_MAX_VALUE(mc6_timing.t_ckclk * 3, mc6_timing.t_cke);
	/* printf("t_cke = %d\n", mc6_timing.t_cke); */
	mc6_timing.t_cke = ceil_div(mc6_timing.t_cke, mc6_timing.t_ckclk);
	/* printf("t_cke = %d\n", mc6_timing.t_cke); */

	/* calculate t_ckesr */
	mc6_timing.t_ckesr = mc6_timing.t_cke + 1;
	/* printf("t_ckesr = %d\n", mc6_timing.t_ckesr); */

	/* calculate t_cksrx */
	mc6_timing.t_cksrx = MV_DDR_MC6_TIMING_T_CKSRX;
	/* printf("t_cksrx = %d\n", mc6_timing.t_cksrx); */
	mc6_timing.t_cksrx = GET_MAX_VALUE(mc6_timing.t_ckclk * 5, mc6_timing.t_cksrx);
	/* printf("t_cksrx = %d\n", mc6_timing.t_cksrx); */
	mc6_timing.t_cksrx = ceil_div(mc6_timing.t_cksrx, mc6_timing.t_ckclk);
	/* printf("t_cksrx = %d\n", mc6_timing.t_cksrx); */

	/* calculate t_cksre */
	mc6_timing.t_cksre = mc6_timing.t_cksrx;
	/* printf("t_cksre = %d\n", mc6_timing.t_cksre); */

	/* calculate t_ras */
	mc6_timing.t_ras = speed_bin_table(speed_bin_index, SPEED_BIN_TRAS);
	/* printf("t_ras = %d\n", mc6_timing.t_ras); */
	mc6_timing.t_ras = ceil_div(mc6_timing.t_ras, mc6_timing.t_ckclk);
	/* printf("t_ras = %d\n", mc6_timing.t_ras); */

	/* calculate t_rcd */
	mc6_timing.t_rcd = speed_bin_table(speed_bin_index, SPEED_BIN_TRCD);
	/* printf("t_rcd = %d\n", mc6_timing.t_rcd); */
	mc6_timing.t_rcd = ceil_div(mc6_timing.t_rcd, mc6_timing.t_ckclk);
	/* printf("t_rcd = %d\n", mc6_timing.t_rcd); */

	/* calculate t_rp */
	mc6_timing.t_rp = speed_bin_table(speed_bin_index, SPEED_BIN_TRP);
	/* printf("t_rp = %d\n", mc6_timing.t_rp); */
	mc6_timing.t_rp = ceil_div(mc6_timing.t_rp, mc6_timing.t_ckclk);
	/* printf("t_rp = %d\n", mc6_timing.t_rp); */

	/*calculate t_rfc */
	mc6_timing.t_rfc = ceil_div(rfc_table[memory_size] * 1000, mc6_timing.t_ckclk);
	/* printf("t_rfc = %d\n", mc6_timing.t_rfc); */

	/* calculate t_rrd_l */
	mc6_timing.t_rrd_l = MV_DDR_MC6_TIMING_T_RRDL;
	/* printf("t_rrd_l = %d\n", mc6_timing.t_rrd_l); */
	mc6_timing.t_rrd_l = GET_MAX_VALUE(mc6_timing.t_ckclk * 4, mc6_timing.t_rrd_l);
	/* printf("t_rrd_l = %d\n", mc6_timing. t_rrd_l); */
	mc6_timing.t_rrd_l = ceil_div(mc6_timing.t_rrd_l, mc6_timing.t_ckclk);
	/* printf("t_rrd_l = %d\n", mc6_timing.t_rrd_l); */

	/* calculate t_ccd_l */
	mc6_timing.t_ccd_l = 4; /* FIXME: insert to speed bin table */
	/* printf("t_ccd_l = %d\n", mc6_timing.t_ccd_l); */

	/* calculate t_rc */
	mc6_timing.t_rc = speed_bin_table(speed_bin_index, SPEED_BIN_TRC);
	/* printf("t_rc = %d\n", mc6_timing.t_rc); */
	mc6_timing.t_rc = ceil_div(mc6_timing.t_rc, mc6_timing.t_ckclk);
	/* printf("t_rc = %d\n", mc6_timing.t_rc); */

	/* constant timing parameters */
	mc6_timing.read_gap_extend = MV_DDR_MC6_TIMING_READ_GAP_EXTEND;
	/* printf("read_gap_extend = %d\n", mc6_timing.read_gap_extend); */

	mc6_timing.t_zqoper = MV_DDR_MC6_TIMING_T_ZQOPER;
	/* printf("t_zqoper = %d\n", mc6_timing.t_zqoper); */

	mc6_timing.t_res = MV_DDR_MC6_TIMING_T_RES;
	/* printf("t_res = %d\n", mc6_timing.t_res); */
	mc6_timing.t_res = ceil_div(mc6_timing.t_res, mc6_timing.t_ckclk);
	/* printf("t_res = %d\n", mc6_timing.t_res); */

	mc6_timing.t_resinit = MV_DDR_MC6_TIMING_T_RESINIT;
	/* printf("t_resinit = %d\n", mc6_timing.t_resinit); */
	mc6_timing.t_resinit = ceil_div(mc6_timing.t_resinit, mc6_timing.t_ckclk);
	/* printf("t_resinit = %d\n", mc6_timing.t_resinit); */

	mc6_timing.t_restcke = MV_DDR_MC6_TIMING_T_RESTCKE;
	/* printf("t_restcke = %d\n", mc6_timing.t_restcke); */
	mc6_timing.t_restcke = ceil_div(mc6_timing.t_restcke, mc6_timing.t_ckclk);
	/* printf("t_restcke = %d\n", mc6_timing.t_restcke); */

	mc6_timing.t_actpden = MV_DDR_MC6_TIMING_T_ACTPDEN;
	/* printf("t_actpden = %d\n", mc6_timing.t_actpden); */

	mc6_timing.t_zqinit = MV_DDR_MC6_TIMING_T_ZQINIT;
	/* printf("t_zqinit = %d\n", mc6_timing.t_zqinit); */

	mc6_timing.t_zqcr = MV_DDR_MC6_TIMING_T_ZQCR;
	/* printf("t_zqcr = %d\n", mc6_timing.t_zqcr); */

	mc6_timing.t_zqcs = MV_DDR_MC6_TIMING_T_ZQCS;
	/* printf("t_zqcs = %d\n", mc6_timing.t_zqcs); */

	mc6_timing.t_ccd = MV_DDR_MC6_TIMING_T_CCD;
	/* printf("t_ccd = %d\n", mc6_timing.t_ccd); */

	mc6_timing.t_mrd = MV_DDR_MC6_TIMING_T_MRD;
	/* printf("t_mrd = %d\n", mc6_timing.t_mrd); */

	mc6_timing.t_caext = MV_DDR_MC6_TIMING_T_CAEXT;
	/* printf("t_caext = %d\n", mc6_timing.t_caext); */

	mc6_timing.t_cackel = MV_DDR_MC6_TIMING_T_CACKEL;
	/* printf("t_cackel = %d\n", mc6_timing.t_cackel); */

	mc6_timing.t_mpx_lh = MV_DDR_MC6_TIMING_T_MPX_LH;
	/* printf("t_mpx_lh = %d\n", mc6_timing.t_mpx_lh); */

	mc6_timing.t_mpx_s = MV_DDR_MC6_TIMING_T_MPX_S;
	/* printf("t_mpx_s = %d\n", mc6_timing.t_mpx_s); */

	mc6_timing.t_xmp = MV_DDR_MC6_TIMING_T_XMP;
	/* printf("t_xmp = %d\n", mc6_timing.t_xmp); */

	mc6_timing.t_mrd_pda = MV_DDR_MC6_TIMING_T_MRD_PDA;
	/* printf("t_mrd_pda = %d\n", mc6_timing.t_mrd_pda); */

	mc6_timing.t_xsdll = MV_DDR_MC6_TIMING_T_XSDLL;
	/* printf("t_xsdll = %d\n", mc6_timing.t_xsdll); */

	mc6_timing.t_rwd_ext_dly = MV_DDR_MC6_TIMING_T_RWD_EXT_DLY;
	/* printf("t_rwd_ext_dly = %d\n", mc6_timing.t_rwd_ext_dly); */

	mc6_timing.t_ccd_ccs_wr_ext_dly = MV_DDR_MC6_TIMMING_T_CCD_CCS_WR_EXT_DLY;
	/* printf("t_ccd_ccs_wr_ext_dly = %d\n", mc6_timing.t_ccd_ccs_wr_ext_dly); */

	mc6_timing.t_ccd_ccs_ext_dly = MV_DDR_MC6_TIMING_T_CCD_CCS_EXT_DLY;
	/* printf("t_ccd_ccs_ext_dly = %d\n", mc6_timing.t_ccd_ccs_ext_dly); */

	mc6_timing.cl = tm->interface_params[IF_ID_0].cas_l;
	mc6_timing.cwl = tm->interface_params[IF_ID_0].cas_wl;

	/* configure the timing registers */
		reg_bit_clrset(MC6_REG_DRAM_CFG1,
			       mc6_timing.cwl << MC6_CWL_OFFS | mc6_timing.cl << MC6_CL_OFFS,
			       MC6_CWL_MASK << MC6_CWL_OFFS | MC6_CL_MASK << MC6_CL_OFFS);
	/* printf("MC6_REG_DRAM_CFG1 addr 0x%x, data 0x%x\n", MC6_REG_DRAM_CFG1,
	       reg_read(MC6_REG_DRAM_CFG1)); */

	reg_bit_clrset(MC6_REG_INIT_TIMING_CTRL_0,
		       mc6_timing.t_restcke << MC6_INIT_COUNT_NOP_OFFS,
		       MC6_INIT_COUNT_NOP_MASK << MC6_INIT_COUNT_NOP_OFFS);
	/* printf("MC6_REG_INIT_TIMING_CTRL_0 addr 0x%x, data 0x%x\n", MC6_REG_INIT_TIMING_CTRL_0,
	       reg_read(MC6_REG_INIT_TIMING_CTRL_0)); */

	reg_bit_clrset(MC6_REG_INIT_TIMING_CTRL_1,
		       mc6_timing.t_resinit << MC6_INIT_COUNT_OFFS,
		       MC6_INIT_COUNT_MASK << MC6_INIT_COUNT_OFFS);
	/* printf("MC6_REG_INIT_TIMING_CTRL_1 addr 0x%x, data 0x%x\n", MC6_REG_INIT_TIMING_CTRL_1,
	       reg_read(MC6_REG_INIT_TIMING_CTRL_1)); */

	reg_bit_clrset(MC6_REG_INIT_TIMING_CTRL_2,
		       mc6_timing.t_res << MC6_RESET_COUNT_OFFS,
		       MC6_RESET_COUNT_MASK << MC6_RESET_COUNT_OFFS);
	/* printf("MC6_REG_INIT_TIMING_CTRL_2 addr 0x%x, data 0x%x\n", MC6_REG_INIT_TIMING_CTRL_2,
	       reg_read(MC6_REG_INIT_TIMING_CTRL_2)); */

	reg_bit_clrset(MC6_REG_ZQC_TIMING_0,
		       mc6_timing.t_zqinit << MC6_TZQINIT_OFFS,
		       MC6_TZQINIT_MASK << MC6_TZQINIT_OFFS);
	/* printf("MC6_REG_ZQC_TIMING_0 addr 0x%x, data 0x%x\n", MC6_REG_ZQC_TIMING_0,
	       reg_read(MC6_REG_ZQC_TIMING_0)); */

	reg_bit_clrset(MC6_REG_ZQC_TIMING_1,
		       mc6_timing.t_zqoper << MC6_TZQCL_OFFS |
		       mc6_timing.t_zqcs << MC6_TZQCS_OFFS,
		       MC6_TZQCL_MASK << MC6_TZQCL_OFFS |
		       MC6_TZQCS_MASK << MC6_TZQCS_OFFS);
	/* printf("MC6_REG_ZQC_TIMING_1 addr 0x%x, data 0x%x\n", MC6_REG_ZQC_TIMING_1,
	       reg_read(MC6_REG_ZQC_TIMING_1)); */

	reg_bit_clrset(MC6_REG_REFRESH_TIMING,
		       mc6_timing.t_refi << MC6_TREFI_OFFS |
		       mc6_timing.t_rfc << MC6_TRFC_OFFS,
		       MC6_TREFI_MASK << MC6_TREFI_OFFS |
		       MC6_TRFC_MASK << MC6_TRFC_OFFS);
	/* printf("MC6_REG_REFRESH_TIMING addr 0x%x, data 0x%x\n", MC6_REG_REFRESH_TIMING,
	       reg_read(MC6_REG_REFRESH_TIMING)); */

	reg_bit_clrset(MC6_REG_SELF_REFRESH_TIMING_0,
		       mc6_timing.t_xsdll << MC6_TXSRD_OFFS |
		       mc6_timing.t_xs << MC6_TXSNR_OFFS,
		       MC6_TXSRD_MASK << MC6_TXSRD_OFFS |
		       MC6_TXSNR_MASK << MC6_TXSNR_OFFS);
	/* printf("MC6_REG_SELF_REFRESH_TIMING_0 addr 0x%x, data 0x%x\n", MC6_REG_SELF_REFRESH_TIMING_0,
	       reg_read(MC6_REG_SELF_REFRESH_TIMING_0)); */

	reg_bit_clrset(MC6_REG_SELF_REFRESH_TIMING_1,
		       mc6_timing.t_cksrx << MC6_TCKSRX_OFFS |
		       mc6_timing.t_cksre << MC6_TCKSRE_OFFS,
		       MC6_TCKSRX_MASK << MC6_TCKSRX_OFFS |
		       MC6_TCKSRE_MASK << MC6_TCKSRE_OFFS);
	/* printf("MC6_REG_SELF_REFRESH_TIMING_1 addr 0x%x, data 0x%x\n", MC6_REG_SELF_REFRESH_TIMING_1,
	       reg_read(MC6_REG_SELF_REFRESH_TIMING_1)); */

	reg_bit_clrset(MC6_REG_POWER_DOWN_TIMING_0,
		       TXARDS << MC6_TCKSRX_OFFS |
		       mc6_timing.t_xp << MC6_TXP_OFFS |
		       mc6_timing.t_ckesr << MC6_TCKESR_OFFS,
		       MC6_TXARDS_MASK << MC6_TCKSRX_OFFS |
		       MC6_TXP_MASK << MC6_TXP_OFFS |
		       MC6_TCKESR_MASK << MC6_TCKESR_OFFS);
	/* printf("MC6_REG_POWER_DOWN_TIMING_0 addr 0x%x, data 0x%x\n", MC6_REG_POWER_DOWN_TIMING_0,
	       reg_read(MC6_REG_POWER_DOWN_TIMING_0)); */

	reg_bit_clrset(MC6_REG_POWER_DOWN_TIMING_1,
		       mc6_timing.t_actpden << MC6_TPDEN_OFFS,
		       MC6_TPDEN_MASK << MC6_TPDEN_OFFS);
	/* printf("MC6_REG_POWER_DOWN_TIMING_1 addr 0x%x, data 0x%x\n", MC6_REG_POWER_DOWN_TIMING_1,
	       reg_read(MC6_REG_POWER_DOWN_TIMING_1)); */

	reg_bit_clrset(MC6_REG_MRS_TIMING,
		       mc6_timing.t_mrd << MC6_TMRD_OFFS |
		       mc6_timing.t_mod << MC6_TMOD_OFFS,
		       MC6_TMRD_MASK << MC6_TMRD_OFFS |
		       MC6_TMOD_MASK << MC6_TMOD_OFFS);
	/* printf("MC6_REG_MRS_TIMING addr 0x%x, data 0x%x\n", MC6_REG_MRS_TIMING,
	       reg_read(MC6_REG_MRS_TIMING)); */

	reg_bit_clrset(MC6_REG_ACT_TIMING,
		       mc6_timing.t_ras << MC6_TRAS_OFFS |
		       mc6_timing.t_rcd << MC6_TRCD_OFFS |
		       mc6_timing.t_rc << MC6_TRC_OFFS |
		       mc6_timing.t_faw << MC6_TFAW_OFFS,
		       MC6_TRAS_MASK << MC6_TRAS_OFFS |
		       MC6_TRCD_MASK << MC6_TRCD_OFFS |
		       MC6_TRC_MASK << MC6_TRC_OFFS |
		       MC6_TFAW_MASK << MC6_TFAW_OFFS);
	/* printf("MC6_REG_ACT_TIMING addr 0x%x, data 0x%x\n", MC6_REG_ACT_TIMING,
	       reg_read(MC6_REG_ACT_TIMING)); */

	reg_bit_clrset(MC6_REG_PRE_CHARGE_TIMING,
		       mc6_timing.t_rp << MC6_TRP_OFFS |
		       mc6_timing.t_rtp << MC6_TRTP_OFFS |
		       mc6_timing.t_wr << MC6_TWR_OFFS |
		       mc6_timing.t_rp << MC6_TRPA_OFFS,
		       MC6_TRP_MASK << MC6_TRP_OFFS |
		       MC6_TRTP_MASK << MC6_TRTP_OFFS |
		       MC6_TWR_MASK << MC6_TWR_OFFS |
		       MC6_TRPA_MASK << MC6_TRPA_OFFS);
	/* printf("MC6_REG_PRE_CHARGE_TIMING addr 0x%x, data 0x%x\n", MC6_REG_PRE_CHARGE_TIMING,
	       reg_read(MC6_REG_PRE_CHARGE_TIMING)); */

	reg_bit_clrset(MC6_REG_CAS_RAS_TIMING_0,
		       mc6_timing.t_wtr << MC6_TWTR_S_OFFS |
		       mc6_timing.t_wtr_l << MC6_TWTR_OFFS |
		       mc6_timing.t_ccd << MC6_TCCD_S_OFFS |
		       mc6_timing.t_ccd_l << MC6_TCCD_OFFS,
		       MC6_TWTR_S_MASK << MC6_TWTR_S_OFFS |
		       MC6_TWTR_MASK << MC6_TWTR_OFFS |
		       MC6_TCCD_S_MASK << MC6_TCCD_S_OFFS |
		       MC6_TCCD_MASK << MC6_TCCD_OFFS);
	/* printf("MC6_REG_CAS_RAS_TIMING_0 addr 0x%x, data 0x%x\n", MC6_REG_CAS_RAS_TIMING_0,
	       reg_read(MC6_REG_CAS_RAS_TIMING_0)); */

	/* TODO: check why change default of 17:16 tDQS2DQ from '1' to '0' */
	reg_bit_clrset(MC6_REG_CAS_RAS_TIMING_1,
		       mc6_timing.t_rrd << MC6_TRRD_S_OFFS |
		       mc6_timing.t_rrd_l << MC6_TRRD_OFFS |
		       TDQS2DQ << MC6_TDQS2DQ_OFFS,
		       MC6_TRRD_S_MASK << MC6_TRRD_S_OFFS |
		       MC6_TRRD_MASK << MC6_TRRD_OFFS |
		       MC6_TDQS2DQ_MASK << MC6_TDQS2DQ_OFFS);
	/* printf("MC6_REG_CAS_RAS_TIMING_1 addr 0x%x, data 0x%x\n", MC6_REG_CAS_RAS_TIMING_1,
	       reg_read(MC6_REG_CAS_RAS_TIMING_1)); */

	reg_bit_clrset(MC6_REG_OFF_SPEC_TIMING_0,
		       mc6_timing.t_ccd_ccs_ext_dly << MC6_TCCD_CCS_EXT_DLY_OFFS |
		       mc6_timing.t_ccd_ccs_wr_ext_dly << MC6_TCCD_CCS_WR_EXT_DLY_OFFS |
		       mc6_timing.t_rwd_ext_dly << MC6_TRWD_EXT_DLY_OFFS,
		       MC6_TCCD_CCS_EXT_DLY_MASK << MC6_TCCD_CCS_EXT_DLY_OFFS |
		       MC6_TCCD_CCS_WR_EXT_DLY_MASK << MC6_TCCD_CCS_WR_EXT_DLY_OFFS |
		       MC6_TRWD_EXT_DLY_MASK << MC6_TRWD_EXT_DLY_OFFS);
	/* printf("MC6_REG_OFF_SPEC_TIMING_0 addr 0x%x, data 0x%x\n", MC6_REG_OFF_SPEC_TIMING_0,
	       reg_read(MC6_REG_OFF_SPEC_TIMING_0)); */

	reg_bit_clrset(MC6_REG_OFF_SPEC_TIMING_1,
		       mc6_timing.read_gap_extend << MC6_READ_GAP_EXTEND_OFFS |
		       mc6_timing.t_ccd_ccs_ext_dly << MC6_TCCD_CCS_EXT_DLY_MIN_OFFS |
		       mc6_timing.t_ccd_ccs_wr_ext_dly << MC6_TCCD_CCS_WR_EXT_DLY_MIN_OFFS,
		       MC6_READ_GAP_EXTEND_MASK << MC6_READ_GAP_EXTEND_OFFS |
		       MC6_TCCD_CCS_EXT_DLY_MIN_MASK << MC6_TCCD_CCS_EXT_DLY_MIN_OFFS |
		       MC6_TCCD_CCS_WR_EXT_DLY_MIN_MASK << MC6_TCCD_CCS_WR_EXT_DLY_MIN_OFFS);
	/* printf("MC6_REG_OFF_SPEC_TIMING_1 addr 0x%x, data 0x%x\n", MC6_REG_OFF_SPEC_TIMING_1,
	       reg_read(MC6_REG_OFF_SPEC_TIMING_1)); */

	/* TODO: check why change default of 3:0 tDQSCK from '3' to '0' */
	reg_bit_clrset(MC6_REG_DRAM_READ_TIMING,
		       TDQSCK << MC6_TDQSCK_OFFS,
		       MC6_TDQSCK_MASK << MC6_TDQSCK_OFFS);
	/* printf("MC6_REG_DRAM_READ_TIMING addr 0x%x, data 0x%x\n", MC6_REG_DRAM_READ_TIMING,
	       reg_read(MC6_REG_DRAM_READ_TIMING)); */

	/* TODO: check why 0x3c8 register is missing from spec */
	reg_bit_clrset(MC6_REG_CA_TRAIN_TIMING,
		       mc6_timing.t_cackel << MC6_TCACKEL_OFFS |
		       mc6_timing.t_caext << MC6_TCAEXT_OFFS,
		       MC6_TCACKEL_MASK << MC6_TCACKEL_OFFS |
		       MC6_TCAEXT_MASK << MC6_TCAEXT_OFFS);
	/* printf("MC6_REG_CA_TRAIN_TIMING addr 0x%x, data 0x%x\n", MC6_REG_CA_TRAIN_TIMING,
	       reg_read(MC6_REG_CA_TRAIN_TIMING)); */

	reg_bit_clrset(MC6_REG_MPD_TIMING,
		       mc6_timing.t_xmp << MC6_TXMP_OFFS |
		       mc6_timing.t_mpx_s << MC6_TMPX_S_OFFS |
		       mc6_timing.t_mpx_lh << MC6_TMPX_LH_OFFS,
		       MC6_TXMP_MASK << MC6_TXMP_OFFS |
		       MC6_TMPX_S_MASK << MC6_TMPX_S_OFFS |
		       MC6_TMPX_LH_MASK << MC6_TMPX_LH_OFFS);
	/* printf("MC6_REG_MPD_TIMING addr 0x%x, data 0x%x\n", MC6_REG_MPD_TIMING,
	       reg_read(MC6_REG_MPD_TIMING)); */

	reg_bit_clrset(MC6_REG_PDA_TIMING,
		       mc6_timing.t_mrd_pda << MC6_TMRD_PDA_OFFS,
		       MC6_TMRD_PDA_MASK << MC6_TMRD_PDA_OFFS);
	/* printf("MC6_REG_PDA_TIMING addr 0x%x, data 0x%x\n", MC6_REG_PDA_TIMING,
	       reg_read(MC6_REG_PDA_TIMING)); */
}

void mv_ddr_mc6_and_dram_timing_set(void)
{
	/* get the frequency */
	u32 freq_mhz;
	struct mv_ddr_topology_map *tm = mv_ddr_topology_map_get();

	/* get the frequency form the topology */
	freq_mhz = freq_val[tm->interface_params[IF_ID_0].memory_freq];

	mv_ddr_mc6_timing_regs_cfg(freq_mhz);
}