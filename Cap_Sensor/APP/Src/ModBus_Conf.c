/****************************************************************************************************************************************************************
** 版权:          2020-2030,深圳市信为科技发展有限公司
** 文件名:        ModbusRtu_Conf.c
** 作者:          杨春林
** 版本:          V1.0.0
** 日期:          2020-04-29
** 描述:          Modbus 功能码处理配置
** 功能:          对上位机发送的指令进行解析并响应,所有功能码和自动上传的程序代码都编写在本文件
*****************************************************************************************************************************************************************
** 修改者:        No
** 版本:          No
** 修改内容:      No
** 日期:          No
****************************************************************************************************************************************************************/

#include "ModBusRtu.h"


#ifdef __PICOCAP_APP_H

static uint32_t Calib_CapMin;                                                          //标定电容零点值
static uint32_t Calib_CapMax;  
//static uint8_t CapDA_ClibEn = DAOUTCLIB_DISABLE;
static uint16_t Calib_CapDAMin;                                                        //标定电容DA零点值
static uint16_t Calib_CapDALow;                                                        //标定电容DA下刻度
static uint16_t Calib_CapDAHigh;                                                       //标定电容DA上刻度
static uint16_t Calib_CapDAMax;                                                        //标定电容DA满点值
static uint16_t Calib_TempDAMin;                                                       //标定温度DA零点值
static uint16_t Calib_TempDAMax;                                                       //标定温度DA满量程
static uint16_t Calib_ADMin;                                                           //标定电容AD零点值
static uint16_t Calib_ADLow;                                                           //标定电容AD下刻度
static uint16_t Calib_ADHigh;                                                          //标定电容AD上刻度
static uint16_t Calib_ADMax;                                                           //标定电容AD满量程
//static float RecvVol[4];

const uint8_t SensorSoftVersion[8] = {0x07, 'S', 'V', '1', '.', '0', '.', '5'};//软件版本号 格式:版本号长度，版本号内容


//Modbus 串口重新初始化回调函数
static int MB_USART_ReInit(ModBusBaseParam_TypeDef *ModBusBaseParam);

/**
* 名称       : ModBus_ReadPreHandle()
* 创建日期   : 2020-05-07
* 作者       : 杨春林
* 功能       : Modbus 读预处理,从接收的原始数据中提取出消息ID和寄存器数量,将设备地址和功能码存入发送缓存中
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;        
* 输出参数   : ReadAddr: 读取地址(消息ID)
*              RegNum: 寄存器数量
* 返回结果   : 无
* 注意和说明 : 寄存器地址不能越界
* 修改内容   :
*/
static void ModBus_ReadPreHandle(ModBusBaseParam_TypeDef *ModBusBaseParam, uint16_t *ReadAddr, uint16_t *RegNum)
{
    //寄存器地址高字节
    *ReadAddr = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[2];
    *ReadAddr <<= 8;
    //寄存器地址高字节
    *ReadAddr |= ModBusBaseParam->ModBus_TX_RX.Receive_Buf[3];

    //寄存器数据高字节
    *RegNum = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[4];
    *RegNum <<= 8;
    //寄存器数量低字节
    *RegNum |= ModBusBaseParam->ModBus_TX_RX.Receive_Buf[5];

    ModBusBaseParam->ModBus_TX_RX.Send_Len = 0;
    //设备地址
    ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] 
        = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[0];
    //功能码
    ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] 
        = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[1];
    //数据字节数
    ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] 
        = *RegNum * 2;
}

/**
* 名称       : ModBus_ModBus_ReadPreHandlePreHandle()
* 创建日期   : 2020-05-07
* 作者       : 杨春林
* 功能       : Modbus 写预处理,从接收的原始数据中提取出消息ID,将设备地址、
*              功能码、消息ID和消息长度存入发送缓存中
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
*              arg:用户自定义的参数,这里为设备参数
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 寄存器地址不能越界
* 修改内容   :
*/
static void ModBus_WritePreHandle(ModBusBaseParam_TypeDef *ModBusBaseParam, uint16_t *WriteAddr)
{
    uint8_t Nr;
    //寄存器地址高字节
    *WriteAddr = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[2];
    *WriteAddr <<= 8;
    //寄存器地址低字节
    *WriteAddr |= ModBusBaseParam->ModBus_TX_RX.Receive_Buf[3];

    //将响应数据存入发送缓存
    for(Nr = 0; Nr < 6; Nr++)
    {
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[Nr] 
            = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[Nr];
    }
    ModBusBaseParam->ModBus_TX_RX.Send_Len = 6;
}

/**
* 名称       : ModbusFunc03()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : Modbus 03功能码消息帧处理
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
*              arg:用户自定义的参数,例如设备参数
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 
* 修改内容   :
*/
void ModbusFunc03(ModBusBaseParam_TypeDef *ModBusBaseParam, void *arg)
{
    uint16_t Nr;
    //寄存器数据
    uint16_t DataBuf;
    //寄存器地址
    uint16_t ReadAddr;
    //寄存器数量
    uint16_t RegNum;
    //设备参数
    ModBus_Device_Param *Device_Param;
        
    Device_Param = (ModBus_Device_Param *)arg;
    //读预处理
    ModBus_ReadPreHandle(ModBusBaseParam, &ReadAddr, &RegNum);
    //访问地址不在有效范围内
    if(!((ReadAddr >= HOLDING_REG_REGION1_BGEIN) && ((ReadAddr + RegNum) <= (HOLDING_REG_REGION1_END + 1))))
    {
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[1] |= MB_REQ_FAILURE;
        //访问地址无效
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[2] = MB_ADDR_EXCEPTION;
        return;
    }

    for(Nr = 0; Nr < RegNum; Nr++)
    {
        switch(ReadAddr)
        {
        case 0x0030:
            //设备地址
            DataBuf = ModBusBaseParam->Device_Addr;
        break;
                
        case 0x0031:
            //波特率
            DataBuf = ModBusBaseParam->BaudRate;
        break;
                
        case 0x0032:
            //奇偶校验
            DataBuf = ModBusBaseParam->Parity;
        break;

        case 0x0033:
            //空
            DataBuf = 0;
        break;

        case 0x0034:
            //补偿使能
            DataBuf = Device_Param->PCap_DataConvert->CompenEn;
        break;

        case 0x0035:
            //滤波系数
            DataBuf = Device_Param->DataFilter->FilterFactor;
        break;

        case 0x0036:
            //自动上传周期
            DataBuf = ModBusBaseParam->AutoUpload;
        break;

        case 0x0037:
            //电容修正系数K
            DataBuf = (uint16_t)(Device_Param->PCap_DataConvert->Correct_K * 100);
        break;

        case 0x0038:
            //电容修正系数B
            DataBuf = (uint16_t)Device_Param->PCap_DataConvert->Correct_B + 100;
        break;

        case 0x0039:
            //输出方式 保留
            DataBuf = ModBusBaseParam->Output_Mode;
        break;

        case 0x003F:
            //量程 保留
            DataBuf = Device_Param->PCap_DataConvert->HeightRange;
        break;

        case 0x0040:
            //空
            DataBuf = 0;
        break;

        case 0x0041:
            //空
            DataBuf = 0;
        break;

        default:
            DataBuf = 0;
        break;
        }
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] 
            = (unsigned char)(DataBuf >> 8);
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] 
            = (unsigned char)DataBuf;
        ReadAddr++;
    }
}

