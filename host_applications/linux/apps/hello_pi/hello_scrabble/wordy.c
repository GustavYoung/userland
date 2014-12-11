/**************************************************************************/
/*                                 Anagram Utility                        */
/*                                                                        */
/*                                     D Cobley                           */
/*                                     22/3/01                            */
/*                                                                        */
/**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//#define Wordfile "f:\\erj\\demo\\jabble\\java\\diction.trie"
#define Wordfile "uk.lxd"

//#include "srch.h"
#define FILE_OPENING_ERROR 3
#define MALLOC_ERROR 2
#define MAXLEN 40
#define NOARGS 1
#define XOUT '@'
#define ANY_LETTER '?'
#define MULTI_LETTER '*'
#define MAX_BUFFER (8*1024)
#define BLANK 27
#define BLANKBIT (1<<5)
#define LAST_EDGE 0x80000000
#define WORD 0x40000000
#define CHAR_SHIFT 22
#define CHAR_MASK 0x1f
#define INDEX_SHIFT 0
#define INDEX_MASK 0x001fffff
#define LEN 15

typedef enum { FALSE, TRUE } Boolean;

typedef struct {                /* structure to hold configuration information */
	char pattern_set[MAXLEN];
	char letter_set[MAXLEN];
	char dictionary_file[MAXLEN];
	char output_file[MAXLEN];
	int endianSwap;
} DIS_CONFIGURATION;

static DIS_CONFIGURATION config; /* configuration options */

static void doWork(char *letter_set, char *pattern_set);
static Boolean get_upper( char *letter_set );
static int *loadtrie(void);
static size_t my_fread( void *buffer, size_t size, size_t count, FILE *stream );
static void swap_endian( void *buffer, size_t size, size_t count );
static void extend(int edge, int c);

static int *rackLetter;
static char letter[MAXLEN];
static int nletters;
static char printString[MAXLEN];
static int wcount = 0;
static char *pattern;
static int *edgeList;

static void swap_endian( void *buffer, size_t size, size_t count )
{
	unsigned char *byte_buffer = (unsigned char *)buffer;
	size_t i;
	//int one = 1, little_endian = (*(char*)&one);
	unsigned char temp;

	//if (little_endian)
	{
		if (size == 2)
		{
			for (i = 0; i<count; i++)
			{
				temp = byte_buffer[0];
				byte_buffer[0] = byte_buffer[1];
				byte_buffer[1] = temp;
				byte_buffer += size;
			}
		}
		else if (size == 4)
		{
			for (i = 0; i<count; i++)
			{
				temp = byte_buffer[0];
				byte_buffer[0] = byte_buffer[3];
				byte_buffer[3] = temp;

				temp = byte_buffer[1];
				byte_buffer[1] = byte_buffer[2];
				byte_buffer[2] = temp;

				byte_buffer += size;
			}
		}
	}
}

static size_t my_fread( void *buffer, size_t size, size_t count, FILE *stream )
{
	/* normal read */
	size_t result = fread(buffer, size, count, stream);

	/* swap endianness after reading */
	/* result may be less than count if less items were read than requested */
	swap_endian(buffer, size, result);
	
	return result;
}


static int *loadtrie()
{
	FILE *fp;
	int *dawg;
	int nedges;	
	
	if (fp = fopen(config.dictionary_file, "rb"), !fp)
	{
		printf( "Cannot open dictionary file (%s)\n", config.dictionary_file);
		exit( FILE_OPENING_ERROR );
	}

	fseek(fp, 0, SEEK_END);
	nedges = ftell(fp)/sizeof(int);

	dawg = (int*)malloc((nedges)*sizeof(int));
	if (!dawg)
	{
		puts( "Memory allocation error!" );
		exit(MALLOC_ERROR);
	}

	fseek(fp, 0, SEEK_SET);
	fread(dawg, sizeof(int), nedges, fp);
	fclose(fp);

	if (config.endianSwap)
	{
		swap_endian( dawg, sizeof(int), nedges );

		if (fp = fopen(config.dictionary_file, "wb"), !fp)
		{
			printf( "Cannot open dictionary file for writing (%s)\n", config.dictionary_file);
			exit( FILE_OPENING_ERROR );
		}
		fwrite(dawg, sizeof(int), nedges, fp);
		fclose(fp);
		printf( "Dictionary written (%s)\n", config.dictionary_file);
		exit(0);
	}

	return dawg;
}


