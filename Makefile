#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#---------------------------------------------------------------------------------
TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	build
SOURCES		:=	src src/scenes src/scenes/title src/scenes/dialogue src/scenes/game src/scenes/gameover src/scenes/game_complete src/scenes/game/levels
DATA		:=	data
INCLUDES	:=	src/include src/scenes $(BUILD)
ROMFS		:=	romfs

APP_TITLE       := Bankiware 3DS
APP_DESCRIPTION := Bankiware Game for Nintendo 3DS
APP_AUTHOR      := Sekibanki Inc.
APP_ICON 				:= $(TOPDIR)/temp_textures/icon.png

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS	:=	-g -Wall -Og -ggdb -mword-relocations \
			-ffunction-sections \
			$(ARCH)

CFLAGS	+=	$(INCLUDE) -D__3DS__

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:= -lcitro2d -lcitro3d -lctru -lm

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(CTRULIB)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PICAFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.v.pica)))
SHLISTFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.shlist)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))
T3XFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.t3x)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES_SOURCES 	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES)) \
			$(PICAFILES:.v.pica=.shbin.o) $(SHLISTFILES:.shlist=.shbin.o) \
			$(T3XFILES:.t3x=.t3x.o)

export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)

export HFILES	:=	$(PICAFILES:.v.pica=_shbin.h) $(SHLISTFILES:.shlist=_shbin.h) \
			$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean all codeonly convert_textures convert_sounds

#---------------------------------------------------------------------------------
all: convert_textures convert_sounds $(BUILD)

#---------------------------------------------------------------------------------
codeonly: $(BUILD)

#---------------------------------------------------------------------------------
convert_textures:
	@echo Converting textures...
	@./tools/convert_textures.sh
	@mkdir -p romfs/textures
	@cp data/textures/*.t3x romfs/textures/

convert_sounds:
	@echo Converting sounds...
	@./tools/convert_sounds.sh

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(OUTPUT).smdh $(TARGET).elf
	@rm -fr data/textures/*.t3x romfs/textures/*.t3x
	@rm -fr romfs/sounds/*.wav

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
.PHONY: all
all: $(OUTPUT).3dsx $(OUTPUT).smdh

$(OUTPUT).3dsx	:	$(OUTPUT).elf $(OUTPUT).smdh
	3dsxtool $< $@ --smdh=$(word 2,$^) --romfs=$(TOPDIR)/$(ROMFS)

$(OUTPUT).elf	:	$(OFILES)

#---------------------------------------------------------------------------------
# rules for assembling GPU shaders
#---------------------------------------------------------------------------------
%.shbin.o : %.v.pica
	@echo $(notdir $<)
	@picasso -o $(notdir $<).shbin $<
	@bin2s $(notdir $<).shbin | $(AS) -o $@
	@printf "extern const u8 %s[];\n" `(echo $(notdir $<) | sed -e 's/^\([0-9]\)/_\1/' -e 's/[^A-Za-z0-9_]/_/g')`_shbin > $(notdir $<)_shbin.h
	@printf "extern const u32 %s_size;\n" `(echo $(notdir $<) | sed -e 's/^\([0-9]\)/_\1/' -e 's/[^A-Za-z0-9_]/_/g')`_shbin >> $(notdir $<)_shbin.h
	@mkdir -p $(CURDIR)/$(BUILD)
	@cp $(notdir $<)_shbin.h $(CURDIR)/$(BUILD)/
	@rm $(notdir $<).shbin $(notdir $<)_shbin.h

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
%.t3x.o	%_t3x.h :	%.t3x
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------