/**
* 名称       : ModbusFunc04()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : Modbus 04功能码消息帧处理
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
*              arg:用户自定义的参数,例如设备参数
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 
* 修改内容   :
*/
void ModbusFunc04(ModBusBaseParam_TypeDef *ModBusBaseParam, void *arg)
{
    //浮点数据
    float fbuf;
    uint16_t Nr;
    //寄存器地址
    uint16_t ReadAddr;
    //寄存器数量
    uint16_t RegNum;
    //寄存器数据
    uint32_t DataBuf;
    //设备参数
    ModBus_Device_Param *Device_Param;
        
    Device_Param = (ModBus_Device_Param *)arg;
    //读预处理
    ModBus_ReadPreHandle(ModBusBaseParam, &ReadAddr, &RegNum);
    //寄存器地址无效
    if((!(((ReadAddr + RegNum) <= (INPUT_REG_REGION1_END + 2)) 
        || ((ReadAddr == INPUT_REG_REGION2_BEGIN) 
        && ((ReadAddr + RegNum) <= (INPUT_REG_REGION2_END + 2))))
        || (0 != (ReadAddr&0x0001)))&&(0 != (RegNum&0x0001)))
    {
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[1] |= MB_REQ_FAILURE;
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[2] = MB_ADDR_EXCEPTION;
        return;
    }

    for(Nr = 0; Nr < RegNum; Nr += 2)
    {
        switch(ReadAddr)
        {
        case 0x0000:
            //液位高度AD值
            fbuf = (float)Device_Param->PCap_DataConvert_Out->LiquidHeightAD;
            DataBuf = *(unsigned long *)&fbuf;
        break;
            
        case 0x0002:
            //环境温度
            DataBuf = *(unsigned long *)&Device_Param->ADC_TemperOut->TemperInAir;
        break;
            
        case 0x0004:
            //液体温度 保留
            DataBuf = *(unsigned long *)&Device_Param->ADC_TemperOut->TemperInLiquid;
        break;

        case 0x0080:
            //PCap原始值
            fbuf = (float)Device_Param->PCap_DataConvert_Out->PCap_ResultValue;
            DataBuf = *(unsigned long *)&fbuf;
        break;

        default:
            DataBuf = 0;
        break;
        }

        ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] 
            = (DataBuf >> 24);
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] 
            = (DataBuf >> 16);
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] 
            = (DataBuf >> 8);
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] 
            = (unsigned char)DataBuf;
        ReadAddr += 2;
    }
}

/**
* 名称       : ModbusFunc05()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : Modbus 05功能码消息帧处理
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
*              arg:用户自定义的参数,例如设备参数
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 
* 修改内容   :
*/
void ModbusFunc05(ModBusBaseParam_TypeDef *ModBusBaseParam, void *arg)
{
    //寄存器地址
    uint16_t ReadAddr;
    //寄存器数据
    uint16_t DataBuf;
    static uint8_t CalibFlag = CALIB_CLEAR;
    //设备参数
    ModBus_Device_Param *Device_Param;
        
    Device_Param = (ModBus_Device_Param *)arg;
    //写预处理
    ModBus_WritePreHandle(ModBusBaseParam, &ReadAddr);    
    //地址无效或内部Flash使能无打开
    if(!(((ReadAddr >= SINGLE_COIL_REGION1_BEGIN)&&(ReadAddr <= SINGLE_COIL_REGION1_END))
       /*&&(FLASH_WR_ENABLE == device_obj->ProductParam->InRomWrEn) */|| (0x004A == ReadAddr)))
    {
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[1] |= MB_REQ_FAILURE;
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[2] = MB_ADDR_EXCEPTION;
        ModBusBaseParam->ModBus_TX_RX.Send_Len = 3;
        return;
    }

    //数据内容高字节
    DataBuf = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[4];
    DataBuf <<= 8;
    //数据内容低字节
    DataBuf |= ModBusBaseParam->ModBus_TX_RX.Receive_Buf[5];

    //数据内容无效
    if((0x0000 != DataBuf)&&(0xFF00 != DataBuf))
    {
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[1] |= MB_REQ_FAILURE;
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[2] = MB_VALU_EXCEPTION;
        ModBusBaseParam->ModBus_TX_RX.Send_Len = 3;
        return;
    }
     
    switch(ReadAddr)
    {
    case 0x004A:                                //为了跟老协议兼容
        //内部存储器使能或禁止
        if(0xFF00 == DataBuf)
        {
            ModBusBaseParam->InRomWrEn = FLASH_WR_ENABLE;
        }
        else
        {
            ModBusBaseParam->InRomWrEn = FLASH_WR_DISABLE;
        }
    break;      
      
    case 0x0050:
        //电容标定
        if(0xFF00 == DataBuf)       //标定满量程
        {
            Calib_CapMax = Device_Param->PCap_DataConvert_Out->PCap_ResultValue;
            CalibFlag |= CALIB_CAPMAX_FLAG;
        }
        else                        //标定零点
        {
            Calib_CapMin = Device_Param->PCap_DataConvert_Out->PCap_ResultValue;
            CalibFlag |= CALIB_CAPMIN_FLAG;
        }
        //电容标定标志位有效就写入标定内容  
        if(CALIB_CAPEOC_FLAG == CalibFlag)
        {
            MB_Cap_Calibration(arg);
            CalibFlag = CALIB_CLEAR;
        }
    break;

    case 0x0051:
        //恢复电容标定出厂值
        if(0xFF00 == DataBuf)
        {
            Calib_CapMin = Device_Param->PCap_DataConvert->CapMinBak;
            Calib_CapMax = Device_Param->PCap_DataConvert->CapMaxBak;
            MB_Cap_Calibration(arg);
        }
    break;

    case 0x0052:
        //设备冻结或解冻
        if(0xFF00 == DataBuf)
        {
            ModBusBaseParam->Freeze = FREEZE_ENABLE;
        }
        else
        {
            ModBusBaseParam->Freeze = FREEZE_DISABLE;
        }
    break;

    case 0x0053:
        //内部Flash使能或禁止
        if(0xFF00 == DataBuf)
        {
            ModBusBaseParam->InRomWrEn = FLASH_WR_ENABLE;
        }
        else
        {
            ModBusBaseParam->InRomWrEn = FLASH_WR_DISABLE;
        }
    break;
            
    default:
    break;
    }	
}

