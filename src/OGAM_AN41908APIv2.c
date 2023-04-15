/**
 * @file OGAM_AN41908v2.c
 *
 * Panasonics AN41908 motor driver using a simple SPI interface.
 * @author Volkan OKBAY <volkan.okbay@metu.edu.tr>
 *
 * Copyright (C) 2015 QWERTY Embedded Design
 *
 * Based on a similar utility written for EMAC Inc. SPI class.
 *
 * Copyright (C) 2009 EMAC, Inc.
 */
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <linux/spi/spidev.h>
#include <unistd.h>
#include <stdlib.h>
//#include <time.h>
#include <sys/time.h>

#define MAX_LENGTH 64 // SPI kelimeleri uzunluğu
#define OSCIN 27000000 // OSCIN frekansı
#define F_MAX 10000 // NOT USED
#define Z_MAX 15000 // NOT USED
#define I_MAX 1023 // Max iris değeri

// Focus,zoom ve iris değerleri 
int Fpos; 
int Zpos;
int Ipos;
int fd; // SPI device id

/* ************************ */
/*      SPI FUNCTIONS       */
/* ************************ */
// Buradaki fonksiyonlar tamamen spi protokolü ile alakalıdır. Bir kısmı hazır alınmıştır.

__u8 char2hex(char value)
{
	__u8 retval; 

	if (value >= '0' && value <= '9')
		retval = value - '0';
	else if (value >= 'A' && value <= 'F')
		retval = value - 'A' + 0xA;
	else if (value >= 'a' && value <= 'f')
		retval = value - 'a' + 0xA;
	else
		retval = 0xF;

	return retval;
}

void string2hex(char *str, __u8 *hex, int length)
{
	int i;
	__u8 nibble;

	for (i = 0; i < (length * 2); i++) {
		if (i < strlen(str))
			nibble = char2hex(str[i]);
		else
			nibble = 0xf;
		if (i % 2 == 0)
			hex[i / 2] = nibble;
		else
			hex[i / 2] = (hex[i / 2] << 4) | nibble;
	}
}

void print_spi_transaction(__u8 *miso, __u8 *mosi, __u32 length)
{
	int i;

	printf("MOSI  MISO\n");
	for (i = 0; i < length; i++)
		printf("%.2X  : %.2X\n", mosi[i], miso[i]);
}

void spi_send(char *mosi_str,int fd) // SPI kelime gönderme
{
	int ret;
	__u8 miso[MAX_LENGTH];
	__u8 mosi[MAX_LENGTH];
	
		struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)mosi,
		.rx_buf = (unsigned long)miso,
		.delay_usecs = 1,
		.len = 3,
		.speed_hz=1000000,
	};
	
	
	string2hex(mosi_str, mosi, tr.len);
	// Ekrana her SPI iletimini basmak için bu iki printi uncomment edin.
	//printf("Sending to %s at %ld Hz\n", "/dev/spidev2.0", tr.speed_hz); 
	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	//print_spi_transaction(miso, mosi, tr.len);
	
}

int spi_send_and_check(char *mosi_str,int fd, char* in) // SPI gönder ve in ile karşılaştır. Doğru ise 0 döner.
{
	char out[6];
	int ret;
	__u8 miso[MAX_LENGTH];
	__u8 mosi[MAX_LENGTH];
	
		struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)mosi,
		.rx_buf = (unsigned long)miso,
		.delay_usecs = 1,
		.len = 3,
		.speed_hz=1000000,
	};
	
	string2hex(mosi_str, mosi, tr.len);

	//printf("Sending to %s at %ld Hz\n", "/dev/spidev2.0", tr.speed_hz);
	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	//print_spi_transaction(miso, mosi, tr.len);
	sprintf(out,"%.2X%.2X%.2X",miso[0],miso[1],miso[2]);
	return strcmp(in, out);
}

int setupSPI()
{
	char *device_name = "/dev/spidev2.0";
	fd = open(device_name, O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "*ERROR: Cannot open SPI device file: %s: %s\n",
		device_name, strerror(errno));
		return -1;
	}
	else
	{
		printf("*INFO: SPI device successfully configured.\n");
		return fd;
	}
}

/* ************************ */
/* AN41908 DRIVER FUNCTIONS */
/* ************************ */
// Bizim yazdığımız sürücü fonksiyonları

