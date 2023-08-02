#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <math.h>

typedef struct {
	unsigned attack;	/* Attack time, in ms */
	unsigned release;	/* Release time, in ms */
	unsigned tone;		/* Tone frequency, in Hz */
	unsigned samplerate;	/* Sample rate, in Hz (TODO: maybe should be uint32_t? It would make 96 kHz possible on 16-bit systems) */
	unsigned hithres;	/* Upper threshold, in percents */
	unsigned lothres;	/* Lower threshold, in percents */
	unsigned volume;	/* Output volume, in percents */
} config_t;

typedef enum {
	RET_CONTINUE,		/* Options parsed OK, can continue */
	RET_EXIT_SUCCESS,	/* -h present, return with success */
	RET_EXIT_FAILURE,	/* Options parsing error, return failure */
} parse_cli_ret_t;

typedef enum {
	GST_OFF,
	GST_ATTACK,
	GST_SUSTAIN,
	GST_RELEASE,
} generator_state_t;

typedef struct {
	uint16_t attack_samples;
	uint16_t release_samples;
	uint16_t rate_high;
	uint16_t rate_low;
	uint16_t volume;
	double angle_step;
} generator_config_t;

typedef struct {
	/* Current generator state */
	generator_state_t state;

	/* Current sine wave angle */
	double cur_angle;

	/* Current sample number during attack and release */
	uint32_t cur_sample;
} generator_ctx_t;

static void help(const config_t *defaults)
{
	printf("Syntax: remorse [options]\n");
	printf("Options:\n");
	printf("   -h: help (this screen)\n");
	printf("   -a <attack>: envelope attack time in ms (default: %u)\n", defaults->attack);
	printf("   -r <release>: envelope release time in ms (default: %u)\n", defaults->release);
	printf("   -t <tone>: output tone frequency in Hz (default: %u)\n", defaults->tone);
	printf("   -s <samplerate>: input and output sample rate, in Hz (default: %u)\n", defaults->samplerate);
	printf("   -u <hithres>: upper detection threshold, in percents (default: %u)\n", defaults->hithres);
	printf("   -l <lothres>: lower detection threshold, in percents (default: %u)\n", defaults->lothres);
	printf("   -v <volume>: output volume, in percents (default: %u)\n", defaults->volume);
	printf("\n");
	printf("Input and output is expected to be a raw, mono stream in 16-bit \n");
	printf("little-endian signed format.\n");
}

static parse_cli_ret_t parse_cli(config_t *config, int argc, char * const argv[])
{
	int opt;
	config_t defaults;

	memcpy(&defaults, config, sizeof(defaults));

	while((opt = getopt(argc, argv, ":a:hl:r:s:t:u:v:")) != -1) {
		switch(opt) {
			case '?':
				fprintf(stderr, "-%c: unknown option; use -h for help\n", optopt);
				return RET_EXIT_FAILURE;

			case ':':
				fprintf(stderr, "-%c: option requires an argument; use -h for help\n", optopt);
				return RET_EXIT_FAILURE;

			case 'a':
				config->attack = atoi(optarg);
				break;

			case 'h':
				help(&defaults);
				return RET_EXIT_SUCCESS;

			case 'l':
				config->lothres = atoi(optarg);
				break;

			case 'r':
				config->release = atoi(optarg);
				break;

			case 's':
				config->samplerate = atoi(optarg);
				break;

			case 't':
				config->tone = atoi(optarg);
				break;

			case 'u':
				config->hithres = atoi(optarg);
				break;

			case 'v':
				config->volume = atoi(optarg);
				break;

			default:
				fprintf(stderr, "Unexpected return value from getopt (%c)\n", opt);
				return RET_EXIT_FAILURE;
		}
	}

	if(argc > optind) {
		fprintf(stderr, "Excess argument after options; use -h for help\n");
		return RET_EXIT_FAILURE;
	}

	// TODO: check sanity of configuration

	return RET_CONTINUE;
}

