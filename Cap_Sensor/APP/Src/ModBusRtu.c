/****************************************************************************************************************************************************************
** 版权:          2020-2030,深圳市信为科技发展有限公司
** 文件名:        ModbusRtu.c
** 作者:          杨春林
** 版本:          V1.0.0
** 日期:          2020-04-29
** 描述:          Modbus RTU 通信协议
** 功能:          读写参数报文处理、标定功能、采集数据交互
*****************************************************************************************************************************************************************
** 修改者:        No
** 版本:          No
** 修改内容:      No
** 日期:          No
****************************************************************************************************************************************************************/

#include "ModBus_Conf.h"
#include "ModBusRtu.h"


//CRC_16正规查询表
/* CRC低位字节值表*/
const uint8_t auchCRCLo[256] =
{
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
};

/* CRC高位字节值表*/
const uint8_t auchCRCHi[256] =
{
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
    0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
    0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
    0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
    0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
    0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
    0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
    0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
    0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
    0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
    0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
    0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
    0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
    0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
    0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
    0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
    0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
    0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
    0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
    0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
    0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
    0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};

static uint8_t USART_Receive_Buf[RECEIVE_SIZE];     //串口接收缓存
static uint8_t ModBus_Receive_Buf[RECEIVE_SIZE];    //ModBus接收缓存(ModBus解析处理时,使用这个缓存)
static uint8_t ModBus_Send_Buf[SEND_SIZE];          //ModBus发送缓存


#ifdef __IN_FLASH_APP_H
/**
* 名称       : ModBus_Init()
* 创建日期   : 2020-05-07
* 作者       : 杨春林
* 功能       : ModBus初始化
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 
* 修改内容   :
*/
void ModBus_Init(ModBusBaseParam_TypeDef *ModBusBaseParam)
{
    uint8_t Read_Data;
    
    InFlash_Read_MultiBytes(DEVICE_ADDR, &Read_Data, 1);
    ModBusBaseParam->Device_Addr = Read_Data;
    
    InFlash_Read_MultiBytes(BAUDRATE, &Read_Data, 1);
    ModBusBaseParam->BaudRate = Read_Data;
    
    InFlash_Read_MultiBytes(PARITY, &Read_Data, 1);
    ModBusBaseParam->Parity = Read_Data;
    
    InFlash_Read_MultiBytes(AUTO_UPLOAD, &Read_Data, 1);
    ModBusBaseParam->AutoUpload = Read_Data;
    
    InFlash_Read_MultiBytes(OUTPUTMODE, &Read_Data, 1);
    ModBusBaseParam->Output_Mode = Read_Data;    
    
    InFlash_Read_MultiBytes(FREEZE, &Read_Data, 1);
    ModBusBaseParam->Freeze = Read_Data;
    
    ModBusBaseParam->InRomWrEn = FLASH_WR_DISABLE;
    ModBusBaseParam->ModBus_CallBack = NULL;
    
    ModBusBaseParam->ModBus_TX_RX.USART_Receive_Buf = USART_Receive_Buf;
    ModBusBaseParam->ModBus_TX_RX.USART_Receive_Buf_Len = RECEIVE_SIZE;
    ModBusBaseParam->ModBus_TX_RX.Receive_Buf = ModBus_Receive_Buf;
    ModBusBaseParam->ModBus_TX_RX.Send_Buf = ModBus_Send_Buf;
    ModBusBaseParam->ModBus_TX_RX.Send_Len = 0;
    
    Sensor_USART_Init(ModBusBaseParam->BaudRate, ModBusBaseParam->Parity, 
                    ModBusBaseParam->ModBus_TX_RX.USART_Receive_Buf, 
                    ModBusBaseParam->ModBus_TX_RX.USART_Receive_Buf_Len);
}
#else
/**
* 名称       : ModBus_Init()
* 创建日期   : 2020-05-07
* 作者       : 杨春林
* 功能       : ModBus初始化
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
*              Param的数据内容(按字节顺序排列,大端模式): 0,设备地址; 
*              1,波特率代码; 2奇偶校验代码; 3自动上传周期;
*              4,输出模式; 5,设备冻结标志;
* 输出参数   : 无
* 返回结果   : OP_SUCCESS(成功)，OP_FAILED(失败)
* 注意和说明 : 
* 修改内容   :
*/
uint8_t ModBus_Init(ModBusBaseParam_TypeDef *ModBusBaseParam,
                uint8_t *Param, uint16_t Param_Size)
{    
    uint8_t cnt = 0;

    if(Param != NULL && Param_Size != 0)
    {
        ModBusBaseParam->Device_Addr = Param[cnt++];
        ModBusBaseParam->BaudRate = Param[cnt++];
        ModBusBaseParam->Parity = Param[cnt++];
        ModBusBaseParam->AutoUpload = Param[cnt++];        
        ModBusBaseParam->Output_Mode = Param[cnt++];
        ModBusBaseParam->Freeze = Param[cnt++];        
        ModBusBaseParam->InRomWrEn = FLASH_WR_DISABLE;
        ModBusBaseParam->ModBus_CallBack = NULL;
        
        ModBusBaseParam->ModBus_TX_RX.USART_Receive_Buf = USART_Receive_Buf;
        ModBusBaseParam->ModBus_TX_RX.USART_Receive_Buf_Len = RECEIVE_SIZE;
        ModBusBaseParam->ModBus_TX_RX.Receive_Buf = ModBus_Receive_Buf;
        ModBusBaseParam->ModBus_TX_RX.Send_Buf = ModBus_Send_Buf;
        ModBusBaseParam->ModBus_TX_RX.Send_Len = 0;
        
        Sensor_USART_Init(ModBusBaseParam->BaudRate, ModBusBaseParam->Parity, 
                        USART_Receive_Buf, RECEIVE_SIZE);
        
        if(cnt > Param_Size)
        {
            return OP_FAILED;
        }
        return OP_SUCCESS;
    }
    return OP_FAILED;
}
#endif

