/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include "stdio.h"     
#include "string.h"    
#include "unistd.h"    
#include "fcntl.h"     
#include "errno.h"     
#include "sys/types.h"
#include "sys/stat.h"
#include "stdlib.h"
#include "stdarg.h"
#include "termios.h"

int open_serial(char * serial) //ex.: "/dev/ttyS1"
{
    int fdserial;
    struct termios tty_attributes;

    if ((fdserial = open(serial, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0) {
        fprintf(stderr, "Open error on %s\n", strerror(errno));
        printf("\nOpen error on %s\n", strerror(errno));
        return 0; //exit(EXIT_FAILURE);
    } 
    else     {
        tcgetattr(fdserial, &tty_attributes);
        // c_cflag
        // Enable receiver
        tty_attributes.c_cflag |= CREAD;
        // 8 data bit
        tty_attributes.c_cflag |= CS8;
        // c_iflag
        // Ignore framing errors and parity errors. 
        tty_attributes.c_iflag |= IGNPAR;
        // c_lflag
        // DISABLE canonical mode.
        // Disables the special characters EOF, EOL, EOL2, 
        // ERASE, KILL, LNEXT, REPRINT, STATUS, and WERASE, and buffers by lines.
        // DISABLE this: Echo input characters.
        tty_attributes.c_lflag &= ~(ICANON);
        tty_attributes.c_lflag &= ~(ECHO);
        // DISABLE this: If ICANON is also set, the ERASE character erases the preceding input  
        // character, and WERASE erases the preceding word.
        tty_attributes.c_lflag &= ~(ECHOE);
        // DISABLE this: When any of the characters INTR, QUIT, SUSP, or DSUSP are received, generate the corresponding signal. 
        tty_attributes.c_lflag &= ~(ISIG);
        // Minimum number of characters for non-canonical read.
        tty_attributes.c_cc[VMIN] = 1;
        // Timeout in deciseconds for non-canonical read.
        tty_attributes.c_cc[VTIME] = 0;
        // Set the baud rate
        cfsetospeed(&tty_attributes, B9600);
        cfsetispeed(&tty_attributes, B9600);
        tcsetattr(fdserial, TCSANOW, &tty_attributes);
    }
    return fdserial;

}

char *serial_read(int fdserial) {
    /*abre a serial*/
char rxBuffer[10];
char str[256];
memset(str,0, sizeof(str));
int countchar = 0;
    while(read(fdserial, &rxBuffer, 1) == 1) 
    {
        str[countchar] =rxBuffer[0];
        countchar++;        
    }
if (strlen(str)>0)
    return strdup(str);
else
    return NULL;            
}

/*if (countchar >= 0 && reception)
            datareceived[countchar] = rxBuffer[0];
        countchar++;
        if (data_ready) {
            char line_cmd[64];
            memset(line_cmd, 0, sizeof (line_cmd));
            strcpy(line_cmd, "/ajm/i2cwrite -d ");
            strcat(line_cmd, datareceived);
            int val_ret = system(line_cmd);
            fprintf(log_stream, "Debug: O programa executado %s retornou o valor: %d\n", line_cmd, val_ret);
            data_ready = false;*/