/**
* 名称       : ModbusFunc10()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : Modbus 10功能码消息帧处理
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
*              arg:用户自定义的参数,例如设备参数
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 
* 修改内容   :
*/
void ModbusFunc10(ModBusBaseParam_TypeDef *ModBusBaseParam, void *arg)
{
    uint16_t Nr;
    //内容索引
    uint16_t Index;
    //寄存器地址
    uint16_t ReadAddr;
    //寄存器数量
    uint16_t RegNum;
    //16位数据暂存
    uint16_t u16temp;
    //8位数据数组
    uint8_t u8array[2];
    //设备参数
    ModBus_Device_Param *Device_Param;
        
    Device_Param = (ModBus_Device_Param *)arg;
    
    //寄存器数量高字节
    RegNum = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[4];
    RegNum <<= 8;
    //寄存器数量低字节
    RegNum |= ModBusBaseParam->ModBus_TX_RX.Receive_Buf[5];
    //写预处理
    ModBus_WritePreHandle(ModBusBaseParam, &ReadAddr);    
    //寄存器地址无效
    if(!((ReadAddr >= MUL_REG_REGION1_BEGIN)&&((ReadAddr + RegNum) <= (MUL_REG_REGION1_END + 1))
       &&(ModBusBaseParam->ModBus_TX_RX.Receive_Buf[6] == (RegNum * 2))))
    {
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[1] |= MB_REQ_FAILURE;
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[2] = MB_ADDR_EXCEPTION;
        ModBusBaseParam->ModBus_TX_RX.Send_Len = 3;
        return;
    }

    //内部ROM访问禁止
    if(FLASH_WR_ENABLE != ModBusBaseParam->InRomWrEn)
    {
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[1] |= MB_REQ_FAILURE;
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[2] = MB_DEVC_EXCEPTION;
        ModBusBaseParam->ModBus_TX_RX.Send_Len = 3;
        return;
    }

    Index = 0;

    for(Nr = 0; Nr < RegNum; Nr++)
    {
        u8array[0] = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[7 + Index];
        u8array[1] = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[8 + Index];
        u16temp = (uint16_t)(ModBusBaseParam->ModBus_TX_RX.Receive_Buf[7 + Index] << 8) 
            | ModBusBaseParam->ModBus_TX_RX.Receive_Buf[8 + Index];

        switch(ReadAddr)
        {
        case 0x0030:
            //设备地址
            if((0 < ModBusBaseParam->ModBus_TX_RX.Receive_Buf[8 + Index]) 
                && (0xF8 > ModBusBaseParam->ModBus_TX_RX.Receive_Buf[8 + Index]))
            {
                ModBusBaseParam->Device_Addr = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[8 + Index];
                InFlash_Write3T_MultiBytes(DEVICE_ADDR, &ModBusBaseParam->Device_Addr, 1);
            }
        break;
                
        case 0x0031:
            //波特率
            if(0x06 > ModBusBaseParam->ModBus_TX_RX.Receive_Buf[8 + Index])
            {
                ModBusBaseParam->BaudRate = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[8 + Index];
                InFlash_Write3T_MultiBytes(BAUDRATE, &ModBusBaseParam->BaudRate, 1);
                ModBusBaseParam->ModBus_CallBack = MB_USART_ReInit;
            }
        break;
                
        case 0x0032:
            //奇偶校验
            if(0x03 > ModBusBaseParam->ModBus_TX_RX.Receive_Buf[8 + Index])
            {
                ModBusBaseParam->Parity = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[8 + Index];
                InFlash_Write3T_MultiBytes(PARITY, &ModBusBaseParam->Parity, 1);
                ModBusBaseParam->ModBus_CallBack = MB_USART_ReInit;
            }
        break;
          
        case 0x0033:
            //空
        break;
          
        case 0x0034:
            //补偿使能
            if(2 > ModBusBaseParam->ModBus_TX_RX.Receive_Buf[8 + Index])
            {
                Device_Param->PCap_DataConvert->CompenEn = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[8 + Index];
                InFlash_Write3T_MultiBytes(COMPENSATE, &Device_Param->PCap_DataConvert->CompenEn, 1);
            }
        break;
          
        case 0x0035:
            //滤波系数
            if(0x0A > ModBusBaseParam->ModBus_TX_RX.Receive_Buf[8 + Index])
            {
                Device_Param->DataFilter->FilterFactor = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[8 + Index];
                InFlash_Write3T_MultiBytes(FILTER, &Device_Param->DataFilter->FilterFactor, 1);
                SwitchCurFilter(Device_Param->DataFilter->FilterFactor, Device_Param->DataFilter);
            }
        break;
          
        case 0x0036:
            //自动上传周期
            ModBusBaseParam->AutoUpload = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[8 + Index];
            InFlash_Write3T_MultiBytes(AUTO_UPLOAD, &ModBusBaseParam->AutoUpload, 1);
        break;
          
        case 0x0037:
            //电容修正系数K
            if(0 < u16temp)
            {
                Device_Param->PCap_DataConvert->Correct_K = (float)u16temp / 100.0;
                InFlash_Write3T_MultiBytes(CORRECT_K, (uint8_t const *)u8array, 2);
            }
        break;
          
        case 0x0038:
            //电容修正系数B
            Device_Param->PCap_DataConvert->Correct_B = (float)u16temp;
            InFlash_Write3T_MultiBytes(CORRECT_B, (uint8_t const *)u8array, 2);
        break;
          
        case 0x0039:
            //输出方式
            if(2 > ModBusBaseParam->ModBus_TX_RX.Receive_Buf[8 + Index])
            {
                ModBusBaseParam->Output_Mode = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[8 + Index];
                InFlash_Write3T_MultiBytes(OUTPUTMODE, &ModBusBaseParam->Output_Mode, 1);
            }
        break;
          
        case 0x003F:
            //量程
            if(u16temp > 0)
            {
                Device_Param->PCap_DataConvert->HeightRange = u16temp;
                InFlash_Write3T_MultiBytes(HEIGHTRANGE, (uint8_t const *)u8array, 2);
            }
        break;
          
        case 0x0040:
            //空
        break;
          
        case 0x0041:
            //空
        break;
          
        default:
            
        break;
        }     
            
        Index += 2;
        ReadAddr++;
    }
}

