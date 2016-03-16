/*
 * Retrieved from https://github.com/vyykn/VC0706
 *
 * Edited By Zach Richard for use on TRAPSat aboard the RockSat-X 2016 Mission
 */

#include "vc0706_core.h"


int init(Camera_t *cam) {
    cam->frameptr = 0;
    cam->bufferLen = 0;
    cam->serialNum = 0;
    cam->motion = 1;
    cam->ready = 1;

    if ((cam->fd = serialOpen("/dev/ttyAMA0", BAUD)) < 0)
    {
        fprintf(stderr, "SPI Setup Failed: %s\n", strerror(errno));
    	printf("Camera SPI Setup failed.\n");
    	    return -1;
    }

    if (wiringPiSetup() == -1)
    {
        printf("wiringPiSetup(0 failed.\n");
	    return -1;
    }

    cam->ready = 1;
	return 0;
}

bool checkReply(Camera_t *cam, int cmd, int size) {
    int reply[size];
    int t_count = 0;
    int length = 0;
    int avail = 0;
    int timeout = 3 * TO_SCALE; // test 3 was 5

    while ((timeout != t_count) && (length != CAMERABUFFSIZ) && length < size)
    {
        avail = serialDataAvail(cam->fd);
        if (avail <= 0)
        {
            usleep(TO_U);
            t_count++;
            continue;
        }
        t_count = 0;
        // there's a byte!
        int newChar = serialGetchar(cam->fd);
        reply[length++] = (char)newChar;
    }

    //Check the reply
    if (reply[0] != 0x76 && reply[1] != 0x00 && reply[2] != cmd)
        return false;
    else
        return true;
}

void clearBuffer(Camera_t *cam) {
    int t_count = 0;
    int length = 0;
    int timeout = 2 * TO_SCALE;

    while ((timeout != t_count) && (length != CAMERABUFFSIZ))
    {
        int avail = serialDataAvail(cam->fd);
        if (avail <= 0)
        {
            t_count++;
            continue;
        }
        t_count = 0;
        // there's a byte!
        serialGetchar(cam->fd);
        length++;
    }
}

void reset(Camera_t *cam) {
    // Camera Reset method
    serialPutchar(cam->fd, (char)0x56);
    serialPutchar(cam->fd, (char)cam->serialNum);
    serialPutchar(cam->fd, (char)RESET);
    serialPutchar(cam->fd, (char)0x00);

    if (checkReply(cam, RESET, 5) != true)
        fprintf(stderr, "Check Reply Status: %s\n", strerror(errno));

    clearBuffer(cam);
}

void resumeVideo(Camera_t *cam)
{
    serialPutchar(cam->fd, (char)0x56);
    serialPutchar(cam->fd, (char)cam->serialNum);
    serialPutchar(cam->fd, (char)FBUF_CTRL);
    serialPutchar(cam->fd, (char)0x01);
    serialPutchar(cam->fd, (char)RESUMEFRAME);

    if (checkReply(cam, FBUF_CTRL, 5) == false)
        printf("Camera did not resume\n");
}

char * getVersion(Camera_t *cam)
{
	printf("getVersion() called.\n");
    serialPutchar(cam->fd, (char)0x56);
    serialPutchar(cam->fd, (char)cam->serialNum);
    serialPutchar(cam->fd, (char)GEN_VERSION);
    serialPutchar(cam->fd, (char)0x00);

    if (checkReply(cam, GEN_VERSION, 5) == false)
    {
        printf("CAMERA NOT FOUND!!!\n");
    }

    int counter = 0;
    cam->bufferLen = 0;
    int avail = 0;
    int timeout = 1 * TO_SCALE;

    while ((timeout != counter) && (cam->bufferLen != CAMERABUFFSIZ))
    {
        avail = serialDataAvail(cam->fd);
        if (avail <= 0)
        {
            usleep(TO_U);
            counter++;
            continue;
        }
        counter = 0;
        // there's a byte!
        int newChar = serialGetchar(cam->fd);
        cam->camerabuff[cam->bufferLen++] = (char)newChar;
    }

    cam->camerabuff[cam->bufferLen] = 0;
	printf("getVersion() returning.\n");
    return cam->camerabuff;
}

void setMotionDetect(Camera_t *cam, int flag)
{
    serialPutchar(cam->fd, (char)0x56);
    serialPutchar(cam->fd, (char)0x00);
    serialPutchar(cam->fd, (char)0x42);
    serialPutchar(cam->fd, (char)0x04);
    serialPutchar(cam->fd, (char)0x01);
    serialPutchar(cam->fd, (char)0x01);
    serialPutchar(cam->fd, (char)0x00);
    serialPutchar(cam->fd, (char)0x00);

    serialPutchar(cam->fd, (char)0x56);
    serialPutchar(cam->fd, (char)cam->serialNum);
    serialPutchar(cam->fd, (char)COMM_MOTION_CTRL);
    serialPutchar(cam->fd, (char)0x01);
    serialPutchar(cam->fd, (char)flag);

    clearBuffer(cam);
}

