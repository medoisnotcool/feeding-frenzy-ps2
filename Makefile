EE_BIN = fishfrenzy.elf
EE_OBJS = src/main.o src/assets.o
EE_INCS += -I$(PS2DEV)/gsKit/include
EE_CFLAGS += -O2 -G0 -Wall -Wno-unused-parameter
EE_LDFLAGS += -L$(PS2DEV)/gsKit/lib
EE_LIBS += -lgskit -ldmakit -lpad -lm -lc

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal

# Embed preconverted RGBA textures directly in the ELF; no runtime filesystem required.
src/assets.o: src/assets.S
	$(EE_CC) $(EE_CFLAGS) -c -x assembler-with-cpp -o $@ $<
