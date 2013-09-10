/* ASN.1 object dumping code, copyright Peter Gutmann
   <pgut001@cs.auckland.ac.nz>, based on ASN.1 dump program by David Kemp
   <dpkemp@missi.ncsc.mil>, with contributions from various people including
   Matthew Hamrick <hamrick@rsa.com>, Bruno Couillard
   <bcouillard@chrysalis-its.com>, Hallvard Furuseth
   <h.b.furuseth@usit.uio.no>, Geoff Thorpe <geoff@raas.co.nz>, David Boyce
   <d.boyce@isode.com>, John Hughes <john.hughes@entegrity.com>, Life is hard,
   and then you die <ronald@trustpoint.com>, Hans-Olof Hermansson
   <hans-olof.hermansson@postnet.se>, Tor Rustad <Tor.Rustad@bbs.no>,
   Kjetil Barvik <kjetil.barvik@bbs.no>, James Sweeny <jsweeny@us.ibm.com>,
   Chris Ridd <chris.ridd@isode.com>, and several other people whose names
   I've misplaced (a number of those email addresses probably no longer
   work, since this code has been around for awhile).
   
   Available from http://www.cs.auckland.ac.nz/~pgut001/dumpasn1.c.
   Last updated 22 June 2006 (version 20060622, if you prefer it that
   way).  To build under Windows, use 'cl /MD dumpasn1.c'.  To build on OS390
   or z/OS, use '/bin/c89 -D OS390 -o dumpasn1 dumpasn1.c'.

   This code grew slowly over time without much design or planning, and with 
   extra features being tacked on as required.  It's not representative of 
   my normal coding style.

   This version of dumpasn1 requires a config file dumpasn1.cfg to be present
   in the same location as the program itself or in a standard directory
   where binaries live (it will run without it but will display a warning
   message, you can configure the path either by hardcoding it in or using an
   environment variable as explained further down).  The config file is
   available from http://www.cs.auckland.ac.nz/~pgut001/dumpasn1.cfg.

   This code assumes that the input data is binary, having come from a MIME-
   aware mailer or been piped through a decoding utility if the original
   format used base64 encoding.  If you need to decode it, it's recommended
   that you use a utility like uudeview, which will strip virtually any kind
   of encoding (MIME, PEM, PGP, whatever) to recover the binary original.

   You can use this code in whatever way you want, as long as you don't try
   to claim you wrote it.

   Editing notes: Tabs to 4, phasers to stun (and in case anyone wants to
   complain about that, see "Program Indentation and Comprehensiblity",
   Richard Miara, Joyce Musselman, Juan Navarro, and Ben Shneiderman,
   Communications of the ACM, Vol.26, No.11 (November 1983), p.861) */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef OS390
  #include <unistd.h>
#endif /* OS390 */

/* The update string, printed as part of the help screen */

#define UPDATE_STRING	"22 June 2006"

/* Useful defines */

#ifndef TRUE
  #define FALSE	0
  #define TRUE	( !FALSE )
#endif /* TRUE */

/* Tandem Guardian NonStop Kernel options */

#ifdef __TANDEM
  #pragma nolist		/* Spare us the source listing, no GUI... */
  #pragma nowarn (1506)	/* Implicit type conversion: int to char etc */
#endif /* __TANDEM */

/* SunOS 4.x doesn't define seek codes or exit codes or FILENAME_MAX (it does
   define _POSIX_MAX_PATH, but in funny locations and to different values
   depending on which include file you use).  Strictly speaking this code
   isn't right since we need to use PATH_MAX, however not all systems define
   this, some use _POSIX_PATH_MAX, and then there are all sorts of variations
   and other defines that you have to check, which require about a page of
   code to cover each OS, so we just use max( FILENAME_MAX, 512 ) which
   should work for everything */

#ifndef SEEK_SET
  #define SEEK_SET	0
  #define SEEK_CUR	2
#endif /* No fseek() codes defined */
#ifndef EXIT_FAILURE
  #define EXIT_FAILURE	1
  #define EXIT_SUCCESS	( !EXIT_FAILURE )
#endif /* No exit() codes defined */
#ifndef FILENAME_MAX
  #define FILENAME_MAX	512
#else
  #if FILENAME_MAX < 128
	#undef FILENAME_MAX
	#define FILENAME_MAX	512
  #endif /* FILENAME_MAX < 128 */
#endif /* FILENAME_MAX */

/* Under Windows we can do special-case handling for paths and Unicode
   strings (although in practice it can't really handle much except
   latin-1) */

#if ( defined( _WINDOWS ) || defined( WIN32 ) || defined( _WIN32 ) || \
	  defined( __WIN32__ ) )
  #define __WIN32__
#endif /* Win32 */

/* Under Unix we can do special-case handling for paths and Unicode strings.
   Detecting Unix systems is a bit tricky but the following should find most
   versions.  This define implicitly assumes that the system has wchar_t
   support, but this is almost always the case except for very old systems,
   so it's best to default to allow-all rather than deny-all */

#if defined( linux ) || defined( __linux__ ) || defined( sun ) || \
	defined( __bsdi__ ) || defined( __FreeBSD__ ) || defined( __NetBSD__ ) || \
	defined( __OpenBSD__ ) || defined( __hpux ) || defined( _M_XENIX ) || \
	defined( __osf__ ) || defined( _AIX ) || defined( __MACH__ )
  #define __UNIX__
#endif /* Every commonly-used Unix */
#if defined( linux ) || defined( __linux__ )
  #define __USE_ISOC99
  #include <wchar.h>
#endif /* Linux */

/* For IBM mainframe OSes we use the Posix environment, so it looks like
   Unix */

#ifdef OS390
  #define __OS390__
  #define __UNIX__
#endif /* OS390 / z/OS */

/* Tandem NSK: Don't tangle with Tandem OSS, which is almost UNIX */

#ifdef __TANDEM
  #ifdef _GUARDIAN_TARGET
	#define __TANDEM_NSK__
  #else
	#define __UNIX__
  #endif /* _GUARDIAN_TARGET */
#endif /* __TANDEM */

/* Some OS's don't define the min() macro */

#ifndef min
  #define min(a,b)		( ( a ) < ( b ) ? ( a ) : ( b ) )
#endif /* !min */

/* The level of recursion can get scary for deeply-nested structures so we
   use a larger-than-normal stack under DOS */

#ifdef  __TURBOC__
  extern unsigned _stklen = 16384;
#endif /* __TURBOC__ */

/* When we dump a nested data object encapsulated within a larger object, the
   length is initially set to a magic value which is adjusted to the actual
   length once we start parsing the object */

#define LENGTH_MAGIC	177545L

/* Tag classes */

#define CLASS_MASK		0xC0	/* Bits 8 and 7 */
#define UNIVERSAL		0x00	/* 0 = Universal (defined by ITU X.680) */
#define APPLICATION		0x40	/* 1 = Application */
#define CONTEXT			0x80	/* 2 = Context-specific */
#define PRIVATE			0xC0	/* 3 = Private */

/* Encoding type */

#define FORM_MASK		0x20	/* Bit 6 */
#define PRIMITIVE		0x00	/* 0 = primitive */
#define CONSTRUCTED		0x20	/* 1 = constructed */

/* Universal tags */

#define TAG_MASK		0x1F	/* Bits 5 - 1 */
#define EOC				0x00	/*  0: End-of-contents octets */
#define BOOLEAN			0x01	/*  1: Boolean */
#define INTEGER			0x02	/*  2: Integer */
#define BITSTRING		0x03	/*  2: Bit string */
#define OCTETSTRING		0x04	/*  4: Byte string */
#define NULLTAG			0x05	/*  5: NULL */
#define OID				0x06	/*  6: Object Identifier */
#define OBJDESCRIPTOR	0x07	/*  7: Object Descriptor */
#define EXTERNAL		0x08	/*  8: External */
#define REAL			0x09	/*  9: Real */
#define ENUMERATED		0x0A	/* 10: Enumerated */
#define EMBEDDED_PDV	0x0B	/* 11: Embedded Presentation Data Value */
#define UTF8STRING		0x0C	/* 12: UTF8 string */
#define SEQUENCE		0x10	/* 16: Sequence/sequence of */
#define SET				0x11	/* 17: Set/set of */
#define NUMERICSTRING	0x12	/* 18: Numeric string */
#define PRINTABLESTRING	0x13	/* 19: Printable string (ASCII subset) */
#define T61STRING		0x14	/* 20: T61/Teletex string */
#define VIDEOTEXSTRING	0x15	/* 21: Videotex string */
#define IA5STRING		0x16	/* 22: IA5/ASCII string */
#define UTCTIME			0x17	/* 23: UTC time */
#define GENERALIZEDTIME	0x18	/* 24: Generalized time */
#define GRAPHICSTRING	0x19	/* 25: Graphic string */
#define VISIBLESTRING	0x1A	/* 26: Visible string (ASCII subset) */
#define GENERALSTRING	0x1B	/* 27: General string */
#define UNIVERSALSTRING	0x1C	/* 28: Universal string */
#define BMPSTRING		0x1E	/* 30: Basic Multilingual Plane/Unicode string */

/* Length encoding */

#define LEN_XTND  0x80		/* Indefinite or long form */
#define LEN_MASK  0x7F		/* Bits 7 - 1 */

/* Various special-case operations to perform on strings */

typedef enum {
	STR_NONE,				/* No special handling */
	STR_UTCTIME,			/* Check it's UTCTime */
	STR_GENERALIZED,		/* Check it's GeneralizedTime */
	STR_PRINTABLE,			/* Check it's a PrintableString */
	STR_IA5,				/* Check it's an IA5String */
	STR_LATIN1,				/* Read and display string as latin-1 */
	STR_BMP,				/* Read and display string as Unicode */
	STR_BMP_REVERSED		/* STR_BMP with incorrect endianness */
	} STR_OPTION;

/* Structure to hold info on an ASN.1 item */

