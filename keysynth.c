#include <fluidsynth.h>
#include <alsa/asoundlib.h>


#define perm_ok(pinfo,bits) ((snd_seq_port_info_get_capability(pinfo) & (bits)) == (bits))

void alsa_list() {
	snd_seq_t *seq;
	snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0);

        snd_seq_client_info_t *cinfo;
        snd_seq_port_info_t *pinfo;
        int match=0;

        snd_seq_client_info_alloca(&cinfo);
        snd_seq_port_info_alloca(&pinfo);
        snd_seq_client_info_set_client(cinfo, -1);
        while (snd_seq_query_next_client(seq, cinfo) >= 0) {
                snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
                snd_seq_port_info_set_port(pinfo, -1);
                while (snd_seq_query_next_port(seq, pinfo) >= 0) {
                        if (perm_ok(pinfo, SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ)) {
				printf("   %s\n", snd_seq_port_info_get_name(pinfo));
			}
		}
	}
}

void alsa_connect(char *name) {
	snd_seq_t *seq;
	snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0);

	snd_seq_addr_t keyboard;
	snd_seq_addr_t dest;

	/* find keyboard and fluidsynth */
        snd_seq_client_info_t *cinfo;
        snd_seq_port_info_t *pinfo;
        int match=0;

        snd_seq_client_info_alloca(&cinfo);
        snd_seq_port_info_alloca(&pinfo);
        snd_seq_client_info_set_client(cinfo, -1);
        while (snd_seq_query_next_client(seq, cinfo) >= 0) {
                snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
                snd_seq_port_info_set_port(pinfo, -1);
                while (snd_seq_query_next_port(seq, pinfo) >= 0) {
			printf("  %3d '%-16s'",
			       snd_seq_port_info_get_port(pinfo),
			       snd_seq_port_info_get_name(pinfo));
                        if (perm_ok(pinfo, SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ)) {
				if(strstr(snd_seq_port_info_get_name(pinfo),name)) {
					printf(".. our keyboard!\n");
 					keyboard.client=snd_seq_port_info_get_client(pinfo);
					keyboard.port=snd_seq_port_info_get_port(pinfo);
					match++;
					if(match==2) goto do_connect;
				}
			}

                        if (perm_ok(pinfo, SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE)) {
				if(strstr(snd_seq_port_info_get_name(pinfo),"Simple Synth")) { 
					printf(".. ourself!\n");
 					dest.client=snd_seq_port_info_get_client(pinfo);
					dest.port=snd_seq_port_info_get_port(pinfo);
					match++;
					if(match==2) goto do_connect;
				}
			}
			printf("\n");
                }
        }

	printf("keyboard not found\n");
	abort();

	int client;
do_connect:
        if ((client = snd_seq_client_id(seq)) < 0) {
                snd_seq_close(seq);
		abort();
        }

        if (snd_seq_set_client_name(seq, "Simple Connector") < 0) {
                snd_seq_close(seq);
		abort();
        }
	

	printf("connect %hhu:%hhu to %hhu:%hhu\n",keyboard.client,keyboard.port,dest.client,dest.port);

	snd_seq_port_subscribe_t *subs;
        snd_seq_port_subscribe_alloca(&subs);
        snd_seq_port_subscribe_set_sender(subs, &keyboard);
        snd_seq_port_subscribe_set_dest(subs, &dest);
        snd_seq_port_subscribe_set_queue(subs, 0);
        snd_seq_port_subscribe_set_exclusive(subs, 0);
        snd_seq_port_subscribe_set_time_update(subs, 0);
        snd_seq_port_subscribe_set_time_real(subs, 0);
	snd_seq_subscribe_port(seq, subs);

	snd_seq_close(seq);

}

int main(int argc, char** argv)
{
	if(argc!=3) {
		printf("Usage: %s <soundfont file> <part of keyboard alsa port name>\n",argv[0]);
		printf("Example: %s WST25FStein_00Sep22.SF2 \"Keystation 61es\"\n",argv[0]);
		printf("\nAvailable ports:\n");
		alsa_list();
		return 0;
	}

	fluid_settings_t* settings = new_fluid_settings();

	fluid_settings_setstr(settings, "audio.driver", "alsa");
	fluid_settings_setstr(settings, "midi.driver", "alsa_seq");
	fluid_settings_setstr(settings, "midi.alsa_seq.id", "Simple Synth");

	fluid_synth_t *synth = new_fluid_synth(settings);
	fluid_midi_driver_t* mdriver = new_fluid_midi_driver(settings, fluid_synth_handle_midi_event, synth);
	fluid_audio_driver_t *adriver = new_fluid_audio_driver(settings, synth);

	fluid_synth_sfload(synth, argv[1], 1);

	printf("soundfont loaded\n");

	alsa_connect(argv[2]);

	for(;;) sleep(1000);

	printf("delete\n");

	delete_fluid_synth(synth);
	delete_fluid_midi_driver(mdriver);
	delete_fluid_settings(settings);

	return 0;
}

