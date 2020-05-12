/****************************************************************************************************************************************************************
** 版权:          2020-2030,深圳市信为科技发展有限公司
** 文件名:        Picocap_app.c
** 作者:          杨春林
** 版本:          V1.0.0
** 日期:          2020-04-30
** 描述:          PCap芯片应用及其数据处理
** 功能:          读取PCap芯片,对读出的数据进行滤波,数据转换
*****************************************************************************************************************************************************************
** 修改者:        No
** 版本:          No
** 修改内容:      No
** 日期:          No
****************************************************************************************************************************************************************/

#include "Picocap_app.h"
#include "VariaType.h"
#include "TypeConvert.h"


/**
* 名称       : SwitchCurFilter()
* 创建日期   : 2020-04-30
* 作者       : 杨春林
* 功能       : 切换滤波水平函数
* 输入参数   : FiltFactor(滤波系数)
* 输出参数   : FilterParam,指定滤波参数结构体;
* 返回结果   : 无
* 注意和说明 : 滤波水平分9级，数字从低到高对应滤波深度从低到高
* 修改内容   :
*/
void SwitchCurFilter(uint8_t FiltFactor, DataFilterParam *FilterParam)
{  
    switch(FiltFactor)
    {
    case 0:
        FilterParam->FilterBufMax   = 1;
        FilterParam->FilterCycle    = 1;
    break;

    case 1:
        FilterParam->FilterBufMax   = 1;
        FilterParam->FilterCycle    = 12;
    break;

    case 2:
        FilterParam->FilterBufMax   = 2;
        FilterParam->FilterCycle    = 12;
    break;

    case 3:
        FilterParam->FilterBufMax   = 3;
        FilterParam->FilterCycle    = 12;
    break;

    case 4:
        FilterParam->FilterBufMax   = 4;
        FilterParam->FilterCycle    = 15;
    break;

    case 5:
        FilterParam->FilterBufMax   = 5;
        FilterParam->FilterCycle    = 24;
    break;

    case 6:
        FilterParam->FilterBufMax   = 6;
        FilterParam->FilterCycle    = 30;
    break;

    case 7:
        FilterParam->FilterBufMax   = 8;
        FilterParam->FilterCycle    = 30;
    break;

    case 8:
        FilterParam->FilterBufMax   = 8;
        FilterParam->FilterCycle    = 60;
    break;

    case 9:
        FilterParam->FilterBufMax   = 10;
        FilterParam->FilterCycle    = 96;
    break;

    default:
        FilterParam->FilterBufMax   = 1;
        FilterParam->FilterCycle    = 10;
    break;
    }

    FilterParam->FilterStart = 0;
}

#ifdef __IN_FLASH_APP_H
/**
* 名称       : DataFilterParam_Init()
* 创建日期   : 2020-05-05
* 作者       : 杨春林
* 功能       : 初始化数据滤波需要的参数结构
* 输入参数   : Filterfactor_CountMax, 滤波系数和累计输入的最大值,滤波系数在高8位,
*              累计输入的最大值在低8位;
* 输出参数   : FilterParam,数据滤波需要的参数结构指针;
* 返回结果   : 无
* 注意和说明 : 要使用本函数,要加入In_Flash_app.c、In_Flash_app.h、
*              In_Flash.ch、In_Flash.h和文件
* 修改内容   :
*/
void DataFilterParam_Init(DataFilterParam *FilterParam, uint16_t Filterfactor_CountMax)
{
    uint8_t Read_Data[4];
    
    InFlash_Read_MultiBytes(FILTER, Read_Data, 1);
    SwitchCurFilter(Read_Data[0], FilterParam);    
    FilterParam->InputCountMax = Filterfactor_CountMax;
    InFlash_Read_MultiBytes(CAPMAX, Read_Data, 4);
    FilterParam->InputRangeMax = HexToUlong(Read_Data);
    InFlash_Read_MultiBytes(CAPMIN, Read_Data, 4);
    FilterParam->InputRangeMin = HexToUlong(Read_Data);
}
#else
/**
* 名称       : DataFilterParam_Init()
* 创建日期   : 2020-05-05
* 作者       : 杨春林
* 功能       : 初始化数据滤波需要的参数结构
* 输入参数   : Filterfactor_CountMax, 滤波系数和累计输入的最大值,滤波系数在高8位,
*              累计输入的最大值在低8位;
*              InputRangeMax, 输入范围最大值;
*              InputRangeMin, 输入范围最小值;
* 输出参数   : FilterParam,数据滤波需要的参数结构指针;
* 返回结果   : 无
* 注意和说明 : 
* 修改内容   : 
*/
void DataFilterParam_Init(DataFilterParam *FilterParam, uint16_t Filterfactor_CountMax,
                            uint32_t InputRangeMax, uint32_t InputRangeMin)
{
    SwitchCurFilter(Filterfactor_CountMax >> 8, FilterParam);
    FilterParam->InputCountMax = Filterfactor_CountMax & 0xFF;
    FilterParam->InputRangeMax = InputRangeMax;
    FilterParam->InputRangeMin = InputRangeMin;
}
#endif

