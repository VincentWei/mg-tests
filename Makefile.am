COMMON_SUBDIRS = m4 src api 5.0 4.0 extra-input drm-engine comm-engines resmgr images

if MGRM_PROCESSES
PROCS_SUBDIRS=compositor
else
PROCS_SUBDIRS=
endif

DIST_SUBDIRS = $(COMMON_SUBDIRS) compositor
SUBDIRS = $(COMMON_SUBDIRS) $(PROCS_SUBDIRS)
EXTRA_DIST = ltmain.sh README.md LICENSE run-auto-tests.sh comm-engines-freertos
