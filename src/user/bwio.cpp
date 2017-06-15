/*
 * bwio.c - busy-wait I/O routines for diagnosis
 *
 * Specific to the TS-7200 ARM evaluation board
 *
 */

#include <ts7200.h>
#include <bwio.h>
#include <io.h>
#include <ns.h>
#include <std.h>

typedef volatile unsigned * register_t;

/*
 * The UARTs are initialized by RedBoot to the following state
 * 	115,200 bps
 * 	8 bits
 * 	no parity
 * 	fifos enabled
 */
int bwsetfifo( int channel, int state ) {
    register_t line;
	switch( channel ) {
	case COM1:
		line = (register_t)( UART1_BASE + UART_LCRH_OFFSET );
	        break;
	case COM2:
	        line = (register_t)( UART2_BASE + UART_LCRH_OFFSET );
	        break;
	default:
	        return -1;
	        break;
	}
	unsigned buf = *line;
	buf = state ? buf | FEN_MASK : buf & ~FEN_MASK;
	*line = buf;
	return 0;
}

int bwsetspeed( int channel, int speed ) {
	register_t high, low;
	switch( channel ) {
	case COM1:
		high = (register_t)( UART1_BASE + UART_LCRM_OFFSET );
		low = (register_t)( UART1_BASE + UART_LCRL_OFFSET );
	        break;
	case COM2:
		high = (register_t)( UART2_BASE + UART_LCRM_OFFSET );
		low = (register_t)( UART2_BASE + UART_LCRL_OFFSET );
	        break;
	default:
	        return -1;
	        break;
	}
	switch( speed ) {
	case 115200:
		*high = 0x0;
		*low = 0x3;
		return 0;
	case 2400:
		*high = 0x0;
		*low = 0xBF;
		return 0;
	default:
		return -1;
	}
}

// Busy wait is initially enabled for early printing. During init, it is
// disabled in favour of interrupts.
bool useBusyWait = true;

int bwputc( int channel, char c ) {
	
	if (!useBusyWait) {
		static ctl::Tid io[2] = {whoIs(ctl::Names::Uart1Tx), whoIs(ctl::Names::Uart2Tx)};
                ASSERT(io[0].underlying() >= 0);
                ASSERT(io[1].underlying() >= 0);
		io::putc(io[channel], channel, c);
		return 0;
	}

	register_t flags, data;
	switch( channel ) {
	case COM1:
		flags = (register_t)( UART1_BASE + UART_FLAG_OFFSET );
		data = (register_t)( UART1_BASE + UART_DATA_OFFSET );
		break;
	case COM2:
		flags = (register_t)( UART2_BASE + UART_FLAG_OFFSET );
		data = (register_t)( UART2_BASE + UART_DATA_OFFSET );
		break;
	default:
		return -1;
		break;
	}
	while( ( *flags & TXFF_MASK ) ) ;
	*data = c;
	return 0;
}

char c2x( char ch ) {
	if ( (ch <= 9) ) return '0' + ch;
	return 'a' + ch - 10;
}

int bwputx( int channel, char c ) {
	char chh, chl;

	chh = c2x( c / 16 );
	chl = c2x( c % 16 );
	bwputc( channel, chh );
	return bwputc( channel, chl );
}

int bwputr( int channel, unsigned int reg ) {
	int byte;
	char *ch = (char *) &reg;

	for( byte = 3; byte >= 0; byte-- ) bwputx( channel, ch[byte] );
	return bwputc( channel, ' ' );
}

int bwputstr( int channel, const char *str ) {
	while( *str ) {
		if( bwputc( channel, *str ) < 0 ) return -1;
		str++;
	}
	return 0;
}

void bwputw( int channel, int n, char fc, char *bf ) {
	char ch;
	char *p = bf;

	while( *p++ && n > 0 ) n--;
	while( n-- > 0 ) bwputc( channel, fc );
	while( ( ch = *bf++ ) ) bwputc( channel, ch );
}

int bwa2d( char ch ) {
	if( ch >= '0' && ch <= '9' ) return ch - '0';
	if( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
	if( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
	return -1;
}

char bwa2i( char ch, const char **src, int base, int *nump ) {
	int num, digit;
	const char *p;

	p = *src; num = 0;
	while( ( digit = bwa2d( ch ) ) >= 0 ) {
		if ( digit > base ) break;
		num = num*base + digit;
		ch = *p++;
	}
	*src = p; *nump = num;
	return ch;
}

void bwui2a( unsigned int num, unsigned int base, char *bf ) {
	int n = 0;
	int dgt;
	unsigned int d = 1;
	
	while( (num / d) >= base ) d *= base;
	while( d != 0 ) {
		dgt = num / d;
		num %= d;
		d /= base;
		if( n || dgt > 0 || d == 0 ) {
			*bf++ = dgt + ( dgt < 10 ? '0' : 'a' - 10 );
			++n;
		}
	}
	*bf = 0;
}

void bwi2a( int num, char *bf ) {
	if( num < 0 ) {
		num = -num;
		*bf++ = '-';
	}
	bwui2a( num, 10, bf );
}

void bwformat ( int channel, const char *fmt, va_list va ) {
	char bf[12];
	char ch, lz;
	int w;

	
	while ( ( ch = *(fmt++) ) ) {
		if ( ch != '%' )
			bwputc( channel, ch );
		else {
			lz = ' '; w = 0;
			ch = *(fmt++);
			switch ( ch ) {
			case '0':
				lz = '0'; ch = *(fmt++);
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				ch = bwa2i( ch, &fmt, 10, &w );
				break;
			}
			switch( ch ) {
			case 0: return;
			case 'c':
				bwputc( channel, va_arg( va, char ) );
				break;
			case 's':
				bwputw( channel, w, 0, va_arg( va, char* ) );
				break;
			case 'u':
				bwui2a( va_arg( va, unsigned int ), 10, bf );
				bwputw( channel, w, lz, bf );
				break;
			case 'd':
				bwi2a( va_arg( va, int ), bf );
				bwputw( channel, w, lz, bf );
				break;
			case 'x':
				bwui2a( va_arg( va, unsigned int ), 16, bf );
				bwputw( channel, w, lz, bf );
				break;
			case '%':
				bwputc( channel, ch );
				break;
			}
		}
	}
}

void bwprintf( int channel, const char *fmt, ... ) {
    va_list va;

    va_start(va,fmt);
    bwformat( channel, fmt, va );
    va_end(va);
}

