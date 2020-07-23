/*
* Copyright (C), 2004-2050, Hisilicon Tech. Co., Ltd.
*
* File Name     : drv_venc.c
* Version       : Initial Draft
* Author        : Hisilicon multimedia software group
* Created       : 2010/04/07
* Last Modified :
* Description   :
* Function List :
*
* History       :
* 1.Date        :
* Author        : j00131665
* Modification  : Created file
*/
#include "drv_venc.h"
#include "venc_regulator.h"
#include "drv_venc_osal.h"

unsigned int b_Regular_down_flag = 1;

/* ��߸�λvedu, ������ʱ�ӣ�������λl00214825  */
int VENC_DRV_BoardInit(void)
{
	int ret = 0;
	HI_DBG_VENC("enter %s()\n", __func__);

	ret = Venc_Regulator_Enable();/*lint !e838 */
	if (ret != 0){
		HI_INFO_VENC("enable regulator failed\n", __func__);
		return HI_FAILURE;
	}

	HI_DBG_VENC("exit %s ()\n", __func__);
	return HI_SUCCESS;
}

void VENC_DRV_BoardDeinit(void)
{
	HI_DBG_VENC("enter %s ()\n", __func__);

	Venc_Regulator_Disable();

	HI_DBG_VENC("exit %s ()\n", __func__);
}


