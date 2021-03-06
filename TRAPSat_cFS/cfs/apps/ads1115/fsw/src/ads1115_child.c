/*
** Required Headers
*/
#include "ads1115.h"
#include "ads1115_perfids.h"
#include "ads1115_msgids.h"
#include "ads1115_msg.h"
#include "ads1115_events.h"
#include "ads1115_version.h"
#include "ads1115_child.h"

extern ads1115_hk_tlm_t ADS1115_HkTelemetryPkt;
extern ADS1115_Ch_Data_t ADS1115_ChannelData;
extern uint8 ads1115_childtask_read_once;
extern uint8 ads1115_adc_read_count;
extern ADS1115_TEMPS_CMD_PKT_t ADS1115_TempsCmdPkt;

char num_reboots[3];

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* ADS1115 child task -- startup initialization                        */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int32 ADS1115_ChildInit(void)
{
    char *TaskText = "ADC Child Task";
    int32 Result;
    
    /*
    ** Read number of reboots
    */
    //snprintf(num_reboots, sizeof(num_reboots),  "%.3s", getNumReboots());    
    setNumReboots();

    /* Create child task - ADS1115 monitor task */
    Result = CFE_ES_CreateChildTask(&ADS1115_ADC_ChildTaskID,
                                     ADS1115_ADC_CHILD_TASK_NAME,
                                     (void *) ADS1115_ADC_ChildTask, 0,
                                     ADS1115_ADC_CHILD_TASK_STACK_SIZE,
                                     ADS1115_ADC_CHILD_TASK_PRIORITY, 0);
    if (Result != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(ADS1115_CHILD_INIT_ERR_EID, CFE_EVS_ERROR,
           "%s initialization error: create task failed: result = %d",
            TaskText, Result);
    }
    else
    {
        CFE_EVS_SendEvent(ADS1115_CHILD_INIT_EID, CFE_EVS_INFORMATION,
           "%s initialization info: create task complete: result = %d",
            TaskText, Result);
    }


    if (Result != CFE_SUCCESS)
    {
    	return(Result);
    }

    return CFE_SUCCESS;

} /* End of ADS1115_ChildInit() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* ADC child task -- task entry point                              */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void ADS1115_ADC_ChildTask(void)
{
    char *TaskText = "ADS1115 Child Task";
    int32 Result;
    /*
    ** The child task runs until the parent dies (normal end) or
    **  until it encounters a fatal error (semaphore error, etc.)...
    */
    Result = CFE_ES_RegisterChildTask();

    if (Result != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(ADS1115_CHILD_INIT_ERR_EID, CFE_EVS_ERROR,
           "%s initialization error: register child failed: result = %d",
            TaskText, Result);
    }
    else
    {
        CFE_EVS_SendEvent(ADS1115_CHILD_INIT_EID, CFE_EVS_INFORMATION,
           "%s initialization complete", TaskText);

        /*
        ** instatiate global ADC Read Samples Counter 
        */
        ads1115_adc_read_count = 0; 

        /* 
        ** Child task process loop 
        */
        ADS1115_ChildLoop();
    }

    /* 
    ** This call allows cFE to clean-up system resources 
    */
    CFE_ES_ExitChildTask();
 
   return;

} /* End of ADS1115_ADC_ChildTask() */

