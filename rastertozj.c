// To get required headers, run
// sudo apt-get install libcups2-dev libcupsimage2-dev
#include <cups/cups.h>
#include <cups/ppd.h>
#include <cups/raster.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>

#ifndef DEBUGFILE
#define DEBUGFILE "/tmp/debugraster.txt"
#endif

// settings and their stuff
struct settings_ {
  int cashDrawer1;
  int cashDrawer2;
  int blankSpace;
  int feedDist;
};
struct settings_ settings;

// get an option
static inline int getOptionChoiceIndex(const char *sChoiceName,
                                       ppd_file_t *pPpd) {
  ppd_choice_t *pChoice = ppdFindMarkedChoice(pPpd, sChoiceName);
  if (!pChoice) {
    ppd_option_t *pOption = ppdFindOption(pPpd, sChoiceName);
    if (pOption)
      pChoice = ppdFindChoice(pOption, pOption->defchoice);
  }
  return pChoice ? atoi(pChoice->choice) : -1;
}

static void initializeSettings(char *commandLineOptionSettings, struct settings_ *pSettings) {
  ppd_file_t *pPpd = ppdOpenFile(getenv("PPD"));
  ppdMarkDefaults(pPpd);

  cups_option_t *pOptions = NULL;
  int iNumOptions = cupsParseOptions(commandLineOptionSettings, 0, &pOptions);
  if (iNumOptions && pOptions) {
    cupsMarkOptions(pPpd, iNumOptions, pOptions);
    cupsFreeOptions(iNumOptions, pOptions);
  }

  memset(pSettings, 0, sizeof(struct settings_));
  pSettings->cashDrawer1 = getOptionChoiceIndex("CashDrawer1Setting", pPpd);
  pSettings->cashDrawer2 = getOptionChoiceIndex("CashDrawer2Setting", pPpd);
  pSettings->blankSpace = getOptionChoiceIndex("BlankSpace", pPpd);
  pSettings->feedDist = getOptionChoiceIndex("FeedDist", pPpd);

  ppdClose(pPpd);
}

#ifndef DEBUGP
static inline void mputchar(char c) { putchar(c); }
#define DEBUGSTARTPRINT()
#define DEBUGFINISHPRINT()
#define DEBUGPRINT(...)
#else
FILE *lfd = 0;
// putchar with debug wrapper
static inline void mputchar(char c) {
  unsigned char m = c;
  if (lfd)
    fprintf(lfd, "%02x ", m);
  putchar(m);
}
#define DEBUGSTARTPRINT() lfd = fopen(DEBUGFILE, "w")
#define DEBUGFINISHPRINT()                                                     \
  if (lfd)                                                                     \
  fclose(lfd)
#define DEBUGPRINT(...) if (lfd) fprintf(lfd, "\n" __VA_ARGS__)
#endif

// procedure to output an array
static inline void outputarray(const char *array, int length) {
  int i = 0;
  for (; i < length; ++i)
    mputchar(array[i]);
}

// output a command. -1 is because we determine them as string literals,
// so \0 implicitly added at the end, but we don't need it at all
#define SendCommand(COMMAND) outputarray((COMMAND),sizeof((COMMAND))-1)

static inline void mputnum(unsigned int val) {
  mputchar(val&0xFFU);
  mputchar((val>>8)&0xFFU);
}

/*
 * zj uses kind of epson ESC/POS dialect code. Here is subset of commands
 *
 * initialize - esc @
 * cash drawer 1 - esc p 0 @ P
 * cash drawer 2 - esc p 1 @ P  // @ and P <N> and <M>
 *    where <N>*2ms is pulse on time, <M>*2ms is pulse off.
 * start raster - GS v 0 // 0 is full-density, may be also 1, 2, 4
 * skip lines - esc J // then N [0..255], each value 1/44 inches (0.176mm)
 * // another commands out-of-spec:
 * esc 'i' - cutter; xprinter android example shows as GS V \1 (1D 56 01)
 * esc '8' - wait
 */

/* todo:
 * generate pulse by esc p 0/ esc p 1/
 * XPrinter 58
 */

// define printer initialize command
static const char escInit[] = "\x1b@";

// define cashDrawerEjector command
static const char escCashDrawer1Eject[] = "\x1bp\0@P";
static const char escCashDrawer2Eject[] = "\x1bp\1@P";

// define raster mode start command
static const char escRasterMode[] = "\x1dv0\0";

// define flush command
static const char escFlush[] = "\x1bJ";

// define cut command
static const char escCut[] = "\x1bi";

// enter raster mode and set up x and y dimensions
static inline void sendRasterHeader(int xsize, int ysize) {
  //  outputCommand(rasterModeStartCommand);
  SendCommand(escRasterMode);
  mputnum(xsize);
  mputnum(ysize);
}

// print all unprinted (i.e. flush the buffer)
static inline void flushBuffer() {
  SendCommand(escFlush);
  mputchar(0);
}

// flush, then feed 24 lines
static inline void flushAndFeed() {
  SendCommand(escFlush);
  mputchar(0x18);
}

// sent on the beginning of print job
void setupJob() {
  if (settings.cashDrawer1 == 1)
    SendCommand(escCashDrawer1Eject);
  if (settings.cashDrawer2 == 1)
    SendCommand(escCashDrawer2Eject);
  SendCommand(escInit);
}

