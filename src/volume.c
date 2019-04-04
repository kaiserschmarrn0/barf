#include <poll.h>
#include <stdio.h>
#include <alsa/asoundlib.h>

#include "volume.h"

static component *this;

static snd_mixer_t *handle;
static snd_mixer_elem_t *elem;

static long min;
static long max;

static int volume_draw(void) {
	long vol;
	snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_MONO, &vol);
	int perc = 100 * (vol - min) / max;

	char text[TEXT_MAX];
	sprintf(text, "    %d", perc);

	int not_mute;
	snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_MONO, &not_mute);

	char icon[ICON_MAX];
	if (not_mute) {
		sprintf(icon, "\uf028");
	} else {
		sprintf(icon, "\uf026");
	}
	
	draw_block(this, icon, text);
}

static int volume_callback(snd_mixer_elem_t *elem, unsigned int mask) {
	if (!(mask & SND_CTL_EVENT_MASK_VALUE)) {
		return 1;	
	}

	volume_draw();

	return 0;
}

int volume_run(void) {
	int err = snd_mixer_handle_events(handle);
	if (err < 0) {
		fprintf(stderr, "Volume: failed to handle events: error %d.\n", err);
		snd_mixer_close(handle);
		return 1;
	}

	return 0;
}

int volume_init(component *ref) {
	this = ref;

	int err = snd_mixer_open(&handle, 0);
	if (err < 0) {
		fprintf(stderr, "Volume: failed to open mixer: error %d.\n", err);
		snd_mixer_close(handle);
		return 1;
	}
	
	err = snd_mixer_attach(handle, "default");
	if (err < 0) {
		fprintf(stderr, "Volume: failed to attach mixer: error %d.\n", err);
		snd_mixer_close(handle);
		return 1;
	}
	
	err = snd_mixer_selem_register(handle, NULL, NULL);
	if (err < 0) {
		fprintf(stderr, "Volume: failed to register mixer: error %d.\n", err);
		snd_mixer_close(handle);
		return 1;
	}
	
	err = snd_mixer_load(handle);
	if (err < 0) {
		fprintf(stderr, "Volume: failed to load mixer: error %d.\n", err);
		snd_mixer_close(handle);
		return 1;
	}

	snd_mixer_selem_id_t *sid;
	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, "Master");
	elem = snd_mixer_find_selem(handle, sid);

	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	snd_mixer_elem_set_callback(elem, volume_callback);

	//sanity check
	int count = snd_mixer_poll_descriptors_count(handle);
	if (count != 1) {
		fprintf(stderr, "Volume: wrong file descriptor count: %d.\n", count);
		snd_mixer_close(handle);
		return 1;
	}

	struct pollfd temp;
	snd_mixer_poll_descriptors(handle, &temp, 1);

	this->fd = temp.fd;
	
	this->fg = xft_color(0xff3b4252);
	change_gc(this->bg, 0xff81a1c1);

	ref = this;
	
	volume_draw();

	return 0;
}

void volume_clean(void) {
	snd_mixer_close(handle);
}