/**
* 名称       : MDB_Get_CRC16()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : 对输入的消息帧进行CRC16校验
* 输入参数   : Msg:消息首地址; MsgLen:消息帧长度;
* 输出参数   : 无
* 返回结果   : CRC16校验码
* 注意和说明 : 消息长度不能为0
* 修改内容   :
*/
uint16_t MDB_Get_CRC16(uint8_t *Msg, uint16_t MsgLen) 
{
    uint8_t CRCLo = 0xFF; 
    uint8_t CRCHi = 0xFF; 
    uint8_t uIndex;  
    uIndex = 0;

    while (MsgLen--)  
    { 
        uIndex = CRCLo ^ *Msg++; 
        CRCLo  = CRCHi ^ auchCRCLo[uIndex]; 
        CRCHi  = auchCRCHi[uIndex]; 
    } 

    return (CRCHi << 8) | CRCLo;
}

/**
* 名称       : MDB_Snd_Msg_RTU()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : 发送 Modbus RTU消息帧
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
*              Msg:消息首地址; MsgLen:消息帧长度;
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 消息长度不能为0，发送时先解冻设备，设备地址不能为广播地址
* 修改内容   :
*/
void MDB_Snd_Msg_RTU(ModBusBaseParam_TypeDef *ModBusBaseParam, uint8_t *pMsg, uint16_t len)
{
    uint8_t ucCRC[2];
    uint16_t uiCRC;
    //无效地址或者数据长度超限
    if(((uint8_t *)0 == pMsg) || (0 == len) || ((SEND_SIZE - 2) < len))
    {
        return;
    }
    //广播地址或者设备被冻结
    if((BROADCASTADDR == pMsg[0]) || (FREEZE_DISABLE != ModBusBaseParam->Freeze))
    {
        return;
    }
    //得到CRC码
    uiCRC = MDB_Get_CRC16(pMsg, len);
    //CRC低字节在前
    ucCRC[0] = (uint8_t)uiCRC;     
    //CRC高字节在后
    ucCRC[1] = (uint8_t)(uiCRC >> 8);     

    pMsg[len] = ucCRC[0];
    pMsg[len+1] = ucCRC[1];

    //开启发送
    Send_Data(pMsg, len + 2);
}

/**
* 名称       : MDB_Snd_Msg_RTU_NoLimit()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : 发送 Modbus RTU消息帧(无设备地址限制)
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
*              Msg:消息首地址; MsgLen:消息帧长度;
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 消息长度不能为0，发送时先解冻设备
* 修改内容   :
*/
void MDB_Snd_Msg_RTU_NoLimit(ModBusBaseParam_TypeDef *ModBusBaseParam, uint8_t *pMsg, uint16_t len)
{
    uint8_t ucCRC[2];
    uint16_t uiCRC;
    //数据地址无效或者数据长度超限
    if(((uint8_t *)0 == pMsg) || (0 == len) || ((SEND_SIZE - 2) < len))
    {
        return;
    }
    //串口冻结不发送数据
    if(FREEZE_DISABLE != ModBusBaseParam->Freeze)
    {
        return;
    }
    //得到CRC码
    uiCRC = MDB_Get_CRC16(pMsg, len);
    //CRC低字节在前
    ucCRC[0] = (uint8_t)uiCRC;   
    //CRC高字节在后
    ucCRC[1] = (uint8_t)(uiCRC >> 8);     

    pMsg[len] = ucCRC[0];
    pMsg[len+1] = ucCRC[1];

    //开启发送
    Send_Data(pMsg, len + 2);
}

