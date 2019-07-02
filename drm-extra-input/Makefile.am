AM_CPPFLAGS = -I$(DRM_INC_DIR)

EXTRA_DIST = README.md MiniGUI.cfg

noinst_PROGRAMS = \
    eventdumper

eventdumper_SOURCES = eventdumper.c mydrmdriver.c ply-hashtable.c ply-hashtable.h ply-bitarray.h
eventdumper_LDADD = -ldrm_intel