void resetRSTB() // NOT USED: RSTB pinini resetlemek için yazıldı ancak SPI bozuyor.
{
	printf("*INFO: Resetting RSTB pin.\n");
	int pin = open("/sys/class/gpio/gpio27/value", O_WRONLY);
	write(pin, "1", 1);
	usleep(100);
	write(pin, "0", 1);
	return;
}

void initIris() //Dokümandaki iris register değerlerini yazar sonra da kontrol ederiz.
{
	printf("*INFO: Initializing iris registers.\n");
	// Iris motor initialization
	spi_send("018A7C",fd);
	spi_send("02F066",fd);
	spi_send("03100E",fd);
	spi_send("04FF80",fd);
	spi_send("052400",fd);
	spi_send("0B8004",fd);
	spi_send("0E0003",fd);
	if((spi_send_and_check("410000",fd,"008A7C") == 0) &&
	(spi_send_and_check("420000",fd,"00F066") == 0) &&
	(spi_send_and_check("430000",fd,"00100E") == 0) &&
	(spi_send_and_check("440000",fd,"00FF80") == 0) &&
	(spi_send_and_check("450000",fd,"002400") == 0) &&
	(spi_send_and_check("4B0000",fd,"008004") == 0) &&
	(spi_send_and_check("4E0000",fd,"000003") == 0))
		 printf("*INFO: Iris registers configured correctly via SPI.\n");
	else printf("*ERROR: Iris registers are not configured correctly via SPI.\n");
	return;

}

void initFocusZoom() //Dokümandaki f&z register değerlerini yazar sonra da kontrol ederiz.
{
	printf("*INFO: Initializing focus&zoom registers.\n");
	// Focus&zoom motor initialization
	spi_send("20015C",fd);
	spi_send("218700",fd);	
	spi_send("220100",fd);	
	spi_send("23FFFF",fd);	
	spi_send("270100",fd);	
	spi_send("28FFFF",fd);
	if((spi_send_and_check("600000",fd,"00015C") == 0) &&
	(spi_send_and_check("610000",fd,"008700") == 0) &&
	(spi_send_and_check("620000",fd,"000100") == 0) &&
	(spi_send_and_check("630000",fd,"00FFFF") == 0) &&
	(spi_send_and_check("670000",fd,"000100") == 0) &&
	(spi_send_and_check("680000",fd,"00FFFF") == 0))
		 printf("*INFO: FZ registers configured correctly via SPI.\n");
	else printf("*ERROR: FZ registers are not configured correctly via SPI.\n");
	return;
}

int setIris(int VAL) // Iris değerini set eder
{
	int word; // SPI word
	char words[6];
	char words2[] = "00"; // iris adresi

	printf("--IRIS--\n");
	if ((VAL < 0) || (VAL > I_MAX)) // Deger aralıgı kontrol edilir.
		printf("*WARNING: Iris value is not between 0-1023.\n");

	// Create SPI word
	word = 0;
	word |= ((VAL & 0xFF) << 8); // Son 8 bit kısmına değer yerleştirilir.
	word |= (VAL >> 8); // Ancak endian formatına uygun yerleştirmeye dikkat et.
	sprintf(words,"%04X",word);
	strcat(words2,words); // Adresi başa koyduk
	printf("*INFO: Iris SPI word -> %s\n",words2);

	// Send word
	spi_send(words2,fd);

	// Read and check written word
	if(spi_send_and_check("400000", fd, words2) == 0)
	{
		// Trigger VD_IS (GPIO19)
		int gp = open("/sys/class/gpio/gpio19/value", O_WRONLY);
		write(gp, "1", 1);
	    	write(gp, "0", 1);
		close(gp);
		Ipos = VAL;
		printf("*INFO: Iris updated to %d.\n", Ipos);
		return 0;
	}
	else
	{
		printf("*ERROR: Iris SPI word is not written correctly!\n");
		return -1;
	}
}

int getIris() // iris değerini döner
{
	return Ipos;
}