typedef struct {
	int id;						/* Tag class + primitive/constructed */
	int tag;					/* Tag */
	long length;				/* Data length */
	int indefinite;				/* Item has indefinite length */
	int headerSize;				/* Size of tag+length */
	unsigned char header[ 8 ];	/* Tag+length data */
	} ASN1_ITEM;

/* Config options */

static int printDots = FALSE;		/* Whether to print dots to align columns */
static int doPure = FALSE;			/* Print data without LHS info column */
static int doDumpHeader = FALSE;	/* Dump tag+len in hex (level = 0, 1, 2) */
static int extraOIDinfo = FALSE;	/* Print extra information about OIDs */
static int doHexValues = FALSE;		/* Display size, offset in hex not dec.*/
static int useStdin = FALSE;		/* Take input from stdin */
static int zeroLengthAllowed = FALSE;/* Zero-length items allowed */
static int dumpText = FALSE;		/* Dump text alongside hex data */
static int printAllData = FALSE;	/* Whether to print all data in long blocks */
static int checkEncaps = TRUE;		/* Print encaps.data in BIT/OCTET STRINGs */
static int checkCharset = TRUE;		/* Check val.of char strs.hidden in OCTET STRs */
#ifndef __OS390__
static int reverseBitString = TRUE;	/* Print BIT STRINGs in natural order */
#else
static int reverseBitString = FALSE;/* Natural order on OS390 is the same as ASN.1 */
#endif /* __OS390__ */
static int rawTimeString = FALSE;	/* Print raw time strings */
static int shallowIndent = FALSE;	/* Perform shallow indenting */
static int outputWidth = 80;		/* 80-column display */

/* The indent size and fixed indent string to the left of the data */

#if 0
#define INDENT_SIZE		14
#define INDENT_STRING	"            : "
#else
#define INDENT_SIZE		11
#define INDENT_STRING	"         : "
#endif /* 0 */

/* Error and warning information */

static int noErrors = 0;			/* Number of errors found */
static int noWarnings = 0;			/* Number of warnings */

/* Position in the input stream */

static int fPos = 0;				/* Absolute position in data */

/* The output stream */

static FILE *output;				/* Output stream */

/* Information on an ASN.1 Object Identifier */

#define MAX_OID_SIZE	32

typedef struct tagOIDINFO {
	struct tagOIDINFO *next;		/* Next item in list */
	char oid[ MAX_OID_SIZE ], *comment, *description;
	int oidLength;					/* Name, rank, serial number */
	int warn;						/* Whether to warn if OID encountered */
	} OIDINFO;

static OIDINFO *oidList = NULL;

/* If the config file isn't present in the current directory, we search the
   following paths (this is needed for Unix with dumpasn1 somewhere in the
   path, since this doesn't set up argv[0] to the full path).  Anything
   beginning with a '$' uses the appropriate environment variable.  In
   addition under Unix we also walk down $PATH looking for it */

#ifdef __TANDEM_NSK__
  #define CONFIG_NAME		"asn1cfg"
#else
  #define CONFIG_NAME		"dumpasn1.cfg"
#endif /* __TANDEM_NSK__ */

#if defined( __TANDEM_NSK__ )

static const char *configPaths[] = {
	"$system.security", "$system.system",

	NULL
	};

#elif defined( __WIN32__ )

static const char *configPaths[] = {
	/* Windoze absolute paths.  Usually things are on C:, but older NT setups
	   are easier to do on D: if the initial copy is done to C: */
	"c:\\dos\\", "d:\\dos\\", "c:\\windows\\", "d:\\windows\\",
	"c:\\winnt\\", "d:\\winnt\\",

	/* It's my program, I'm allowed to hardcode in strange paths that no-one
	   else uses */
	"c:\\program files\\bin\\",

	/* This one seems to be popular as well */
	"c:\\program files\\utilities\\",

	/* General environment-based paths */
	"$DUMPASN1_PATH/",

	NULL
	};

#elif defined( __OS390__ )

static const char *configPaths[] = {
	/* General environment-based paths */
	"$DUMPASN1_PATH/",

	NULL
	};

#else

static const char *configPaths[] = {
  #ifndef DEBIAN
	/* Unix absolute paths */
	"/usr/bin/", "/usr/local/bin/", "/etc/dumpasn1/",

	/* Unix environment-based paths */
	"$HOME/", "$HOME/bin/",

	/* It's my program, I'm allowed to hardcode in strange paths that no-one
	   else uses */
	"$HOME/BIN/",
  #else
	/* Debian has specific places where you're supposed to dump things */
	"$HOME/", "/etc/dumpasn1/",
  #endif /* DEBIAN-specific paths */

	/* General environment-based paths */
	"$DUMPASN1_PATH/",

	NULL
	};
#endif /* OS-specific search paths */

#define isEnvTerminator( c )	\
	( ( ( c ) == '/' ) || ( ( c ) == '.' ) || ( ( c ) == '$' ) || \
	  ( ( c ) == '\0' ) || ( ( c ) == '~' ) )

/****************************************************************************
*																			*
*					Object Identification/Description Routines				*
*																			*
****************************************************************************/

/* Return descriptive strings for universal tags */

static char *idstr( const int tagID )
	{
	switch( tagID )
		{
		case EOC:
			return( "End-of-contents octets" );
		case BOOLEAN:
			return( "BOOLEAN" );
		case INTEGER:
			return( "INTEGER" );
		case BITSTRING:
			return( "BIT STRING" );
		case OCTETSTRING:
			return( "OCTET STRING" );
		case NULLTAG:
			return( "NULL" );
		case OID:
			return( "OBJECT IDENTIFIER" );
		case OBJDESCRIPTOR:
			return( "ObjectDescriptor" );
		case EXTERNAL:
			return( "EXTERNAL" );
		case REAL:
			return( "REAL" );
		case ENUMERATED:
			return( "ENUMERATED" );
		case EMBEDDED_PDV:
			return( "EMBEDDED PDV" );
		case UTF8STRING:
			return( "UTF8String" );
		case SEQUENCE:
			return( "SEQUENCE" );
		case SET:
			return( "SET" );
		case NUMERICSTRING:
			return( "NumericString" );
		case PRINTABLESTRING:
			return( "PrintableString" );
		case T61STRING:
			return( "TeletexString" );
		case VIDEOTEXSTRING:
			return( "VideotexString" );
		case IA5STRING:
			return( "IA5String" );
		case UTCTIME:
			return( "UTCTime" );
		case GENERALIZEDTIME:
			return( "GeneralizedTime" );
		case GRAPHICSTRING:
			return( "GraphicString" );
		case VISIBLESTRING:
			return( "VisibleString" );
		case GENERALSTRING:
			return( "GeneralString" );
		case UNIVERSALSTRING:
			return( "UniversalString" );
		case BMPSTRING:
			return( "BMPString" );
		default:
			return( "Unknown (Reserved)" );
		}
	}

/* Return information on an object identifier */

static OIDINFO *getOIDinfo( char *oid, const int oidLength )
	{
	OIDINFO *oidPtr;

	memset( oid + oidLength, 0, 2 );
	for( oidPtr = oidList; oidPtr != NULL; oidPtr = oidPtr->next )
		{
		if( oidLength == oidPtr->oidLength - 2 && \
			!memcmp( oidPtr->oid + 2, oid, oidLength ) )
			return( oidPtr );
		}

	return( NULL );
	}

/* Add an OID attribute */

static int addAttribute( char **buffer, char *attribute )
	{
	if( ( *buffer = ( char * ) malloc( strlen( attribute ) + 1 ) ) == NULL )
		{
		puts( "Out of memory." );
		return( FALSE );
		}
	strcpy( *buffer, attribute );
	return( TRUE );
	}

/* Table to identify valid string chars (taken from cryptlib).  Note that
   IA5String also allows control chars, but we warn about these since
   finding them in a certificate is a sign that there's something
   seriously wrong */

#define P	1						/* PrintableString */
#define I	2						/* IA5String */
#define PI	3						/* IA5String and PrintableString */

static int charFlags[] = {
	/* 00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F */
		0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
	/* 10  11  12  13  14  15  16  17  18  19  1A  1B  1C  1D  1E  1F */
		0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
	/*		!	"	#	$	%	&	'	(	)	*	+	,	-	.	/ */
	   PI,	I,	I,	I,	I,	I,	I, PI, PI, PI,	I, PI, PI, PI, PI, PI,
	/*	0	1	2	3	4	5	6	7	8	9	:	;	<	=	>	? */
	   PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI,	I,	I, PI,	I, PI,
	/*	@	A	B	C	D	E	F	G	H	I	J	K	L	M	N	O */
		I, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI,
	/*	P	Q	R	S	T	U	V	W	X	Y	Z	[	\	]	^ _ */
	   PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI,	I,	I,	I,	I,	I,
	/*	`	a	b	c	d	e	f	g	h	i	j	k	l	m	n	o */
		I, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI,
	/*	p	q	r	s	t	u	v	w	x	y	z	{	|	}	~  DL */
	   PI, PI, PI, PI, PI, PI, PI, PI, PI, PI, PI,	I,	I,	I,	I,	0
	};

static int isPrintable( int ch )
	{
	if( ch >= 128 || !( charFlags[ ch ] & P ) )
		return( FALSE );
	return( TRUE );
	}

static int isIA5( int ch )
	{
	if( ch >= 128 || !( charFlags[ ch ] & I ) )
		return( FALSE );
	return( TRUE );
	}

/****************************************************************************
*																			*
*							Config File Read Routines						*
*																			*
****************************************************************************/

/* Files coming from DOS/Windows systems may have a ^Z (the CP/M EOF char)
   at the end, so we need to filter this out */

#define CPM_EOF	0x1A		/* ^Z = CPM EOF char */

/* The maximum input line length */

#define MAX_LINESIZE	512

/* Read a line of text from the config file */

static int lineNo;

