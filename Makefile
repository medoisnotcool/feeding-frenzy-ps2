EE_BIN = fishfrenzy.elf
EE_OBJS = src/main.o src/assets.o src/audio_assets.o
EE_INCS += -I$(PS2DEV)/gsKit/include
EE_CFLAGS += -O2 -G0 -Wall -Wno-unused-parameter
EE_LDFLAGS += -L$(PS2DEV)/gsKit/lib
EE_LIBS += -lgskit -ldmakit -lpad -laudsrv -lm -lc

AUDIO_ADP = assets/converted/music.adp \
            assets/converted/eat.adp \
            assets/converted/dash.adp \
            assets/converted/grow.adp \
            assets/converted/frenzy.adp \
            assets/converted/death.adp \
            assets/converted/game_over.adp \
            assets/converted/extra_life.adp

all: $(EE_BIN)

assets/converted/%.adp: assets/audio/%.wav
	$(PS2SDK)/bin/adpenc $< $@

assets/converted/audsrv.irx:
	cp $(PS2SDK)/iop/irx/audsrv.irx $@

src/assets.o: src/assets.S
	$(EE_CC) $(EE_CFLAGS) -c -x assembler-with-cpp -o $@ $<

src/audio_assets.o: src/audio_assets.S $(AUDIO_ADP) assets/converted/audsrv.irx
	$(EE_CC) $(EE_CFLAGS) -c -x assembler-with-cpp -o $@ $<

clean:
	rm -f $(EE_BIN) $(EE_OBJS) $(AUDIO_ADP) assets/converted/audsrv.irx

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
