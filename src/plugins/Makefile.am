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

if BUILD_DOCKLET_SYSTEM_TRAY
DOCKLET=docklet
endif

if BUILD_DOCKLET_DOCKAPP
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

if BUILD_SOUND_EXTERNAL
SOUND=sound
endif

if BUILD_SMS
SMS=sms
endif

if BUILD_AAWAY
AAWAY=aaway
endif

if BUILD_REMOTE
REMOTE=remote
endif

if BUILD_DBUS
DBUS=dbus
endif

if BUILD_UPDATE
UPDATE=update
endif

if BUILD_HISTORY_EXTERNAL_VIEWER_PLUGIN
HISTORY=history
endif


SUBDIRS = $(GUI) $(GG) $(TLEN) $(XOSD) $(DOCKLET) $(SOUND) $(JABBER) $(SMS) $(REMOTE) $(UPDATE) $(HISTORY) $(DBUS) $(AAWAY)

install-data-local:
	$(mkinstalldirs) $(DESTDIR)/$(pkglibdir)
