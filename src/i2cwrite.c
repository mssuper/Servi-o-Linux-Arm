/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <getopt.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>    // read/write usleep
#include <stdlib.h>    // exit function
#include <inttypes.h>  // uint8_t, etc
#include <linux/i2c-dev.h> // I2C bus definitions
#include <stdbool.h>



bool writedata(int dataw) {

    int mcp4725_address = 0x62;
    uint8_t writeBuf[3];
    int fd;
    char *i2c_path = "/dev/i2c-0";
    // open device on /dev/i2c-1 the default on Raspberry Pi B
    if ((fd = open(i2c_path, O_RDWR)) < 0) {
        printf("Erro: Não foi possivel abrir dispositivo! %d\n", fd);
        exit(0x1);
    }
    printf("Dispositivo: %s\n", i2c_path);

    // connect to ads1115 as i2c slave
    if (ioctl(fd, I2C_SLAVE, mcp4725_address) < 0) {
        printf("Erro: Endereço inexistente!\n");
        exit(0x2);
    }
    printf("Endereço: 0x%x\n", mcp4725_address);

    // 12-bit device values from 0-4095

    // page 18-19 spec sheet
    writeBuf[0] = 0b01000000; // control byte
    // bits 7-5; 010 write DAC; 011 write DAC and EEPROM
    // bits 4-3 unused
    // bits 2-1 PD1, PD0 PWR down P19 00 normal.
    // bit 0 unused

    writeBuf[1] = 0b00000000; // HIGH data
    // bits 7-0 D11-D4


    writeBuf[2] = 0b00000000; // LOW data
    // bits 7-4 D3-D0
    // bits 3-0 unused

    // input number from 0-4095
    // 2048 50% Vcc
    // string to int
    // write number to MCP4725 DAC
    writeBuf[1] = dataw >> 4; // MSB 11-4 shift right 4 places
    writeBuf[2] = dataw << 4; // LSB 3-0 shift left 4 places
    if (write(fd, writeBuf, 3) != 3) {
        perror("Erro ao escrever no registrador 1\n");
        exit(0x3);
    }
    printf("MCP-4725 ajustado para:  %d  \n", dataw);
    close(fd);
    return true;

}

