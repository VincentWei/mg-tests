AM_CPPFLAGS = -I$(DRM_INC_DIR)

EXTRA_DIST = README.md LICENSE MiniGUI.cfg

noinst_PROGRAMS = testdrm

testdrm_SOURCES = testdrm.c flying_window.c

testdrm_LDADD = -lmgeff

lib_LTLIBRARIES = libtestdrmdriver.la

libtestdrmdriver_la_SOURCES = intel_i915_driver.c intel_batchbuffer.h intel_context.h intel_reg.h \
                              intel_chipset.c intel_chipset.h i915_pciids.h libdrm_macros.h

libtestdrmdriver_la_LIBADD = -ldrm_intel

libtestdrmdriver_la_LDFLAGS = -release 0.1 -version-info 0:1:0
