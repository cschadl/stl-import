CXX=g++
INCLUDE=-I /usr/include/eigen3 -I ../mathstuff -I ../stlutil
CFLAGS=-Wall -O3 -std=c++17
CFLAGS_DEP=-std=c++17
OUTDIR=Release

OBJS=$(shell ls *.cpp | perl -pe 's/\.cpp$$/\.o/' | tr "\n" " ")
OBJS_LIB=$(shell find . ! '(' -iregex '.*test.+' ')' -name '*.cpp' | perl -pe 's/^\.\/(.+?)\.cpp$$/$$1\.o/' | tr "\n" " ")
EXECUTABLE=stl-import
SHAREDLIB=stlimport.so

OUTOBJS=$(addprefix $(OUTDIR)/, $(OBJS))
OUTEXE=$(OUTDIR)/$(EXECUTABLE)

# Debug configuration
OUTDIR_DEBUG=Debug
OUTOBJS_DEBUG=$(addprefix $(OUTDIR_DEBUG)/, $(OBJS))
OUTEXE_DEBUG=$(OUTDIR_DEBUG)/$(EXECUTABLE)
CFLAGS_DEBUG=-Wall -O0 -ggdb3 -std=c++17
CPPFLAGS_DEBUG=-DDEBUG

# Shared library configuration
OUTDIR_SHARED=Release
OUTOBJS_SHARED=$(addprefix $(OUTDIR)/, $(OBJS_LIB))
OUTSHARED=$(OUTDIR_SHARED)/$(SHAREDLIB)

all: $(OUTEXE)

$(OUTEXE) : $(OUTOBJS)
	$(CXX) $(OUTOBJS) -o $(OUTEXE)

# This generates dependency rules
-include $(OUTOBJS:.o=.d)

## Default rule
$(OUTDIR)/%.o: %.cpp
	$(CXX) -c $(CFLAGS) $(INCLUDE) -o $@ $<
	$(CXX) -MM $(CFLAGS_DEP) $(INCLUDE) $< | perl -pe 's/^\w+\.o:/$(OUTDIR)\/$$&/' > $(OUTDIR)/$*.d

shared: CFLAGS:=$(CFLAGS) -fPIC
shared: $(OUTSHARED)

$(OUTSHARED) : $(OUTOBJS_SHARED)
	$(CXX) -shared $(OUTOBJS_SHARED) -o $(OUTSHARED)

# This generates dependency rules
-include $(OUTOBJS_SHARED:.o=.d)

## Default rule
$(OUTDIR_SHARED)/%.o: %.cpp
	$(CXX) -c $(CFLAGS) $(INCLUDE) -o $@ $<
	$(CXX) -MM $(CFLAGS_DEP) $(INCLUDE) $< | perl -pe 's/^\w+\.o:/$(OUTDIR_SHARED)\/$$&/' > $(OUTDIR_SHARED)/$*.d

clean:
	rm -rf $(OUTDIR)/*.o $(OUTEXE) $(OUTSHARED) $(OUTDIR)/*.d

debug: CFLAGS=$(CFLAGS_DEBUG) $(CPPFLAGS_DEBUG)
debug: $(OUTEXE_DEBUG)

$(OUTEXE_DEBUG): $(OUTOBJS_DEBUG)
	$(CXX) $(OUTOBJS_DEBUG) -o $(OUTEXE_DEBUG)

# This generates dependency rules
-include $(OUTOBJS_DEBUG:.o=.d)

## Default rule
$(OUTDIR_DEBUG)/%.o: %.cpp
	$(CXX) -c $(CFLAGS) $(INCLUDE) -o $@ $<
	$(CXX) -MM $(CFLAGS_DEP) $(INCLUDE) $< | perl -pe 's/^\w+\.o:/$(OUTDIR_DEBUG)\/$$&/' > $(OUTDIR_DEBUG)/$*.d

debug_clean: OUTDIR=$(OUTDIR_DEBUG)
debug_clean: clean
