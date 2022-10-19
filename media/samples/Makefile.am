
EXTRA_DIST = playback_prompt.wav
EXTRA_DIST += record_prompt.wav
EXTRA_DIST += music.wav

bin_PROGRAMS = prompt2wav

prompt2wav_SOURCES = prompt2wav.cxx $(BUILT_SOURCES)

BUILT_SOURCES = playback_prompt.h record_prompt.h

%.raw:	%.wav
	sox $< -r 8k -e signed -b 16 -c 1 $@

%.h:	%.raw
	xxd -i -u $< $@
	sed -i -e 's/_raw//g' $@
