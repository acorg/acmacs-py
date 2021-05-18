# -*- Makefile -*-
# ----------------------------------------------------------------------

TARGETS = \
  $(ACMACS_PY_LIB)

ACMACS_PY_SOURCES = \
  py-common.cc \
  py-antigen.cc \
  py-chart.cc \
  py-chart-util.cc \
  py-merge.cc \
  py-mapi.cc \
  py-titers.cc \
  py.cc

ACMACS_PY_LIB_MAJOR = 1
ACMACS_PY_LIB_MINOR = 0
ACMACS_PY_LIB_NAME = acmacs
ACMACS_PY_LIB = $(DIST)/$(ACMACS_PY_LIB_NAME)$(PYTHON_MODULE_SUFFIX)

# ----------------------------------------------------------------------

SRC_DIR = $(abspath $(ACMACSD_ROOT)/sources)

all: install

CONFIGURE_CAIRO = 1
CONFIGURE_PYTHON = 1
include $(ACMACSD_ROOT)/share/Makefile.config

LDLIBS = \
  $(AD_LIB)/$(call shared_lib_name,libacmacsbase,1,0) \
  $(AD_LIB)/$(call shared_lib_name,liblocationdb,1,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacsvirus,1,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacswhoccdata,1,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacschart,2,0) \
  $(AD_LIB)/$(call shared_lib_name,libhidb,5,0) \
  $(AD_LIB)/$(call shared_lib_name,libseqdb,3,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacsdraw,1,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacsdraw,1,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacsmapdraw,2,0) \
  $(CAIRO_LIBS) $(XZ_LIBS) $(CXX_LIBS)

# ----------------------------------------------------------------------

install: make-installation-dirs $(TARGETS)
	$(call install_all,$(AD_PACKAGE_NAME))
	$(call install_py_all)

test: install
	@#test/test
.PHONY: test

# ----------------------------------------------------------------------

$(ACMACS_PY_LIB): $(patsubst %.cc,$(BUILD)/%.o,$(ACMACS_PY_SOURCES)) | $(DIST)
	$(call echo_shared_lib,$@)
	$(call make_shared_lib,$(ACMACS_PY_LIB_NAME),$(ACMACS_PY_LIB_MAJOR),$(ACMACS_PY_LIB_MINOR)) $(LDFLAGS) -o $@ $^ $(LDLIBS) $(PYTHON_LIBS)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
