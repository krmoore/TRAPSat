/*******************************************************************************
** File: vc0706_device.c
**
** Purpose:
**   This is a source file for the VC0706 application
**
**   int VC0706_takePics(void) "VC0706 Camera capture Loop"
**
**
*******************************************************************************/
/*
** VC0706 Master Header
*/
#include "vc0706.h"


/*
** Local Functions
*/
char * getTime(void);


/*
** External References -- maybe make the Camera struct external so we can use it in other files? idk bro
*/
extern struct Camera_t cam;

/*
** VC0706 take Pictures Loop
*/
int VC0706_takePics(void)
{
    /*
    ** Path that pictures should be stored in
    **
    ** NOTE: if path is greater than 16 chars, imageName[] in vc0706_core.h will need to be enlarged accordingly.
    */
    char * path = malloc(OS_MAX_PATH_LEN);

    /*
    ** Main Camera structure
    **
    ** WARNING: may need to  be moved to a global later for CI and TO
    */
    // Camera cam;

    if(init(&cam) == -1) // Error
    {
        OS_printf("Camera initialization error.\n");
        return -1;
    }

    /*
    ** infinite Camera loop
    ** w/ no delay
    */
    int i;
    for (i=0; ;i++) // NOTE: we will need to add flash and MUX implementation. Easy, but should be broken into separate headers.
    {
	char *v;
        /*
        ** Get camera version, another way to check that the camera is working properly. Also necessary for initialization.
        */
        if ((v = getVersion(&cam)) == NULL) // function will return NULL upon failure
        {
            OS_printf( "Failed communication to Camera.\n"); // maybe more information would be good, we'll see what's relevant durring testing.
            return -1;
        }
        /*else // pulled to limit debug prints
        {
            OS_printf("Debug: Camera open with version = %s\n", v);
        }*/

        /*
        ** Set Path for the new image
        */
	OS_printf("VC0706: Calling sprintf()...\n");
        //sprintf(path, "/home/pi/TRAPSat/images/%s.jpg", getTime());
	int ret = sprintf(path, "/home/pi/TRAPSat/images/test_%d.jpg", i);

	printf("sprintf returned: %d\n", ret);
	/*
        ** Actually takes the picture
        */
	OS_printf("VC0706: Calling takePicture()...\n");
        char* loc = takePicture(&cam, path);

        OS_printf("Debug: Camera took picture. Stored at: %s\n", loc);

	} /* Infinite Camera capture Loop End Here */

    return(0);
}


char * getTime(void)
{
    char * curr_time;
    time_t raw_t;
    time(&raw_t);
    struct tm * timeinfo;
    timeinfo = localtime( &raw_t );
    //printf("%s\n", asctime(timeinfo));

    //sprintf(curr_time, "%s", (char[])asctime( timeinfo ) );
    curr_time = asctime(timeinfo);
    char * i;
    for(i=curr_time; *i ; i++)
    {
        if( *i == ' ')
            *i = '_';
        if( *i == ':')
            *i = '-';
    }

    return curr_time;
}