/**
* 名称       : ModbusFunc25()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : Modbus 25功能码消息帧处理
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
*              arg:用户自定义的参数,例如设备参数
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 
* 修改内容   :
*/
/*void ModbusFunc25(ModBusBaseParam_TypeDef *ModBusBaseParam, void *arg)
{
    //寄存器地址
    uint16_t ReadAddr;
    //数据内容
    uint16_t DataBuf;
    //设备参数
    ModBus_Device_Param *Device_Param;
    static uint8_t CapDA_ClibFlag = CALIB_CLEAR;    
    static uint8_t CapAD_ClibFlag = CALIB_CLEAR;
    static uint8_t CapAD_ClibEn = ADCLIB_DISABLE;
    static uint8_t TempDA_ClibFlag = CALIB_CLEAR;
    static uint8_t TempDA_ClibEn = DAOUTCLIB_DISABLE;
        
    Device_Param = (ModBus_Device_Param *)arg;

    //写预处理
    ModBus_WritePreHandle(ModBusBaseParam, &ReadAddr);
    //寄存器地址无效
    if(!((ReadAddr < 0x000A)&&(FLASH_WR_ENABLE == ModBusBaseParam->InRomWrEn)))
    {
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[1] |= MB_REQ_FAILURE;
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[2] = MB_ADDR_EXCEPTION;
        ModBusBaseParam->ModBus_TX_RX.Send_Len = 3;
        return;
    }
    //寄存器内容高字节
    DataBuf = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[4];
    DataBuf <<= 8;
    //寄存器内容低字节
    DataBuf |= ModBusBaseParam->ModBus_TX_RX.Receive_Buf[5];

    //寄存内容无效
    if((0x0000 != DataBuf)&&(0xFF00 != DataBuf))
    {
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[1] |= MB_REQ_FAILURE;
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[2] = MB_VALU_EXCEPTION;
        ModBusBaseParam->ModBus_TX_RX.Send_Len = 3;
        return;
    }

    switch(ReadAddr)
    {
    case 0x0000:
    //空
    break;

    case 0x0001:
        //电容DA标定上下刻度线
        if(DAOUTCLIB_ENABLE == CapDA_ClibEn)
        {
            if(0xFF00 == DataBuf)
            {
                CapDA_ClibFlag |= CALIB_CAPDAHIH_FLAG;
                Calib_CapDAHigh = Device_Param->PCap_DataConvert_Out->PCapDA_ResultValue;
            }
            else
            {
                CapDA_ClibFlag |= CALIB_CAPDALOW_FLAG;
                Calib_CapDALow = Device_Param->PCap_DataConvert_Out->PCapDA_ResultValue;
            }
        }
    break;

    case 0x0002:
        //电容DA标定使能
        if(0xFF00 == DataBuf)
        {
            CapDA_ClibEn = DAOUTCLIB_ENABLE;
        }
        else if(0x0000 == DataBuf)
        {
            CapDA_ClibEn = DAOUTCLIB_DISABLE;
        }

        if(DAOUTCLIB_DISABLE == CapDA_ClibEn && CapDA_ClibFlag == CALIB_CAPDAEOC_FLAG)
        {
            MB_CapDAOut_Calibration(arg);
            CapDA_ClibFlag = CALIB_CLEAR;
        }
    break;

    case 0x0003:
        //电容DA标定零点满量程
        if(DAOUTCLIB_ENABLE == CapDA_ClibEn)
        {
            if(0xFF00 == DataBuf)
            {
                CapDA_ClibFlag |= CALIB_CAPDAMAX_FLAG;
                Calib_CapDAMax = Device_Param->PCap_DataConvert_Out->PCapDA_ResultValue;
            }
            else
            {
                CapDA_ClibFlag |= CALIB_CAPDAMIN_FLAG;
                Calib_CapDAMin = Device_Param->PCap_DataConvert_Out->PCapDA_ResultValue;
            }
        }
    break;

    case 0x0004:
        //温度标定使能
        if(0xFF00 == DataBuf)
        {
            TempDA_ClibEn = DAOUTCLIB_ENABLE;
        }
        else
        {
            TempDA_ClibEn = DAOUTCLIB_DISABLE;
        }

        if((DAOUTCLIB_DISABLE == TempDA_ClibEn) && (CALIB_TEMPDAEOC_FLAG == TempDA_ClibFlag))
        {
            MB_TempDAOut_Calibration(arg);
            TempDA_ClibFlag = CALIB_CLEAR;
        }
    break;

    case 0x0005:
        //温度标定
        if(DAOUTCLIB_ENABLE == TempDA_ClibEn)
        {
            if(0xFF00 == DataBuf)
            {
                TempDA_ClibFlag |= CALIB_TEMPDAMAX_FLAG;
                Calib_TempDAMax = Device_Param->ADC_TemperOut->TemperInAirAD;
            }
            else
            {
                TempDA_ClibFlag |= CALIB_TEMPDAMIN_FLAG;
                Calib_TempDAMin = Device_Param->ADC_TemperOut->TemperInAirAD;
            }
        }
    break;

    case 0x0007:
        //电容AD标定使能
        if(0xFF00 == DataBuf)
        {
            CapAD_ClibFlag = CALIB_CLEAR;
            CapAD_ClibEn = ADCLIB_ENABLE;
        }
        else
        {
            CapAD_ClibEn = ADCLIB_DISABLE;
        }

        if((ADCLIB_DISABLE == CapAD_ClibEn) && (CALIB_CAPADEOC_FLAG == CapAD_ClibFlag))
        {
            MB_CapAD_Calibration(arg);
            CapAD_ClibFlag = CALIB_CLEAR;
        }
    break;

    case 0x0008:
        //电容AD标定上下刻度线
        if(ADCLIB_ENABLE == CapAD_ClibEn)
        {
            if(0xFF00 == DataBuf)
            {
                Calib_ADHigh = Device_Param->PCap_DataConvert_Out->LiquidHeightAD;
                CapAD_ClibFlag |= CALIB_CAPADHIH_FLAG;
            }
            else
            {
                Calib_ADLow = Device_Param->PCap_DataConvert_Out->LiquidHeightAD;
                CapAD_ClibFlag |= CALIB_CAPADLOW_FLAG;
            }
        }
    break;

    case 0x0009:
        //电容AD标定零点满量程
        if(ADCLIB_ENABLE == CapAD_ClibEn)
        {
            if(0xFF00 == DataBuf)
            {
                Calib_ADMax = Device_Param->PCap_DataConvert_Out->LiquidHeightAD;
                CapAD_ClibFlag |= CALIB_CAPADMAX_FLAG;
            }
            else
            {
                Calib_ADMin = Device_Param->PCap_DataConvert_Out->LiquidHeightAD;
                CapAD_ClibFlag |= CALIB_CAPADMIN_FLAG;
            }
        }
    break;

    default:
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[1] |= MB_REQ_FAILURE;
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[2] = MB_VALU_EXCEPTION;
        ModBusBaseParam->ModBus_TX_RX.Send_Len = 3;
    break;
    }
}*/

/**
* 名称       : ModbusFunc26()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : Modbus 26功能码消息帧处理
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
*              arg:用户自定义的参数,例如设备参数
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 
* 修改内容   :
*/
void ModbusFunc26(ModBusBaseParam_TypeDef *ModBusBaseParam, void *arg)
{
    uint8_t i;
    //浮点数据
    float fBuf;
    uint16_t Nr;
    //寄存器地址
    uint16_t ReadAddr;
    //寄存器数量
    uint16_t RegNum;
    //寄存器数据内容
    uint32_t DataBuf;
    //设备参数
    ModBus_Device_Param *Device_Param;
        
    Device_Param = (ModBus_Device_Param *)arg;
    //读预处理
    ModBus_ReadPreHandle(ModBusBaseParam, &ReadAddr, &RegNum);
    //寄存器访问地址无效
    if(!((((DEF_MUL_REG_REGION1_BEGIN <= ReadAddr) && (DEF_MUL_REG_REGION1_END >= ReadAddr) 
        && ((ReadAddr + RegNum) <= (DEF_MUL_REG_REGION1_END + 2))))
     && (0x00 != RegNum) && (0 == (RegNum & 0x01)) && (0 == (ReadAddr & 0x01))))
    {
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[1] |= MB_REQ_FAILURE;
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[2] = MB_ADDR_EXCEPTION;
        ModBusBaseParam->ModBus_TX_RX.Send_Len = 3;
        return;
    }

    for(Nr = 0; Nr < RegNum; Nr += 2, ReadAddr += 2)
    {
        switch(ReadAddr)
        {
        case 0x0080:
            fBuf = (float)Device_Param->PCap_DataConvert->CapMax;                                       //电容满量程
            DataBuf = *(uint32_t *)&fBuf;
        break;

        case 0x0082:
            fBuf = (float)Device_Param->PCap_DataConvert->CapMin;                                       //电容零点
            DataBuf = *(uint32_t *)&fBuf;
        break;

        case 0x0084:     
            fBuf = (float)Device_Param->PCap_DataConvert->CapDAMax;                                     //DA满度
            DataBuf = *(uint32_t *)&fBuf;
        break;
          
        case 0x0086:
            fBuf = (float)Device_Param->PCap_DataConvert->CapDAHigh;                                     //DA上刻度
            DataBuf = *(uint32_t *)&fBuf;
        break;
          
        case 0x0088:
            fBuf = (float)Device_Param->PCap_DataConvert->CapDALow;                                     //DA下刻度
            DataBuf = *(uint32_t *)&fBuf;
        break;  
          
        case 0x008A:
            fBuf = (float)Device_Param->PCap_DataConvert->CapDAMin;                                     //DA零点
            DataBuf = *(uint32_t *)&fBuf;
        break; 
          
        case 0x008C: 
            fBuf = (float)Device_Param->PCap_DataConvert->CapADMax;                                     //AD满度
            DataBuf = *(uint32_t *)&fBuf;
        break;

        case 0x008E:
            fBuf = (float)Device_Param->PCap_DataConvert->CapADHigh;                                     //AD上刻度
            DataBuf = *(uint32_t *)&fBuf;
        break;       
            
        case 0x0090:
            fBuf = (float)Device_Param->PCap_DataConvert->CapADLow;                                     //AD下刻度
            DataBuf = *(uint32_t *)&fBuf;
        break;  
          
        case 0x0092:
            fBuf = (float)Device_Param->PCap_DataConvert->CapADMin;                                      //AD零点
            DataBuf = *(uint32_t *)&fBuf;
        break;       
          
        case 0x0094:  
            
        break;
          
        case 0x0096:  
            
        break;

        case 0x0098:  
            
        break;

        case 0x009A:  
            
        break;      
          
        case 0x009C:                                                                
            fBuf = Device_Param->ADC_TemperParam->Temper_K1;                                               //环境温度修正系数K1   
            DataBuf = *(uint32_t *)&fBuf;
        break;
          
        case 0x009E:
            fBuf = Device_Param->ADC_TemperParam->Temper_B1;                                               //环境温度修正系数B1
            DataBuf = *(uint32_t *)&fBuf;
        break;
          
        case 0x00A0:
            fBuf = Device_Param->ADC_TemperParam->Temper_K2;                                               //液体温度修正系数K2
            DataBuf = *(uint32_t *)&fBuf;
        break;
          
        case 0x00A2:
            fBuf = Device_Param->ADC_TemperParam->Temper_B2;                                               //液体温度修正系数B2
            DataBuf = *(uint32_t *)&fBuf;
        break;
          
        default:
            
        break;
        }

        for(i = 4; i > 0; i--)
        {
            ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] 
                = (uint8_t)(DataBuf >> ((i - 1)*8));
        }
    }
}

