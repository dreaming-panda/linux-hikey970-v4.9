/*-----------------------------------------------------------------------*/
/*!!Warning: Huawei key information asset. No spread without permission. */
/*CODEMARK:EG4uRhTwMmgcVFBsBnYHCDadN5jJKSuVyxmmaCmKFU6eJEbB2fyHF9weu4/jer/hxLHb+S1e
E0zVg4C3NiZh4Rryzsvo1gOdvy7M+qFCBFQKTTAFAVC3Q4e533WXdeQrddo4r2cqTmRg3Xeb
SI3trXaSV012ETxvJrJ/pkfs27/lT6wemL9iW3PaGW8//pmW7hQ7qCDBgWp7sMvcMuyYAWRh
jMb6+4xlgVl55z+iUl5XDCi0pMRG2hXB2hXZd5i/HJastZrWJFR4dVOatPlImg==#*/
/*--!!Warning: Deleting or modifying the preceding information is prohibited.--*/



/******************************************************************************

  ��Ȩ���� (C), 2001-2015, ��Ϊ�������޹�˾

******************************************************************************
    �� �� ��   : vdm_hal_vp9.c
    �� �� ��   : ����
    ��        ��   : z00290437
    ��������: 2015-02-03
    ����޸� :
    �������� : VDMV300 Ӳ������

  �޸���ʷ     :
    1.��    ��       :
    2.��    ��       :
    3.�޸�����:

******************************************************************************/

#ifndef __VDM_HAL_VP9_C__
#define __VDM_HAL_VP9_C__

//#include "public.h"
#include "vdm_hal.h"
#include "vdm_hal_api.h"
#include "vdm_hal_local.h"
#include "vdm_hal_vp9.h"
#include <linux/printk.h>

static int VP9HAL_CfgReg(OMXVDH_REG_CFG_S *pVdhRegCfg)
{
	unsigned int D32;

	//BASIC_CFG0
	D32 = 0;
	((BASIC_CFG0*)(&D32))->mbamt_to_dec      = ((BASIC_CFG0 *)(&pVdhRegCfg->VdhBasicCfg0))->mbamt_to_dec;
	((BASIC_CFG0*)(&D32))->load_qmatrix_flag = 0;
	//((BASIC_CFG0*)(&D32))->repair_en       = 0;

	WR_VREG(VREG_BASIC_CFG0, D32, 0);

	//BASIC_CFG1
	D32 = 0;
	((VP9_BASIC_CFG1*)(&D32))->video_standard     = 0x0E;  //VFMW_VP9;
  //  ((VP9_BASIC_CFG1*)(&D32))->ddr_stride       = pVp9DecParam->ddr_stride >> 6;
	((VP9_BASIC_CFG1*)(&D32))->uv_order_en        = ((VP9_BASIC_CFG1 *)(&pVdhRegCfg->VdhBasicCfg1))->uv_order_en;
	((VP9_BASIC_CFG1*)(&D32))->fst_slc_grp        = 1;
	((VP9_BASIC_CFG1*)(&D32))->mv_output_en       = 1;
	((VP9_BASIC_CFG1*)(&D32))->max_slcgrp_num     = 3;
	((VP9_BASIC_CFG1*)(&D32))->line_num_output_en = 0;
	//   ((BASIC_CFG1*)(&D32))->compress_en       = pVp9DecParam->Compress_en;
	((VP9_BASIC_CFG1*)(&D32))->vdh_2d_en          = ((VP9_BASIC_CFG1 *)(&pVdhRegCfg->VdhBasicCfg1))->vdh_2d_en; ////1;   by j00367396
	((VP9_BASIC_CFG1*)(&D32))->frm_cmp_en         = ((VP9_BASIC_CFG1 *)(&pVdhRegCfg->VdhBasicCfg1))->frm_cmp_en; //for tmp linear
	((VP9_BASIC_CFG1*)(&D32))->ppfd_en            = 0;

	WR_VREG(VREG_BASIC_CFG1, D32, 0);

	//AVM_ADDR
	D32 = 0;
	((AVM_ADDR*)(&D32))->av_msg_addr = (pVdhRegCfg->VdhAvmAddr) & 0xFFFFFFF0;
	WR_VREG(VREG_AVM_ADDR, D32, 0);

	//VAM_ADDR
	D32 = 0;
	((VAM_ADDR*)(&D32))->va_msg_addr = (pVdhRegCfg->VdhVamAddr) & 0xFFFFFFF0;
	WR_VREG(VREG_VAM_ADDR, D32, 0);

	//STREAM_BASE_ADDR
	WR_VREG(VREG_STREAM_BASE_ADDR, pVdhRegCfg->VdhStreamBaseAddr, 0);

	//PRC_CACHE_TYPE
	D32 = 0x0;
	WR_VREG(VREG_FF_APT_EN, D32, 0);

	//EMAR_ADDR
	D32 = 0x101;
	WR_SCDREG(REG_EMAR_ID, D32);

	//TIME_OUT
	D32 = 0x00300C03;
	WR_VREG(VREG_SED_TO,    D32, 0);
	WR_VREG(VREG_ITRANS_TO, D32, 0);
	WR_VREG(VREG_PMV_TO,    D32, 0);
	WR_VREG(VREG_PRC_TO,    D32, 0);
	WR_VREG(VREG_RCN_TO,    D32, 0);
	WR_VREG(VREG_DBLK_TO,   D32, 0);
	WR_VREG(VREG_PPFD_TO,   D32, 0);

	//DEC_OVER_INT_LEVEL
	D32 = 60;
	WR_VREG(VREG_PART_DEC_OVER_INT_LEVEL, D32, 0);

	//YSTADDR_1D
	D32 = 0;
	((YSTADDR_1D *)(&D32))->ystaddr_1d = (pVdhRegCfg->VdhYstAddr) & 0xFFFFFFF0;
	WR_VREG(VREG_YSTADDR_1D, D32, 0);

	//YSTRIDE_1D
	WR_VREG(VREG_YSTRIDE_1D, pVdhRegCfg->VdhYstride, 0);

	//UVOFFSET_1D
	WR_VREG(VREG_UVOFFSET_1D, pVdhRegCfg->VdhUvoffset, 0);

	//UVSTRIDE_1D
	WR_VREG(VREG_UVSTRIDE_1D, pVdhRegCfg->VdhUvstride, 0);

	//CFGINFO_ADDR
	WR_VREG(VREG_CFGINFO_ADDR, pVdhRegCfg->VdhCfgInfoAddr, 0);

	//DDR_INTERLEAVE_MODE
	D32 = 0x3;
	WR_VREG(VREG_DDR_INTERLEAVE_MODE, D32, 0);

	return VDMHAL_OK;
}

int VP9HAL_StartDec(OMXVDH_REG_CFG_S *pVdhRegCfg)
{
	int Ret;

	Ret = VP9HAL_CfgReg(pVdhRegCfg);
	if (Ret != VDMHAL_OK) {
		printk(KERN_ERR "VP9 register config failed\n");
		return VDMHAL_ERR;
	}

	return VDMHAL_OK;
}

#endif
