if BUILD_GUI
GUI=GUI
endif

if BUILD_GG
GG=gadu_gadu
endif

if BUILD_TLEN
TLEN=tlen
endif

if BUILD_JABBER
JABBER=jabber
endif

if BUILD_XOSD
XOSD=xosd
endif

if BUILD_DOCKLET
DOCKLET=docklet
endif

if BUILD_SOUND_ESD
SOUND=sound
endif

if BUILD_SOUND_ARTS
SOUND=sound
endif

if BUILD_SOUND_OSS
SOUND=sound
endif

if BUILD_SMS
SMS=sms
endif

if BUILD_REMOTE
REMOTE=remote
endif

if BUILD_UPDATE
UPDATE=update
endif


SUBDIRS = $(GUI) $(GG) $(TLEN) $(XOSD) $(DOCKLET) $(SOUND) $(JABBER) $(SMS) $(REMOTE) $(UPDATE)

install-data-local:
	$(mkinstalldirs) $(DESTDIR)/$(pkglibdir)