/**
* 名称       : ModbusFunc27()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : Modbus 27功能码消息帧处理
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
*              arg:用户自定义的参数,例如设备参数
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 
* 修改内容   :
*/
void ModbusFunc27(ModBusBaseParam_TypeDef *ModBusBaseParam, void *arg)
{
    //浮点数据
    float fbuf;
    uint16_t Nr;
    //索引
    uint16_t index;
    //寄存器地址
    uint16_t ReadAddr;
    //寄存器数量
    uint16_t RegNum;
    //设备参数
    ModBus_Device_Param *Device_Param;
    uint8_t Cur_Param[4];
            
    //uint16_t DA_BakHih, DA_BakLow;
          
    Device_Param = (ModBus_Device_Param *)arg;    
    //写预处理
    ModBus_WritePreHandle(ModBusBaseParam, &ReadAddr);  
    //内部ROM未使能访问失败
    if(FLASH_WR_ENABLE != ModBusBaseParam->InRomWrEn)
    {
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[1] |= MB_REQ_FAILURE;
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[2] = MB_DEVC_EXCEPTION;
        ModBusBaseParam->ModBus_TX_RX.Send_Len = 3;
        return;
    }
    //寄存器数量高字节
    RegNum = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[4];
    RegNum <<= 8;
    //寄存器数量低字节
    RegNum |= ModBusBaseParam->ModBus_TX_RX.Receive_Buf[5];
    //寄存器访问地址无效
    if(!((((DEF_MUL_REG_REGION1_BEGIN <= ReadAddr) && (DEF_MUL_REG_REGION1_END >= ReadAddr) 
        && ((ReadAddr + RegNum) <= (DEF_MUL_REG_REGION1_END + 2))))
        && (0x00 != RegNum) && (0 == (RegNum & 0x01)) && (0 == (ReadAddr & 0x01))))
    {
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[1] |= MB_REQ_FAILURE;
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[2] = MB_ADDR_EXCEPTION;
        ModBusBaseParam->ModBus_TX_RX.Send_Len = 3;
        return;
    }
    index = 0;
    for(Nr = 0; Nr < RegNum; Nr += 2, ReadAddr += 2)
    {
        switch(ReadAddr)
        {
        case 0x0080:
            fbuf = HexToFloat(&ModBusBaseParam->ModBus_TX_RX.Receive_Buf[7 + index]);
            Device_Param->PCap_DataConvert->CapMax = (uint32_t)fbuf;
            Device_Param->DataFilter->InputRangeMax = (uint32_t)fbuf;
            Cur_Param[0] = (uint8_t)(Device_Param->PCap_DataConvert->CapMax >> 24);
            Cur_Param[1] = (uint8_t)(Device_Param->PCap_DataConvert->CapMax >> 16);;       
            Cur_Param[2] = (uint8_t)(Device_Param->PCap_DataConvert->CapMax >> 8);
            Cur_Param[3] = (uint8_t)Device_Param->PCap_DataConvert->CapMax;   
            InFlash_Write3T_MultiBytes(CAPMAX, Cur_Param, 4);
        break;
          
        case 0x0082:
            fbuf = HexToFloat(&ModBusBaseParam->ModBus_TX_RX.Receive_Buf[7 + index]);
            Device_Param->PCap_DataConvert->CapMin = (uint32_t)fbuf;
            Device_Param->DataFilter->InputRangeMin = (uint32_t)fbuf;
            Cur_Param[0] = (uint8_t)(Device_Param->PCap_DataConvert->CapMin >> 24);
            Cur_Param[1] = (uint8_t)(Device_Param->PCap_DataConvert->CapMin >> 16);;       
            Cur_Param[2] = (uint8_t)(Device_Param->PCap_DataConvert->CapMin >> 8);
            Cur_Param[3] = (uint8_t)Device_Param->PCap_DataConvert->CapMin;  
            InFlash_Write3T_MultiBytes(CAPMIN, Cur_Param, 4);
        break;

        case 0x0084:
            fbuf = HexToFloat(&ModBusBaseParam->ModBus_TX_RX.Receive_Buf[7 + index]);
            if(fbuf <= PCAP_DAC_MAX_VALUE && fbuf >= PCAP_DAC_MIN_VALUE)
            {
                Device_Param->PCap_DataConvert->CapDAMax = (uint16_t)fbuf;
                Cur_Param[0] = (uint8_t)(Device_Param->PCap_DataConvert->CapDAMax >> 8);
                Cur_Param[1] = (uint8_t)Device_Param->PCap_DataConvert->CapDAMax;               
                InFlash_Write3T_MultiBytes(CAPDAMAX, Cur_Param, 2);
            }            
        break;
               
        case 0x0086:
            fbuf = HexToFloat(&ModBusBaseParam->ModBus_TX_RX.Receive_Buf[7 + index]);
            if(fbuf <= PCAP_DAC_MAX_VALUE && fbuf >= PCAP_DAC_MIN_VALUE)
            {
                Device_Param->PCap_DataConvert->CapDAHigh = (uint16_t)fbuf;
                Cur_Param[0] = (uint8_t)(Device_Param->PCap_DataConvert->CapDAHigh >> 8);
                Cur_Param[1] = (uint8_t)Device_Param->PCap_DataConvert->CapDAHigh;               
                InFlash_Write3T_MultiBytes(CAPDAHIGH, Cur_Param, 2); 
            }
        break;
          
        case 0x0088:
            fbuf = HexToFloat(&ModBusBaseParam->ModBus_TX_RX.Receive_Buf[7 + index]);
            if(fbuf <= PCAP_DAC_MAX_VALUE && fbuf >= PCAP_DAC_MIN_VALUE)
            {
                Device_Param->PCap_DataConvert->CapDALow = (uint16_t)fbuf;
                Cur_Param[0] = (uint8_t)(Device_Param->PCap_DataConvert->CapDALow >> 8);
                Cur_Param[1] = (uint8_t)Device_Param->PCap_DataConvert->CapDALow;               
                InFlash_Write3T_MultiBytes(CAPDALOW, Cur_Param, 2); 
            }      
        break;
              
        case 0x008A:
            fbuf = HexToFloat(&ModBusBaseParam->ModBus_TX_RX.Receive_Buf[7 + index]);
            if(fbuf <= PCAP_DAC_MAX_VALUE && fbuf >= PCAP_DAC_MIN_VALUE)
            {
                Device_Param->PCap_DataConvert->CapDAMin = (uint16_t)fbuf;
                Cur_Param[0] = (uint8_t)(Device_Param->PCap_DataConvert->CapDAMin >> 8);
                Cur_Param[1] = (uint8_t)Device_Param->PCap_DataConvert->CapDAMin;               
                InFlash_Write3T_MultiBytes(CAPDAMIN, Cur_Param, 2);
            }     
        break; 
          
        case 0x008C:
            fbuf = HexToFloat(&ModBusBaseParam->ModBus_TX_RX.Receive_Buf[7 + index]);
            if(fbuf <= PCAP_ADC_MAX_VALUE && fbuf >= PCAP_ADC_MIN_VALUE)
            {
                Device_Param->PCap_DataConvert->CapADMax = (uint16_t)fbuf;
                Cur_Param[0] = (uint8_t)(Device_Param->PCap_DataConvert->CapADMax >> 8);
                Cur_Param[1] = (uint8_t)Device_Param->PCap_DataConvert->CapADMax;               
                InFlash_Write3T_MultiBytes(CAPADMAX, Cur_Param, 2);      
            }            
        break; 
               
        case 0x008E:
            fbuf = HexToFloat(&ModBusBaseParam->ModBus_TX_RX.Receive_Buf[7 + index]);
            if(fbuf <= PCAP_ADC_MAX_VALUE && fbuf >= PCAP_ADC_MIN_VALUE)
            {
                Device_Param->PCap_DataConvert->CapADHigh = (uint16_t)fbuf;
                Cur_Param[0] = (uint8_t)(Device_Param->PCap_DataConvert->CapADHigh >> 8);
                Cur_Param[1] = (uint8_t)Device_Param->PCap_DataConvert->CapADHigh;               
                InFlash_Write3T_MultiBytes(CAPADHIGH, Cur_Param, 2);       
            }                
        break; 
          
        case 0x0090:
            fbuf = HexToFloat(&ModBusBaseParam->ModBus_TX_RX.Receive_Buf[7 + index]);
            if(fbuf <= PCAP_ADC_MAX_VALUE && fbuf >= PCAP_ADC_MIN_VALUE)
            {
                Device_Param->PCap_DataConvert->CapADLow = (uint16_t)fbuf;
                Cur_Param[0] = (uint8_t)(Device_Param->PCap_DataConvert->CapADLow >> 8);
                Cur_Param[1] = (uint8_t)Device_Param->PCap_DataConvert->CapADLow;               
                InFlash_Write3T_MultiBytes(CAPADLOW, Cur_Param, 2);           
            }
        break; 
               
        case 0x0092:
            fbuf = HexToFloat(&ModBusBaseParam->ModBus_TX_RX.Receive_Buf[7 + index]);
            if(fbuf <= PCAP_ADC_MAX_VALUE && fbuf >= PCAP_ADC_MIN_VALUE)
            {
                Device_Param->PCap_DataConvert->CapADMin = (uint16_t)fbuf;
                Cur_Param[0] = (uint8_t)(Device_Param->PCap_DataConvert->CapADMin >> 8);
                Cur_Param[1] = (uint8_t)Device_Param->PCap_DataConvert->CapADMin;               
                InFlash_Write3T_MultiBytes(CAPADMIN, Cur_Param, 2);   
            }            
        break;       

        default:
            
        break;
        }
        index += 4;
    }
}