// sent at the very end of print job
void finalizeJob() {
  if (settings.cashDrawer1 == 2)
    SendCommand(escCashDrawer1Eject);
  if (settings.cashDrawer2 == 2)
    SendCommand(escCashDrawer2Eject);
  SendCommand(escInit);
}

// sent at the end of every page
#ifndef __sighandler_t
typedef void (*__sighandler_t)(int);
#endif

__sighandler_t old_signal;
void finishPage() {
  int i;
  for (i = 0; i < settings.feedDist; ++i)
    flushAndFeed();
  signal(15, old_signal);
}

// sent on job canceling
void cancelJob() {
  int i = 0;
  for (; i < 0x258; ++i)
    mputchar(0);
  finishPage();
  finalizeJob();
}

// invoked before starting to print a page
void beforePage() { old_signal = signal(15, cancelJob); }

static inline int min(int a, int b) {
  if (a > b)
    return b;
  return a;
}

//////////////////////////////////////////////
//////////////////////////////////////////////
int main(int argc, char *argv[]) {
  if (argc < 6 || argc > 7) {
    fputs("ERROR: rastertozj job-id user title copies options [file]\n",
          stderr);
    return EXIT_FAILURE;
  }

  int fd = STDIN_FILENO; // File descriptor providing CUPS raster data
  if (argc == 7) {
    if ((fd = open(argv[6], O_RDONLY)) == -1) {
      perror("ERROR: Unable to open raster file - ");
      sleep(1);
      return EXIT_FAILURE;
    }
  }

  int iCurrentPage = 0;
  // CUPS Page tHeader
  cups_page_header2_t tHeader;
  unsigned char *pRasterBuf = NULL; // Pointer to raster data from CUPS
  // Raster stream for printing
  cups_raster_t *pRasterSrc = cupsRasterOpen(fd, CUPS_RASTER_READ);
  initializeSettings(argv[5],&settings);

  setupJob();

  DEBUGSTARTPRINT();

  // loop over the whole raster, page by page
  while (cupsRasterReadHeader2(pRasterSrc, &tHeader)) {
    if ((!tHeader.cupsHeight) || (!tHeader.cupsBytesPerLine))
      break;

    DEBUGPRINT("tHeader.cupsHeight=%d, tHeader.cupsBytesPerLine=%d\n",
               tHeader.cupsHeight, tHeader.cupsBytesPerLine);

    if (!pRasterBuf) {
      pRasterBuf = malloc(tHeader.cupsBytesPerLine * 24);
      if (!pRasterBuf) { // hope it never goes here...
        cupsRasterClose(pRasterSrc);
        if (fd)
          close(fd);
        return EXIT_FAILURE;
      }
    }

    fprintf(stderr, "PAGE: %d %d\n", ++iCurrentPage, tHeader.NumCopies);

    beforePage();

    // calculate num of bytes to print given width having 1 bit per pixel.
    int foo = min(tHeader.cupsWidth, 0x180);
    foo = (foo + 7) & 0xFFFFFFF8;
    int width_bytes = foo >> 3; // in bytes, [0..0x30]

    DEBUGPRINT("tHeader.cupsWidth=%d, foo=%d, width_size=%d\n", tHeader.cupsWidth,
               foo, width_bytes);

    int iPageYPos = 0; // Vertical position in page. [0..tHeader.cupsHeight]
    int zeroy = 0;

    // loop over one page, top to bottom by blocks of most 24 scan lines
    while (iPageYPos < tHeader.cupsHeight) {
      if (iPageYPos & 127)
        fprintf(stderr, "INFO: Printing iCurrentPage %d, %d%% complete...\n", iCurrentPage,
                (100 * iPageYPos / tHeader.cupsHeight));

      int iBlockHeight = min(tHeader.cupsHeight - iPageYPos, 24);

      DEBUGPRINT("Processing block of %d, starting from %d lines\n", iBlockHeight, iPageYPos);
      iPageYPos += iBlockHeight;

      if (iPageYPos) {
        unsigned char *buf = pRasterBuf;
        int j;
        for (j = 0; j < iBlockHeight; ++j) {
          if (!cupsRasterReadPixels(pRasterSrc, buf, tHeader.cupsBytesPerLine))
            break;
          buf += width_bytes;
        }
        DEBUGPRINT("Read %d lines\n", j);
        if (j < iBlockHeight) // wtf?
          continue;
      }
      int rest_bytes = iBlockHeight * width_bytes;
      int j;
      for (j = 0; j < rest_bytes; ++j)
        if (pRasterBuf[j])
          break;

      DEBUGPRINT("Checked %d bytes of %d for zero\n", j, rest_bytes);

      if (j >= rest_bytes) { // true, if whole block iz zeroes
        ++zeroy;
        continue;
      }

      for (; zeroy > 0; --zeroy)
        flushAndFeed();

      sendRasterHeader(width_bytes, iBlockHeight);
      outputarray((char *)pRasterBuf, rest_bytes);
      flushBuffer();
    }

    if (settings.blankSpace)
      for (; zeroy > 0; --zeroy)
        flushAndFeed();

    finishPage();
  }

  finalizeJob();
  if (pRasterBuf)
    free(pRasterBuf);
  cupsRasterClose(pRasterSrc);
  if (fd)
    close(fd);
  fputs(iCurrentPage ? "INFO: Ready to print.\n" : "ERROR: No pages found!\n",
        stderr);
  DEBUGFINISHPRINT();
  return iCurrentPage ? EXIT_SUCCESS : EXIT_FAILURE;
}

// end of rastertozj.c
