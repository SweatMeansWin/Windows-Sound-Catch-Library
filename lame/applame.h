#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

extern size_t (*lame_read_sample_buffer)(void *buffer, size_t size,  size_t count, FILE *stream , char* pData, int dataLen);
extern int lame_main(lame_t gf, int argc, char **argv);
extern int get_audio(lame_t gfp, int buffer[2][1152], char* pData, int dataLen);

#if defined(__cplusplus)
}
#endif