/**
* 名称       : ModbusFunc2A()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : Modbus 2A功能码消息帧处理
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 
* 修改内容   :
*/
void ModbusFunc2A(ModBusBaseParam_TypeDef *ModBusBaseParam)
{
    uint8_t i;
    uint8_t j;
    uint8_t objlen;
    uint8_t RegNum;
    uint16_t ReadAddr;

    RegNum = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[5];
    //写预处理
    ModBus_WritePreHandle(ModBusBaseParam, &ReadAddr);    
    if(!((ReadAddr >= MUL_VERSION_INF_BEGIN)
        &&(ReadAddr <= MUL_VERSION_INF_END)
        &&((ReadAddr + RegNum) <= (MUL_VERSION_INF_END + 1))
        &&(0 != RegNum)&&(0x00 == ModBusBaseParam->ModBus_TX_RX.Receive_Buf[4])))
    {
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[1] |= MB_REQ_FAILURE;
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[2] = MB_ADDR_EXCEPTION;
        ModBusBaseParam->ModBus_TX_RX.Send_Len = 3;
        return;
    }

    j = 6;
    for(i = 0; i < RegNum; i++)
    {
        switch(ReadAddr)
        {
        //机构名称
        case 0x00E0:
            objlen = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[j] + 1;
            if(0x2F < objlen)
            {
                j += objlen;
                break;
            }
            InFlash_Write_MultiBytes(ORGANIZATION, 
                (uint8_t const *)&ModBusBaseParam->ModBus_TX_RX.Receive_Buf[j], objlen);                                    
            j += objlen;
        break;
        //产品代号    
        case 0x00E1:
            objlen = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[j] + 1;
            if(0x2F < objlen)
            {
                j += objlen;
                break;
            }
            InFlash_Write_MultiBytes(PRODUCTION, 
                (uint8_t const *)&ModBusBaseParam->ModBus_TX_RX.Receive_Buf[j], objlen);                               
            j += objlen;
        break;
        //硬件版本    
        case 0x00E2:
            objlen = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[j] + 1;
            if(0x2F < objlen)
            {
                j += objlen;
                break;
            }
            InFlash_Write_MultiBytes(HARDWAREVER, 
                (uint8_t const *)&ModBusBaseParam->ModBus_TX_RX.Receive_Buf[j], objlen);                                  
            j += objlen;
        break;
        //软件版本    
        case 0x00E3:
            objlen = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[j] + 1;
            //                                                                                                    
            j += objlen;
        break;
        //设备ID    
        case 0x00E4:
            objlen = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[j] + 1;
            if(0x2F < objlen)
            {
                j += objlen;
                break;
            }
            InFlash_Write_MultiBytes(DEVICENUM, 
                (uint8_t const *)&ModBusBaseParam->ModBus_TX_RX.Receive_Buf[j], objlen);                                     
            j += objlen;
        break;
        //客户编码    
        case 0x00E5:
            objlen = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[j] + 1;
            if(0x2F < objlen)
            {
                j += objlen;
                break;
            }
            InFlash_Write_MultiBytes(CUSTOMER, 
                (uint8_t const *)&ModBusBaseParam->ModBus_TX_RX.Receive_Buf[j], objlen);                               
            j += objlen;
        break;
            
        default:
            
        break;
        }
        ReadAddr++;
    }
}