/*
** ADS1115 Infinite Child Loop
** 
** Check State
**  - Read ADC or not
** Loop
*/
void ADS1115_ChildLoop(void)
{
    /* 
    ** Buffer for ADC/OS call return values 
    */
    int ret_val = 0; 

    /*
    ** infinite read loop w/ 5 second delay
    ** Should never return from this loop during runtime
    */
    for ( ; ; )
    {
        //OS_printf("\nInside ADS Child Loop!\n");
        /* 
        ** Clear ret before every ADC Read call 
        */
        ret_val = 0;

        /*
        ** Determine ADC Read Action based on childloop_state
        */
        switch(ADS1115_HkTelemetryPkt.ads1115_childloop_state)
        {
            case 0:     /* Infinite Read && Store Loop */
                        
                        if ((ret_val = ADS1115_ReadADCChannels()) < 0)
                        {
                            CFE_EVS_SendEvent(ADS1115_CHILD_ADC_ERR_EID,CFE_EVS_ERROR,
                                "ADC Read Ret Val=[%d]. Expected non-negative val.", ret_val);
                        }
                        else
                        {
                            /*
                            ** Log ADC Data to cFS Event Message Output
                            */
                            /*
                            CFE_EVS_SendEvent(ADS1115_CHILD_ADC_INF_EID, CFE_EVS_INFORMATION,
                                   "ADC Channel Data: { 0x%.2X%.2X, 0x%.2X%.2X, 0x%.2X%.2X, 0x%.2X%.2X }",
                                    ADS1115_ChannelData.adc_ch_0[1], ADS1115_ChannelData.adc_ch_0[0],
                                    ADS1115_ChannelData.adc_ch_1[1], ADS1115_ChannelData.adc_ch_1[0],
                                    ADS1115_ChannelData.adc_ch_2[1], ADS1115_ChannelData.adc_ch_2[0],
                                    ADS1115_ChannelData.adc_ch_3[1], ADS1115_ChannelData.adc_ch_3[0]);
                            */                            

                            /* Store Data */
                            if( (ret_val = ADS1115_StoreADCChannels()) < 0)
                            {
                                CFE_EVS_SendEvent(ADS1115_CHILD_ADC_ERR_EID,CFE_EVS_ERROR, "ADS1115_StoreADCChannels() = [%d]. Expected non-negative val.", ret_val);
                            }
                            else
                            {
                                /*
                                ** Log File Data to cFS Event Message Output
                                */
                                /*
                                CFE_EVS_SendEvent(ADS1115_CHILD_ADC_INF_EID, CFE_EVS_INFORMATION,
                                   "Last Data File Stored = %s.", ADS1115_HkTelemetryPkt.ads1115_datafilename);
                                */
                                CFE_EVS_SendEvent(ADS1115_CHILD_ADC_INF_EID, CFE_EVS_INFORMATION,
                                   "Temp File \'%s\':{%.2X%.2X, %.2X%.2X, %.2X%.2X, %.2X%.2X, }",
                                    ADS1115_HkTelemetryPkt.ads1115_datafilename,
                                    ADS1115_ChannelData.adc_ch_0[0], ADS1115_ChannelData.adc_ch_0[1],
                                    ADS1115_ChannelData.adc_ch_1[0], ADS1115_ChannelData.adc_ch_1[1],
                                    ADS1115_ChannelData.adc_ch_2[0], ADS1115_ChannelData.adc_ch_2[1],
                                    ADS1115_ChannelData.adc_ch_3[0], ADS1115_ChannelData.adc_ch_3[1]);

                                /*
                                ** Send file name to TIM
                                */
                                ADS1115_SendTimFileName(ADS1115_HkTelemetryPkt.ads1115_datafilename);
                                
                            }
                        }
                        break;

            case 1:     /* Read Once && Set Flag to not read again && store to file */
                        if(ads1115_childtask_read_once == 0)
                        {
                            if ((ret_val = ADS1115_ReadADCChannels()) < 0)
                            {
                                CFE_EVS_SendEvent(ADS1115_CHILD_ADC_ERR_EID,CFE_EVS_ERROR,
                                    "ADC Read Ret Val=[%d]. Expected non-negative val.", ret_val);
                            }
                            else
                            {
                                /* Set Flag */
                                ads1115_childtask_read_once = 1;

                                /* Store Data */
                                if( (ret_val = ADS1115_StoreADCChannels()) < 0)
                                {
                                    CFE_EVS_SendEvent(ADS1115_CHILD_ADC_ERR_EID,CFE_EVS_ERROR,
                                        "OS Store Ret Val=[%d]. Expected non-negative val.", ret_val);
                                }
                                else
                                {
                                    CFE_EVS_SendEvent(ADS1115_CHILD_ADC_INF_EID, CFE_EVS_INFORMATION,
                                       "One Temp File \'%s\':{%.2X%.2X, %.2X%.2X, %.2X%.2X, %.2X%.2X, }",
                                        ADS1115_HkTelemetryPkt.ads1115_datafilename,
                                        ADS1115_ChannelData.adc_ch_0[1], ADS1115_ChannelData.adc_ch_0[0],
                                        ADS1115_ChannelData.adc_ch_1[1], ADS1115_ChannelData.adc_ch_1[0],
                                        ADS1115_ChannelData.adc_ch_2[1], ADS1115_ChannelData.adc_ch_2[0],
                                        ADS1115_ChannelData.adc_ch_3[1], ADS1115_ChannelData.adc_ch_3[0]);
                                    /*
                                    ** Send file name to TIM
                                    */
                                    ADS1115_SendTimFileName(ADS1115_HkTelemetryPkt.ads1115_datafilename);
                                }
                            }
                            
                        }
                        break;

            default:    /* Unknown State: Log it, Reset Flags, Continue */
                        CFE_EVS_SendEvent(ADS1115_CHILD_ADC_ERR_EID,CFE_EVS_ERROR,
                                    "ADC Child Loop State Argument [%d] unrecognized.", 
                                    ADS1115_HkTelemetryPkt.ads1115_childloop_state);
                        CFE_EVS_SendEvent(ADS1115_CHILD_ADC_ERR_EID,CFE_EVS_ERROR,
                                    "Resetting \'Child Loop State\' and \'Read Once Flag\'.");
                        ads1115_childtask_read_once = 0;
                        ADS1115_HkTelemetryPkt.ads1115_childloop_state = 0;
                        break;
        }

        /*
        ** Delay 5 Seconds (5000 milliseconds)
        */
        /*
        OS_printf("ADS1115: ADC Loop current state [%d]. Delay 5s.\n", ADS1115_HkTelemetryPkt.ads1115_childloop_state);
        */
        OS_TaskDelay(1000);
        
    } /* Infinite ADC Read Loop End Here */

    return;
} /* End of: int ADS1115_ChildLoop(void) */

