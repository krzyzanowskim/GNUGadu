# $Revision: 1.19 $, $Date: 2004/08/23 14:41:32 $
#
# Conditional build: 
%bcond_with	arts
%bcond_without	perl
%bcond_without	esd
%bcond_without	gtkspell
#
Summary:	GNU Gadu 2 - free talking
Summary(es):	GNU Gadu 2 - charlar libremente
Summary(pl):	GNU Gadu 2 - wolne gadanie
Name:		gg2
Version:	2.0.5.1
Release:	1
Epoch:		3
License:	GPL v2+
Group:		Applications/Communications
Source0:	http://osdn.dl.sourceforge.net/sourceforge/ggadu/%{name}-%{version}.tar.bz2
URL:		http://www.gnugadu.org/
%{?with_arts:BuildRequires:	arts-devel}
BuildRequires:	autoconf
BuildRequires:	automake >= 1.7
%{?with_esd:BuildRequires:	esound-devel >= 0.2.7}
BuildRequires:	gettext-devel >= 0.11.0
BuildRequires:	glib2-devel  >= 2.2.0
BuildRequires:	gtk+2-devel  >= 2.4.0
BuildRequires:	libtlen-devel
BuildRequires:	libtool
BuildRequires:	loudmouth-devel >= 0.16-4
BuildRequires:	openssl-devel >= 0.9.7d
%{?with_perl:BuildRequires:	perl-devel}
%{?with_gtkspell:BuildRequires:	gtkspell-devel}
%{?with_gtkspell:BuildRequires:	aspell-devel}
BuildRequires:	pkgconfig
BuildRequires:	xosd-devel   >= 2.0.0
Requires:	gg2-ui
BuildRoot:	%{tmpdir}/%{name}-%{version}-root-%(id -u -n)

%description
Gadu-Gadu, Tlen.pl and others instant messenger client with
GTK+2 GUI on GNU/GPL.

%description -l es
Un cliente para Gadu-Gadu, Tlen.pl y otros protocolos con un GUI de
GTK+2, bajo la licencia GNU/GPL.

%description -l pl
Klient Gadu-Gadu, Tlen.pl oraz innych protoko³ów z GUI pod GTK+2 na
licencji GNU/GPL.

%package devel
Summary:	Headers for libgg2_core library to develop plugins
Summary(es):	Cabeceras para la biblioteca libgg2_core para desarrollar plugins
Summary(pl):	Pliki nag³ówkowe biblioteki libgg2_core potrzebne do rozwijania wtyczek
Group:		Applications/Communications
Requires:	%{name} = %{epoch}:%{version}-%{release}
Requires:	glib2-devel
Requires:	perl-devel

%description devel
This package contains header files for libgg2_core library, needed to
develop plugins for GNU Gadu 2.

%description devel -l es
Este paquete contiene los ficheros de cabeceras de la biblioteca
libgg2_core necesarios para desarrollar plugins para GNU Gadu 2.

%description devel -l pl
Ten pakiet zawiera pliki nag³ówkowe biblioteki libgg2_core, potrzebne
do rozwijania wtyczek do GNU Gadu 2.

%package gui-gtk+2
Summary:	GTK+2 GUI plugin
Summary(es):	Plugin de GUI en GTK+2
Summary(pl):	Wtyczka z GUI w GTK+2
Group:		Applications/Communications
Provides:	gg2-ui
Requires:	%{name} = %{epoch}:%{version}-%{release}

%description gui-gtk+2
GTK+2 GUI plugin for GNU Gadu 2.

%description gui-gtk+2 -l es
Un plugin con un GUI en GTK+2 para GNU Gadu 2.

%description gui-gtk+2 -l pl
Wtyczka z GUI w GTK+2 do GNU Gadu 2.

%package emoticons
Summary:	Emoticons
Summary(es):	Emoticons
Summary(pl):	Emotikony
Group:		Applications/Communications
Requires:	%{name} = %{epoch}:%{version}-%{release}

%description emoticons
Emotions icons and description files.

%description emoticons -l es
Iconas de emociones y sus ficheros de descripción.

%description emoticons -l pl
Zestaw ikon z emotikonami, oraz plikiem konfiguracyjnym.