static void extend(int edge, int c) {
	int node;
	int nextEdge;
	//if (wcount >= 10000) return;

	// Maybe we've completed a word
	if (pattern[c] == 0) {
		if ((edge & WORD) != 0) {
			int j;
			for (j = 0; j < nletters; j++)
				printString[j] = (char)(letter[j]+'A'-1);

			printString[nletters] = ' ';
			printString[nletters+1] = 0;
			printf(printString);
			wcount++;
		}
		return;
	}

	/* also call without multi letter wildcard */
	if ((pattern[c] == (char)MULTI_LETTER)) extend(edge, c+1);

	node = (edge >> INDEX_SHIFT) & INDEX_MASK;
	// Go further only if we can proceed in the DAWG
	if (node == 0) return;

    // There IS NOT a letter at c
    // Try all the possible edges from node,
    // making sure they don't stuff up cross-words.
    do {
        char ech;
		nextEdge = edgeList[node++];
        // letter on outgoing edge
        ech  = (char)((nextEdge >> CHAR_SHIFT) & CHAR_MASK);

		//System.out.print((char)('A'-1+ech));
		// if letter on edge is possible crossmove
		if ((pattern[c] == ech) || (pattern[c] == ANY_LETTER) || (pattern[c] == MULTI_LETTER)) {

			// if no letter and no blank, not possible word
			if ((rackLetter[(int)ech] == 0) && (rackLetter[BLANK] == 0))
				continue;

			// remove letter from rack, continue looking, put letter back in rack
			if (rackLetter[(int)ech] > 0) {
				rackLetter[(int)ech]--;
			    letter[nletters++] = ech;
				if ((pattern[c] == MULTI_LETTER)) extend(nextEdge, c);
				else extend(nextEdge, c+1);
			    nletters--;
				rackLetter[(int)ech]++;
			// remove blank from rack, continue looking, put blank back in rack
			} else {
				rackLetter[BLANK]--;
			    letter[nletters++] = (char)(ech | BLANKBIT);
				if ((pattern[c] == MULTI_LETTER)) extend(nextEdge, c);
				else extend(nextEdge, c+1);
			    nletters--;
				rackLetter[BLANK]++;
			}
		}
    } while ((nextEdge & LAST_EDGE) == 0);
	//if (((++counter)&16383)==0) myThread.yield();
}

static Boolean get_upper( char *letter_set )
{
	int i,j;

    fgets( letter_set, MAXLEN-1, stdin );

	for (i=0, j=0; letter_set[i]; i++)
	{
		if (letter_set[i] == 8)
			j = j>0 ? j-1:0;
		else if (isgraph(letter_set[i]))
			letter_set[j++] = letter_set[i];
	}
	letter_set[j] = 0;

	return (Boolean)j;
}