static int readLine( FILE *file, char *buffer )
	{
	int bufCount = 0, ch;

	/* Skip whitespace */
	while( ( ( ch = getc( file ) ) == ' ' || ch == '\t' ) && !feof( file ) );

	/* Get a line into the buffer */
	while( ch != '\r' && ch != '\n' && ch != CPM_EOF && !feof( file ) )
		{
		/* Check for an illegal char in the data.  Note that we don't just
		   check for chars with high bits set because these are legal in
		   non-ASCII strings */
		if( !isprint( ch ) )
			{
			printf( "Bad character '%c' in config file line %d.\n",
					ch, lineNo );
			return( FALSE );
			}

		/* Check to see if it's a comment line */
		if( ch == '#' && !bufCount )
			{
			/* Skip comment section and trailing whitespace */
			while( ch != '\r' && ch != '\n' && ch != CPM_EOF && !feof( file ) )
				ch = getc( file );
			break;
			}

		/* Make sure that the line is of the correct length */
		if( bufCount > MAX_LINESIZE )
			{
			printf( "Config file line %d too long.\n", lineNo );
			return( FALSE );
			}
		else
			if( ch )	/* Can happen if we read a binary file */
				buffer[ bufCount++ ] = ch;

		/* Get next character */
		ch = getc( file );
		}

	/* If we've just passed a CR, check for a following LF */
	if( ch == '\r' )
		{
		if( ( ch = getc( file ) ) != '\n' )
			ungetc( ch, file );
		}

	/* Skip trailing whitespace and add der terminador */
	while( bufCount > 0 &&
		   ( ( ch = buffer[ bufCount - 1 ] ) == ' ' || ch == '\t' ) )
		bufCount--;
	buffer[ bufCount ] = '\0';

	/* Handle special-case of ^Z if file came off an MSDOS system */
	if( ch == CPM_EOF )
		{
		while( !feof( file ) )
			{
			/* Keep going until we hit the true EOF (or some sort of error) */
			ch = getc( file );
			}
		}

	return( ferror( file ) ? FALSE : TRUE );
	}

/* Process an OID specified as space-separated hex digits */

static int processHexOID( OIDINFO *oidInfo, char *string )
	{
	int value, index = 0;

	while( *string && index < MAX_OID_SIZE - 1 )
		{
		if( sscanf( string, "%x", &value ) != 1 || value > 255 )
			{
			printf( "Invalid hex value in config file line %d.\n", lineNo );
			return( FALSE );
			}
		oidInfo->oid[ index++ ] = value;
		string += 2;
		if( *string && *string++ != ' ' )
			{
			printf( "Invalid hex string in config file line %d.\n", lineNo );
			return( FALSE );
			}
		}
	oidInfo->oid[ index ] = 0;
	oidInfo->oidLength = index;
	if( index >= MAX_OID_SIZE - 1 )
		{
		printf( "OID value in config file line %d too long.\n", lineNo );
		return( FALSE );
		}
	return( TRUE );
	}

/* Read a config file */

static int readConfig( const char *path, const int isDefaultConfig )
	{
	OIDINFO dummyOID = { NULL, "Dummy", "Dummy", "Dummy", 1, 1 }, *oidPtr;
	FILE *file;
	char buffer[ MAX_LINESIZE ];
	int status;

	/* Try and open the config file */
	if( ( file = fopen( path, "rb" ) ) == NULL )
		{
		/* If we can't open the default config file, issue a warning but
		   continue anyway */
		if( isDefaultConfig )
			{
			puts( "Cannot open config file 'dumpasn1.cfg', which should be in the same" );
			puts( "directory as the dumpasn1 program, a standard system directory, or" );
			puts( "in a location pointed to by the DUMPASN1_PATH environment variable." );
			puts( "Operation will continue without the ability to display Object " );
			puts( "Identifier information." );
			puts( "" );
			puts( "If the config file is located elsewhere, you can set the environment" );
			puts( "variable DUMPASN1_PATH to the path to the file." );
			return( TRUE );
			}

		printf( "Cannot open config file '%s'.\n", path );
		return( FALSE );
		}

	/* Add the new config entries at the appropriate point in the OID list */
	if( oidList == NULL )
		oidPtr = &dummyOID;
	else
		for( oidPtr = oidList; oidPtr->next != NULL; oidPtr = oidPtr->next );

	/* Read each line in the config file */
	lineNo = 1;
	while( ( status = readLine( file, buffer ) ) == TRUE && !feof( file ) )
		{
		/* If it's a comment line, skip it */
		if( !*buffer )
			{
			lineNo++;
			continue;
			}

		/* Check for an attribute tag */
		if( !strncmp( buffer, "OID = ", 6 ) )
			{
			/* Make sure that all of the required attributes for the current
			   OID are present */
			if( oidPtr->description == NULL )
				{
				printf( "OID ending on config file line %d has no "
						"description attribute.\n", lineNo - 1 );
				return( FALSE );
				}

			/* Allocate storage for the new OID */
			if( ( oidPtr->next = ( struct tagOIDINFO * ) \
								 malloc( sizeof( OIDINFO ) ) ) == NULL )
				{
				puts( "Out of memory." );
				return( FALSE );
				}
			oidPtr = oidPtr->next;
			if( oidList == NULL )
				oidList = oidPtr;
			memset( oidPtr, 0, sizeof( OIDINFO ) );

			/* Add the new OID */
			if( !processHexOID( oidPtr, buffer + 6 ) )
				return( FALSE );
			}
		else if( !strncmp( buffer, "Description = ", 14 ) )
			{
			if( oidPtr->description != NULL )
				{
				printf( "Duplicate OID description in config file line %d.\n",
						lineNo );
				return( FALSE );
				}
			if( !addAttribute( &oidPtr->description, buffer + 14 ) )
				return( FALSE );
			}
		else if( !strncmp( buffer, "Comment = ", 10 ) )
			{
			if( oidPtr->comment != NULL )
				{
				printf( "Duplicate OID comment in config file line %d.\n",
						lineNo );
				return( FALSE );
				}
			if( !addAttribute( &oidPtr->comment, buffer + 10 ) )
				return( FALSE );
			}
		else if( !strncmp( buffer, "Warning", 7 ) )
			{
			if( oidPtr->warn )
				{
				printf( "Duplicate OID warning in config file line %d.\n",
						lineNo );
				return( FALSE );
				}
			oidPtr->warn = TRUE;
			}
		else
			{
			printf( "Unrecognised attribute '%s', line %d.\n", buffer,
					lineNo );
			return( FALSE );
			}

		lineNo++;
		}
	fclose( file );

	return( status );
	}

/* Check for the existence of a config file path (access() isn't available
   on all systems) */

static int testConfigPath( const char *path )
	{
	FILE *file;

	/* Try and open the config file */
	if( ( file = fopen( path, "rb" ) ) == NULL )
		return( FALSE );
	fclose( file );

	return( TRUE );
	}

/* Build a config path by substituting environment strings for $NAMEs */

static void buildConfigPath( char *path, const char *pathTemplate )
	{
	char pathBuffer[ FILENAME_MAX ], newPath[ FILENAME_MAX ];
	int pathLen, pathPos = 0, newPathPos = 0;

	/* Add the config file name at the end */
	strcpy( pathBuffer, pathTemplate );
	strcat( pathBuffer, CONFIG_NAME );
	pathLen = strlen( pathBuffer );

	while( pathPos < pathLen )
		{
		char *strPtr;
		int substringSize;

		/* Find the next $ and copy the data before it to the new path */
		if( ( strPtr = strstr( pathBuffer + pathPos, "$" ) ) != NULL )
			substringSize = ( int ) ( ( strPtr - pathBuffer ) - pathPos );
		else
			substringSize = pathLen - pathPos;
		if( substringSize > 0 )
			memcpy( newPath + newPathPos, pathBuffer + pathPos,
					substringSize );
		newPathPos += substringSize;
		pathPos += substringSize;

		/* Get the environment string for the $NAME */
		if( strPtr != NULL )
			{
			char envName[ MAX_LINESIZE ], *envString;
			int i;

			/* Skip the '$', find the end of the $NAME, and copy the name
			   into an internal buffer */
			pathPos++;	/* Skip the $ */
			for( i = 0; !isEnvTerminator( pathBuffer[ pathPos + i ] ); i++ );
			memcpy( envName, pathBuffer + pathPos, i );
			envName[ i ] = '\0';

			/* Get the env.string and copy it over */
			if( ( envString = getenv( envName ) ) != NULL )
				{
				const int envStrLen = strlen( envString );

				if( newPathPos + envStrLen < FILENAME_MAX - 2 )
					{
					memcpy( newPath + newPathPos, envString, envStrLen );
					newPathPos += envStrLen;
					}
				}
			pathPos += i;
			}
		}
	newPath[ newPathPos ] = '\0';	/* Add der terminador */

	/* Copy the new path to the output */
	strcpy( path, newPath );
	}

/* Read the global config file */

