UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
  CC := clang++  -arch x86_64 -arch arm64
  DEFINES = -DAPL=1 -DIBM=0 -DLIN=0
else ifeq ($(UNAME_S),Linux)
  CC := g++
  DEFINES = -DAPL=0 -DIBM=0 -DLIN=1
endif

SRC_BASE	:=	.
TARGET		:=  rext
BUILDDIR	:=	./rext
OBJDIR      :=  ./obj

SOURCES = rext.cpp

DEFINES += -DXPLM200=1 -DXPLM210=1 -DXPLM300=1

# get the latest commit hash in the short form
COMMIT := $(shell git rev-parse --short HEAD)
# get the latest commit date in the form of YYYYmmdd
DATE := $(shell git log -1 --format=%cd --date=format:"%Y%m%d")
VERSION := $(COMMIT)-$(DATE)
DEFINES += -DBUILD_VERSION=\"$(VERSION)\"


ifeq ($(UNAME_S),Darwin)
LIBS = -framework XPLM -F SDK/Libraries/Mac/
endif

INCLUDES = \
	-I$(SRC_BASE)/SDK/CHeaders/XPLM


############################################################################


VPATH = $(SRC_BASE)

CSOURCES	:= $(filter %.c, $(SOURCES))
CXXSOURCES	:= $(filter %.cpp, $(SOURCES))

COBJECTS64	:= $(patsubst %.c, $(OBJDIR)/obj64/%.o, $(CSOURCES))
CXXOBJECTS64	:= $(patsubst %.cpp, $(OBJDIR)/obj64/%.o, $(CXXSOURCES))
ALL_OBJECTS64	:= $(sort $(COBJECTS64) $(CXXOBJECTS64))

CPPFLAGS := $(DEFINES) $(INCLUDES) -O3 -std=c++11
ifeq ($(UNAME_S),Linux)
  CPPFLAGS +=  -fPIC -fvisibility=hidden
else ifeq ($(UNAME_S),Darwin)
  #CFLAGS += -stdlib=libc++ -fvisibility=hidden
  CPPFLAGS += -nostdinc++ -I/Library/Developer/CommandLineTools/usr/include/c++/v1 -fvisibility=hidden 
endif


# Phony directive tells make that these are "virtual" targets, even if a file named "clean" exists.
.PHONY: all clean $(TARGET)
# Secondary tells make that the .o files are to be kept - they are secondary derivatives, not just
# temporary build products.
.SECONDARY: $(ALL_OBJECTS) $(ALL_OBJECTS64) 



# Target rules - these just induce the right .xpl files.

ifeq ($(UNAME_S),Linux)
$(TARGET): $(BUILDDIR)/64/lin.xpl

$(BUILDDIR)/64/lin.xpl: $(ALL_OBJECTS64)
	@echo Linux Linking $@
	mkdir -p $(dir $@)
	$(CC) -m64 -std=gnu++17 -static-libgcc -static-libstdc++ -shared -Wl,--version-script=exports.txt -o $@ $(ALL_OBJECTS64) $(LIBS)
else ifeq ($(UNAME_S),Darwin)
$(TARGET): $(BUILDDIR)/64/mac.xpl

$(BUILDDIR)/64/mac.xpl: $(ALL_OBJECTS64)
	@echo Darwin Linking $@
	mkdir -p $(dir $@)
	$(CC) -m64 -shared -o $@ $(ALL_OBJECTS64) $(LIBS)
endif

# Compiler rules

$(OBJDIR)/obj64/%.o : %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -m64 -c $< -o $@

$(OBJDIR)/obj64/%.o : %.cpp
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -m64 -c $< -o $@

clean:
	@echo Cleaning out everything.
	rm -rf $(OBJDIR)/* $(TARGET)/*



