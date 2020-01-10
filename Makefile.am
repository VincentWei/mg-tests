COMMON_SUBDIRS = m4 src 4.0 extra-input drm-engine comm-engines resmgr

if MGUSE_COMPOSITING
COMPOS_SUBDIRS=compositor
else
COMPOS_SUBDIRS=
endif

DIST_SUBDIRS = $(COMMON_SUBDIRS) compositor
SUBDIRS = $(COMMON_SUBDIRS) $(COMPOS_SUBDIRS)
EXTRA_DIST = ltmain.sh README.md LICENSE
