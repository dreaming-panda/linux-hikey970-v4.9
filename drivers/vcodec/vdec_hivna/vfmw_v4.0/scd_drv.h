#ifndef __SCD_DRV_H__
#define __SCD_DRV_H__

#include "mem_manage.h"
//#include "../omxvdec/memory.h"
#include "vfmw.h"

#define SCDDRV_OK                      (0)
#define SCDDRV_ERR                     (-1)

#define SCD_TIME_OUT_COUNT             (200)

#define REG_SCD_START                  (0x800)
#define REG_LIST_ADDRESS               (0x804)
#define REG_UP_ADDRESS                 (0x808)
#define REG_UP_LEN                     (0x80c)
#define REG_BUFFER_FIRST               (0x810)
#define REG_BUFFER_LAST                (0x814)
#define REG_BUFFER_INI                 (0x818)
#define REG_SCD_PROTOCOL               (0x820)

/* state registers */
#define REG_SCD_OVER                   (0x840)
#define REG_SCD_INT                    (0x844)
#define REG_SCD_NUM                    (0x84c)
#define REG_ROLL_ADDR                  (0x850)
#define REG_SRC_EATEN                  (0x854)

#define REG_SCD_SAFE_INT_MASK          (0x884)
#define REG_SCD_SAFE_INI_CLR           (0x888)
#define REG_SCD_NORM_INT_MASK          (0x81c)
#define REG_SCD_NORM_INI_CLR           (0x824)
 
#define REG_SCD_INT_MASK               REG_SCD_NORM_INT_MASK
#define REG_SCD_INI_CLR                REG_SCD_NORM_INI_CLR 

#define REG_AVS_FLAG                   (0x0000)
#define REG_EMAR_ID                    (0x0004)
#define REG_VDH_SELRST                 (0x0008)

#define SM_SCD_UP_INFO_NUM             (2)
#ifdef CFG_MAX_RAW_NUM
#define MAX_STREAM_RAW_NUM             (CFG_MAX_RAW_NUM)
#else
#define MAX_STREAM_RAW_NUM             (1024)
#endif
#ifdef CFG_MAX_SEG_NUM
#define MAX_STREAM_SEG_NUM             (CFG_MAX_SEG_NUM)
#else
#define MAX_STREAM_SEG_NUM             (1024 + 128)
#endif
#define SM_MAX_DOWNMSG_SIZE            (3 * MAX_STREAM_RAW_NUM * sizeof(SINT32))
#define SM_MAX_UPMSG_SIZE              (SM_SCD_UP_INFO_NUM * MAX_STREAM_SEG_NUM * sizeof(SINT32))
typedef enum {
	FMW_OK          = 0,
	FMW_ERR_PARAM   = -1,
	FMW_ERR_NOMEM   = -2,
	FMW_ERR_NOTRDY  = -3,
	FMW_ERR_BUSY    = -4,
	FMW_ERR_RAWNULL = -5,
	FMW_ERR_SEGFULL = -6,
	FMW_ERR_SCD     = -7
} FMW_RETVAL_E;

typedef enum {
	SCDDRV_SLEEP_STAGE_NONE = 0,
	SCDDRV_SLEEP_STAGE_PREPARE,
	SCDDRV_SLEEP_STAGE_SLEEP
} SCDDRV_SLEEP_STAGE_E;

typedef enum {
	SCD_IDLE = 0,
	SCD_WORKING,
} SCD_STATE_E;

/* register operator */
#define RD_SCDREG(reg)       MEM_ReadPhyWord((gScdRegBaseAddr + reg))
#define WR_SCDREG(reg, dat)  MEM_WritePhyWord((gScdRegBaseAddr + reg),(dat))

#define FMW_ASSERT_RET( cond, ret )                     \
do{                                     \
	if (!(cond))                             \
		return (ret);                           \
} while (0)

/*######################################################
       struct defs.
 ######################################################*/
typedef enum {
	SCD_SHAREFD_MESSAGE_POOL = 0,
	SCD_SHAREFD_OUTPUT_BUF   = 1,
	SCD_SHAREFD_MAX
}SCD_SHAREFD;

typedef struct {
	int Scdover;
	int ScdInt;
	int ShortScdNum;
	int ScdNum;
	unsigned int ScdRollAddr;
	int SrcEaten;
} SM_STATEREG_S;

typedef struct
{
	char SliceCheckFlag;
	char ScdStart;
	unsigned int DownMsgPhyAddr;
	unsigned int UpMsgPhyAddr;
	int UpLen;
	unsigned int BufferFirst;
	unsigned int BufferLast;
	unsigned int BufferIni;
	int ScdProtocol;
	int ScdLowdlyEnable;
	int scd_share_fd[SCD_SHAREFD_MAX];
	int IsScdAllBufRemap;
} SCD_CONFIG_REG_S;

typedef struct {
	int ScdProtocol;
	int Scdover;
	int ScdInt;
	int ScdNum;
	unsigned int ScdRollAddr;
	int SrcEaten;
	int UpLen;
} SCD_STATE_REG_S;

typedef enum hi_CONFIG_SCD_CMD {
	CONFIG_SCD_REG_CMD = 100,
} CONFIG_SCD_CMD;

typedef struct {
	CONFIG_SCD_CMD cmd;
	int         eVidStd;
	unsigned int         SResetFlag;
	unsigned int         GlbResetFlag;
	SCD_CONFIG_REG_S SmCtrlReg;
} OMXSCD_REG_CFG_S;

int SCDDRV_PrepareSleep(void);

SCDDRV_SLEEP_STAGE_E SCDDRV_GetSleepStage(void);
void SCDDRV_SetSleepStage(SCDDRV_SLEEP_STAGE_E sleepState);

void SCDDRV_ForceSleep(void);

void SCDDRV_ExitSleep(void);

int SCDDRV_ResetSCD(void);

int SCDDRV_WriteReg(SCD_CONFIG_REG_S *pSmCtrlReg );

void SCDDRV_GetRegState(SCD_STATE_REG_S *pScdStateReg);

void SCDDRV_ISR(void);

void SCDDRV_init(void);

void SCDDRV_DeInit(void);

#ifdef ENV_ARMLINUX_KERNEL
int SCDDRV_IsScdIdle(void);
#endif
#endif