/**
* 名称       : ModbusFunc2B()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : Modbus 2B功能码消息帧处理
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 
* 修改内容   :
*/
void ModbusFunc2B(ModBusBaseParam_TypeDef *ModBusBaseParam)
{
    uint8_t i;
    uint8_t objlen;
    uint16_t RegNum;
    uint16_t ReadAddr;

    //读预处理
    ModBus_ReadPreHandle(ModBusBaseParam, &ReadAddr, &RegNum);
    ModBusBaseParam->ModBus_TX_RX.Send_Buf[2] = RegNum >> 8;
    ModBusBaseParam->ModBus_TX_RX.Send_Buf[3] = RegNum & 0xFF;
    
    if(!((ReadAddr >= MUL_VERSION_INF_BEGIN)
        &&(ReadAddr <= MUL_VERSION_INF_END)
        &&((ReadAddr + RegNum) <= (MUL_VERSION_INF_END + 1))
        &&(0 != RegNum)&&(0x00 == ModBusBaseParam->ModBus_TX_RX.Receive_Buf[4])))
    {
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[1] |= MB_REQ_FAILURE;
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[2] = MB_ADDR_EXCEPTION;
        ModBusBaseParam->ModBus_TX_RX.Send_Len = 3;
        return;
    }
    memcpy(&ModBusBaseParam->ModBus_TX_RX.Send_Buf[4], &ModBusBaseParam->ModBus_TX_RX.Receive_Buf[2], 2);
    ModBusBaseParam->ModBus_TX_RX.Send_Len = 6;
    for(i = 0; i < RegNum; i++)
    {
        switch(ReadAddr)
        {
        //机构名称  
        case 0x00E0:
            objlen = InFlash_Read_OneByte(ORGANIZATION);                                                       
            ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] = objlen;
            if((objlen > (SEND_SIZE / 3))||(0 == objlen))
            {
                ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len - 1] = 1;
                ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] = 0;
                break;
            }
            InFlash_Read_MultiBytes((ORGANIZATION + 1), 
                &ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len], objlen);
            ModBusBaseParam->ModBus_TX_RX.Send_Len += objlen;
        break;
        //产品代号  
        case 0x00E1:
            objlen = InFlash_Read_OneByte(PRODUCTION);                                                  
            ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] = objlen;
            if((objlen > (SEND_SIZE / 3))||(0 == objlen))
            {
                ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len - 1] = 1;
                ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] = 0;
                break;
            }
            InFlash_Read_MultiBytes((PRODUCTION + 1), 
                &ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len], objlen);
            ModBusBaseParam->ModBus_TX_RX.Send_Len += objlen;
        break;
        //硬件版本  
        case 0x00E2:
            objlen = InFlash_Read_OneByte(HARDWAREVER);                                                     
            ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] = objlen;
            if((objlen > (SEND_SIZE / 3))||(0 == objlen))
            {
                ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len - 1] = 1;
                ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] = 0;
                break;
            }
            InFlash_Read_MultiBytes((HARDWAREVER + 1), 
                &ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len], objlen);
            ModBusBaseParam->ModBus_TX_RX.Send_Len += objlen;
        break;
        //软件版本 
        case 0x00E3:                                                    
            ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] = SensorSoftVersion[0];

            for(objlen = 0; objlen < 7; objlen++)                                                        
            {
                ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] 
                    = SensorSoftVersion[objlen + 1];
            }
        break;
        //设备ID  
        case 0x00E4:
            objlen = InFlash_Read_OneByte(DEVICENUM);                                                        
            ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] = objlen;
            if((objlen > (SEND_SIZE / 3))||(0 == objlen))
            {
                ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len - 1] = 1;
                ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] = 0;
                break;
            }
            InFlash_Read_MultiBytes((DEVICENUM + 1), 
                &ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len], objlen);
            ModBusBaseParam->ModBus_TX_RX.Send_Len += objlen;
        break;
        //客户编码  
        case 0x00E5:
            objlen = InFlash_Read_OneByte(CUSTOMER);                                                  
            ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] = objlen;
            if((objlen > (SEND_SIZE / 3))||(0 == objlen))
            {
                ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len - 1] = 1;
                ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] = 0;
                break;
            }
            InFlash_Read_MultiBytes((CUSTOMER + 1), 
                &ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len], objlen);
            ModBusBaseParam->ModBus_TX_RX.Send_Len += objlen;
        break;

        default:
            ModBusBaseParam->ModBus_TX_RX.Send_Buf[1] |= MB_REQ_FAILURE;
            ModBusBaseParam->ModBus_TX_RX.Send_Buf[2] = MB_ADDR_EXCEPTION;
            ModBusBaseParam->ModBus_TX_RX.Send_Len = 3;
        break;
        }
        ReadAddr++;
    }
}

/**
* 名称       : ModbusFunc41()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : Modbus 41功能码消息帧处理
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 
* 修改内容   :
*/
void ModbusFunc41(ModBusBaseParam_TypeDef *ModBusBaseParam)
{
    uint16_t temp[2];
    
    temp[0] = ((uint16_t)ModBusBaseParam->ModBus_TX_RX.Receive_Buf[2] << 8) 
                | ModBusBaseParam->ModBus_TX_RX.Receive_Buf[3];
    temp[1] = ((uint16_t)ModBusBaseParam->ModBus_TX_RX.Receive_Buf[4] << 8) 
                | ModBusBaseParam->ModBus_TX_RX.Receive_Buf[5];
    if(temp[0] != 0x0001 || temp[1] != 0x0000)
    {
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[0] = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[0];
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[1] = 0x41 | MB_REQ_FAILURE;
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[2] = MB_VALU_EXCEPTION;
        ModBusBaseParam->ModBus_TX_RX.Send_Len = 3;
    }
    else
    {
        memcpy(ModBusBaseParam->ModBus_TX_RX.Send_Buf, ModBusBaseParam->ModBus_TX_RX.Receive_Buf, 5);
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[5] = 0x01;
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[6] = 0x00;
        ModBusBaseParam->ModBus_TX_RX.Send_Len = 7;
        MDB_Snd_Msg_RTU(ModBusBaseParam, 
            ModBusBaseParam->ModBus_TX_RX.Send_Buf, ModBusBaseParam->ModBus_TX_RX.Send_Len);
        HAL_Delay(10);
        HAL_NVIC_SystemReset();
    }
}

/**
* 名称       : ModbusAutoUpload()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : Modbus 消息帧自动上传处理
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
*              arg:用户自定义的参数,例如设备参数
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 
* 修改内容   :
*/
void ModbusAutoUpload(ModBusBaseParam_TypeDef *ModBusBaseParam, void *arg)
{
    float fbuf;
    uint8_t Nr;
    uint32_t DataBuf;
    //设备参数
    ModBus_Device_Param *Device_Param;
        
    Device_Param = (ModBus_Device_Param *)arg;

    ModBusBaseParam->ModBus_TX_RX.Send_Buf[0] = ModBusBaseParam->Device_Addr;
    ModBusBaseParam->ModBus_TX_RX.Send_Buf[1] = 0x04;
    ModBusBaseParam->ModBus_TX_RX.Send_Buf[2] = 0x08;

    ModBusBaseParam->ModBus_TX_RX.Send_Len = 3;
    fbuf = (float)Device_Param->PCap_DataConvert_Out->LiquidHeightAD;
    DataBuf = *(unsigned long *)&fbuf;
    for(Nr = 4; Nr > 0; Nr--)
    {
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] 
            = (uint8_t)(DataBuf >> ((Nr - 1)*8));
    }

    ModBusBaseParam->ModBus_TX_RX.Send_Len = 7;
    DataBuf = *(unsigned long *)&Device_Param->ADC_TemperOut->TemperInAir;
    for(Nr = 4; Nr > 0; Nr--)
    {
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[ModBusBaseParam->ModBus_TX_RX.Send_Len++] 
            = (uint8_t)(DataBuf >> ((Nr - 1)*8));
    }
    MDB_Snd_Msg_RTU_NoLimit(ModBusBaseParam, 
        ModBusBaseParam->ModBus_TX_RX.Send_Buf, ModBusBaseParam->ModBus_TX_RX.Send_Len);
}