static int readGlobalConfig( const char *path )
	{
	char buffer[ FILENAME_MAX ];
	char *searchPos = ( char * ) path, *namePos, *lastPos = NULL;
#ifdef __UNIX__
	char *envPath;
#endif /* __UNIX__ */
	int i;

	/* First, try and find the config file in the same directory as the
	   executable by walking down the path until we find the last occurrence
	   of the program name.  This requires that argv[0] be set up properly,
	   which isn't the case if Unix search paths are being used, and seems
	   to be pretty broken under Windows */
	do
		{
		namePos = lastPos;
		lastPos = strstr( searchPos, "dumpasn1" );
		if( lastPos == NULL )
			lastPos = strstr( searchPos, "DUMPASN1" );
		searchPos = lastPos + 1;
		}
	while( lastPos != NULL );
#ifdef __UNIX__
	if( namePos == NULL && ( namePos = strrchr( path, '/' ) ) != NULL )
		{
		const int endPos = ( int ) ( namePos - path ) + 1;

		/* If the executable isn't called dumpasn1, we won't be able to find
		   it with the above code, fall back to looking for directory
		   separators.  This requires a system where the only separator is
		   the directory separator (ie it doesn't work for Windows or most
		   mainframe environments) */
		if( endPos < FILENAME_MAX - 13 )
			{
			memcpy( buffer, path, endPos );
			strcpy( buffer + endPos, CONFIG_NAME );
			if( testConfigPath( buffer ) )
				return( readConfig( buffer, TRUE ) );
			}

		/* That didn't work, try the absolute locations and $PATH */
		namePos = NULL;
		}
#endif /* __UNIX__ */
	if( strlen( path ) < FILENAME_MAX - 13 && namePos != NULL )
		{
		strcpy( buffer, path );
		strcpy( buffer + ( int ) ( namePos - ( char * ) path ), CONFIG_NAME );
		if( testConfigPath( buffer ) )
			return( readConfig( buffer, TRUE ) );
		}

	/* Now try each of the possible absolute locations for the config file */
	for( i = 0; configPaths[ i ] != NULL; i++ )
		{
		buildConfigPath( buffer, configPaths[ i ] );
		if( testConfigPath( buffer ) )
			return( readConfig( buffer, TRUE ) );
		}

#ifdef __UNIX__
	/* On Unix systems we can also search for the config file on $PATH */
	if( ( envPath = getenv( "PATH" ) ) != NULL )
		{
		char *pathPtr = strtok( envPath, ":" );

		do
			{
			sprintf( buffer, "%s/%s", pathPtr, CONFIG_NAME );
			if( testConfigPath( buffer ) )
				return( readConfig( buffer, TRUE ) );
			pathPtr = strtok( NULL, ":" );
			}
		while( pathPtr != NULL );
		}
#endif /* __UNIX__ */

	/* Default to just the config name (which should fail as it was the
	   first entry in configPaths[]).  readConfig() will display the
	   appropriate warning */
	return( readConfig( CONFIG_NAME, TRUE ) );
	}

/****************************************************************************
*																			*
*							Output/Formatting Routines						*
*																			*
****************************************************************************/

#ifdef __OS390__

static int asciiToEbcdic( const int ch )
	{
	char convBuffer[ 2 ];

	convBuffer[ 0 ] = ch;
	convBuffer[ 1 ] = '\0';
	__atoe( convBuffer ); /* Convert ASCII to EBCDIC for 390 */
	return( convBuffer[ 0 ] );
	}
#endif /* __OS390__ */

/* Indent a string by the appropriate amount */

static void doIndent( const int level )
	{
	int i;

	for( i = 0; i < level; i++ )
		fprintf( output, printDots ? ". " : \
						 shallowIndent ? " " : "  " );
	}

/* Complain about an error in the ASN.1 object */

static void complain( const char *message, const int level )
	{
	if( !doPure )
		fprintf( output, INDENT_STRING );
	doIndent( level + 1 );
	fprintf( output, "Error: %s.\n", message );
	noErrors++;
	}

/* Dump data as a string of hex digits up to a maximum of 128 bytes */

static void dumpHex( FILE *inFile, long length, int level, int isInteger )
	{
	const int lineLength = ( dumpText ) ? 8 : 16;
	char printable[ 9 ];
	long noBytes = length;
	int zeroPadded = FALSE, warnPadding = FALSE, warnNegative = isInteger;
	int singleLine = FALSE;
	int maxLevel = ( doPure ) ? 15 : 8, i;

	/* Check if LHS status info + indent + "OCTET STRING" string + data will
	   wrap */
	if( ( ( doPure ) ? 0 : INDENT_SIZE ) + ( level * 2 ) + 12 + \
		( length * 3 ) < outputWidth )
		singleLine = TRUE;

	if( noBytes > 128 && !printAllData )
		noBytes = 128;	/* Only output a maximum of 128 bytes */
	if( level > maxLevel )
		level = maxLevel;	/* Make sure that we don't go off edge of screen */
	printable[ 8 ] = printable[ 0 ] = '\0';
	for( i = 0; i < noBytes; i++ )
		{
		int ch;

		if( !( i % lineLength ) )
			{
			if( singleLine )
				putchar( ' ' );
			else
				{
				if( dumpText )
					{
					/* If we're dumping text alongside the hex data, print
					   the accumulated text string */
					fputs( "    ", output );
					fputs( printable, output );
					}
				fputc( '\n', output );
				if( !doPure )
					fprintf( output, INDENT_STRING );
				doIndent( level + 1 );
				}
			}
		ch = getc( inFile );
		fprintf( output, "%s%02X", i % lineLength ? " " : "", ch );
		printable[ i % 8 ] = ( ch >= ' ' && ch < 127 ) ? ch : '.';
		fPos++;

		/* If we need to check for negative values and zero padding, check
		   this now */
		if( !i )
			{
			if( !ch )
				zeroPadded = TRUE;
			if( !( ch & 0x80 ) )
				warnNegative = FALSE;
			}
		if( i == 1 && zeroPadded && ch < 0x80 )
			warnPadding = TRUE;
		}
	if( dumpText )
		{
		/* Print any remaining text */
		i %= lineLength;
		printable[ i ] = '\0';
		while( i < lineLength )
			{
			fprintf( output, "   " );
			i++;
			}
		fputs( "    ", output );
		fputs( printable, output );
		}
	if( length > 128 && !printAllData )
		{
		length -= 128;
		fputc( '\n', output );
		if( !doPure )
			fprintf( output, INDENT_STRING );
		doIndent( level + 5 );
		fprintf( output, "[ Another %ld bytes skipped ]", length );
		fPos += length;
		if( useStdin )
			{
			while( length-- )
				getc( inFile );
			}
		else
			fseek( inFile, length, SEEK_CUR );
		}
	fputs( "\n", output );

	if( isInteger )
		{
		if( warnPadding )
			complain( "Integer has non-DER encoding", level );
		if( warnNegative )
			complain( "Integer has a negative value", level );
		}
	}

/* Dump a bitstring, reversing the bits into the standard order in the
   process */

static void dumpBitString( FILE *inFile, const int length, const int unused,
						   const int level )
	{
	unsigned int bitString = 0, currentBitMask = 0x80, remainderMask = 0xFF;
	int bitFlag, value = 0, noBits, bitNo = -1, i;
	char *errorStr = NULL;

	if( unused < 0 || unused > 7 )
		complain( "Invalid number of unused bits", level );
	noBits = ( length * 8 ) - unused;

	/* ASN.1 bitstrings start at bit 0, so we need to reverse the order of
	   the bits if necessary */
	if( length )
		{
		bitString = fgetc( inFile );
		fPos++;
		}
	for( i = noBits - 8; i > 0; i -= 8 )
		{
		bitString = ( bitString << 8 ) | fgetc( inFile );
		currentBitMask <<= 8;
		remainderMask = ( remainderMask << 8 ) | 0xFF;
		fPos++;
		}
	if( reverseBitString )
		{
		for( i = 0, bitFlag = 1; i < noBits; i++ )
			{
			if( bitString & currentBitMask )
				value |= bitFlag;
			if( !( bitString & remainderMask ) )
				{
				/* The last valid bit should be a one bit */
				errorStr = "Spurious zero bits in bitstring";
				}
			bitFlag <<= 1;
			bitString <<= 1;
			}
		if( noBits < sizeof( int ) && \
			( ( remainderMask << noBits ) & value ) )
			{
			/* There shouldn't be any bits set after the last valid one.  We
			   have to do the noBits check to avoid a fencepost error when
			   there's exactly 32 bits */
			errorStr = "Spurious one bits in bitstring";
			}
		}
	else
		value = bitString;

	/* Now that it's in the right order, dump it.  If there's only one bit
	   set (which is often the case for bit flags) we also print the bit
	   number to save users having to count the zeroes to figure out which
	   flag is set */
	fputc( '\n', output );
	if( !doPure )
		fprintf( output, INDENT_STRING );
	doIndent( level + 1 );
	fputc( '\'', output );
	if( reverseBitString )
		currentBitMask = 1 << ( noBits - 1 );
	for( i = 0; i < noBits; i++ )
		{
		if( value & currentBitMask )
			{
			bitNo = ( bitNo == -1 ) ? ( noBits - 1 ) - i : -2;
			fputc( '1', output );
			}
		else
			fputc( '0', output );
		currentBitMask >>= 1;
		}
	if( bitNo >= 0 )
		fprintf( output, "'B (bit %d)\n", bitNo );
	else
		fputs( "'B\n", output );

	if( errorStr != NULL )
		complain( errorStr, level );
	}

/* Display data as a text string up to a maximum of 240 characters (8 lines
   of 48 chars to match the hex limit of 8 lines of 16 bytes) with special
   treatement for control characters and other odd things that can turn up
   in BMPString and UniversalString types.

   If the string is less than 40 chars in length, we try to print it on the
   same line as the rest of the text (even if it wraps), otherwise we break
   it up into 48-char chunks in a somewhat less nice text-dump format */