int* calRegs(int* VD, int speed, int target, char* mode) // f&z için registerlara yazılacak degerler hesaplanır
{
	int PSUM, INTCT, CCWCW, numVD;
	static int regs[4]  = {-1, -1, -1, -1};
	// Find PSUM and INTCT depending on speed selection
	switch (speed)
	{
		case 1:
		{
			PSUM = 1;
			INTCT = OSCIN / (*VD * PSUM * 24);
			break;
		}
		case 2:
		{
			PSUM = 8;
			INTCT = OSCIN / (*VD * PSUM * 24);
			break;
		}
		case 3:
		{
			PSUM = 32;
			INTCT = OSCIN / (*VD * PSUM * 24);
			break;
		}
		case 4:
		{
			PSUM = 64;
			INTCT = OSCIN / (*VD * PSUM * 24);
			break;
		}
		case 5:
		{
			PSUM = 128;
			INTCT = OSCIN / (*VD * PSUM * 24);
			break;
		}
		case 6:
		{
			PSUM = 255;
			INTCT = OSCIN / (*VD * PSUM * 24);
			break;
		}
		case 7: // Async speed mode
		{
			PSUM = 255;
			INTCT = 110;
			*VD = OSCIN / (INTCT * PSUM * 24); //40 Hz
			break;
		}
		default:
		{
			printf("*ERROR: Wrong FZ speed.\n");
			return regs;
		}
	}
	
	// güncel ve hedef pozisyon arası yolu hesapla
	numVD = 0;	
	if ((strcmp(mode, "z") == 0) || (strcmp(mode, "Z") == 0))
	{
		numVD = target - Zpos;
	}
	else if ((strcmp(mode, "f") == 0) || (strcmp(mode, "F") == 0))
	{
		numVD = target - Fpos;	
	}
	else
	{
		printf("*WARNING: Wrong input for mode selection.\n");
	}
	// yönü bul
	if (numVD < 0)	CCWCW = 1;
	else 			CCWCW = 0;
	// kaç adet VD atılması gerektiğini hesapla
	numVD = abs(numVD)/PSUM;
	// outputları düzenle
	printf("*INFO: PSUM: %d INTCT: %d CCWCW: %d Operation duration: %d periods of %d Hz.\n", PSUM, INTCT, CCWCW, numVD, *VD);
	regs[0] = PSUM;
	regs[1] = INTCT;
	regs[2] = CCWCW;
	regs[3] = numVD;
	return regs;
}


void createWords(char *words[], int PSUM, int INTCT, int CCWCW, char* mode) // f&z için spi kelimelerini oluşturur
{
	int word1;
	int word2;
	char word1s[6];
	char word2s[6];

	char BRAKE;
	char ENDIS;
	unsigned char MICRO;

	BRAKE = 0x0; // 0:normal, 1:brake
	ENDIS = 0x1; // 0:off, 1:on
	MICRO = 0x0; // 0x0: or 0x1: 256, 0x2: 128, 0x3: 64
	// f&z için adreslemeler
	if ((strcmp(mode, "z") == 0) || (strcmp(mode, "Z") == 0))
	{
		printf("--ZOOM--\n");
		word1 = 0x240000;
		word2 = 0x250000;
	}
	else if ((strcmp(mode, "f") == 0) || (strcmp(mode, "F") == 0)) 
	{
		printf("--FOCUS--\n");
		word1 = 0x290000;
		word2 = 0x2A0000;
	}
	// register değerlerini kelimelere yerleştir
	word1 |= (PSUM << 8);
	word1 |= CCWCW;
	word1 |= (BRAKE << 1);
	word1 |= (ENDIS << 2);
	word1 |= (MICRO << 4);
	sprintf(word1s,"%X",word1);
	words[0] = word1s;
	printf("*INFO: SPI word1 -> %s \n", words[0]);
	
	word2 |= ((INTCT & 0xFF) << 8);
	word2 |= (INTCT >> 8);
	sprintf(word2s,"%X",word2);
	words[1] = word2s;
	printf("*INFO: SPI word2 -> %s \n", words[1]);
	return;

}

void moveLens(int gp, int VD, int numVD) // VD_FZ tetikleme fonksiyonu
{
	printf("*INFO: Moving lens.\n");
	struct timeval t1,t2;
	gettimeofday(&t1, NULL);
	for(int i=0;i<numVD;i++) // numVD tane periyot sayar
	{
		write(gp, "1", 1);
		usleep(80);
    		write(gp, "0", 1);
		usleep(1000000/VD-80-50); // VD periyotu kadar bekle (80 us uptime telafisi için, 50 bir sonraki SPI yazımı için)
	}
	gettimeofday(&t2, NULL);
	printf("*INFO: Elapsed time (us) %f\n", 1e6*(t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec));
	return;
}