int main( int argc, char **argv )
{
	/* macro for pulling out the value supplied with a command line argument */
	#define GET_ARG(argv) ((argv[1] != NULL && argv[1][0] != '-') ? *++argv : (argv[0][2] != '\0') ? *argv+2 : NULL)
	/* default input filename */
	char *arg;
	char *invoked = argv[0];
	int i;
	int *dawg;

	/* store command line options in a structure */
	/* This consists of the option (ie -ifile), type of value, and description */
	typedef struct
	{
		char *options;
		char *value;
		char *description;
	} ARG_OPTS;

	static ARG_OPTS arg_opt[] = {
		{"?", "", "Print this help text"},
		{"help", "", "Print this help text"},
		{"letter", "(string)", "Letter set to use (?,* wildcards)"},
		{"pattern", "(string)", "Pattern to match (?,* wildcards)"},
		{"dictionary", "(filename)", "Dictionary file"},
		{"outfile", "(filename)", "Output file"},
		{"endian", "[01]", "Endian swap dictionary file"}
	};

	/* corresponding enumerations for previous structure. Make sure they tie up. */
	enum {ARG_HELP, ARG_HELP2, ARG_LETTERSET, ARG_PATTERNSET, ARG_DICTIONARY_FILE, ARG_FILE_OUT, ARG_ENDIAN, ARG_UNKNOWN };

	/* store defaults for arguments not supplied */
	strcpy(config.dictionary_file, Wordfile);
	strcpy(config.output_file, "");
	strcpy(config.pattern_set, "");
	strcpy(config.letter_set, "");
	

	/* scan through all supplied arguments */
	while (*++argv != NULL)
	{
		/* options begin with a '-' */
		if (**argv == '-')
		{ /* command switch parsing */
			int option, found = FALSE;

			/* look for supplied option matching one in arg_opt */
			/* can abbreviate if unique (so -c equivalent to -confidence) */
			for (option = 0; option < ARG_UNKNOWN; option++)
			{
				char *ptr1 = arg_opt[option].options, *ptr2 = argv[0]+1;
				while (tolower(*ptr1) == tolower(*ptr2))
				{ 
					ptr1++; ptr2++;
					if (*ptr2 == '\0') {found = TRUE; break;}
				}
				if (found) break;
			}
			
			/* see if option is in list */
			switch (option)
			{
				/* print help screen */
				case ARG_HELP: case ARG_HELP2:
				printf("Usage: %s <options>\n\n", invoked);
				printf("Options:\n");
				for (i=ARG_HELP2; i<ARG_UNKNOWN; i++)
					printf("    -%-12s %-10s %-30s\n", arg_opt[i].options, arg_opt[i].value, arg_opt[i].description);
				printf("\n");
				exit(1);
				break;

				case ARG_FILE_OUT:
				if (arg = GET_ARG(argv), arg)
					strcpy(config.output_file, arg);
				break;
				case ARG_DICTIONARY_FILE:
				if (arg = GET_ARG(argv), arg)
					strcpy(config.dictionary_file, arg);
				break;
				case ARG_LETTERSET:
				if (arg = GET_ARG(argv), arg)
					strcpy(config.letter_set, arg);
				break;
				case ARG_PATTERNSET:
				if (arg = GET_ARG(argv), arg)
					strcpy(config.pattern_set, arg);
				break;
				case ARG_ENDIAN:
				if (arg = GET_ARG(argv), arg)
					config.endianSwap = atoi(arg);
				break;

				/* not known */
				default:
				printf("Unknown switch: '%s'\n", *argv);
				exit(1);
				break;
			}
		}
	}

	dawg = loadtrie();
	edgeList = dawg+0x10;

	/* run in interactive mode */
	if( config.letter_set[0] == 0 && config.pattern_set[0] == 0 )
	{
		while (1)
		{
			printf("Enter letters to use (%c,%c are wildcards)   ", ANY_LETTER, MULTI_LETTER);
			get_upper(config.letter_set);
			printf("Enter pattern to match (%c,%c are wildcards) ", ANY_LETTER, MULTI_LETTER);
			get_upper(config.pattern_set);

			if (config.letter_set[0] == 0 && config.pattern_set[0] == 0)
				break;
			doWork(config.letter_set, config.pattern_set);
		}
	}
	else 
	{
		doWork(config.letter_set, config.pattern_set);
	}
	
	free (dawg);
	return 0;
}

static void doWork(char *letter_set, char *pattern_set)
{
	int i,j;

	/* upper case */
	for (i=0; letter_set[i]; i++)
		letter_set[i] = (char)toupper(letter_set[i]);
	for (i=0; pattern_set[i]; i++)
		pattern_set[i] = (char)toupper(pattern_set[i]);
	
	//pattern = pattern_set;
	wcount = 0;
	nletters = 0;

	rackLetter = (int*)calloc(28, sizeof(int));
	pattern = (char*)calloc(strlen(pattern_set)+1+1, sizeof(char));

	if (pattern_set[0] == 0)
		{pattern[0] = (char)MULTI_LETTER; pattern[1] = 0;}
	else
	{
		j = 0;
		for (i=0; pattern_set[i]; i++) {
			int pat = pattern_set[i];
			if (pat == '*') pattern[j++] = (char)MULTI_LETTER;
			else if (pat == '?') pattern[j++] = (char)ANY_LETTER;
			else if (pat >= 'A' && pat <= 'Z') pattern[j++] = (char)(pat-'A'+1);
		}
		pattern[j] = 0;
	}


	if (letter_set[0] == 0)
		{rackLetter[BLANK]=LEN;}
	else 
	{
		for (i=0; letter_set[i]; i++)
		{
			int let = letter_set[i];
			int pos = let-'A'+1;
			if (let == '*') rackLetter[BLANK] = LEN;
			else if (let == '?') rackLetter[BLANK]++;
			else if ((pos >= 0) && (pos < 28)) rackLetter[pos]++;
		}
	}

	//print(edgeList, 0, edgeList[1]); checkWord(NULL);
	extend(edgeList[1], 0);
	printf("(%d)\n", wcount);
	free(rackLetter);
	free(pattern);
}