static void displayString( FILE *inFile, long length, int level,
						   STR_OPTION strOption )
	{
	char timeStr[ 64 ];
#ifdef __OS390__
	char convBuffer[ 2 ];
#endif /* __OS390__ */
	long noBytes = length;
	int lineLength = 48, maxLevel = ( doPure ) ? 15 : 8, i;
	int firstTime = TRUE, doTimeStr = FALSE, warnIA5 = FALSE;
	int warnPrintable = FALSE, warnTime = FALSE, warnBMP = FALSE;

	if( noBytes > 384 && !printAllData )
		noBytes = 384;	/* Only output a maximum of 384 bytes */
	if( strOption == STR_UTCTIME || strOption == STR_GENERALIZED )
		{
		if( ( strOption == STR_UTCTIME && length != 13 ) || \
			( strOption == STR_GENERALIZED && length != 15 ) )
			warnTime = TRUE;
		else
			doTimeStr = rawTimeString ? FALSE : TRUE;
		}
	if( !doTimeStr && length <= 40 )
		fprintf( output, " '" );		/* Print string on same line */
	if( level > maxLevel )
		level = maxLevel;	/* Make sure that we don't go off edge of screen */
	for( i = 0; i < noBytes; i++ )
		{
		int ch;

		/* If the string is longer than 40 chars, break it up into multiple
		   sections */
		if( length > 40 && !( i % lineLength ) )
			{
			if( !firstTime )
				fputc( '\'', output );
			fputc( '\n', output );
			if( !doPure )
				fprintf( output, INDENT_STRING );
			doIndent( level + 1 );
			fputc( '\'', output );
			firstTime = FALSE;
			}
		ch = getc( inFile );
#if defined( __WIN32__ ) || defined( __UNIX__ ) || defined( __OS390__ )
		if( strOption == STR_BMP )
			{
			if( i == noBytes - 1 && ( noBytes & 1 ) )
				/* Odd-length BMP string, complain */
				warnBMP = TRUE;
			else
				{
				const wchar_t wCh = ( ch << 8 ) | getc( inFile );
				char outBuf[ 8 ];
#ifdef __OS390__
				char *p;
#endif /* OS-specific charset handling */
				int outLen;

				/* Attempting to display Unicode characters is pretty hit and
				   miss, and if it fails nothing is displayed.  To try and
				   detect this we use wcstombs() to see if anything can be
				   displayed, if it can't we drop back to trying to display
				   the data as non-Unicode.  There's one exception to this
				   case, which is for a wrong-endianness Unicode string, for
				   which the first character looks like a single ASCII char */
				outLen = wcstombs( outBuf, &wCh, 1 );
				if( outLen < 1 )
					{
					/* Can't be displayed as Unicode, fall back to
					   displaying it as normal text */
					ungetc( wCh & 0xFF, inFile );
					}
				else
					{
					lineLength++;
					i++;	/* We've read two characters for a wchar_t */
#if defined( __WIN32__ ) 
					fputwc( wCh, output );
#elif defined( __UNIX__ ) && !( defined( __MACH__ ) || defined( __OpenBSD__ ) )
					/* Some Unix environments differentiate between char 
					   and wide-oriented stdout (!!!), so it's necessary to 
					   manually switch the orientation of stdout to make it 
					   wide-oriented before calling a widechar output 
					   function or nothing will be output (exactly what 
					   level of braindamage it takes to have an 
					   implementation function like this is a mystery).  In 
					   order to safely display widechars, we therefore have 
					   to use the fwide() kludge function to change stdout 
					   modes around the display of the widechar */
					if( fwide( output, 1 ) > 0 )
						{
						fputwc( wCh, output );
						fwide( output, -1 );
						}
					else
						fputc( wCh, output );
#else
  #ifdef __OS390__
					/* This could use some improvement */
					for( p = outBuf; *p != '\0'; p++ )
						*p = asciiToEbcdic( *p );
  #endif /* IBM ASCII -> EBCDIC conversion */
					fprintf( output, "%s", outBuf );
#endif /* OS-specific charset handling */
					fPos += 2;
					continue;
					}
				}
			}
#endif /* __WIN32__ || __UNIX__ || __OS390__ */
		switch( strOption )
			{
			case STR_PRINTABLE:
			case STR_IA5:
			case STR_LATIN1:
				if( strOption == STR_PRINTABLE && !isPrintable( ch ) )
					warnPrintable = TRUE;
				if( strOption == STR_IA5 && !isIA5( ch ) )
					warnIA5 = TRUE;
				if( strOption == STR_LATIN1 )
					{
					if( !isprint( ch & 0x7F ) )
						ch = '.';	/* Convert non-ASCII to placeholders */
					}
				else
					{
					if( !isprint( ch ) )
						ch = '.';	/* Convert non-ASCII to placeholders */
					}
#ifdef __OS390__
				ch = asciiToEbcdic( ch );
#endif /* __OS390__ */
				break;

			case STR_UTCTIME:
			case STR_GENERALIZED:
				if( !isdigit( ch ) && ch != 'Z' )
					{
					warnTime = TRUE;
					if( !isprint( ch ) )
						ch = '.';	/* Convert non-ASCII to placeholders */
					}
#ifdef __OS390__
				ch = asciiToEbcdic( ch );
#endif /* __OS390__ */
				break;

			case STR_BMP_REVERSED:
				if( i == noBytes - 1 && ( noBytes & 1 ) )
					{
					/* Odd-length BMP string, complain */
					warnBMP = TRUE;
					}

				/* Wrong-endianness BMPStrings (Microsoft Unicode) can't be
				   handled through the usual widechar-handling mechanism
				   above since the first widechar looks like an ASCII char
				   followed by a null terminator, so we just treat them as
				   ASCII chars, skipping the following zero byte.  This is
				   safe since the code that detects reversed BMPStrings
				   has already checked that every second byte is zero */
				getc( inFile );
				i++;
				fPos++;
				/* Drop through */

			default:
				if( !isprint( ch ) )
					ch = '.';	/* Convert control chars to placeholders */
#ifdef __OS390__
				ch = asciiToEbcdic( ch );
#endif /* __OS390__ */
			}
		if( doTimeStr )
			timeStr[ i ] = ch;
		else
			fputc( ch, output );
		fPos++;
		}
	if( length > 384 && !printAllData )
		{
		length -= 384;
		fprintf( output, "'\n" );
		if( !doPure )
			fprintf( output, INDENT_STRING );
		doIndent( level + 5 );
		fprintf( output, "[ Another %ld characters skipped ]", length );
		fPos += length;
		while( length-- )
			{
			int ch = getc( inFile );

			if( strOption == STR_PRINTABLE && !isPrintable( ch ) )
				warnPrintable = TRUE;
			if( strOption == STR_IA5 && !isIA5( ch ) )
				warnIA5 = TRUE;
			}
		}
	else
		if( doTimeStr )
			{
			const char *timeStrPtr = ( strOption == STR_UTCTIME ) ? \
									 timeStr : timeStr + 2;

			fprintf( output, " %c%c/%c%c/", timeStrPtr[ 4 ], timeStrPtr[ 5 ],
					 timeStrPtr[ 2 ], timeStrPtr[ 3 ] );
			if( strOption == STR_UTCTIME )
				fprintf( output, ( timeStr[ 0 ] < '5' ) ? "20" : "19" );
			else
				fprintf( output, "%c%c", timeStr[ 0 ], timeStr[ 1 ] );
			fprintf( output, "%c%c %c%c:%c%c:%c%c GMT", timeStrPtr[ 0 ],
					 timeStrPtr[ 1 ], timeStrPtr[ 6 ], timeStrPtr[ 7 ],
					 timeStrPtr[ 8 ], timeStrPtr[ 9 ], timeStrPtr[ 10 ],
					 timeStrPtr[ 11 ] );
			}
		else
			fputc( '\'', output );
	fputc( '\n', output );

	/* Display any problems we encountered */
	if( warnPrintable )
		complain( "PrintableString contains illegal character(s)", level );
	if( warnIA5 )
		complain( "IA5String contains illegal character(s)", level );
	if( warnTime )
		complain( "Time is encoded incorrectly", level );
	if( warnBMP )
		complain( "BMPString has missing final byte/half character", level );
	}

/****************************************************************************
*																			*
*								ASN.1 Parsing Routines						*
*																			*
****************************************************************************/

/* Get an integer value */

static long getValue( FILE *inFile, const long length )
	{
	long value;
	char ch;
	int i;

	ch = getc( inFile );
	value = ch;
	for( i = 0; i < length - 1; i++ )
		value = ( value << 8 ) | getc( inFile );
	fPos += length;

	return( value );
	}

/* Get an ASN.1 objects tag and length */

static int getItem( FILE *inFile, ASN1_ITEM *item )
	{
	int tag, length, index = 0;

	memset( item, 0, sizeof( ASN1_ITEM ) );
	item->indefinite = FALSE;
	tag = item->header[ index++ ] = fgetc( inFile );
	item->id = tag & ~TAG_MASK;
	tag &= TAG_MASK;
	if( tag == TAG_MASK )
		{
		int value;

		/* Long tag encoded as sequence of 7-bit values.  This doesn't try to
		   handle tags > INT_MAX, it'd be pretty peculiar ASN.1 if it had to
		   use tags this large */
		tag = 0;
		do
			{
			value = fgetc( inFile );
			tag = ( tag << 7 ) | ( value & 0x7F );
			item->header[ index++ ] = value;
			fPos++;
			}
		while( value & LEN_XTND && index < 5 && !feof( inFile ) );
		if( index == 5 )
			{
			fPos++;		/* Tag */
			return( FALSE );
			}
		}
	item->tag = tag;
	if( feof( inFile ) )
		{
		fPos++;
		return( FALSE );
		}
	fPos += 2;			/* Tag + length */
	length = item->header[ index++ ] = fgetc( inFile );
	item->headerSize = index;
	if( length & LEN_XTND )
		{
		int i;

		length &= LEN_MASK;
		if( length > 4 )
			{
			/* Impossible length value, probably because we've run into
			   the weeds */
			return( -1 );
			}
		item->headerSize += length;
		item->length = 0;
		if( !length )
			item->indefinite = TRUE;
		for( i = 0; i < length; i++ )
			{
			int ch = fgetc( inFile );

			item->length = ( item->length << 8 ) | ch;
			item->header[ i + index ] = ch;
			}
		fPos += length;
		}
	else
		item->length = length;

	return( TRUE );
	}

/* Check whether a BIT STRING or OCTET STRING encapsulates another object */

static int checkEncapsulate( FILE *inFile, const int length )
	{
	ASN1_ITEM nestedItem;
	const int currentPos = fPos;
	int diffPos;

	/* If we're not looking for encapsulated objects, return */
	if( !checkEncaps )
		return( FALSE );

	/* Read the details of the next item in the input stream */
	getItem( inFile, &nestedItem );
	diffPos = fPos - currentPos;
	fPos = currentPos;
	fseek( inFile, -diffPos, SEEK_CUR );

	/* If it fits exactly within the current item and has a valid-looking
	   tag, treat it as nested data */
	if( ( ( nestedItem.id & CLASS_MASK ) == UNIVERSAL || \
		  ( nestedItem.id & CLASS_MASK ) == CONTEXT ) && \
		( nestedItem.tag > 0 && nestedItem.tag <= 0x31 ) && \
		nestedItem.length == length - diffPos )
		return( TRUE );

	return( FALSE );
	}

/* Check whether a zero-length item is OK */