/**
* 名称       : Send_Data()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : 发送 Modbus 数据
* 输入参数   : Msg:消息首地址; MsgLen:消息帧长度;
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 
* 修改内容   :
*/
void Send_Data(uint8_t *pMsg, uint16_t len)
{
#ifdef  USART_USING_485
    //发送使能
    TX_ON;
#endif
    //DMA发送
    BSP_UART_Transmit_DMA(pMsg, len);
}

/**
* 名称       : ModbusHandle()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : Modbus RTU消息帧处理函数
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
*              arg:用户自定义的参数,例如设备参数
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 
* 修改内容   :
*/
void ModbusHandle(ModBusBaseParam_TypeDef *ModBusBaseParam, void *arg)
{
    if(Sensor_USART_Get_RX_Updata_Flag() == 0)
    {
        return;
    }
    else
    {
        if(Sensor_USART_Get_RXBuf_Used_Len() 
            > Sensor_USART_Get_RX_Len())
        {
            memcpy( ModBusBaseParam->ModBus_TX_RX.Receive_Buf, 
                    ModBusBaseParam->ModBus_TX_RX.USART_Receive_Buf 
                    + Sensor_USART_Get_RXBuf_Used_Len() 
                    - Sensor_USART_Get_RX_Len(), Sensor_USART_Get_RX_Len());
        }
        else
        {
            memcpy( ModBusBaseParam->ModBus_TX_RX.Receive_Buf, 
                    ModBusBaseParam->ModBus_TX_RX.USART_Receive_Buf 
                    + (RECEIVE_SIZE - Sensor_USART_Get_RX_Len() 
                    + Sensor_USART_Get_RXBuf_Used_Len()), 
                    Sensor_USART_Get_RX_Len() - Sensor_USART_Get_RXBuf_Used_Len());
            memcpy( ModBusBaseParam->ModBus_TX_RX.Receive_Buf + Sensor_USART_Get_RX_Len() 
                    - Sensor_USART_Get_RXBuf_Used_Len(), 
                    ModBusBaseParam->ModBus_TX_RX.USART_Receive_Buf, 
                    Sensor_USART_Get_RXBuf_Used_Len());
        }
    }
    //校验错误，地址错误都不处理消息帧
    if(MDB_Get_CRC16(ModBusBaseParam->ModBus_TX_RX.Receive_Buf, Sensor_USART_Get_RX_Len())
        || ((ModBusBaseParam->Device_Addr != ModBusBaseParam->ModBus_TX_RX.Receive_Buf[0]) 
        && (BROADCASTADDR != ModBusBaseParam->ModBus_TX_RX.Receive_Buf[0])))
    {     
        Sensor_USART_Clear_RX_Updata_Flag();
        return;
    }
    switch(ModBusBaseParam->ModBus_TX_RX.Receive_Buf[1])
    {
    case 0x03:
        ModbusFunc03(ModBusBaseParam, arg);
        break;

    case 0x04:
        ModbusFunc04(ModBusBaseParam, arg);
        break;

    case 0x05:
        ModbusFunc05(ModBusBaseParam, arg);
        break;
 
    case 0x10:
        ModbusFunc10(ModBusBaseParam, arg);
        break;

    case 0x25:
        ModbusFunc25(ModBusBaseParam, arg);
        break;
  
    case 0x26:
        ModbusFunc26(ModBusBaseParam, arg);
        break;
   
    case 0x27:
        ModbusFunc27(ModBusBaseParam, arg);
        break;
 
    case 0x2A:
        ModbusFunc2A(ModBusBaseParam);
        break;

    case 0x2B:
        ModbusFunc2B(ModBusBaseParam);
        break;
      
   case 0x41:
        ModbusFunc41(ModBusBaseParam);
        break;   

    default:
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[0] 
            = ModBusBaseParam->ModBus_TX_RX.Receive_Buf[0];
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[1] 
            = (ModBusBaseParam->ModBus_TX_RX.Receive_Buf[1] | MB_REQ_FAILURE);
        ModBusBaseParam->ModBus_TX_RX.Send_Buf[2] = MB_FUNC_EXCEPTION;
        ModBusBaseParam->ModBus_TX_RX.Send_Len = 3;
        break;
    }
    Sensor_USART_Clear_TX_Cplt_Flag();
    //发送Modbus RTU
    MDB_Snd_Msg_RTU(ModBusBaseParam, ModBusBaseParam->ModBus_TX_RX.Send_Buf, 
                    ModBusBaseParam->ModBus_TX_RX.Send_Len);
    if(ModBusBaseParam->ModBus_CallBack != NULL)
    {
        while(Sensor_USART_Get_TX_Cplt_Flag() == 0);
        ModBusBaseParam->ModBus_CallBack(ModBusBaseParam);
        ModBusBaseParam->ModBus_CallBack = NULL;
    }
    Sensor_USART_Clear_RX_Updata_Flag();
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
* 注意和说明 : 这里使用弱定义,实际函数在ModBus_Conf.c里面定义
* 修改内容   :
*/
__weak void ModbusFunc03(ModBusBaseParam_TypeDef *ModBusBaseParam, void *arg)
{
    UNUSED(ModBusBaseParam);
    UNUSED(arg);
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
* 注意和说明 : 这里使用弱定义,实际函数在ModBus_Conf.c里面定义
* 修改内容   :
*/
__weak void ModbusFunc04(ModBusBaseParam_TypeDef *ModBusBaseParam, void *arg)
{
    UNUSED(ModBusBaseParam);
    UNUSED(arg);
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
* 注意和说明 : 这里使用弱定义,实际函数在ModBus_Conf.c里面定义
* 修改内容   :
*/
__weak void ModbusFunc05(ModBusBaseParam_TypeDef *ModBusBaseParam, void *arg)
{
    UNUSED(ModBusBaseParam);
    UNUSED(arg);
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
* 注意和说明 : 这里使用弱定义,实际函数在ModBus_Conf.c里面定义
* 修改内容   :
*/
__weak void ModbusFunc10(ModBusBaseParam_TypeDef *ModBusBaseParam, void *arg)
{
    UNUSED(ModBusBaseParam);
    UNUSED(arg);
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
* 注意和说明 : 这里使用弱定义,实际函数在ModBus_Conf.c里面定义
* 修改内容   :
*/
__weak void ModbusFunc25(ModBusBaseParam_TypeDef *ModBusBaseParam, void *arg)
{
    UNUSED(ModBusBaseParam);
    UNUSED(arg);
}

/**
* 名称       : ModbusFunc26()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : Modbus 26功能码消息帧处理
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
*              arg:用户自定义的参数,例如设备参数
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 这里使用弱定义,实际函数在ModBus_Conf.c里面定义
* 修改内容   :
*/
__weak void ModbusFunc26(ModBusBaseParam_TypeDef *ModBusBaseParam, void *arg)
{
    UNUSED(ModBusBaseParam);
    UNUSED(arg);
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
* 注意和说明 : 这里使用弱定义,实际函数在ModBus_Conf.c里面定义
* 修改内容   :
*/
__weak void ModbusFunc27(ModBusBaseParam_TypeDef *ModBusBaseParam, void *arg)
{
    UNUSED(ModBusBaseParam);
    UNUSED(arg);
}

/**
* 名称       : ModbusFunc2A()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : Modbus 2A功能码消息帧处理
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 这里使用弱定义,实际函数在ModBus_Conf.c里面定义
* 修改内容   :
*/
__weak void ModbusFunc2A(ModBusBaseParam_TypeDef *ModBusBaseParam)
{
    UNUSED(ModBusBaseParam);
}

/**
* 名称       : ModbusFunc2B()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : Modbus 2B功能码消息帧处理
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 这里使用弱定义,实际函数在ModBus_Conf.c里面定义
* 修改内容   :
*/
__weak void ModbusFunc2B(ModBusBaseParam_TypeDef *ModBusBaseParam)
{
    UNUSED(ModBusBaseParam);
}

/**
* 名称       : ModbusFunc41()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : Modbus 41功能码消息帧处理
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 这里使用弱定义,实际函数在ModBus_Conf.c里面定义
* 修改内容   :
*/
__weak void ModbusFunc41(ModBusBaseParam_TypeDef *ModBusBaseParam)
{
    UNUSED(ModBusBaseParam);
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
* 注意和说明 : 这里使用弱定义,实际函数在ModBus_Conf.c里面定义
* 修改内容   :
*/
__weak void ModbusAutoUpload(ModBusBaseParam_TypeDef *ModBusBaseParam, void *arg)
{
    UNUSED(ModBusBaseParam);
    UNUSED(arg);
}

/**
* 名称       : SensorEvent()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : 传感器事件处理
* 输入参数   : ModBusBaseParam: ModBus处理的基本参数结构体;
*              arg:用户自定义的参数,例如设备参数
* 输出参数   : 无
* 返回结果   : 无
* 注意和说明 : 
* 修改内容   :
*/
void SensorEvent(ModBusBaseParam_TypeDef *ModBusBaseParam, void *arg)
{
    static uint32_t Old_Tick = 0;
    //如果使能自动上传且自动上传定时时间到
    if((0 != ModBusBaseParam->AutoUpload) 
        && ((ModBusBaseParam->AutoUpload * AUTOUPLOAD_CYCLE) <= (HAL_GetTick() - Old_Tick)))
    {
        //Modbus帧自动上传
        ModbusAutoUpload(ModBusBaseParam, arg);
        Old_Tick = HAL_GetTick();
    }
}