/**
* 名称       : MB_Cap_Calibration()
* 创建日期   : 2020-05-06
* 作者       : 庄明群
* 功能       : 电容标定功能
* 输入参数   : arg:用户自定义的参数,这里为设备参数
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 :
* 修改内容   :
*/
void MB_Cap_Calibration(void *arg)
{
    uint8_t Cur_Param[8];
    //设备参数
    ModBus_Device_Param *Device_Param;
        
    Device_Param = (ModBus_Device_Param *)arg;
    
    if(Calib_CapMin < Calib_CapMax)
    {
        //电容零点存入缓存
        Cur_Param[0] = (uint8_t)(Calib_CapMin >> 24);
        Cur_Param[1] = (uint8_t)(Calib_CapMin >> 16);
        Cur_Param[2] = (uint8_t)(Calib_CapMin >> 8);
        Cur_Param[3] = (uint8_t)Calib_CapMin;
        //电容满点存入缓存
        Cur_Param[4] = (uint8_t)(Calib_CapMax >> 24);
        Cur_Param[5] = (uint8_t)(Calib_CapMax >> 16);
        Cur_Param[6] = (uint8_t)(Calib_CapMax >> 8);
        Cur_Param[7] = (uint8_t)Calib_CapMax;
        //电容零点
        Device_Param->PCap_DataConvert->CapMin = Calib_CapMin;
        //电容满点
        Device_Param->PCap_DataConvert->CapMax = Calib_CapMax;
        
        Device_Param->DataFilter->InputRangeMin = Calib_CapMin;
        Device_Param->DataFilter->InputRangeMax = Calib_CapMax;
        //电容标定数据写入内部Flash
        InFlash_Write3T_MultiBytes(CAPMIN, Cur_Param, 8);
        //如果电容满点备份值是缺省值就写入标定值
        if(0xFFFFFFFF == Device_Param->PCap_DataConvert->CapMaxBak)
        {
            //电容备份数据写入内部Flash
            InFlash_Write3T_MultiBytes(CAPMINBAK, Cur_Param, 8);
            //电容零点备份
            Device_Param->PCap_DataConvert->CapMinBak = Calib_CapMin;
            //电容满点备份
            Device_Param->PCap_DataConvert->CapMaxBak = Calib_CapMax;
        }
    }
    //Device_Param->PCap_DataConvert->CapRange = Device_Param->PCap_DataConvert->CapMax - Device_Param->PCap_DataConvert->CapMin;
    //设置液位波动阈值为量程的百分之三
    //Device_Param->PCap_DataConvert->CapWaveLimit = (uint32_t)(Device_Param->PCap_DataConvert->CapRange * 0.03);
}

/**
* 名称       : MB_CapAD_Calibration()
* 创建日期   : 2020-05-06
* 作者       : 杨春林
* 功能       : 电容AD值标定功能
* 输入参数   : arg:用户自定义的参数,这里为设备参数
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 标定值不能超过65535
* 修改内容   :
*/
void MB_CapAD_Calibration(void *arg)
{
    uint8_t Cur_Param[8];
    //设备参数
    ModBus_Device_Param *Device_Param;
        
    Device_Param = (ModBus_Device_Param *)arg;
    //如果电容AD标定值有效就写入内部Flash
    if((Calib_ADMin < Calib_ADLow) && (Calib_ADLow < Calib_ADHigh) && (Calib_ADHigh < Calib_ADMax))
    {
        Cur_Param[0] = (uint8_t)(Calib_ADMin >> 8);
        Cur_Param[1] = (uint8_t)Calib_ADMin;
        Cur_Param[2] = (uint8_t)(Calib_ADLow >> 8);
        Cur_Param[3] = (uint8_t)Calib_ADLow;
        Cur_Param[4] = (uint8_t)(Calib_ADHigh >> 8);
        Cur_Param[5] = (uint8_t)Calib_ADHigh;
        Cur_Param[6] = (uint8_t)(Calib_ADMax >> 8);
        Cur_Param[7] = (uint8_t)Calib_ADMax;

        Device_Param->PCap_DataConvert->CapADMin = Calib_ADMin;
        Device_Param->PCap_DataConvert->CapADLow = Calib_ADLow;
        Device_Param->PCap_DataConvert->CapADHigh = Calib_ADHigh;
        Device_Param->PCap_DataConvert->CapADMax = Calib_ADMax;
        InFlash_Write3T_MultiBytes(CAPADMIN, Cur_Param, 8);
    }
}

/**
* 名称       : MB_CapDAOut_Calibration()
* 创建日期   : 2020-05-06
* 作者       : 杨春林
* 功能       : 电容DA值标定功能
* 输入参数   : arg:用户自定义的参数,这里为设备参数
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 标定值不能超过4095
* 修改内容   :
*/
void MB_CapDAOut_Calibration(void *arg)
{
    uint8_t Cur_Param[8];
    //设备参数
    ModBus_Device_Param *Device_Param;
    
    Device_Param = (ModBus_Device_Param *)arg;
    if((Calib_CapDAMax > Calib_CapDAHigh) && (Calib_CapDAHigh > Calib_CapDALow) 
        && (Calib_CapDALow > Calib_CapDAMin))
    {                        
        Cur_Param[0] = (uint8_t)(Calib_CapDAMin >> 8);
        Cur_Param[1] = (uint8_t)Calib_CapDAMin;
        Cur_Param[2] = (uint8_t)(Calib_CapDALow >> 8);
        Cur_Param[3] = (uint8_t)Calib_CapDALow;
        Cur_Param[4] = (uint8_t)(Calib_CapDAHigh >> 8);
        Cur_Param[5] = (uint8_t)Calib_CapDAHigh;
        Cur_Param[6] = (uint8_t)(Calib_CapDAMax >> 8);
        Cur_Param[7] = (uint8_t)Calib_CapDAMax;
        InFlash_Write3T_MultiBytes(CAPDAMIN, Cur_Param, 8);
        
        Device_Param->PCap_DataConvert->CapDAMin = Calib_CapDAMin;
        Device_Param->PCap_DataConvert->CapDALow = Calib_CapDALow;
        Device_Param->PCap_DataConvert->CapDAHigh = Calib_CapDAHigh;
        Device_Param->PCap_DataConvert->CapDAMax = Calib_CapDAMax;
    }
}

/**
* 名称       : MB_TempDAOut_Calibration()
* 创建日期   : 2020-05-06
* 作者       : 杨春林
* 功能       : 温度DA值标定功能
* 输入参数   : arg:用户自定义的参数,这里为设备参数
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 标定值不能超过4095
* 修改内容   :
*/
void MB_TempDAOut_Calibration(void *arg)
{
    uint8_t Cur_Param[4];
    //设备参数
    ModBus_Device_Param *Device_Param;
    
    Device_Param = (ModBus_Device_Param *)arg;
    //如果温度DA标定值有效就写入内部Flash
    if((Calib_TempDAMax > Calib_TempDAMin) && (DAC_VALUE_MAX > Calib_TempDAMax) && (DAC_VALUE_MAX > Calib_TempDAMin))
    {
        Cur_Param[0] = (unsigned char)(Calib_TempDAMin >> 8);
        Cur_Param[1] = (unsigned char)Calib_TempDAMin;
        Cur_Param[2] = (unsigned char)(Calib_TempDAMax >> 8);
        Cur_Param[3] = (unsigned char)Calib_TempDAMax;
        InFlash_Write3T_MultiBytes(TEMPDAMIN, Cur_Param, 4);

        Device_Param->ADC_TemperParam->TempDAMin = Calib_TempDAMin;
        Device_Param->ADC_TemperParam->TempDAMax = Calib_TempDAMax;
        Device_Param->ADC_TemperParam->TempDARange = Device_Param->ADC_TemperParam->TempDAMax 
                                                - Device_Param->ADC_TemperParam->TempDAMin;
    }
}

/**
* 名称       : MB_USART_ReInit()
* 创建日期   : 2020-05-08
* 作者       : 杨春林
* 功能       : Modbus 串口重新初始化
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;  
* 输出参数   : 无
* 返回结果   : int整型值
* 注意和说明 : 本函数在ModBus响应上位机后,调用
* 修改内容   :
*/
static int MB_USART_ReInit(ModBusBaseParam_TypeDef *ModBusBaseParam)
{        
    Sensor_USART_Init(  ModBusBaseParam->BaudRate, 
                        ModBusBaseParam->Parity, 
                        ModBusBaseParam->ModBus_TX_RX.USART_Receive_Buf, 
                        ModBusBaseParam->ModBus_TX_RX.USART_Receive_Buf_Len);
    
    return OP_SUCCESS;
}

#endif
