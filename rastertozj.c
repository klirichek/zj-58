// To get required headers, run
// sudo apt-get install libcups2-dev libcupsimage2-dev
#include <cups/cups.h>
#include <cups/ppd.h>
#include <cups/raster.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>

// uncomment next line in order to have verbose dump in DEBUGFILE
// after print

//#define DEBUGP

#define DEBUGFILE "/tmp/debugraster.txt"

struct settings_
{
	int cashDrawer1;
	int cashDrawer2;
	int blankSpace;
	int feedDist;
};
struct settings_ settings;

struct command
{
	int    length;
	char* command;
};

// define printer initialize command
static const struct command printerInitializeCommand =
{2,(char[2]){0x1b,0x40}};

// define cashDrawerEjector command
static const struct command cashDrawerEject [2] =
{{5,(char[5]){0x1b,0x70,0,0x40,0x50}},
 {5,(char[5]){0x1b,0x70,1,0x40,0x50}}};

// define raster mode start command
static const struct command rasterModeStartCommand =
{4,(char[4]){0x1d,0x76,0x30,0}};

#ifdef DEBUGP
FILE* lfd = 0;
#endif

// putchar with debug wrapper
inline void mputchar(char c)
{
	unsigned char m = c;
#ifdef DEBUGP
	if (lfd)
		fprintf (lfd,"%02x ",m);
#endif
	putchar(m);
}

// procedure to output an array
inline void outputarray(const char * array, int length)
{
	int i = 0;
	for (;i<length;++i)
		mputchar(array[i]);
}

// output a command
inline void outputCommand(struct command output)
{
	outputarray(output.command,output.length);
}

inline int lo(int val)
{
	return val & 0xFF;
}

inline int hi (int val)
{
	return lo (val>>8);
}

// enter raster mode and set up x and y dimensions
inline void rasterheader(int xsize, int ysize)
{
	outputCommand(rasterModeStartCommand);
	mputchar(lo(xsize));
	mputchar(hi(xsize));
	mputchar(lo(ysize));
	mputchar(hi(ysize));
}

// print all unprinted (i.e. flush the buffer)
// then feed given number of lines
inline void skiplines(int size)
{
	mputchar(0x1b);
	mputchar(0x4a);
	mputchar(size);
}

// get an option
inline int getOptionChoiceIndex(const char * choiceName, ppd_file_t * ppd)
{
	ppd_choice_t * choice;
	ppd_option_t * option;

	choice = ppdFindMarkedChoice(ppd, choiceName);

	if (choice == NULL)
	{
		if ((option = ppdFindOption(ppd, choiceName))          == NULL) return -1;
		if ((choice = ppdFindChoice(option,option->defchoice)) == NULL) return -1;
	}

	return atoi(choice->choice);
}


inline void initializeSettings(char * commandLineOptionSettings)
{
	ppd_file_t *    ppd         = NULL;
	cups_option_t * options     = NULL;
	int             numOptions  = 0;

	ppd = ppdOpenFile(getenv("PPD"));

	ppdMarkDefaults(ppd);

	numOptions = cupsParseOptions(commandLineOptionSettings, 0, &options);
	if ((numOptions != 0) && (options != NULL))
	{
		cupsMarkOptions(ppd, numOptions, options);
		cupsFreeOptions(numOptions, options);
	}

	memset(&settings, 0x00, sizeof(struct settings_));

	settings.cashDrawer1  = getOptionChoiceIndex("CashDrawer1Setting", ppd);
	settings.cashDrawer2  = getOptionChoiceIndex("CashDrawer2Setting", ppd);
	settings.blankSpace   = getOptionChoiceIndex("BlankSpace"        , ppd);
	settings.feedDist     = getOptionChoiceIndex("FeedDist"          , ppd);

	ppdClose(ppd);
}

// sent on the beginning of print job
void jobSetup()
{
	if ( settings.cashDrawer1==1 )
		outputCommand(cashDrawerEject[0]);
	if ( settings.cashDrawer2==1 )
		outputCommand(cashDrawerEject[1]);
	outputCommand(printerInitializeCommand);
}

// sent at the very end of print job
void ShutDown()
{
	if ( settings.cashDrawer1==2 )
		outputCommand(cashDrawerEject[0]);
	if ( settings.cashDrawer2==2 )
		outputCommand(cashDrawerEject[1]);
	outputCommand(printerInitializeCommand);
}

// sent at the end of every page
__sighandler_t old_signal;
void EndPage()
{
	int i;
	for (i=0; i<settings.feedDist; ++i)
		skiplines(0x18);
	signal(15,old_signal);
}

// sent on job canceling
void cancelJob(int foo)
{
	int i=0;
	for(;i<0x258;++i)
		mputchar(0);
	EndPage();
	ShutDown();
}

// invoked before starting to print a page
void pageSetup()
{
	old_signal = signal(15,cancelJob);
}