char * takePicture(Camera_t *cam, char * file_path)
{
    cam->frameptr = 0;

	printf("takePicture() called.\n");

    //Clear Buffer
    clearBuffer(cam);

    serialPutchar(cam->fd, (char)0x56);
    serialPutchar(cam->fd, (char)cam->serialNum);
    serialPutchar(cam->fd, (char)FBUF_CTRL);
    serialPutchar(cam->fd, (char)0x01);
    serialPutchar(cam->fd, (char)STOPCURRENTFRAME);

    if (checkReply(cam, FBUF_CTRL, 5) == false)
    {
        printf("Frame checkReply Failed\n");
        return &cam->empty;
    }

    serialPutchar(cam->fd, (char)0x56);
    serialPutchar(cam->fd, (char)cam->serialNum);
    serialPutchar(cam->fd, (char)GET_FBUF_LEN);
    serialPutchar(cam->fd, (char)0x01);
    serialPutchar(cam->fd, (char)0x00);

    if (checkReply(cam, GET_FBUF_LEN, 5) == false)
    {
        printf("FBUF_LEN REPLY NOT VALID!!!\n");
        return &cam->empty;
    }

    while(serialDataAvail(cam->fd) <= 0){;}

    printf("Serial Data Avail %d \n", serialDataAvail(cam->fd));

    unsigned int len;
    len = serialGetchar(cam->fd);
    len <<= 8;
    len |= serialGetchar(cam->fd);
    len <<= 8;
    len |= serialGetchar(cam->fd);
    len <<= 8;
    len |= serialGetchar(cam->fd);

    printf("Length %u \n", len);

    if(len > 20000){
        printf("To Large... \n");
        resumeVideo(cam);
        clearBuffer(cam);
        return takePicture(cam, file_path);
    }
    //char image[len];
    char * image = malloc(len+1);
    //image[len+1] = NULL;

    int imgIndex = 0;

    while (len > 0)
    {
        unsigned int readBytes = len;

        serialPutchar(cam->fd, (char)0x56);
        serialPutchar(cam->fd, (char)cam->serialNum);
        serialPutchar(cam->fd, (char)READ_FBUF);
        serialPutchar(cam->fd, (char)0x0C);
        serialPutchar(cam->fd, (char)0x0);
        serialPutchar(cam->fd, (char)0x0A);
        serialPutchar(cam->fd, (char)(cam->frameptr >> 24 & 0xff));
        serialPutchar(cam->fd, (char)(cam->frameptr >> 16 & 0xff));
        serialPutchar(cam->fd, (char)(cam->frameptr >> 8 & 0xff));
        serialPutchar(cam->fd, (char)(cam->frameptr & 0xFF));
        serialPutchar(cam->fd, (char)(readBytes >> 24 & 0xff));
        serialPutchar(cam->fd, (char)(readBytes >> 16 & 0xff));
        serialPutchar(cam->fd, (char)(readBytes >> 8 & 0xff));
        serialPutchar(cam->fd, (char)(readBytes & 0xFF));
        serialPutchar(cam->fd, (char)(CAMERADELAY >> 8));
        serialPutchar(cam->fd, (char)(CAMERADELAY & 0xFF));

        if (checkReply(cam, READ_FBUF, 5) == false)
        {
	    printf("VC0706: Error! checkReply(cam, READ_FBUF, 5) returned false.\n");
            return &cam->empty;
        }

        int counter = 0;
        cam->bufferLen = 0;
        int avail = 0;
        int timeout = 20 * TO_SCALE;

        while ((timeout != counter) && cam->bufferLen < readBytes)
        {
            avail = serialDataAvail(cam->fd);

            if (avail <= 0)
            {
                usleep(TO_U);
                counter++;
                continue;
            }
            counter = 0;
            int newChar = serialGetchar(cam->fd);
            //printf("VC0706: writing byte %x to image[%d]\n", newChar, imgIndex);
	    image[imgIndex++] = (char)newChar;
	    //memcpy((&image + (sizeof(image[0])*imgIndex++)), (char)&newChar, sizeof(char));
	    //printf("VC0706: image[%d]: %x\n\n", imgIndex-1, image[imgIndex-1]);

            cam->bufferLen++;
        }

        cam->frameptr += readBytes;
        len -= readBytes;

        if (checkReply(cam, READ_FBUF, 5) == false)
        {
            printf("ERROR READING END OF CHUNK| start: %u | length: %u\n", cam->frameptr, len);
        }
    }

    printf("VC0706_CORE: Attempting to open file...\n");
    FILE *jpg = fopen(file_path, "w");
    if (jpg != NULL)
    {
	// test
	int i=0;
	while(image[i] != '\0')
	{
		i++;
	}
	//printf("VC0706: Manual determined length of image: %d bytes\n", i);
        size_t stored = fwrite(image, sizeof(image[0]), imgIndex, jpg);
        fclose(jpg);
	//printf("VC0706: number of stored bytes:%zu\n", stored);
	if((size_t)i != stored)
	{
    	    printf("VC0706 ERROR: image stored %zu bytes, expected to store %d bytes\n", stored, i);
	}
    }
    else
    {
        printf("IMAGE COULD NOT BE OPENED/MADE!\n");
    }

    printf("VC0706: copying file_path <%s> to imageName\n", file_path);
    strcpy(cam->imageName, file_path);
    resumeVideo(cam);

    //Clear Buffer
    clearBuffer(cam);

    return cam->imageName;
}