%package gadu-gadu
Summary:	Gadu-Gadu plugin
Summary(es):	Plugin de Gadu-Gadu
Summary(pl):	Wtyczka protoko³u Gadu-Gadu
Group:		Applications/Communications
Requires:	%{name} = %{epoch}:%{version}-%{release}

%description gadu-gadu
Gadu-Gadu protocol plugin.

%description gadu-gadu -l es
Un plugin para el protocolo Gadu-Gadu.

%description gadu-gadu -l pl
Wtyczka protoko³u Gadu-Gadu.

%package tlen
Summary:	Tlen.pl plugin
Summary(es):	Plugin de Tlen.pl
Summary(pl):	Wtyczka protoko³u Tlen.pl
Group:		Applications/Communications
Requires:	%{name} = %{epoch}:%{version}-%{release}

%description tlen
Tlen.pl protocol plugin.

%description tlen -l es
Un plugin para el protocolo Tlen.pl.

%description tlen -l pl
Wtyczka protoko³u Tlen.pl.

%package jabber
Summary:	Jabber.org plugin
Summary(es):	Plugin de Jabber.org
Summary(pl):	Wtyczka protoko³u Jabber
Group:		Applications/Communications
Requires:	%{name} = %{epoch}:%{version}-%{release}
Requires:	loudmouth >= 0.16-4

%description jabber
Jabber protocol plugin.

%description jabber -l es
Un plugin para el protocolo Jabber.

%description jabber -l pl
Wtyczka protoko³u Jabber.

%package sound-esd
Summary:	Sound support with ESD
Summary(es):	Soporte de sonido a través de ESD
Summary(pl):	Obs³uga d¼wiêku poprzez ESD
Group:		Applications/Communications
Requires:	%{name} = %{epoch}:%{version}-%{release}

%description sound-esd
Sound support with ESD.

%description sound-esd -l es
Soporte de sonido a través de ESD.

%description sound-esd -l pl
Obs³uga d¼wiêku poprzez ESD.

%package sound-oss
Summary:	OSS sound support
Summary(es):	Soporte de sonido a través de OSS
Summary(pl):	Obs³uga d¼wiêku OSS
Group:		Applications/Communications
Requires:	%{name} = %{epoch}:%{version}-%{release}

%description sound-oss
OSS sound support.

%description sound-oss -l es
Soporte de sonido a través de OSS.

%description sound-oss -l pl
Obs³uga d¼wiêku OSS.

%package sound-external
Summary:	Sound support with external player
Summary(es):	Soporte de sonido vía un reproductor externo
Summary(pl):	Obs³uga d¼wiêku przez zewnêtrzny program
Group:		Applications/Communications
Requires:	%{name} = %{epoch}:%{version}-%{release}

%description sound-external
Sound support with external player.

%description sound-external -l es
Soporte de sonido a través de un reproductor externo.

%description sound-external -l pl
Obs³uga d¼wiêku przez zewnêtrzny program.

%package sound-aRts
Summary:	Sound support with aRts
Summary(es):	Soporte de sonido a través de aRts
Summary(pl):	Obs³uga d¼wiêku poprzez aRts
Group:		Applications/Communications
Requires:	%{name} = %{version}

%description sound-aRts
Sound support with aRts.

%description sound-aRts -l es
Soporte de sonido a través de aRts.

%description sound-aRts -l pl
Obs³uga d¼wiêku poprzez aRts.

%package xosd
Summary:	Support for X On Screen Display
Summary(es):	Soporte para plasmar mensajes sobre el fondo de X
Summary(pl):	Wy¶wietlanie komunikatów na ekranie X
Group:		Applications/Communications
Requires:	%{name} = %{epoch}:%{version}-%{release}

%description xosd
Support for X On Screen Display.

%description xosd -l es
Soporte para plasmar mensajes sobre el fondo (XOSD).

%description xosd -l pl
Wy¶wietlanie komunikatów na ekranie X.

%package docklet-system-tray
Summary:	Support for Window Managers notification areas
Summary(es):	Soporte para áreas de notificación de los Manejantes de Ventanas
Summary(pl):	Obs³uga obszarów powiadomieñ w ró¿nych zarz±dcach okien
Group:		Applications/Communications
Requires:	%{name} = %{epoch}:%{version}-%{release}
Obsoletes:	%{name}-docklet

%description docklet-system-tray
Support for Window Managers notification areas (GNOME, KDE).