//////////////////////////////////////////////
//////////////////////////////////////////////
int main(int argc, char *argv[])
{
	int fd = 0;					// File descriptor providing CUPS raster data
	cups_raster_t * ras = NULL; // Raster stream for printing
	cups_page_header2_t header; // CUPS Page header
	int page = 0;				// Current page
	int y = 0;					// Vertical position in page 0 <= y <= header.cupsHeight

	unsigned char * rasterData   = NULL;	// Pointer to raster data from CUPS
	unsigned char * originalRasterDataPtr   = NULL; // Copy of original pointer for freeing buffer

	if (argc < 6 || argc > 7)
	{
		fputs("ERROR: rastertozj job-id user title copies options [file]\n", stderr);
		return EXIT_FAILURE;
	}

	if (argc == 7)
	{
		if ((fd = open(argv[6], O_RDONLY)) == -1)
		{
			perror("ERROR: Unable to open raster file - ");
			sleep(1);
			return EXIT_FAILURE;
		}
	} else
		fd = 0;

#ifdef DEBUGP
	lfd = fopen ("/tmp/raster.txt","w");
#endif

	initializeSettings(argv[5]);
	jobSetup();
	ras = cupsRasterOpen(fd, CUPS_RASTER_READ);
	page = 0;

	while (cupsRasterReadHeader2(ras, &header))
	{
		if ((header.cupsHeight == 0) || (header.cupsBytesPerLine == 0))
			break;

#ifdef DEBUGP
	if (lfd) fprintf(lfd,"\nheader.cupsHeight=%d, header.cupsBytesPerLine=%d\n",header.cupsHeight,header.cupsBytesPerLine);
#endif

		if (rasterData == NULL)
		{
			rasterData = malloc(header.cupsBytesPerLine*24);
			if (rasterData == NULL)
			{
				if (originalRasterDataPtr   != NULL) free(originalRasterDataPtr);
					cupsRasterClose(ras);
				if (fd != 0)
			        close(fd);
				return EXIT_FAILURE;

			}
			originalRasterDataPtr = rasterData;  // used to later free the memory
		}

		fprintf(stderr, "PAGE: %d %d\n", ++page, header.NumCopies);
		pageSetup();

		int foo = ( header.cupsWidth > 0x180 ) ? 0x180 : header.cupsWidth;
		foo = (foo+7) & 0xFFFFFFF8;
		int width_size = foo >> 3;

#ifdef DEBUGP
	if (lfd) fprintf(lfd,"\nheader.cupsWidth=%d, foo=%d, width_size=%d\n",header.cupsWidth,foo,width_size);
#endif
		y = 0;
		int zeroy = 0;

		while ( y < header.cupsHeight )
		{
            if ((y & 127) != 0)
				fprintf(stderr, "INFO: Printing page %d, %d%% complete...\n", page, (100 * y / header.cupsHeight));

			int rest = header.cupsHeight-y;
			if (rest>24)
				rest=24;

#ifdef DEBUGP
	if (lfd) fprintf(lfd,"\nProcessing block of %d, starting from %d lines\n",rest,y);
#endif
			y+=rest;

			if (y)
			{
				unsigned char * buf = rasterData;
				int j;
				for ( j=0; j<rest; ++j)
				{
					if (!cupsRasterReadPixels(ras,buf,header.cupsBytesPerLine))
						break;
					buf+=width_size;
				}
#ifdef DEBUGP
	if (lfd) fprintf(lfd,"\nReaded %d lines\n",j);
#endif
				if (j<rest)
					continue;
			}
			int rest_bytes = rest * width_size;
			int j;
			for (j=0; j<rest_bytes;++j)
				if (rasterData[j]!=0)
					break;

#ifdef DEBUGP
	if (lfd) fprintf(lfd,"\nChecked %d bytes of %d for zero\n",j,rest_bytes);
#endif

			if (j>=rest_bytes)
			{
				++zeroy;
				continue;
			}

			for (;zeroy>0;--zeroy)
				skiplines(24);
			
			rasterheader(width_size,rest);
			outputarray((char*)rasterData,width_size*24);
			skiplines(0);
		}
		
		if (!settings.blankSpace)
			for(;zeroy>0;--zeroy)
				skiplines(24);

		EndPage();
	}

	ShutDown();

	if (originalRasterDataPtr   != NULL) free(originalRasterDataPtr);
	cupsRasterClose(ras);
	if (fd != 0)
		close(fd);

	if (page == 0)
		fputs("ERROR: No pages found!\n", stderr);
	else
		fputs("INFO: Ready to print.\n", stderr);

#ifdef DEBUGP
	if (lfd)
		fclose(lfd);
#endif

	return (page == 0)?EXIT_FAILURE:EXIT_SUCCESS;
}

// end of rastertozj.c
