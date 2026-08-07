#ifndef ATG_ENGINE_SIM_STB_VORBIS_H
#define ATG_ENGINE_SIM_STB_VORBIS_H

// Minimal prototype for the single function this project uses out of
// stb_vorbis.c (public domain, https://github.com/nothings/stb). The
// declarations live in stb_vorbis.c itself, not a separate header, so
// callers outside that translation unit need this instead.

#ifdef __cplusplus
extern "C" {
#endif

// Decodes an entire .ogg file to interleaved 16-bit PCM in one call.
// Returns the number of samples decoded per channel (0 on failure).
// *output is malloc'd internally by stb_vorbis -- free() it, not delete[].
extern int stb_vorbis_decode_filename(
    const char *filename, int *channels, int *sample_rate, short **output);

#ifdef __cplusplus
}
#endif

#endif /* ATG_ENGINE_SIM_STB_VORBIS_H */