%description docklet-system-tray -l es
Soporte para áreas de notificación de los Manejantes de Ventanas
(GNOME, KDE).

%description docklet-system-tray -l pl
Obs³uga obszarów powiadomieñ w ró¿nych zarz±dcach okien (GNOME, KDE).

%package docklet-dockapp
Summary:	Support for WindowMaker-style dockapp
Summary(es):	Soporte de dockapp estilo WindowMaker
Summary(pl):	Obs³uga dokowalnego apletu zgodnego z WindowMakerem
Group:		Applications/Communications
Requires:	%{name} = %{epoch}:%{version}-%{release}
Obsoletes:	%{name}-docklet

%description docklet-dockapp
Support for WindowMaker-style dockapp.

%description docklet-dockapp -l es
Suporte de dockapp estilo WindowMaker.

%description docklet-dockapp -l pl
Obs³uga dokowalnego apletu zgodnego z WindowMakerem.

%package sms
Summary:	SMS Gateway
Summary(es):	Puerta SMS
Summary(pl):	Bramka SMS
Group:		Applications/Communications
Requires:	%{name} = %{epoch}:%{version}-%{release}

%description sms
Send SMS to cellular phones via web gateways.

%description sms -l es
Manda mensajes SMS a móviles vía puertas del Web.

%description sms -l pl
Wtyczka wysy³aj±ca wiadomo¶ci SMS na telefony komórkowe przez bramki
WWW.

%package remote
Summary:	Remote access from other applications
Summary(es):	Acceso remoto desde otras aplicaciones
Summary(pl):	Dostêp do programu z innych aplikacji
Group:		Applications/Communications
Provides:	gg2-ui
Requires:	%{name} = %{epoch}:%{version}-%{release}

%description remote
Make possible exchange data with other applications.

%description remote -l es
Permite intercambiar los datos con otras aplicaciones.


%description remote -l pl
Wtyczka umo¿liwiaj±ca wymianê informacji z innymi aplikacjami.


%package history-external
Summary:	Allow to view GNU Gadu chat history
Summary(pl):	Pozwala przegl±daæ historiê rozmów GNU Gadu.
Group:		Applications/Communications
Requires:	gtk+2
Requires:	%{name} = %{epoch}:%{version}-%{release}

%description history-external
Allow to view GNU Gadu chat history

%description history-external -l pl
Pozwala przegl±daæ historiê rozmów GNU Gadu.

%package update
Summary:	Check for new GNU Gadu newer version
Summary(es):	Verifica si hay versiones nuevas de GNU Gadu
Summary(pl):	Sprawdza czy jest dostêpna nowsza wersja GNU Gadu
Group:		Applications/Communications
Requires:	%{name} = %{epoch}:%{version}-%{release}

%description update
Check for new GNU Gadu newer version.

%description update -l es
Verifica si hay nuevas versiones de GNU Gadu.

%description update -l pl
Sprawdza czy jest dostêpna nowsza wersja GNU Gadu

%package themes
Summary:	Themes for GNU Gadu 2 GUI
Summary(es):	Temas para el GUI de GNU Gadu 2
Summary(pl):	Motywy graficzne dla GUI GNU Gadu 2
Group:		Applications/Communications
Requires:	%{name} = %{epoch}:%{version}-%{release}
Requires:	%{name}-gui-gtk+2

%description themes
Themes for GNU Gadu 2 GUI.

%description themes -l es
Temas para el GUI de GNU Gadu 2.

%description themes -l pl
Motywy graficzne dla GUI GNU Gadu 2.

%prep
%setup -q

%build
rm -f missing
%{__gettextize}
%{__libtoolize}
%{__aclocal} -I src/plugins/gadu_gadu/libgadu/m4
%{__automake}
%{__autoconf}

%configure \
	%{!?debug:--disable-gdb} \
	%{!?debug:--disable-debug} \
 	--with-gui \
 	--with-gadu \
 	--with-tlen \
 	--with-jabber \
 	--with-xosd \
 	--with-sms \
 	--with-docklet_system_tray \
	--with-docklet_dockapp \
	--with%{!?with_esd:out}-esd \
	--with%{!?with_arts:out}-arts \
 	--with-oss \
 	--with-external \
 	--with-update \
	--with-history-external-viewer \
	--with-gghist \
	--with%{!?with_gtkspell:out}-gtkspell \
	--%{?with_perl:with}%{!?with_perl:without}-perl \
 	--with-remote