static bool read_sample(int16_t *sample)
{
	uint8_t buf[2];
	if(fread(buf, sizeof(buf), 1, stdin) != 1) {
		return false;
	}

	*sample = (buf[1] << 8) | buf[0];
	return true;
}

static void write_sample(int16_t sample)
{
	uint8_t buf[2];
	buf[0] = sample & 0xff;
	buf[1] = sample >> 8;

	// TODO: check for return value
	fwrite(buf, sizeof(buf), 1, stdout);
}

static double envelope(uint16_t idx, uint16_t cnt)
{
	return sin((double) idx * (M_PI / 2) / (double) cnt);
}

static int16_t generate(const generator_config_t *genconf, const generator_ctx_t *ctx)
{
	double sample;

	if(ctx->state == GST_OFF) {
		return 0;
	}

	sample = sin(ctx->cur_angle);

	if(ctx->state == GST_ATTACK) {
		sample *= envelope(ctx->cur_sample, genconf->attack_samples);
	}
	else if(ctx->state == GST_RELEASE) {
		sample *= envelope(genconf->release_samples - ctx->cur_sample - 1, genconf->release_samples);
	}

	return sample * genconf->volume;
}

static void remorse(const config_t *config)
{
	generator_config_t genconf;
	generator_ctx_t ctx;

	genconf.attack_samples = (uint32_t) config->samplerate * config->attack / 1000;
	genconf.release_samples = (uint32_t) config->samplerate * config->release / 1000;
	genconf.rate_high = (uint32_t) config->hithres * 32768 / 100;
	genconf.rate_low = (uint32_t) config->lothres * 32768 / 100;
	genconf.volume = (uint32_t) config->volume * 32768 / 100;
	genconf.angle_step = 2 * M_PI * config->tone / config->samplerate;
	ctx.state = GST_OFF;
	ctx.cur_angle = 0;
	ctx.cur_sample = 0;

	if(genconf.rate_high > 32767) {
		genconf.rate_high = 32767;
	}

	if(genconf.rate_low > 32767) {
		genconf.rate_low = 32767;
	}

	if(genconf.volume > 32767) {
		genconf.volume = 32767;
	}

	double peak = 0;
	for(;;) {
		int16_t sample;

		if(!read_sample(&sample)) {
			break;
		}

		if(abs(sample) > peak) {
			peak = abs(sample);
		}
		else {
			// TODO: parametrize it
			peak *= 0.95;
		}

		switch(ctx.state) {
			case GST_OFF:
				if(peak > genconf.rate_high) {
					ctx.state = GST_ATTACK;
					ctx.cur_angle = 0;
					ctx.cur_sample = 0;
				}
				break;

			case GST_ATTACK:
				if(++ctx.cur_sample == genconf.attack_samples) {
					ctx.state = GST_SUSTAIN;
				}
				break;

			case GST_SUSTAIN:
				if(peak < genconf.rate_low) {
					ctx.state = GST_RELEASE;
					ctx.cur_sample = 0;
				}
				break;

			case GST_RELEASE:
				if(++ctx.cur_sample == genconf.release_samples) {
					ctx.state = GST_OFF;
				}
				break;
		}

		write_sample(generate(&genconf, &ctx));

		if(ctx.state != GST_OFF) {
			ctx.cur_angle += genconf.angle_step;
			if(ctx.cur_angle > 2 * M_PI) {
				ctx.cur_angle -= 2 * M_PI;
			}
		}
	}
}

int main(int argc, char * const argv[])
{
	config_t config;
	parse_cli_ret_t parse_cli_ret;

	/* Fill defaults */
	config.attack		= 10;
	config.release		= 10;
	config.tone		= 600;
	config.samplerate	= 48000;
	config.hithres		= 40;
	config.lothres		= 10;
	config.volume		= 90;

	parse_cli_ret = parse_cli(&config, argc, argv);
	if(parse_cli_ret == RET_EXIT_SUCCESS) {
		return EXIT_SUCCESS;
	}
	else if(parse_cli_ret == RET_EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	remorse(&config);
	return EXIT_SUCCESS;
}
