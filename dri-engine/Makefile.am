AM_CPPFLAGS = -I$(DRM_INC_DIR)

EXTRA_DIST = README.md MiniGUI.cfg

noinst_PROGRAMS = \
    testdri

testdri_SOURCES = testdri.c mydridriver.c intel_batchbuffer.c intel_batchbuffer.h intel_context.h intel_reg.h \
    ply-hashtable.c ply-hashtable.h ply-bitarray.h

testdri_LDADD = -ldrm_intel