%{__make}

%install
rm -rf $RPM_BUILD_ROOT

%{__make} install \
	DESTDIR=$RPM_BUILD_ROOT

install -d $RPM_BUILD_ROOT%{_desktopdir}
install gg2.desktop $RPM_BUILD_ROOT%{_desktopdir}/gg2.desktop

%find_lang %{name} --all-name --with-gnome

rm -f $RPM_BUILD_ROOT%{_libdir}/%{name}/*.la

%clean
rm -rf $RPM_BUILD_ROOT

%post	-p /sbin/ldconfig
%postun	-p /sbin/ldconfig

%files -f %{name}.lang
%defattr(644,root,root,755)
%doc A* C* N* R* T* contrib doc/*
%attr(755,root,root) %{_bindir}/gg2
%attr(755,root,root) %{_libdir}/libgg2_core.so.*.*.*
%dir %{_libdir}/gg2
%{_datadir}/%{name}/sounds

%files gui-gtk+2
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libGUI_plugin.so
%dir %{_datadir}/gg2
%dir %{_datadir}/gg2/pixmaps
%{_datadir}/gg2/pixmaps/*.png
%{_datadir}/gg2/pixmaps/*.gif
%{_pixmapsdir}/%{name}.png
%{_desktopdir}/gg2.desktop

%files devel
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/libgg2_core.so
%{_libdir}/libgg2_core.la
%{_includedir}/gg2_core.h
%{_pkgconfigdir}/gg2_core.pc

%files emoticons
%defattr(644,root,root,755)
%{_datadir}/gg2/pixmaps/emoticons

%files gadu-gadu
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libgadu_gadu_plugin.so

%files tlen
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libtlen_plugin.so

%files jabber
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libjabber_plugin.so

%if %{with esd}
%files sound-esd
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libsound_esd_plugin.so
%endif

%files sound-oss
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libsound_oss_plugin.so

%files sound-external
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libsound_external_plugin.so

%if %{with arts}
%files sound-aRts
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libsound_arts_plugin.so
%endif

%files xosd
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libxosd_plugin.so

%files docklet-system-tray
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libdocklet_system_tray_plugin.so

%files docklet-dockapp
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libdocklet_dockapp_plugin.so

%files sms
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libsms_plugin.so

%files remote
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libremote_plugin.so

%files history-external
%defattr(644,root,root,755)
%attr(755,root,root) %{_bindir}/gghist
%attr(755,root,root) %{_libdir}/gg2/libhistory_external_plugin.so

%files update
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libupdate_plugin.so

%files themes
%defattr(644,root,root,755)
%dir %{_datadir}/gg2/themes
%{_datadir}/gg2/themes/*.theme
%dir %{_datadir}/gg2/pixmaps/icons
%dir %{_datadir}/gg2/pixmaps/icons/bubble
%dir %{_datadir}/gg2/pixmaps/icons/classic
%dir %{_datadir}/gg2/pixmaps/icons/modern
%dir %{_datadir}/gg2/pixmaps/icons/rozgwiazda
%dir %{_datadir}/gg2/pixmaps/icons/ghosts
%dir %{_datadir}/gg2/pixmaps/icons/tlen-classic
%{_datadir}/gg2/pixmaps/icons/bubble/*.png
%{_datadir}/gg2/pixmaps/icons/bubble/README
%{_datadir}/gg2/pixmaps/icons/classic/*.png
%{_datadir}/gg2/pixmaps/icons/classic/README
%{_datadir}/gg2/pixmaps/icons/modern/*.png
%{_datadir}/gg2/pixmaps/icons/modern/README
%{_datadir}/gg2/pixmaps/icons/rozgwiazda/*.png
%{_datadir}/gg2/pixmaps/icons/rozgwiazda/license.txt
%{_datadir}/gg2/pixmaps/icons/ghosts/*.png
%{_datadir}/gg2/pixmaps/icons/ghosts/README
%{_datadir}/gg2/pixmaps/icons/tlen-classic/*.png

%define date	%(echo `LC_ALL="C" date +"%a %b %d %Y"`)
%changelog
* %{date} PLD Team <feedback@pld-linux.org>
All persons listed below can be reached at <cvs_login>@pld-linux.org

$Log: gg2.spec,v $
Revision 1.19  2004/08/23 14:41:32  krzyzak
23.08.2004
- 2.0.5.1 bugfix release - now jabber plugin compile and work with pre/post loudmouth 0.16

Revision 1.18  2004/08/22 20:37:14  krzyzak
- release 2.0.5

Revision 1.113  2004/08/04 23:21:39  twittner
- play with distfiles (or sf)

Revision 1.112  2004/08/04 23:01:39  krzak
- MD5 update

Revision 1.111  2004/08/04 22:55:40  krzak
- up to 2.0.4
- history-external subpackage added

Revision 1.110  2004/06/22 16:13:52  malekith
- gg2-jabber R: new enough loudmouth

Revision 1.109  2004/06/22 16:05:14  malekith
- release 2
- add patch with  functions needed to login into server diffrent then the @-part
  of the jid (like jabber.pld-linux.org)

Revision 1.108  2004/06/12 07:55:03  krzak
- up to 2.0.3
- remove tmpdir.patch (already in sources)

Revision 1.107  2004/05/06 08:14:41  krzak
- rel 2
- gg2-tmpdir.patch

Revision 1.106  2004/05/05 19:18:00  krzak
- remove gg2-path.patch, path is proper.

Revision 1.105  2004/05/04 08:25:42  adamg
- simplifed %%setup

Revision 1.104  2004/05/04 08:23:51  adamg
- added path.patch - improper dir in initialize_plugin() function (from
  src/plugins/gadu_gadu/gadu_gadu_plugin.c)

Revision 1.103  2004/05/04 08:03:39  krzak
- up to 2.0.2

Revision 1.102  2004/04/26 19:58:38  emes
- fixed a typo

Revision 1.101  2004/04/23 20:50:41  krzak
- up to 2.0.1

Revision 1.100  2004/04/04 16:22:58  adamg
- use dl.sourceforge.net in Source0 URL

Revision 1.99  2004/04/03 15:27:40  krzak
- aspell-devel conditionaly

Revision 1.98  2004/04/03 14:49:12  pawelb
- BR aspell-devel

Revision 1.97  2004/04/02 19:35:22  averne
- updated Source0 again

Revision 1.96  2004/04/02 19:31:26  averne
- updated Source0

Revision 1.95  2004/04/02 19:11:14  krzak
- up to 2.0

Revision 1.94  2004/03/19 23:46:26  ankry
- massive change: BR openssl-devel >= 0.9.7d

Revision 1.93  2004/03/14 08:58:53  krzak
- up to 2.0pre8
- bcond without_gtkspell
- removed emoticons-fixup_and_tlen.patch - already in sources

Revision 1.92  2004/03/10 06:26:33  gotar
- fixed URL

Revision 1.91  2004/03/10 00:09:34  krzak
- URL: http://www.gnugadu.org

Revision 1.90  2004/02/22 08:49:33  gotar
- added emoticons* patch, release 0.pre7.2.

Revision 1.89  2004/02/21 23:12:44  krzak
- BR: loudmouth >= 0.15.1

Revision 1.88  2004/02/21 23:08:16  krzak
- up to 2.0pre7

Revision 1.87  2004/01/27 10:33:55  tiwek
--release up to rebuild with new libloudmouth

Revision 1.86  2004/01/15 12:33:56  tiwek
- s/TryExec/Exec in desktop file

Revision 1.85  2004/01/14 21:04:28  krzak
- version 2.0pre6

Revision 1.84  2004/01/11 21:02:18  krzak
- up to version 2.0pre5

Revision 1.83  2004/01/09 18:38:16  mmazur
- added virtual package gg2-ui, so that gg2 doesn't install without a user
  interface

Revision 1.82  2003/12/21 10:36:41  gotar
- chmod -x emoticons

Revision 1.81  2003/12/14 00:22:35  krzak
- finally release 0.pre4.1 (sorry)

Revision 1.80  2003/12/13 23:50:12  krzak
- version: 2.0pre4
- rel 1

Revision 1.79  2003/12/13 17:13:18  krzak
- up to version pre4
- remove gg2.desktop source, it's in source already.

Revision 1.78  2003/12/12 20:41:14  tiwek
- update to 20031211 snap, more change (see ChangeLog)
- added missing BR openssl-devel

Revision 1.77  2003/12/03 09:13:36  krzak
- fixed rel number

Revision 1.76  2003/12/03 02:12:29  krzak
- up to 20031202

Revision 1.75  2003/11/22 18:50:10  saq
- missing es translations
- typo

Revision 1.74  2003/11/22 18:39:52  qboosh
- some cosmetics and simplifications
- fixed pl summary for dockapp, removed bogus es one

Revision 1.73  2003/11/22 15:31:11  krzak
- up to snap 20031122
- additional sub package docklet-dockapp
- remove tyop.patch (already in sources)

Revision 1.72  2003/11/18 21:15:19  tiwek
- BR libgadu >=4:1.4 fore rebuild with new libgadu

Revision 1.71  2003/11/17 23:03:17  radek
- reverted (not a typo)

Revision 1.70  2003/11/17 22:43:41  radek
- typo (?): s/3:1.3.1/3:1.3-1/ in libgadu req

Revision 1.69  2003/11/17 22:04:55  saq
- fix the libgadu-devel BR

Revision 1.68  2003/11/17 21:56:19  saq
- es translations, fixed the pl ones
- don't ignore --without*
- typo.patch (for configure.in)

Revision 1.67  2003/11/17 20:51:32  krzak
- up to 20031117
- docklet -> docklet-system-tray
- bcond esd
- added package "update"

Revision 1.66  2003/11/11 10:58:56  matkor
- Release 2. Rebuild against ekg-1.4-1.rc2.1.

Revision 1.65  2003/11/11 10:30:08  gotar
- debug ready

Revision 1.64  2003/10/30 10:32:49  qboosh
- removed BR fontconfig-devel and xcursor-devel - already in proper places

Revision 1.63  2003/10/30 08:57:32  luzik
 - added xcursor-devel BR

Revision 1.62  2003/10/30 00:23:55  krzak
- changed BR: libgadu-devel
- package gadu-gadu R: libgadu <= 3:1.3-1

Revision 1.61  2003/10/21 09:43:28  saq
- snapshot 20031020
- some es translations

Revision 1.60  2003/10/10 13:47:51  radek
- added epoch to BR'ed libgadu-devel version

Revision 1.59  2003/10/09 21:55:24  krzak
- BR: libgadu-devel <= 1.3-1

Revision 1.58  2003/09/17 21:34:03  krzak
- epoch 2

Revision 1.57  2003/09/17 21:22:59  misi3k
- typo

Revision 1.56  2003/09/17 21:12:29  krzak
- up to snap 20030917
- bcond without perl (perl support)

Revision 1.55  2003/09/12 09:57:21  krzak
- add bcond for arts plugin

Revision 1.54  2003/08/18 08:08:06  gotar
- mass commit: cosmetics (removed trailing white spaces)

Revision 1.53  2003/08/13 22:52:22  ankry
- description fixes / unification

Revision 1.52  2003/08/11 22:46:30  ankry
- cosmetics

Revision 1.51  2003/08/11 20:44:54  ankry
cosmetics

Revision 1.50  2003/07/13 14:37:43  erjot
- fix release, epoch++ ?

Revision 1.49  2003/06/27 13:10:45  mmazur
- mass commit; now req: name = epoch:version

Revision 1.48  2003/06/12 10:19:35  qboosh
- cosmetics

Revision 1.47  2003/06/10 08:08:26  qboosh
- libgg2_core is regular shared library, so use standard procedure

Revision 1.46  2003/06/10 07:58:44  qboosh
- missing defattr for devel

Revision 1.45  2003/06/10 06:09:33  aflinta
- typo

Revision 1.44  2003/06/10 06:07:53  aflinta
- simplification, don't be owner of pkgconfigdir!

Revision 1.43  2003/06/10 05:59:28  aflinta
- updated source

Revision 1.42  2003/06/09 22:53:54  krzak
- update to 2.0pre3

Revision 1.4  2003/06/03 11:30:54  krzyzak
SPEC file update

Revision 1.41  2003/05/25 11:00:51  malekith
- massive attack, adding Source-md5

Revision 1.40  2003/05/25 05:47:50  misi3k
- massive attack s/pld.org.pl/pld-linux.org/

Revision 1.39  2003/04/11 21:07:02  adgor
- Fixed %%doc
- Version 2.0; Release 0.%%{_pre}.1

Revision 1.38  2003/04/11 20:45:13  misi3k
- changed source from gz to bz2.

Revision 1.37  2003/04/11 20:39:18  krzak
- upgrade to pre2

Revision 1.36  2003/03/25 11:10:37  krzak
- release 2
- updated source0, url
- no more gettextize
- missing license.txt file
- STBR

Revision 1.35  2003/03/25 10:56:02  krzak
- s/glib-gettextize/gettextize/
- BR: libgadu-devel >= 1.0
- BR: gettext-devel >= 0.11.0

Revision 1.34  2003/03/15 13:19:41  qboosh
- pl fixes (sms,remote)

Revision 1.33  2003/03/15 12:00:24  aflinta
- pure fontconfig is enough

Revision 1.32  2003/03/15 11:27:05  krzak
- update to 2.0pre1

Revision 1.31  2003/03/04 01:35:46  djrzulf
- Source0 corrected (From: kahless@stovokor.eu.org),

Revision 1.30  2003/03/02 10:22:07  martii
BR: XFree86-fontconfig-devel added - maybe fontconfig-devel will be enough

Revision 1.29  2003/02/18 11:55:27  areq
- BuildRequires:        libgadu-devel >= 20030211

Revision 1.28  2003/02/17 14:14:03  radek
- fixed %%files (*.xpm->*.png)
- installed %{_datadir}/%{name}/sounds (whatever it's for)

Revision 1.27  2003/02/17 00:12:30  qboosh
- s/tematy/motywy/, cosmetics

Revision 1.26  2003/02/15 11:55:15  krzak
xpm->png

Revision 1.25  2003/02/15 11:46:48  krzak
- version 2.0pre0

Revision 1.24  2003/02/14 21:09:11  krzak
- themes subpackage

Revision 1.23  2003/01/25 19:21:35  aflinta
- updated to snap 20030125

Revision 1.22  2003/01/25 17:03:16  aflinta
- corrected gettext support

Revision 1.21  2003/01/25 13:29:39  aflinta
- we are still waitnig for gettext solution ;)

Revision 1.20  2003/01/24 17:50:00  aflinta
- added BR

Revision 1.19  2003/01/22 10:47:54  krzak
- snap 20030122

Revision 1.18  2003/01/14 18:42:29  aflinta
- updated to snap 20030113

Revision 1.17  2003/01/05 16:51:54  qboosh
- copy-pasto

Revision 1.16  2003/01/04 15:28:33  aflinta
- updates

Revision 1.15  2003/01/04 15:08:56  krzak
- snap 20030104
- gg2.desktop
- subpackage gg2-jabber

Revision 1.14  2003/01/02 12:09:06  krzak
BR: intltool

Revision 1.13  2002/12/13 15:24:04  ankry
- adapterized

Revision 1.12  2002/12/06 10:41:04  krzak
-  snap 20021204
-  locales added

Revision 1.11  2002/12/01 22:33:37  qboosh
- added missing dir (%%{_datadir}/gg2) back
- 755 perms not needed for images - changed to default 644
- fixed some pl descriptions (missing subpackage names in %%description tags)

Revision 1.10  2002/12/01 15:46:31  krzak
- docklet package
- release 20021201

Revision 1.9  2002/11/30 16:57:50  krzak
- snap 20021130
- plugins to {_libdir}/gg2 moved
- little fixes

Revision 1.8  2002/11/29 21:10:15  qboosh
- pl fixes, cleanups (let autoreq work), added missing dirs and important TODO

Revision 1.7  2002/11/28 18:04:05  krzak
- snap 20021127
- package sound-oss

Revision 1.6  2002/11/25 01:41:01  ankry
Massive attack:
- s/man[ea]d[z¿][ae]r/zarz±dca/g
- (some) new %%doc

Revision 1.5  2002/11/24 16:24:19  krzak
- added package emoticons
- snapshot 20021124

Revision 1.4  2002/11/23 15:41:35  krzak
- fixes
- missing BR
- missing --disable-debug --disable-gdb

Revision 1.3  2002/11/23 15:26:42  krzak
- fixes

Revision 1.2  2002/11/23 15:05:28  krzak
- subpackages added

Revision 1.1  2002/11/23 14:16:46  aflinta
PLD initial and experimental release
