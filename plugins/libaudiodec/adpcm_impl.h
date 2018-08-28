#ifndef ADPCM_H
#define ADPCM_H

/*
 * adpcm.h - include file for adpcm coder.
 */
#ifdef __cplusplus
extern "C" {
#endif

struct adpcm_state {
    short	valprev;	/* Previous output value */
    char	index;		/* Index into stepsize table */
};

void adpcm_coder(unsigned char*, const short*, unsigned int, struct adpcm_state*);
void adpcm_decoder(short*, const void*, unsigned int, struct adpcm_state*, unsigned int);
void adpcm_init_table(void);

#ifdef __cplusplus
}
#endif

#endif /* ADPCM_H */
