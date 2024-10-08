#include <gpio.h>
#include <stdarg.h>
//#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#define NULL 0

/*  *********************************************************************
    *  Externs								*
    ********************************************************************* */

/*  *********************************************************************
    *  Globals								*
    ********************************************************************* */

static const char digits[17] = "0123456789ABCDEF";
static const char ldigits[17] = "0123456789abcdef";

int (*xprinthook)(const char *str) = NULL;

/*  *********************************************************************
    *  __atox(buf,num,radix,width)
    *
    *  Convert a number to a string
    *
    *  Input Parameters:
    *      buf - where to put characters
    *      num - number to convert
    *      radix - radix to convert number to (usually 10 or 16)
    *      width - width in characters
    *
    *  Return Value:
    *      number of digits placed in output buffer
    ********************************************************************* */
static int __atox(char *buf,unsigned int num,unsigned int radix,int width,
		     const char *digits)
{
    char buffer[16];
    char *op;
    int retval;

    op = &buffer[0];
    retval = 0;

    do {
	*op++ = digits[num % radix];
	retval++;
	num /= radix;
	} while (num != 0);

    if (width && (width > retval)) {
	width = width - retval;
	while (width) {
	    *op++ = '0';
	    retval++;
	    width--;
	    }
	}

    while (op != buffer) {
	op--;
	*buf++ = *op;
	}

    return retval;
}


/*  *********************************************************************
    *  __llatox(buf,num,radix,width)
    *
    *  Convert a long number to a string
    *
    *  Input Parameters:
    *      buf - where to put characters
    *      num - number to convert
    *      radix - radix to convert number to (usually 10 or 16)
    *      width - width in characters
    *
    *  Return Value:
    *      number of digits placed in output buffer
    ********************************************************************* */
static int __llatox(char *buf,unsigned long long num,unsigned int radix,
		    int width,const char *digits)
{
    char buffer[16];
    char *op;
    int retval;

    op = &buffer[0];
    retval = 0;

#if CPUCFG_REGS32
    /*
     * Hack: to avoid pulling in the helper library that isn't necessarily
     * compatible with PIC code, force radix to 16, use shifts and masks
     */
    do {
	*op++ = digits[num & 0x0F];
	retval++;
	num >>= 4;
	} while (num != 0);
#else
    do {
	*op++ = digits[num % radix];
	retval++;
	num /= radix;
	} while (num != 0);
#endif

    if (width && (width > retval)) {
	width = width - retval;
	while (width) {
	    *op++ = '0';
	    retval++;
	    width--;
	    }
	}

    while (op != buffer) {
	op--;
	*buf++ = *op;
	}

    return retval;
}

/*  *********************************************************************
    *  xvsprintf(outbuf,template,arglist)
    *
    *  Format a string into the output buffer
    *
    *  Input Parameters:
    *	   outbuf - output buffer
    *      template - template string
    *      arglist - parameters
    *
    *  Return Value:
    *      number of characters copied
    ********************************************************************* */
