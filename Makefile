# Time-stamp: <[Makefile] modified by Guangxu Pan on 2020-06-01 14:13:59 Monday>

BASEDIR := $(shell pwd)
SRCDIR  := src
LIBDIR  := lib
BINDIR  := bin
INSTALLDIR := $(HOME)/sys/lib
vpath %.h $(SRCDIR)
vpath %.c $(SRCDIR)
vpath %.cc $(SRCDIR)
vpath %.cpp $(SRCDIR)
vpath % $(SRCDIR)

# --------------------------------------------------
# mode: debug/release
#MODE = debug
MODE = release

# --------------------------------------------------
# base settings
# --------------------------------------------------
CC  = gcc
CXX = g++
AR  = ar
LD  = ld

# Extra flags to give to the C compiler.
CFLAGS   = -Wall -fPIC
# Extra flags to give to the C++ compiler.
CXXFLAGS = -Wall -fPIC -std=c++17
ARFLAGS  = crs
LDFLAGS  = 
# Library flags or names given to compilers when they are supposed to invoke the linker, 'ld'.
LDLIBS   =

ifeq ($(MODE), release)
CFLAGS += -O2 -s
CXXFLAGS += -O2 -s
else
CFLAGS += -g
CXXFLAGS += -g
endif

# --------------------------------------------------
I = -I. -I$(SRCDIR) -Ilib/DAAPI_v1.15/include
L = -L$(BINDIR) -Llib/DAAPI_v1.15/Linux/gcc5.4.0-6

LIBS_COMMON = -lfeeder -lcoral -lswordfish -lx -lstdc++fs -lyaml-cpp
LIBS_COMMON += -lboost_date_time -lboost_filesystem -lboost_regex -lboost_system  -lboost_chrono -lboost_log -lboost_program_options -lboost_thread -lboost_iostreams -lz
LIBS_COMMON += -lprotobuf -lprotobuf-lite -lsodium -lzmq -lssl -lcrypto -liconv -lpthread -ldl

# --------------------------------------------------
ifneq ($(MAKECMDGOALS), clean)
%.d: %.cc
	$(CC) -MM $(CXXFLAGS) $(I) $< | sed 's,^$(*F).o[\s:]*,$*.o $@: ,g' > $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(I) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(I) -c $< -o $@

endif

# ######################################################################
APP = zhida_daapi_feeder
#all: $(APP) 
all: $(APP) image save
.PHONY : all

.PHONY: clean
clean : 
	-@find . -type f -regextype egrep -regex ".*\.(log|o|d|pyc|pyo|docker.tar.gz)$$" -exec rm -rf \{\} \;
	-@find bin -type f -name "core" -exec rm -rf \{\} \;
	-@find bin -type f -name $(APP) -exec rm -rf \{\} \;

.PHONY: image
image:
	@echo "[$(APP)] build docker image ......"
	@tag=`cd bin; ./$(APP) -v`; [ -z $$tag ] && tag="v0.0.0"; \
	img="$(APP):$$tag"; \
	docker rmi $$(docker images --filter "dangling=true" -q) >/dev/null 2>&1; \
	docker build -t $$img .
	@echo "[$(APP)] build docker image ok"

.PHONY: save
save:
	@echo "[$(APP)] save docker image ......"
	@tag=`cd bin; ./$(APP) -v`; [ -z $$tag ] && tag="v0.0.0"; \
	img="$(APP):$$tag"; \
	docker rmi $$(docker images --filter "dangling=true" -q) >/dev/null 2>&1; \
	rm -rf "$(APP)_$$tag.docker.tar.gz"; \
	docker save $$img > $(APP)_$$tag.docker.tar && gzip $(APP)_$$tag.docker.tar;
	@echo "[$(APP)] save docker image ok"

# ==================================================

# --------------------------------------------------
# bin: zhida_daapi_feeder
# --------------------------------------------------
SRC_ZHIDA_DAAPI_FEEDER :=  $(wildcard src/*.cc)
BIN_ZHIDA_DAAPI_FEEDER  = $(APP) 
LIB_ZHIDA_DAAPI_FEEDER = -lDAApi $(LIBS_COMMON)

OBJ_ZHIDA_DAAPI_FEEDER := $(SRC_ZHIDA_DAAPI_FEEDER:%.cc=%.o)
$(BIN_ZHIDA_DAAPI_FEEDER): $(OBJ_ZHIDA_DAAPI_FEEDER)
	@echo ">>> build $(BIN_ZHIDA_DAAPI_FEEDER) ......"
	$(CXX) $(CXXFLAGS) $(L) -o $@ $^ $(LIB_ZHIDA_DAAPI_FEEDER)
	@mv $@ $(BINDIR)
	@cp lib/DAAPI_v1.15/Linux/gcc5.4.0-6/*.so* $(BINDIR)
	@echo ">>> build $(BIN_ZHIDA_DAAPI_FEEDER) ok"