static int zeroLengthOK( const ASN1_ITEM *item )
	{
	/* An implicitly-tagged NULL can have a zero length.  An occurrence of this
	   type of item is almost always an error, however OCSP uses a weird status
	   encoding that encodes result values in tags and then has to use a NULL
	   value to indicate that there's nothing there except the tag that encodes
	   the status, so we allow this as well if zero-length content is explicitly
	   enabled */
	if( zeroLengthAllowed && ( item->id & CLASS_MASK ) == CONTEXT )
		return( TRUE );

	/* If we can't recognise the type from the tag, reject it */
	if( ( item->id & CLASS_MASK ) != UNIVERSAL )
		return( FALSE );

	/* The following types are zero-length by definition */
	if( item->tag == EOC || item->tag == NULLTAG )
		return( TRUE );

	/* A real with a value of zero has zero length */
	if( item->tag == REAL )
		return( TRUE );

	/* Everything after this point requires input from the user to say that
	   zero-length data is OK (usually it's not, so we flag it as a
	   problem) */
	if( !zeroLengthAllowed )
		return( FALSE );

	/* String types can have zero length except for the Unrestricted
	   Character String type ([UNIVERSAL 29]) which has to have at least one
	   octet for the CH-A/CH-B index */
	if( item->tag == OCTETSTRING || item->tag == NUMERICSTRING || \
		item->tag == PRINTABLESTRING || item->tag == T61STRING || \
		item->tag == VIDEOTEXSTRING || item->tag == VISIBLESTRING || \
		item->tag == IA5STRING || item->tag == GRAPHICSTRING || \
		item->tag == GENERALSTRING || item->tag == UNIVERSALSTRING || \
		item->tag == BMPSTRING || item->tag == UTF8STRING || \
		item->tag == OBJDESCRIPTOR )
		return( TRUE );

	/* SEQUENCE and SET can be zero if there are absent optional/default
	   components */
	if( item->tag == SEQUENCE || item->tag == SET )
		return( TRUE );

	return( FALSE );
	}

/* Check whether the next item looks like text */

static STR_OPTION checkForText( FILE *inFile, const int length )
	{
	char buffer[ 16 ];
	int isBMP = FALSE, isUnicode = FALSE;
	int sampleLength = min( length, 16 ), i;

	/* If the sample is very short, we're more careful about what we
	   accept */
	if( sampleLength < 4 )
		{
		/* If the sample size is too small, don't try anything */
		if( sampleLength <= 2 )
			return( STR_NONE );

		/* For samples of 3-4 characters we only allow ASCII text.  These
		   short strings are used in some places (eg PKCS #12 files) as
		   IDs */
		sampleLength = fread( buffer, 1, sampleLength, inFile );
		fseek( inFile, -sampleLength, SEEK_CUR );
		for( i = 0; i < sampleLength; i++ )
			{
			if( !( isalpha( buffer[ i ] ) || isdigit( buffer[ i ] ) || \
				   isspace( buffer[ i ] ) ) )
				return( STR_NONE );
			}
		return( STR_IA5 );
		}

	/* Check for ASCII-looking text */
	sampleLength = fread( buffer, 1, sampleLength, inFile );
	fseek( inFile, -sampleLength, SEEK_CUR );
	if( isdigit( buffer[ 0 ] ) && ( length == 13 || length == 15 ) && \
		buffer[ length - 1 ] == 'Z' )
		{
		/* It looks like a time string, make sure that it really is one */
		for( i = 0; i < length - 1; i++ )
			{
			if( !isdigit( buffer[ i ] ) )
				break;
			}
		if( i == length - 1 )
			return( ( length == 13 ) ? STR_UTCTIME : STR_GENERALIZED );
		}
	for( i = 0; i < sampleLength; i++ )
		{
		/* If even bytes are zero, it could be a BMPString.  Initially
		   we set isBMP to FALSE, if it looks like a BMPString we set it to
		   TRUE, if we then encounter a nonzero byte it's neither an ASCII
		   nor a BMPString */
		if( !( i & 1 ) )
			{
			if( !buffer[ i ] )
				{
				/* If we thought we were in a Unicode string but we've found a
				   zero byte where it'd occur in a BMP string, it's neither a
				   Unicode nor BMP string */
				if( isUnicode )
					return( STR_NONE );

				/* We've collapsed the eigenstate (in an earlier incarnation
				   isBMP could take values of -1, 0, or 1, with 0 being
				   undecided, in which case this comment made a bit more
				   sense) */
				if( i < sampleLength - 2 )
					{
					/* If the last char(s) are zero but preceding ones
					   weren't, don't treat it as a BMP string.  This can
					   happen when storing a null-terminated string if the
					   implementation gets the length wrong and stores the
					   null as well */
					isBMP = TRUE;
					}
				continue;
				}
			else
				{
				/* If we thought we were in a BMPString but we've found a
				   nonzero byte where there should be a zero, it's neither
				   an ASCII nor BMP string */
				if( isBMP )
					return( STR_NONE );
				}
			}
		else
			{
			/* Just to make it tricky, Microsoft stuff Unicode strings into
			   some places (to avoid having to convert them to BMPStrings,
			   presumably) so we have to check for these as well */
			if( !buffer[ i ] )
				{
				if( isBMP )
					return( STR_NONE );
				isUnicode = TRUE;
				continue;
				}
			else
				if( isUnicode )
					return( STR_NONE );
			}
		if( buffer[ i ] < 0x20 || buffer[ i ] > 0x7E )
			return( STR_NONE );
		}

	/* It looks like a text string */
	return( isUnicode ? STR_BMP_REVERSED : isBMP ? STR_BMP : STR_IA5 );
	}

/* Dump the header bytes for an object, useful for vgrepping the original
   object from a hex dump */

static void dumpHeader( FILE *inFile, const ASN1_ITEM *item )
	{
	int extraLen = 24 - item->headerSize, i;

	/* Dump the tag and length bytes */
	if( !doPure )
		fprintf( output, "    " );
	fprintf( output, "<%02X", *item->header );
	for( i = 1; i < item->headerSize; i++ )
		fprintf( output, " %02X", item->header[ i ] );

	/* If we're asked for more, dump enough extra data to make up 24 bytes.
	   This is somewhat ugly since it assumes we can seek backwards over the
	   data, which means it won't always work on streams */
	if( extraLen > 0 && doDumpHeader > 1 )
		{
		/* Make sure that we don't print too much data.  This doesn't work
		   for indefinite-length data, we don't try and guess the length with
		   this since it involves picking apart what we're printing */
		if( extraLen > item->length && !item->indefinite )
			extraLen = ( int ) item->length;

		for( i = 0; i < extraLen; i++ )
			{
			int ch = fgetc( inFile );

			if( feof( inFile ) )
				extraLen = i;	/* Exit loop and get fseek() correct */
			else
				fprintf( output, " %02X", ch );
			}
		fseek( inFile, -extraLen, SEEK_CUR );
		}

	fputs( ">\n", output );
	}

/* Print a constructed ASN.1 object */

static int printAsn1( FILE *inFile, const int level, long length, 
					  const int isIndefinite );

static void printConstructed( FILE *inFile, int level, const ASN1_ITEM *item )
	{
	int result;

	/* Special case for zero-length objects */
	if( !item->length && !item->indefinite )
		{
		fputs( " {}\n", output );
		return;
		}

	fputs( " {\n", output );
	result = printAsn1( inFile, level + 1, item->length, item->indefinite );
	if( result )
		{
		fprintf( output, "Error: Inconsistent object length, %d byte%s "
				 "difference.\n", result, ( result > 1 ) ? "s" : "" );
		noErrors++;
		}
	if( !doPure )
		fprintf( output, INDENT_STRING );
	fprintf( output, ( printDots ) ? ". " : "  " );
	doIndent( level );
	fputs( "}\n", output );
	}

/* Print a single ASN.1 object */