int updatePos(int PSUM, int numVD, int CCWCW, char* mode) // Başarılı hareket sonrası içeride tutulan pozisyon değerini günceller
{
	if ((strcmp(mode, "z") == 0) || (strcmp(mode, "Z") == 0))
	{
		if (CCWCW == 1) Zpos = Zpos - PSUM * numVD;
		else if (CCWCW == 0) Zpos = Zpos + PSUM * numVD;
		return Zpos;
	}
	else if ((strcmp(mode, "f") == 0) || (strcmp(mode, "Z") == 0))
	{
		if (CCWCW == 1) Fpos = Fpos - PSUM * numVD;
		else if (CCWCW == 0) Fpos = Fpos + PSUM * numVD;
		return Fpos;
	}
}

void clearRegs(int gp, char* mode, int VD) // kullanım sonrası spi registerlarındaki bilgileri sıfırlayıp bir periyot triggerlama yapar (lens hareketi olmaz)
{
	char* words[2];
	// Clear Registers via SPI
	printf("*INFO: Clearing register values.\n");
	createWords(words, 0, 0, 0, mode);
	spi_send(words[0],fd); // Step count
	spi_send(words[1],fd); // Step cycle
	moveLens(gp, VD, 1); // One period to update register
	return;
}

int setFZ(int SPD, int POS, char* mode, int VD, int CCWCW, int numVD) // f&z lenslerini hareket ettirmek için ana fonksiyon
{
	int gp = open("/sys/class/gpio/gpio24/value", O_WRONLY); // VD_FZ pini
	char* words[2];
	int* regs;

	if ((strcmp(mode, "i") == 0) || (strcmp(mode, "I") == 0)) // init mod, pozisyonları sıfıra çeker
	{
		printf("--FZ INIT MODE--\n");
		createWords(words, 255, 200, 1, "f");
		spi_send(words[0],fd); // Step count
		spi_send(words[1],fd); // Step cycle

		createWords(words, 255, 200, 1, "z");
		spi_send(words[0],fd); // Step count
		spi_send(words[1],fd); // Step cycle
		moveLens(gp, 27000000 / (255 * 200 * 24), 60);
		Fpos = 0;
		Zpos = 0;
		clearRegs(gp, "f", 27000000 / (255 * 200 * 24));
		clearRegs(gp, "z", 27000000 / (255 * 200 * 24));
	}
	else if ((strcmp(mode, "tf") == 0) || (strcmp(mode, "TF") == 0)) // focus test modu, formül kullanmadan direkt registerlara yazmak için
	{
		printf("--FZ TEST MODE--\n");
		printf("*INFO: PSUM: %d INTCT: %d CCWCW: %d Operation duration: %d periods of %d Hz.\n", SPD, POS, CCWCW, numVD, 27000000 / (SPD * POS * 24));
		createWords(words, SPD, POS, CCWCW, "f");
		spi_send(words[0],fd); // Step count
		spi_send(words[1],fd); // Step cycle
		moveLens(gp, 27000000 / (SPD * POS * 24), numVD);
		updatePos(SPD, numVD, CCWCW, "f");
		clearRegs(gp, "f", 27000000 / (SPD * POS * 24));
	}
	else if ((strcmp(mode, "tz") == 0) || (strcmp(mode, "TZ") == 0)) // zoom test modu, formül kullanmadan direkt registerlara yazmak için
	{
		printf("--FZ TEST MODE--\n");
		printf("*INFO: PSUM: %d INTCT: %d CCWCW: %d Operation duration: %d periods of %d Hz.\n", SPD, POS, CCWCW, numVD, 27000000 / (SPD * POS * 24));
		createWords(words, SPD, POS, CCWCW, "z");
		spi_send(words[0],fd); // Step count
		spi_send(words[1],fd); // Step cycle
		moveLens(gp, 27000000 / (SPD * POS * 24), numVD);
		updatePos(SPD, numVD, CCWCW, "z");
		clearRegs(gp, "z", 27000000 / (SPD * POS * 24));
	}
	else
	{
		printf("--FZ NORMAL MODE--\n"); // f&z hareketi için standart mod
		for(int i = SPD; i > 0; i--)
		{
			regs = calRegs(&VD, i, POS, mode);
			if (regs[3] > 0)
			{
				createWords(words, regs[0], regs[1], regs[2], mode);
				spi_send(words[0],fd); // Step count
				spi_send(words[1],fd); // Step cycle
				moveLens(gp, VD, regs[3]);
				if(POS == updatePos(regs[0], regs[3], regs[2], mode))
					break;
			}
		}
		clearRegs(gp, mode, VD);
	}
    close(gp);
    printf("*INFO: Setting done. Focus pos: %d Zoom pos: %d.\n", Fpos, Zpos);
    return 0;
}

