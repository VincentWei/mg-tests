AM_CPPFLAGS = -I$(DRM_INC_DIR)

EXTRA_DIST = README.md MiniGUI.cfg

noinst_PROGRAMS = \
    testdri

testdri_SOURCES = testdri.c mydridriver.c ply-hashtable.c ply-hashtable.h ply-bitarray.h
testdri_LDADD = -ldrm_intel