static void printASN1object( FILE *inFile, ASN1_ITEM *item, int level )
	{
	OIDINFO *oidInfo;
	STR_OPTION stringType;
	char buffer[ MAX_OID_SIZE ];
	long value;
	int x, y;

	if( ( item->id & CLASS_MASK ) != UNIVERSAL )
		{
		static const char *const classtext[] =
			{ "UNIVERSAL ", "APPLICATION ", "", "PRIVATE " };

		/* Print the object type */
		fprintf( output, "[%s%d]",
				 classtext[ ( item->id & CLASS_MASK ) >> 6 ], item->tag );

		/* Perform a sanity check */
		if( ( item->tag != NULLTAG ) && ( item->length < 0 ) )
			{
			int i;

			fprintf( stderr, "\nError: Object has bad length field, tag = %02X, "
					 "length = %lX, value =", item->tag, item->length );
			fprintf( stderr, "<%02X", *item->header );
			for( i = 1; i < item->headerSize; i++ )
				fprintf( stderr, " %02X", item->header[ i ] );
			fputs( ">.\n", stderr );
			exit( EXIT_FAILURE );
			}

		if( !item->length && !item->indefinite && !zeroLengthOK( item ) )
			{
			fputc( '\n', output );
			complain( "Object has zero length", level );
			return;
			}

		/* If it's constructed, print the various fields in it */
		if( ( item->id & FORM_MASK ) == CONSTRUCTED )
			{
			printConstructed( inFile, level, item );
			return;
			}

		/* It's primitive, if it's a seekable stream try and determine
		   whether it's text so we can display it as such */
		if( !useStdin && \
			( stringType = checkForText( inFile, item->length ) ) != STR_NONE )
			{
			/* It looks like a text string, dump it as text */
			displayString( inFile, item->length, level, stringType );
			return;
			}

		/* This could be anything, dump it as hex data */
		dumpHex( inFile, item->length, level, FALSE );

		return;
		}

	/* Print the object type */
	fprintf( output, "%s", idstr( item->tag ) );

	/* Perform a sanity check */
	if( ( item->tag != NULLTAG ) && ( item->length < 0 ) )
		{
		int i;

		fprintf( stderr, "\nError: Object has bad length field, tag = %02X, "
				 "length = %lX, value =", item->tag, item->length );
		fprintf( stderr, "<%02X", *item->header );
		for( i = 1; i < item->headerSize; i++ )
			fprintf( stderr, " %02X", item->header[ i ] );
		fputs( ">.\n", stderr );
		exit( EXIT_FAILURE );
		}

	/* If it's constructed, print the various fields in it */
	if( ( item->id & FORM_MASK ) == CONSTRUCTED )
		{
		printConstructed( inFile, level, item );
		return;
		}

	/* It's primitive */
	if( !item->length && !zeroLengthOK( item ) )
		{
		fputc( '\n', output );
		complain( "Object has zero length", level );
		return;
		}
	switch( item->tag )
		{
		case BOOLEAN:
			x = getc( inFile );
			fprintf( output, " %s\n", x ? "TRUE" : "FALSE" );
			if( x != 0 && x != 0xFF )
				complain( "BOOLEAN has non-DER encoding", level );
			fPos++;
			break;

		case INTEGER:
		case ENUMERATED:
			if( item->length > 4 )
				dumpHex( inFile, item->length, level, TRUE );
			else
				{
				value = getValue( inFile, item->length );
				fprintf( output, " %ld\n", value );
				if( value < 0 )
					complain( "Integer has a negative value", level );
				}
			break;

		case BITSTRING:
			if( ( x = getc( inFile ) ) != 0 )
				fprintf( output, " %d unused bit%s",
						 x, ( x != 1 ) ? "s" : "" );
			fPos++;
			if( !--item->length && !x )
				{
				fputc( '\n', output );
				complain( "Object has zero length", level );
				return;
				}
			if( item->length <= sizeof( int ) )
				{
				/* It's short enough to be a bit flag, dump it as a sequence
				   of bits */
				dumpBitString( inFile, ( int ) item->length, x, level );
				break;
				}
			/* Drop through to dump it as an octet string */

		case OCTETSTRING:
			if( checkEncapsulate( inFile, item->length ) )
				{
				/* It's something encapsulated inside the string, print it as
				   a constructed item */
				fprintf( output, ", encapsulates" );
				printConstructed( inFile, level, item );
				break;
				}
			if( !useStdin && !dumpText && \
				( stringType = checkForText( inFile, item->length ) ) != STR_NONE )
				{
				/* If we'd be doing a straight hex dump and it looks like
				   encapsulated text, display it as such.  If the user has
				   overridden character set type checking and it's a string
				   type for which we normally perform type checking, we reset
				   its type to none */
				displayString( inFile, item->length, level, \
					( !checkCharset && ( stringType == STR_IA5 || \
										 stringType == STR_PRINTABLE ) ) ? \
					STR_NONE : stringType );
				return;
				}
			dumpHex( inFile, item->length, level, FALSE );
			break;

		case OID:
			/* Hierarchical Object Identifier: The first two levels are
			   encoded into one byte, since the root level has only 3 nodes
			   (40*x + y).  However if x = joint-iso-itu-t(2) then y may be
			   > 39, so we have to add special-case handling for this */
			if( item->length > MAX_OID_SIZE )
				{
				fprintf( stderr, "\nError: Object identifier length %ld too "
						 "large.\n", item->length );
				exit( EXIT_FAILURE );
				}
			fread( buffer, 1, ( size_t ) item->length, inFile );
			fPos += item->length;
			if( ( oidInfo = getOIDinfo( buffer, ( int ) item->length ) ) != NULL )
				{
				/* Check if LHS status info + indent + "OID " string + oid
				   name will wrap */
				if( ( ( doPure ) ? 0 : INDENT_SIZE ) + ( level * 2 ) + 18 + \
					strlen( oidInfo->description ) >= outputWidth )
					{
					fputc( '\n', output );
					if( !doPure )
						fprintf( output, INDENT_STRING );
					doIndent( level + 1 );
					}
				else
					fputc( ' ', output );
				fprintf( output, "%s\n", oidInfo->description );

				/* Display extra comments about the OID if required */
				if( extraOIDinfo && oidInfo->comment != NULL )
					{
					if( !doPure )
						fprintf( output, INDENT_STRING );
					doIndent( level + 1 );
					fprintf( output, "(%s)\n", oidInfo->comment );
					}

				/* If there's a warning associated with this OID, remember
				   that there was a problem */
				if( oidInfo->warn )
					noWarnings++;

				break;
				}

			/* Pick apart the OID */
			x = ( unsigned char ) buffer[ 0 ] / 40;
			y = ( unsigned char ) buffer[ 0 ] % 40;
			if( x > 2 )
				{
				/* Handle special case for large y if x = 2 */
				y += ( x - 2 ) * 40;
				x = 2;
				}
			fprintf( output, " '%d %d", x, y );
			value = 0;
			for( x = 1; x < item->length; x++ )
				{
				value = ( value << 7 ) | ( buffer[ x ] & 0x7F );
				if( !( buffer[ x ] & 0x80 ) )
					{
					fprintf( output, " %ld", value );
					value = 0;
					}
				}
			fprintf( output, "'\n" );
			break;

		case EOC:
		case NULLTAG:
			fputc( '\n', output );
			break;

		case OBJDESCRIPTOR:
		case GRAPHICSTRING:
		case VISIBLESTRING:
		case GENERALSTRING:
		case UNIVERSALSTRING:
		case NUMERICSTRING:
		case VIDEOTEXSTRING:
		case UTF8STRING:
			displayString( inFile, item->length, level, STR_NONE );
			break;
		case PRINTABLESTRING:
			displayString( inFile, item->length, level, STR_PRINTABLE );
			break;
		case BMPSTRING:
			displayString( inFile, item->length, level, STR_BMP );
			break;
		case UTCTIME:
			displayString( inFile, item->length, level, STR_UTCTIME );
			break;
		case GENERALIZEDTIME:
			displayString( inFile, item->length, level, STR_GENERALIZED );
			break;
		case IA5STRING:
			displayString( inFile, item->length, level, STR_IA5 );
			break;
		case T61STRING:
			displayString( inFile, item->length, level, STR_LATIN1 );
			break;

		default:
			fputc( '\n', output );
			if( !doPure )
				fprintf( output, INDENT_STRING );
			doIndent( level + 1 );
			fprintf( output, "Unrecognised primitive, hex value is:");
			dumpHex( inFile, item->length, level, FALSE );
			noErrors++;		/* Treat it as an error */
		}
	}

/* Print a complex ASN.1 object */

static int printAsn1( FILE *inFile, const int level, long length,
					  const int isIndefinite )
	{
	ASN1_ITEM item;
	long lastPos = fPos;
	int seenEOC = FALSE, status;

	/* Special-case for zero-length objects */
	if( !length && !isIndefinite )
		return( 0 );

	while( ( status = getItem( inFile, &item ) ) > 0 )
		{
		/* Perform various special checks the first time we're called */
		if( length == LENGTH_MAGIC )
			{
			/* If the length isn't known and the item has a definite length,
			   set the length to the item's length */
			if( !item.indefinite )
				length = item.headerSize + item.length;

			/* If the input isn't seekable, turn off some options that
			   require the use of fseek().  This check isn't perfect (some
			   streams are slightly seekable due to buffering) but it's
			   better than nothing */
			if( fseek( inFile, -item.headerSize, SEEK_CUR ) )
				{
				useStdin = TRUE;
				checkEncaps = FALSE;
				puts( "Warning: Input is non-seekable, some functionality "
					  "has been disabled." );
				}
			else
				fseek( inFile, item.headerSize, SEEK_CUR );
			}

		/* Dump the header as hex data if requested */
		if( doDumpHeader )
			dumpHeader( inFile, &item );

		/* Print offset into buffer, tag, and length */
		if( item.header[ 0 ] == EOC )
			{
			seenEOC = TRUE;
			if( !isIndefinite)
				complain( "Spurious EOC in definite-length item", level );
			}
		if( !doPure )
			{
#if 0
			/* Don't print hex tags any more to save display space */
			if( item.indefinite )
				fprintf( output, ( doHexValues ) ? "%04lX %02X NDEF: " :
						 "%4ld %02X NDEF: ", lastPos, item.id | item.tag );
			else
				if( !seenEOC )
					fprintf( output, ( doHexValues ) ? "%04lX %02X %4lX: " :
							 "%4ld %02X %4ld: ", lastPos, item.id | item.tag,
							 item.length );
#else
			if( item.indefinite )
				fprintf( output, ( doHexValues ) ? "%04lX NDEF: " :
						 "%4ld NDEF: ", lastPos );
			else
				if( !seenEOC )
					fprintf( output, ( doHexValues ) ? "%04lX %4lX: " :
							 "%4ld %4ld: ", lastPos, item.length );
#endif
			}

		/* Print details on the item */
		if( !seenEOC )
			{
			doIndent( level );
			printASN1object( inFile, &item, level );
			}

		/* If it was an indefinite-length object (no length was ever set) and
		   we've come back to the top level, exit */
		if( length == LENGTH_MAGIC )
			return( 0 );

		length -= fPos - lastPos;
		lastPos = fPos;
		if( isIndefinite )
			{
			if( seenEOC )
				return( 0 );
			}
		else
			{
			if( length <= 0 )
				{
				if( length < 0 )
					return( ( int ) -length );
				return( 0 );
				}
			else
				{
				if( length == 1 )
					{
					const int ch = fgetc( inFile );

					/* No object can be one byte long, try and recover.  This
					   only works sometimes because it can be caused by
					   spurious data in an OCTET STRING hole or an incorrect
					   length encoding.  The following workaround tries to
					   recover from spurious data by skipping the byte if
					   it's zero or a non-basic-ASN.1 tag, but keeping it if
					   it could be valid ASN.1 */
					if( ch && ch <= 0x31 )
						ungetc( ch, inFile );
					else
						{
						fPos++;
						return( 1 );
						}
					}
				}
			}
		}
	if( status == -1 )
		{
		int i;

		fprintf( stderr, "\nError: Invalid data encountered at position "
				 "%d:", fPos );
		for( i = 0; i < item.headerSize; i++ )
			fprintf( stderr, " %02X", item.header[ i ] );
		fprintf( stderr, ".\n" );
		exit( EXIT_FAILURE );
		}

	/* If we see an EOF and there's supposed to be more data present,
	   complain */
	if( length && length != LENGTH_MAGIC )
		{
		fprintf( output, "Error: Inconsistent object length, %ld byte%s "
				 "difference.\n", length, ( length > 1 ) ? "s" : "" );
		noErrors++;
		}
	return( 0 );
	}