int* trapezoid_profile(int Amax, int Vmax, int displacement, int* v_array_len, int* num_stp_max)
{
    const int MAX_RAMP_LEN = 10;
    static int v_array[21]; //2*MAX_RAMP_LEN+1

    int num_stp, num_stp_plus, num_stp_base, base_stp;
    int area1, area2, area;
    int rem_total, rem_flag;

    // Üçgen adımlarını hesaplama
    num_stp = MAX_RAMP_LEN + 1 - Amax;
    base_stp = Vmax / num_stp;
    num_stp_plus = Vmax - (base_stp*num_stp);
    num_stp_base = num_stp - num_stp_plus;
    // Üçgen alanı hesaplama
    area1 = base_stp * num_stp_base * (num_stp_base + 1) / 2;
    area2 = base_stp * num_stp_base * num_stp_plus + (base_stp + 1) * num_stp_plus * (num_stp_plus + 1) / 2;
    area = area1 + area2;
    int critical_area = (2*area - Vmax);

    // Profile karar verme
    int n = 0;
    int buffer = 0;
    rem_flag = 0;
    if(displacement>=critical_area) // En az bir Vmax var.
    {
        *num_stp_max = (displacement - critical_area) / Vmax + 1;
        rem_total   = (displacement - critical_area) % Vmax;

        if (rem_total>0)    *v_array_len = 2*(num_stp_base+num_stp_plus);
        else                *v_array_len = 2*(num_stp_base+num_stp_plus) - 1;

        for(n=0;n<*v_array_len;n++)
        {
            if (n<num_stp_base)                                 v_array[n] = (n+1)*base_stp;
            else if (n<num_stp_base+num_stp_plus)               v_array[n] = num_stp_base*base_stp+(n+1-num_stp_base)*(base_stp+1);
            else if (n<num_stp_base+2*num_stp_plus)
            {
                buffer = num_stp_base*base_stp+((num_stp_base+2*num_stp_plus)-n-1)*(base_stp+1);
                if ((rem_total<v_array[n-1]) && (rem_total>=buffer) && (rem_flag==0))
                {
                    v_array[n] = rem_total;
                    rem_flag = 1;
                }
                v_array[n+rem_flag] = buffer;
            }
            else if (n<2*num_stp_base+2*num_stp_plus)
            {
                buffer = ((2*num_stp_base+2*num_stp_plus)-n-1)*base_stp;
                if ((rem_total<v_array[n-1]) && (rem_total>=buffer) && (rem_flag==0))
                {
                    v_array[n] = rem_total;
                    rem_flag = 1;
                }
                v_array[n+rem_flag] = buffer;
            }
        }
    }
    else // Vmax'a ulasilamiyor.
    {
        *num_stp_max = 0;
        rem_flag    = -1;
        rem_total   = displacement;
        for(n=0;n<(num_stp_base+num_stp_plus);n++)
        {
            if (n<num_stp_base)                     buffer = (n+1)*base_stp;
            else if (n<num_stp_base+num_stp_plus)   buffer = num_stp_base*base_stp+(n+1-num_stp_base)*(base_stp+1);

            if (rem_total<=buffer)
            {
                v_array[n] = rem_total;
                *v_array_len = 2*n+1;
                rem_flag = n;
                break;
            }
            else if (rem_total<=(2*buffer))
            {
                v_array[n]      = buffer;
                v_array[20-n]   = rem_total - buffer;
                *v_array_len    = 2*(n+1);
                rem_total       = rem_total - buffer;
                rem_flag        = 20-n;
                break;
            }
            else
            {
                v_array[n]      = buffer;
                v_array[20-n]   = buffer;
                rem_total       -= 2*buffer;
            }
        }

        for(n=(*v_array_len+1)/2;n<*v_array_len;n++)
        {
            v_array[n] = v_array[21 - *v_array_len + n];
            if (rem_flag == 21 - *v_array_len + n)
            {
                rem_flag = n;
            }
            else
            {
                if (rem_total<v_array[n])
                {
                    v_array[rem_flag]   = v_array[n];
                    v_array[n]          = rem_total;
                    rem_flag            = n;
                }
            }
        }
    }
    return v_array;

}