int ADS1115_SendTimFileName(char *file_name)
{
    /*
    ** Initialize message with TIM app info
    ** ADS1115_TEMPS_CMD_MID = TIM_APP_CMD_MID = (0x188A)
    ** ADS1115_TEMPS_CMD_CODE = TIM_APP_SEND_TEMPS_CC = 4
    ** ADS1115_TEMPS_CMD_LNGTH = sizeof( ADS1115_TEMPS_CMD_PKT_t )
    */
    CFE_SB_InitMsg((void *) &ADS1115_TempsCmdPkt, (CFE_SB_MsgId_t) ADS1115_TEMPS_CMD_MID, (uint16) ADS1115_TEMPS_CMD_LNGTH, (boolean) 1 );

    int32 ret = CFE_SB_SetCmdCode((CFE_SB_MsgPtr_t) &ADS1115_TempsCmdPkt, (uint16) ADS1115_TEMPS_CMD_CODE);

    if(ret < 0)
    {
        OS_printf("ADS1115: SendTimFileName() Set Cmd Code Ret [%d].\n", ret);
    }

    //OS_printf("Copying filename [%s] into command packet.\n", file_name);

    snprintf(ADS1115_TempsCmdPkt.TempsName, sizeof(ADS1115_TempsCmdPkt.TempsName), "%s", file_name);

    //OS_printf("Command packet holds: [%s].\n", ADS1115_TempsCmdPkt.TempsName);

    CFE_SB_GenerateChecksum((CFE_SB_MsgPtr_t) &ADS1115_TempsCmdPkt);

    CFE_SB_SendMsg((CFE_SB_Msg_t *) &ADS1115_TempsCmdPkt);

    CFE_EVS_SendEvent(ADS1115_CHILD_ADC_INF_EID, CFE_EVS_INFORMATION, "ADS1115 Message sent to TIM.");
    //OS_printf("ADS1115 Message sent to TIM.\n");

    CFE_EVS_SendEvent(ADS1115_TIM_MSG_EID,CFE_EVS_INFORMATION, "ADS1115 Message sent to TIM");


    return 0;
}

void setNumReboots(void)
{
    //char buff[3];
    //OS_printf("Inside ADS get reboot\n");

    memset(num_reboots, '0', sizeof(num_reboots));

    mode_t mode = S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IROTH;

    int32 fd = OS_open((const char *) "/ram/logs/reboot.txt", (int32) OS_READ_ONLY, (uint32) mode);

    if(fd < OS_FS_SUCCESS)
    {
        OS_printf("\tCould not open reboot file in ADS1115, ret = %d!\n", fd);
    }

    int os_ret = OS_read((int32) fd, (void *) num_reboots, (uint32) 3);

    if( os_ret < OS_FS_SUCCESS)
    {
        memset(num_reboots, '9', sizeof(num_reboots));
        OS_printf("\tCould not read from reboot file in ADS1115, ret = %d!\n", os_ret);
    }

    OS_close(fd);


    return;
}

