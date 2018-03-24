#ifndef PARSE_H_INCLUDED
#define PARSE_H_INCLUDED


#ifndef PATH_MAX
#define PATH_MAX 1024
#endif


typedef struct ReaderConfig
{
    sound_file_format input_format;
    int   swapbytes;                /* force byte swapping   default=0 */
    int   swap_channel;             /* 0: no-op, 1: swaps input channels */
    int   input_samplerate;
} ReaderConfig;

typedef struct WriterConfig
{
    int   flush_write;
} WriterConfig;

typedef struct UiConfig
{
    int   silent;                   /* Verbosity */
    int   brhist;
    int   print_clipping_info;      /* print info whether waveform clips */
    float update_interval;          /* to use Frank's time status display */
} UiConfig;

typedef struct DecoderConfig
{
    int   mp3_delay;                /* to adjust the number of samples truncated during decode */
    int   mp3_delay_set;            /* user specified the value of the mp3 encoder delay to assume for decoding */
    int   disable_wav_header;
    mp3data_struct mp3input_data;
} DecoderConfig;

typedef enum ByteOrder { ByteOrderLittleEndian, ByteOrderBigEndian } ByteOrder;

typedef struct RawPCMConfig
{
    int     in_bitwidth;
    int     in_signed;
    ByteOrder in_endian;
} RawPCMConfig;



extern ReaderConfig global_reader;
extern WriterConfig global_writer;
extern UiConfig global_ui_config;
extern DecoderConfig global_decoder;
extern RawPCMConfig global_raw_pcm;



#if defined(__cplusplus)
extern "C" {
#endif

int     usage(FILE * const fp, const char *ProgramName);
int     short_help(const lame_global_flags * gfp, FILE * const fp, const char *ProgramName);
int     long_help(const lame_global_flags * gfp, FILE * const fp, const char *ProgramName,
                  int lessmode);
int     display_bitrates(FILE * const fp);

int     parse_args(lame_global_flags * gfp, int argc, char **argv, char *const inPath,
                   char *const outPath, char **nogap_inPath, int *max_nogap);

void    parse_close();

#if defined(__cplusplus)
}
#endif

#endif
/* end of parse.h */
