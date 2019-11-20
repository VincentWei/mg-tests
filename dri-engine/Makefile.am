AM_CPPFLAGS = -I$(DRM_INC_DIR)

EXTRA_DIST = README.md LICENSE MiniGUI.cfg

noinst_PROGRAMS = testdri

testdri_SOURCES = testdri.c flying_window.c

testdri_LDADD = -lmgeff

lib_LTLIBRARIES = libtestdridriver.la

libtestdridriver_la_SOURCES = intel_i915_driver.c intel_batchbuffer.h intel_context.h intel_reg.h \
    ply-hashtable.c ply-hashtable.h ply-bitarray.h

libtestdridriver_la_LIBADD = -ldrm_intel

libtestdridriver_la_LDFLAGS = -release 0.1 -version-info 0:1:0