/* Show usage and exit */

static void usageExit( void )
	{
	puts( "DumpASN1 - ASN.1 object dump/syntax check program." );
	puts( "Copyright Peter Gutmann 1997 - 2006.  Last updated " UPDATE_STRING "." );
	puts( "" );

	puts( "Usage: dumpasn1 [-acdefhlprstuxz] <file>" );
	puts( "  Input options:" );
	puts( "       - = Take input from stdin (some options may not work properly)" );
	puts( "       -<number> = Start <number> bytes into the file" );
	puts( "       -- = End of arg list" );
	puts( "       -c<file> = Read Object Identifier info from alternate config file" );
	puts( "            (values will override equivalents in global config file)" );
	puts( "" );

	puts( "  Output options:" );
	puts( "       -f<file> = Dump object at offset -<number> to file (allows data to be" );
	puts( "            extracted from encapsulating objects)" );
	puts( "       -w<number> = Set width of output, default = 80 columns" );
	puts( "" );

	puts( "  Display options:" );
	puts( "       -a = Print all data in long data blocks, not just the first 128 bytes" );
	puts( "       -d = Print dots to show column alignment" );
	puts( "       -h = Hex dump object header (tag+length) before the decoded output" );
	puts( "       -hh = Same as -h but display more of the object as hex data" );
	puts( "       -i = Use shallow indenting, for deeply-nested objects" );
	puts( "       -l = Long format, display extra info about Object Identifiers" );
	puts( "       -p = Pure ASN.1 output without encoding information" );
	puts( "       -t = Display text values next to hex dump of data" );
	puts( "" );

	puts( "  Format options:" );
	puts( "       -e = Don't print encapsulated data inside OCTET/BIT STRINGs" );
	puts( "       -r = Print bits in BIT STRING as encoded in reverse order" );
	puts( "       -u = Don't format UTCTime/GeneralizedTime string data" );
	puts( "       -x = Display size and offset in hex not decimal" );
	puts( "" );

	puts( "  Checking options:" );
	puts( "       -o = Don't check validity of character strings hidden in octet strings" );
	puts( "       -s = Syntax check only, don't dump ASN.1 structures" );
	puts( "       -z = Allow zero-length items" );
	puts( "" );

	puts( "Warnings generated by deprecated OIDs require the use of '-l' to be displayed." );
	puts( "Program return code is the number of errors found or EXIT_SUCCESS." );
	exit( EXIT_FAILURE );
	}

int main( int argc, char *argv[] )
	{
	FILE *inFile, *outFile = NULL;
#ifdef __OS390__
	char pathPtr[ FILENAME_MAX ];
#else
	char *pathPtr = argv[ 0 ];
#endif /* __OS390__ */
	long offset = 0;
	int moreArgs = TRUE, doCheckOnly = FALSE;

#ifdef __OS390__
	memset( pathPtr, '\0', sizeof( pathPtr ) );
	getcwd( pathPtr, sizeof( pathPtr ) );
	strcat( pathPtr, "/" );
#endif /* __OS390__ */

	/* Skip the program name */
	argv++; argc--;

	/* Display usage if no args given */
	if( argc < 1 )
		usageExit();
	output = stdout;	/* Needs to be assigned at runtime */

	/* Check for arguments */
	while( argc && *argv[ 0 ] == '-' && moreArgs )
		{
		char *argPtr = argv[ 0 ] + 1;

		if( !*argPtr )
			useStdin = TRUE;
		while( *argPtr )
			{
			if( isdigit( *argPtr ) )
				{
				offset = atol( argPtr );
				break;
				}
			switch( toupper( *argPtr ) )
				{
				case '-':
					moreArgs = FALSE;	/* GNU-style end-of-args flag */
					break;

				case 'A':
					printAllData = TRUE;
					break;

				case 'C':
					if( !readConfig( argPtr + 1, FALSE ) )
						exit( EXIT_FAILURE );
					while( argPtr[ 1 ] )
						argPtr++;	/* Skip rest of arg */
					break;

				case 'D':
					printDots = TRUE;
					break;

				case 'E':
					checkEncaps = FALSE;
					break;

				case 'F':
					if( ( outFile = fopen( argPtr + 1, "wb" ) ) == NULL )
						{
						perror( argPtr + 1 );
						exit( EXIT_FAILURE );
						}
					while( argPtr[ 1 ] )
						argPtr++;	/* Skip rest of arg */
					break;

				case 'I':
					shallowIndent = TRUE;
					break;

				case 'L':
					extraOIDinfo = TRUE;
					break;

				case 'H':
					doDumpHeader++;
					break;

				case 'O':
					checkCharset = TRUE;
					break;

				case 'P':
					doPure = TRUE;
					break;

				case 'R':
					reverseBitString = !reverseBitString;
					break;

				case 'S':
					doCheckOnly = TRUE;
#if defined( __WIN32__ )
					/* Under Windows we can't fclose( stdout ) because the
					   VC++ runtime reassigns the stdout handle to the next
					   open file (which is valid) but then scribbles stdout
					   garbage all over it for files larger than about 16K
					   (which isn't), so we have to make sure that the
					   stdout handle is pointed to something somewhere */
					freopen( "nul", "w", stdout );
#elif defined( __UNIX__ )
					/* Safety feature in case any Unix libc is as broken
					   as the Win32 version */
					freopen( "/dev/null", "w", stdout );
#else
					fclose( stdout );
#endif /* OS-specific bypassing of stdout */
					break;

				case 'T':
					dumpText = TRUE;
					break;

				case 'U':
					rawTimeString = TRUE;
					break;

				case 'W':
					outputWidth = atoi( argPtr + 1 );
					if( outputWidth < 40 )
						{
						puts( "Invalid output width." );
						exit( EXIT_FAILURE );
						}
					while( argPtr[ 1 ] )
						argPtr++;	/* Skip rest of arg */
					break;

				case 'X':
					doHexValues = TRUE;
					break;

				case 'Z':
					zeroLengthAllowed = TRUE;
					break;

				default:
					printf( "Unknown argument '%c'.\n", *argPtr );
					return( EXIT_SUCCESS );
				}
			argPtr++;
			}
		argv++;
		argc--;
		}

	/* We can't use options that perform an fseek() if reading from stdin */
	if( useStdin && ( doDumpHeader || outFile != NULL ) )
		{
		puts( "Can't use -f or -h when taking input from stdin" );
		exit( EXIT_FAILURE );
		}

	/* Check args and read the config file.  We don't bother weeding out
	   dups during the read because (a) the linear search would make the
	   process n^2, (b) during the dump process the search will terminate on
	   the first match so dups aren't that serious, and (c) there should be
	   very few dups present */
	if( argc != 1 && !useStdin )
		usageExit();
	if( !readGlobalConfig( pathPtr ) )
		exit( EXIT_FAILURE );

	/* Dump the given file */
	if( useStdin )
		inFile = stdin;
	else
		if( ( inFile = fopen( argv[ 0 ], "rb" ) ) == NULL )
			{
			perror( argv[ 0 ] );
			exit( EXIT_FAILURE );
			}
	if( useStdin )
		{
		while( offset-- )
			getc( inFile );
		}
	else
		fseek( inFile, offset, SEEK_SET );
	if( outFile != NULL )
		{
		ASN1_ITEM item;
		long length;
		int i, status;

		/* Make sure that there's something there, and that it has a
		   definite length */
		status = getItem( inFile, &item );
		if( status == -1 )
			{
			puts( "Non-ASN.1 data encountered." );
			exit( EXIT_FAILURE );
			}
		if( status == 0 )
			{
			puts( "Nothing to read." );
			exit( EXIT_FAILURE );
			}
		if( item.indefinite )
			{
			puts( "Cannot process indefinite-length item." );
			exit( EXIT_FAILURE );
			}

		/* Copy the item across, first the header and then the data */
		for( i = 0; i < item.headerSize; i++ )
			putc( item.header[ i ], outFile );
		for( length = 0; length < item.length && !feof( inFile ); length++ )
			putc( getc( inFile ), outFile );
		fclose( outFile );

		fseek( inFile, offset, SEEK_SET );
		}
	printAsn1( inFile, 0, LENGTH_MAGIC, 0 );
	if( !useStdin && offset == 0 )
		{
		unsigned char buffer[ 16 ];
		long position = ftell( inFile );

		/* If we're dumping a standalone ASN.1 object and there's further
		   data appended to it, warn the user of its existence.  This is a
		   bit hit-and-miss since there may or may not be additional EOCs
		   present, dumpasn1 always stops once it knows that the data should
		   end (without trying to read any trailing EOCs) because data from
		   some sources has the EOCs truncated, and most apps know that they
		   have to stop at min( data_end, EOCs ).  To avoid false positives,
		   we skip at least 4 EOCs worth of data and if there's still more
		   present, we complain */
		fread( buffer, 1, 8, inFile );	/* Skip 4 EOCs */
		if( !feof( inFile ) )
			{
			fprintf( output, "Warning: Further data follows ASN.1 data at "
					 "position %ld.\n", position );
			noWarnings++;
			}
		}
	fclose( inFile );

	/* Print a summary of warnings/errors if it's required or appropriate */
	if( !doPure )
		{
		if( !doCheckOnly )
			fputc( '\n', stderr );
		fprintf( stderr, "%d warning%s, %d error%s.\n", noWarnings,
				( noWarnings != 1 ) ? "s" : "", noErrors,
				( noErrors != 1 ) ? "s" : "" );
		}

	return( ( noErrors ) ? noErrors : EXIT_SUCCESS );
	}