int setFZv2(int Amax, int Vmax, int POS, char* mode, int VD) // f&z lenslerini hareket ettirmek için ana fonksiyon
{
	int gp = open("/sys/class/gpio/gpio24/value", O_WRONLY); // VD_FZ pini
	char* words[2];
	int* regs;
	int ax, vx; // İvme ve hız maksimum değerleri
	
        int* velocity;
        int velocity_len;
        int number_of_max_steps;
        
        int PSUM, INTCT, CCWCW;
        int displacement = 0;
	
	if (Amax > 10) 	ax = 10;
	else if (Amax < 1) 	ax = 1;
	else 			ax = Amax;
	
	if (Vmax > 255) 	vx = 255;
	else if (Vmax < 1) 	vx = 1;
	else 			vx = Vmax;
	
	if ((strcmp(mode, "z") == 0) || (strcmp(mode, "Z") == 0))
	{
		displacement = POS - Zpos;

	}
	else if ((strcmp(mode, "f") == 0) || (strcmp(mode, "F") == 0))
	{
		displacement = POS - Fpos;	
	}
	else
	{
		printf("*WARNING: Wrong input for mode selection.\n");
	}
	
	if (displacement < 0)
	{
		CCWCW = 1;
		displacement = -displacement;
	}
	else	CCWCW = 0;	
	
	velocity = trapezoid_profile(ax,vx,displacement,&velocity_len, &number_of_max_steps);
	
	printf("--FZ NORMAL MODE with Trapezoidal Profile--\n"); // f&z hareketi için standart mod

	int disp = 0; 
	for(int k=0;k<velocity_len;k++)
	{
		printf("Velocity: %d\n",velocity[k]);
		disp += velocity[k];
		
		PSUM = velocity[k];
		INTCT = OSCIN / (VD * PSUM * 24);
		
		createWords(words, PSUM, INTCT, CCWCW, mode);
		spi_send(words[0],fd); // Step count
		spi_send(words[1],fd); // Step cycle
		moveLens(gp, VD, 1);
		updatePos(PSUM, 1, CCWCW, mode);
		
		if (velocity[k] == Vmax)
		{
		    for(int m=0;m<number_of_max_steps-1;m++)
		    {
		        printf("Velocity: %d\n",Vmax);
		        disp += Vmax;

        		PSUM = Vmax;
			INTCT = OSCIN / (VD * PSUM * 24);
			
			createWords(words, PSUM, INTCT, CCWCW, mode);
			spi_send(words[0],fd); // Step count
			spi_send(words[1],fd); // Step cycle
			moveLens(gp, VD, 1);
			updatePos(PSUM, 1, CCWCW, mode);
		    }
		}
	}
	printf("\nDISPLACEMENT: %d\n",disp);
		
	clearRegs(gp, mode, VD);
	close(gp);
	printf("*INFO: Setting done. Focus pos: %d Zoom pos: %d.\n", Fpos, Zpos);
	return 0;
}

int getFZ(char* mode) // f&z güncel pozisyonları döner
{
	if ((strcmp(mode, "f") == 0) || (strcmp(mode, "F") == 0))
	{
		return Fpos;
	}
	else if ((strcmp(mode, "z") == 0) || (strcmp(mode, "Z") == 0))
	{
		return Zpos;
	}
}

int initDriver(char* opt) // tüm initialization fonksiyonlarını çalıştırır ve istenirse f&z sıfır pozisyonuna çeker, irisi 200'e çeker.
{
	printf("*INFO: Initializing driver.\n");
	//resetRSTB();
	if ( setupSPI() == -1)
	{
		return -1;
	}
	else
	{
		initIris();
		initFocusZoom();
		if (strcmp(opt, "init_pos") == 0)
		{
			setIris(200);
			setFZ(-1, -1, "i", -1, -1, -1);
		}
		return 0;
	}
}

/*JUNK
struct timeval t1,t2;
gettimeofday(&t1, NULL);
gettimeofday(&t2, NULL);
printf("%f usec\n", 1e6*(t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec));

	//INTCT = OSCIN / (ROT * 768);
	//printf("int: %d \n",INTCT);
	//PSUM = OSCIN / (VD * INTCT * 24);
	//if (PSUM > 255) PSUM = 255;
	//if (PSUM != 0 ) INTCT = OSCIN / (VD * PSUM * 24);

*/