#ifdef __IN_FLASH_APP_H
/**
* 名称       : DataConvertParam_Init()
* 创建日期   : 2020-05-05
* 作者       : 杨春林
* 功能       : 初始化数据转换需要的参数结构
* 输入参数   : 无
* 输出参数   : DataConvert_Param,数据转换需要的参数结构指针;
* 返回结果   : 无
* 注意和说明 : 要使用本函数,要加入In_Flash_app.c、In_Flash_app.h、
*              In_Flash.ch、In_Flash.h和文件
* 修改内容   :
*/
void DataConvertParam_Init(PCap_DataConvert_Param *DataConvert_Param)
{
    uint8_t Read_Data[4];
    
    InFlash_Read_MultiBytes(COMPENSATE, Read_Data, 1);
    DataConvert_Param->CompenEn = Read_Data[0];
    
    InFlash_Read_MultiBytes(HEIGHTRANGE, Read_Data, 2);
    DataConvert_Param->HeightRange = (uint16_t)(Read_Data[0] << 8) | Read_Data[1];
    
    InFlash_Read_MultiBytes(CAPMIN, Read_Data, 4);
    DataConvert_Param->CapMin = HexToUlong(Read_Data);
    
    InFlash_Read_MultiBytes(CAPMAX, Read_Data, 4);
    DataConvert_Param->CapMax = HexToUlong(Read_Data);
    
    InFlash_Read_MultiBytes(CAPMINBAK, Read_Data, 4);
    DataConvert_Param->CapMinBak = HexToUlong(Read_Data);
    
    InFlash_Read_MultiBytes(CAPMAXBAK, Read_Data, 4);
    DataConvert_Param->CapMaxBak = HexToUlong(Read_Data);
    
    InFlash_Read_MultiBytes(CAPADMIN, Read_Data, 2);
    DataConvert_Param->CapADMin = (uint16_t)(Read_Data[0] << 8) | Read_Data[1];
    
    InFlash_Read_MultiBytes(CAPADLOW, Read_Data, 2);
    DataConvert_Param->CapADLow = (uint16_t)(Read_Data[0] << 8) | Read_Data[1];
    
    InFlash_Read_MultiBytes(CAPADHIGH, Read_Data, 2);
    DataConvert_Param->CapADHigh = (uint16_t)(Read_Data[0] << 8) | Read_Data[1];
    
    InFlash_Read_MultiBytes(CAPADMAX, Read_Data, 2);
    DataConvert_Param->CapADMax = (uint16_t)(Read_Data[0] << 8) | Read_Data[1];
    
    InFlash_Read_MultiBytes(CAPDAMIN, Read_Data, 2);
    DataConvert_Param->CapDAMin = (uint16_t)(Read_Data[0] << 8) | Read_Data[1];
    
    InFlash_Read_MultiBytes(CAPDALOW, Read_Data, 2);
    DataConvert_Param->CapDALow = (uint16_t)(Read_Data[0] << 8) | Read_Data[1];
    
    InFlash_Read_MultiBytes(CAPDAHIGH, Read_Data, 2);
    DataConvert_Param->CapDAHigh = (uint16_t)(Read_Data[0] << 8) | Read_Data[1];
    
    InFlash_Read_MultiBytes(CAPDAMAX, Read_Data, 2);
    DataConvert_Param->CapDAMax = (uint16_t)(Read_Data[0] << 8) | Read_Data[1];
    
    InFlash_Read_MultiBytes(CORRECT_K, Read_Data, 2);
    DataConvert_Param->Correct_K = (float)((Read_Data[0] << 8) | Read_Data[1]) / 100.0;
    
    InFlash_Read_MultiBytes(CORRECT_B, Read_Data, 2);
    DataConvert_Param->Correct_B = (float)((Read_Data[0] << 8) | Read_Data[1]) / 100.0;
}
#else
/**
* 名称       : DataConvertParam_Init()
* 创建日期   : 2020-05-05
* 作者       : 杨春林
* 功能       : 初始化数据转换需要的参数结构
* 输入参数   : CompenEn, 补偿使能;
*              Param, 要写入DataConvert_Param的参数数组指针;
*              Param_Size, Param的大小;
* 输出参数   : DataConvert_Param,数据转换需要的参数结构指针;
* 返回结果   : OP_SUCCESS(成功)，OP_FAILED(失败)
* 注意和说明 : Param的数据分布(按字节分布): 0~1,位移高度; 2~5,测量电容零点;
*              6~9,测量电容满点; 10~13,测量电容零点; 14~17,测量电容满点; 
*              18~19,电容AD值零点; 20~21,电容AD值下刻度; 22~23,电容AD值上刻度; 
*              24~25,电容AD值满量程; 26~27,电容DA值零点; 28~29,电容DA值下刻度; 
*              30~31,电容DA值上刻度; 32~33,电容DA值满量程; 34~35,电容修正系数K; 
*              36~37,电容修正系数B;
* 修改内容   :
*/
uint8_t DataConvertParam_Init(PCap_DataConvert_Param *DataConvert_Param, uint8_t CompenEn,
                            uint8_t *Param, uint16_t Param_Size)
{
    uint8_t cnt = 0;
    
    DataConvert_Param->CompenEn = CompenEn;
    if(Param != NULL && Param_Size != 0)
    {
        DataConvert_Param->HeightRange = (Param[cnt] << 8) | Param[cnt + 1];
        cnt += 2;    
        DataConvert_Param->CapMin = HexToUlong(&Param[cnt]);
        cnt += 4;
        DataConvert_Param->CapMax = HexToUlong(&Param[cnt]);
        cnt += 4;
        DataConvert_Param->CapADMin = (Param[cnt] << 8) | Param[cnt + 1];
        cnt += 2;
        DataConvert_Param->CapADLow = (Param[cnt] << 8) | Param[cnt + 1];
        cnt += 2;
        DataConvert_Param->CapADHigh = (Param[cnt] << 8) | Param[cnt + 1];
        cnt += 2;
        DataConvert_Param->CapADMax = (Param[cnt] << 8) | Param[cnt + 1];
        cnt += 2;
        DataConvert_Param->CapDAMin = (Param[cnt] << 8) | Param[cnt + 1];
        cnt += 2;
        DataConvert_Param->CapDALow = (Param[cnt] << 8) | Param[cnt + 1];
        cnt += 2;
        DataConvert_Param->CapDAHigh = (Param[cnt] << 8) | Param[cnt + 1];
        cnt += 2;
        DataConvert_Param->CapDAMax = (Param[cnt] << 8) | Param[cnt + 1];
        cnt += 2;    
        DataConvert_Param->Correct_K = (float)((Param[cnt] << 8) | Param[cnt + 1]) / 100.0;
        cnt += 2;
        DataConvert_Param->Correct_B = (float)((Param[cnt] << 8) | Param[cnt + 1]) / 100.0;
        
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
* 名称       : Sensor_PCap_GetResult()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : 获取PCap原始采集值
* 输入参数   : reg_addr,结果寄存器的地址;
* 输出参数   : PCap_Result,保存PCap的输出结果
* 返回结果   : 操作成功/失败
* 注意和说明 : 
* 修改内容   : 
*/
uint8_t Sensor_PCap_GetResult(uint8_t reg_addr, uint32_t *PCap_Result)
{
    uint32_t PCap_Status;
    uint8_t Result = OP_FAILED;
    static uint32_t Tick_Old = 0;

    //定时时间到采集电容值
    if(HAL_GetTick() - Tick_Old > PCAP_COLLECT_CYCLE)
    {
        //读取PCap状态
        PCap_Status = PCap_Res_Sta();
        if(PCap_Status & PCAP_RUNBIT)
        {
            *PCap_Result = PCap_Res_Capacitance(reg_addr);
            Result = OP_SUCCESS;
        }
        PCap_Measure();
        Tick_Old = HAL_GetTick();
    }
    
    return Result;
}

/**
* 名称       : Sensor_DataFilter()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : 数据按指定滤波参数进行滤波
* 输入参数   : FilterParam,指定滤波参数结构体;InputValue,输入的数据
* 输出参数   : OutputValue,滤波后的值
* 返回结果   : 操作成功/失败
* 注意和说明 : 使用本函数前,先将FilterParam初始化(即对它的每一个成员赋合适的值),否则不能正常使用
* 修改内容   : 
*/
uint8_t Sensor_DataFilter(DataFilterParam *FilterParam, uint32_t InputValue, uint32_t *OutputValue)
{
    static uint32_t Data_1st_FilterBuf[DATA_1ST_FILTER_BUF_MAX];        //一阶滤波缓存
    static uint32_t Data_1st_FilterBufBak[DATA_1ST_FILTER_BUF_MAX];     //一阶滤波缓存备份
    static uint32_t Data_2nd_FilterBuf[DATA_2ND_FILTER_BUF_MAX];        //二阶滤波缓存
    static uint32_t Data_2nd_FilterBufBak[DATA_2ND_FILTER_BUF_MAX];     //二阶滤波缓存备份
    static uint32_t DataResBuf[DATA_BUF_MAX];                           //接收原始数据值的缓存
    static uint8_t DataResBuf_Count;
    uint8_t i;
    uint32_t Data_Avg1;
    uint32_t Data_Avg2;
    uint32_t Data_Avg3;
    uint64_t Data_FilterSum;                                            //滤波数据累计和
    
    if(InputValue > FilterParam->InputRangeMax)
    {
        InputValue = FilterParam->InputRangeMax;
    }
    if(InputValue < FilterParam->InputRangeMin)
    {
        InputValue = FilterParam->InputRangeMin;
    }
    
    if(FilterParam->FilterStart == 0)
    {
        DataResBuf_Count = 0;
        for(i = 0; i < DATA_1ST_FILTER_BUF_MAX; i++)
        {
            Data_1st_FilterBuf[i] = InputValue;
        }
        for(i = 0; i < DATA_2ND_FILTER_BUF_MAX; i++)
        {
            Data_2nd_FilterBuf[i] = InputValue;
        }
        FilterParam->FilterStart = 1;
        
        return OP_FAILED;
    }
    
    DataResBuf[DataResBuf_Count++] = InputValue;
    if(DataResBuf_Count < DATA_BUF_MAX)
    {
        return OP_FAILED;
    }
    else
    {
        DataResBuf_Count = 0;
    }
    
    Data_Avg1 = GetDelExtremeAndAverage(DataResBuf, DATA_BUF_MAX, 2, 5);
    
    if(FilterParam->FilterCycle < 2)
    {
        *OutputValue = Data_Avg1;
        return OP_SUCCESS;
    }
    else
    {
        for(i = 0; i < FilterParam->FilterCycle - 1; i++)
        {
            Data_1st_FilterBuf[i] = Data_1st_FilterBuf[i + 1];
        }
        Data_1st_FilterBuf[i] = Data_Avg1;
        
        if(FilterParam->FilterCycle >= 3)
        {
            for(i = 0; i < FilterParam->FilterCycle; i++)
            {
                Data_1st_FilterBufBak[i] = Data_1st_FilterBuf[i];
            }
            Data_Avg2 = GetDelExtremeAndAverage(Data_1st_FilterBufBak, 
                                            FilterParam->FilterCycle, 
                                            FilterParam->FilterCycle / 3, 
                                            FilterParam->FilterCycle / 3);
        }
        else
        {
            Data_FilterSum = 0;                                                               
            for(i = 0; i < FilterParam->FilterCycle; i++)
            {
                Data_FilterSum += Data_1st_FilterBuf[i];
            }           
            Data_Avg2 = Data_FilterSum / FilterParam->FilterCycle;        /* 计算滤波数组的平均值 */
        }
        
        if(FilterParam->FilterBufMax < 2)            
        {
            *OutputValue = Data_Avg2;
            return OP_SUCCESS;
        }
        else
        {
            for(i = 0; i < FilterParam->FilterBufMax - 1; i++)
            {
                Data_2nd_FilterBuf[i] = Data_2nd_FilterBuf[i + 1];
            }
            Data_2nd_FilterBuf[i] = Data_Avg2;
            
            if(FilterParam->FilterBufMax >= 3)
            {
                for(i = 0; i < FilterParam->FilterBufMax; i++)
                {
                    Data_2nd_FilterBufBak[i] = Data_2nd_FilterBuf[i];
                }
                Data_Avg3 = GetDelExtremeAndAverage(Data_2nd_FilterBufBak, 
                                                FilterParam->FilterBufMax, 
                                                FilterParam->FilterBufMax / 3, 
                                                FilterParam->FilterBufMax / 3);
            }
            else
            {
                Data_FilterSum = 0;                                                               
                for(i = 0; i < FilterParam->FilterBufMax; i++)
                {
                    Data_FilterSum += Data_2nd_FilterBuf[i];
                }           
                Data_Avg3 = Data_FilterSum / FilterParam->FilterBufMax;        /* 计算滤波数组的平均值 */
            }
            
            *OutputValue = Data_Avg3;
            return OP_SUCCESS;
        }       
    }    
}

/**
* 名称       : Sensor_PCap_DataConvert()
* 创建日期   : 2020-04-29
* 作者       : 杨春林
* 功能       : 数据按指定控制参数进行数据转换
* 输入参数   : DataConvert_Param,指定控制参数结构体;InputValue,输入的数据
* 输出参数   : DataConvert_Out,输出数据的参数结构体
* 返回结果   : 无
* 注意和说明 : 使用本函数前,先将DataConvert_Param初始化(即对它的每一个成员赋合适的值),否则不能正常使用
* 修改内容   : 
*/
void Sensor_PCap_DataConvert(PCap_DataConvert_Param *DataConvert_Param, 
                                uint32_t InputValue, 
                                PCap_DataConvert_Out_Param *DataConvert_Out)
{
    uint16_t LiquidHeightAD;
    uint32_t PCap_Result;
    float LiquidHeightRate;
    float Rate;
    //补偿使能,修正K,B值
    if(DataConvert_Param->CompenEn == COMPENSATE_ENABLE)        
    {
        PCap_Result = (uint32_t)(InputValue * DataConvert_Param->Correct_K 
                                + (DataConvert_Param->Correct_B - 100));
    }
    else
    {
        PCap_Result = InputValue;
    }
    //计算电容比率
    LiquidHeightRate = (float)(PCap_Result - DataConvert_Param->CapMin) 
                                / (float)(DataConvert_Param->CapMax - DataConvert_Param->CapMin);
    
    if(1.0 < LiquidHeightRate)
    {
        LiquidHeightRate = 1.0;
    }
    else if (0.0 > LiquidHeightRate)
    {
        LiquidHeightRate = 0.0;
    }
    //转换成0--65535范围内的AD值
    LiquidHeightAD = (uint16_t)(LiquidHeightRate * PCAP_ADC_MAX_VALUE);
    //转换成液位高度
    DataConvert_Out->LiquidHeight = (float)(LiquidHeightRate 
                                            * DataConvert_Param->HeightRange);
    DataConvert_Out->PCapDA_ResultValue = (uint16_t)(LiquidHeightRate * (PCAP_DAC_MAX_VALUE 
                                                    - PCAP_DAC_MIN_VALUE)) 
                                                    + PCAP_DAC_MIN_VALUE;
    //如果没有4点差值法则直接计算DA输出值
    if((0x0000 == DataConvert_Param->CapADLow) || (0x0000 == DataConvert_Param->CapDAHigh))
    {
        DataConvert_Out->PCapDA_OutValue = (uint16_t)(LiquidHeightRate * (DataConvert_Param->CapDAMax 
                                                    - DataConvert_Param->CapDAMin)) 
                                                    + DataConvert_Param->CapDAMin;
    }
    else
    {                 
        //AD值在零点AD以下
        if(LiquidHeightAD < DataConvert_Param->CapADMin)  
        {
            DataConvert_Out->PCapDA_OutValue = DataConvert_Param->CapDAMin;
        }
        //AD值在零点AD和下刻度AD之间
        else if((LiquidHeightAD < DataConvert_Param->CapADLow) 
            && (LiquidHeightAD) >= DataConvert_Param->CapADMin)
        {
            Rate = (float)(LiquidHeightAD - DataConvert_Param->CapADMin)
                / (float)(DataConvert_Param->CapADLow - DataConvert_Param->CapADMin);
            DataConvert_Out->PCapDA_OutValue = DataConvert_Param->CapDALow - DataConvert_Param->CapDAMin;
            DataConvert_Out->PCapDA_OutValue = (uint16_t)(Rate * DataConvert_Out->PCapDA_OutValue) 
                                            + DataConvert_Param->CapDAMin;
        }
        //AD值在下刻度AD值和上刻度AD值之间
        else if((LiquidHeightAD < DataConvert_Param->CapADHigh) 
            && (LiquidHeightAD >= DataConvert_Param->CapADLow))
        {
            Rate = (float)(LiquidHeightAD - DataConvert_Param->CapADLow) 
                / (float)(DataConvert_Param->CapADHigh - DataConvert_Param->CapADLow);
            DataConvert_Out->PCapDA_OutValue = DataConvert_Param->CapDAHigh - DataConvert_Param->CapDALow;
            DataConvert_Out->PCapDA_OutValue = (uint16_t)(Rate * DataConvert_Out->PCapDA_OutValue) 
                                            + DataConvert_Param->CapDALow;
        }
        //AD值在上刻度AD值和满量程AD值之间
        else if((LiquidHeightAD < DataConvert_Param->CapADMax) 
            && (LiquidHeightAD >= DataConvert_Param->CapADHigh))
        {
            Rate = (float)(LiquidHeightAD - DataConvert_Param->CapADHigh) 
                / (float)(DataConvert_Param->CapADMax - DataConvert_Param->CapADHigh);
            DataConvert_Out->PCapDA_OutValue = DataConvert_Param->CapDAMax - DataConvert_Param->CapDAHigh;
            DataConvert_Out->PCapDA_OutValue = (uint16_t)(Rate * DataConvert_Out->PCapDA_OutValue) 
                                                    + DataConvert_Param->CapDAHigh;
        }                                            
        //AD值在满量程以上
        else                                                       
        {
            DataConvert_Out->PCapDA_OutValue = DataConvert_Param->CapDAMax;
        }
    } 
    DataConvert_Out->LiquidHeightAD = LiquidHeightAD;
}