#define isdigit(x) (((x) >= '0') && ((x) <= '9'))
int xvsprintf(char *outbuf,const char *templat,va_list marker)
{
    char *optr;
    const char *iptr;
    unsigned char *tmpptr;
    long x;
    unsigned long ux;
    unsigned long long ulx;
    int i;
    long long ll;
    int leadingzero;
    int leadingnegsign;
    int islong;
    int width;
    int width2 = 0;
    int hashash = 0;

    optr = outbuf;
    iptr = templat;

    while (*iptr) {
	if (*iptr != '%') {*optr++ = *iptr++; continue;}

	iptr++;

	if (*iptr == '#') { hashash = 1; iptr++; }
	if (*iptr == '-') {
	    leadingnegsign = 1;
	    iptr++;
	    }
	else leadingnegsign = 0;

	if (*iptr == '0') leadingzero = 1;
	else leadingzero = 0;

	width = 0;
	while (*iptr && isdigit(*iptr)) {
	    width += (*iptr - '0');
	    iptr++;
	    if (isdigit(*iptr)) width *= 10;
	    }
	if (*iptr == '.') {
	    iptr++;
	    width2 = 0;
	    while (*iptr && isdigit(*iptr)) {
		width2 += (*iptr - '0');
		iptr++;
		if (isdigit(*iptr)) width2 *= 10;
		}
	    }

	islong = 0;
	if (*iptr == 'l') { islong++; iptr++; }
	if (*iptr == 'l') { islong++; iptr++; }

	switch (*iptr) {
	    case 'I':
		tmpptr = (unsigned char *) va_arg(marker,unsigned char *);
		optr += __atox(optr,*tmpptr++,10,0,digits);
		*optr++ = '.';
		optr += __atox(optr,*tmpptr++,10,0,digits);
		*optr++ = '.';
		optr += __atox(optr,*tmpptr++,10,0,digits);
		*optr++ = '.';
		optr += __atox(optr,*tmpptr++,10,0,digits);
		break;
	    case 's':
		tmpptr = (unsigned char *) va_arg(marker,unsigned char *);
		if (!tmpptr) tmpptr = (unsigned char *) "(null)";
		if ((width == 0) & (width2 == 0)) {
		    while (*tmpptr) *optr++ = *tmpptr++;
		    break;
		    }
		while (width && *tmpptr) {
		    *optr++ = *tmpptr++;
		    width--;
		    }
		while (width) {
		    *optr++ = ' ';
		    width--;
		    }
		break;
	    case 'a':
		tmpptr = (unsigned char *) va_arg(marker,unsigned char *);
		for (x = 0; x < 5; x++) {
		    optr += __atox(optr,*tmpptr++,16,2,digits);
		    *optr++ = '-';
		    }
		optr += __atox(optr,*tmpptr++,16,2,digits);
		break;
	    case 'd':
		switch (islong) {
		    case 0:
		    case 1:
			i = va_arg(marker,int);
			if (i < 0) { *optr++='-'; i = -i;}
			optr += __atox(optr,i,10,width,digits);
			break;
		    case 2:
			ll = va_arg(marker,long long int);
			if (ll < 0) { *optr++='-'; ll = -ll;}
			optr += __llatox(optr,ll,10,width,digits);
			break;
		    }
		break;
	    case 'u':
		switch (islong) {
		    case 0:
		    case 1:
			ux = va_arg(marker,unsigned int);
			optr += __atox(optr,ux,10,width,digits);
			break;
		    case 2:
			ulx = va_arg(marker,unsigned long long);
			optr += __llatox(optr,ulx,10,width,digits);
			break;
		    }
		break;
	    case 'X':
	    case 'x':
		switch (islong) {
		    case 0:
		    case 1:
			ux = va_arg(marker,unsigned int);
			optr += __atox(optr,ux,16,width,
				       (*iptr == 'X') ? digits : ldigits);
			break;
		    case 2:
			ulx = va_arg(marker,unsigned long long);
			optr += __llatox(optr,ulx,16,width,
				       (*iptr == 'X') ? digits : ldigits);
			break;
		    }
		break;
	    case 'p':
	    case 'P':
#ifdef __long64
		lx = va_arg(marker,long long);
		optr += __llatox(optr,lx,16,16,
				 (*iptr == 'P') ? digits : ldigits);
#else
		x = va_arg(marker,long);
		optr += __atox(optr,x,16,8,
			       (*iptr == 'P') ? digits : ldigits);
#endif
		break;
	    case 'w':
		x = va_arg(marker,unsigned int);
	        x &= 0x0000FFFF;
		optr += __atox(optr,x,16,4,digits);
		break;
	    case 'b':
		x = va_arg(marker,unsigned int);
	        x &= 0x0000FF;
		optr += __atox(optr,x,16,2,digits);
		break;
	    case 'Z':
		x = va_arg(marker,unsigned int);
		tmpptr = va_arg(marker,unsigned char *);
		while (x) {
		    optr += __atox(optr,*tmpptr++,16,2,digits);
		    x--;
		    }
		break;
	    case 'c':
		x = va_arg(marker, int);
		*optr++ = x & 0xff;
		break;

	    default:
		*optr++ = *iptr;
		break;
	    }
	iptr++;
	}

 //   *optr++ = '\r';
 //   *optr++ = '\n';
    *optr = '\0';

    return (optr - outbuf);
}


/*  *********************************************************************
    *  xsprintf(buf,template,params..)
    *
    *  format messages from template into a buffer.
    *
    *  Input Parameters:
    *      buf - output buffer
    *      template - template string
    *      params... parameters
    *
    *  Return Value:
    *      number of bytes copied to buffer
    ********************************************************************* */
int xsprintf(char *buf,const char *templat,...)
{
    va_list marker;
    int count;

    va_start(marker,templat);
    count = xvsprintf(buf,templat,marker);
    va_end(marker);

    return count;
}

/*  *********************************************************************
    *  xprintf(template,...)
    *
    *  A miniature printf.
    *
    *      %a - Ethernet address (16 bytes)
    *      %s - unpacked string, null terminated
    *      %x - hex word (machine size)
    *      %w - hex word (16 bits)
    *      %b - hex byte (8 bits)
    *      %Z - buffer (put length first, then buffer address)
    *
    *  Return value:
    *  	   number of bytes written
    ********************************************************************* */

int printf(const char *templat,...)
{
    va_list marker;
    int count;
    char buffer[512];

    va_start(marker,templat);
    count = xvsprintf(buffer,templat,marker);
    va_end(marker);


    for(int i = 0; i< count; i++){
    	putc(buffer[i]);
    }

    //putc('\r');
    //putc('\n');

    return count;
}


int xvprintf(const char *templat,va_list marker)
{
    int count;
    char buffer[512];

    count = xvsprintf(buffer,templat,marker);

    if (xprinthook) (*xprinthook)(buffer);

    return count;
}

void putstring(char* str)
{
	for(int i = 0; i< strlen(str); i++)
		putc(*str++);
}


void putc(char c) {

    //volatile uint32_t *usart_isr = &UART->USART_ISR;
    volatile uint32_t *usart_isr = (volatile uint32_t *)(0x40013800 + 0x1C);

    //volatile uint32_t *usart_tdr = &UART->USART_TDR;
    volatile uint32_t *usart_tdr = (volatile uint32_t *)(0x40013800 + 0x28);

    if (c == '\n'){

		 while((*usart_isr & (1 << 7)) == 0);
		 *usart_tdr = '\r';
    }

    while((*usart_isr & (1 << 7)) == 0);



    *usart_tdr = c;
}

void puts(const char *templat,...) {
    printf(templat);
}

void putchar(char c) {
	putc(c);
}


