# $Revision: 1.2 $, $Date: 2003/04/15 18:03:47 $

%define		_pre	pre2

Summary:	GNU Gadu 2 - free talking
Summary(pl):	GNU Gadu 2 - wolne gadanie
Name:		gg2
Version:	2.0
Release:	0.%{_pre}.1
Epoch:		1
License:	GPL v2+
Group:		Applications/Communications
#Source0:	http://www.hakore.com/~krzak/gg2/%{name}-%{snap}.tar.gz
Source0:	http://telia.dl.sourceforge.net/sourceforge/ggadu/%{name}-%{version}%{_pre}.tar.bz2
Source1:	%{name}.desktop
URL:		http://www.gadu.gnu.pl/
#BuildRequires:	arts-devel
BuildRequires:	autoconf
BuildRequires:	automake >= 1.6
BuildRequires:	esound-devel >= 0.2.7
BuildRequires:	iksemel-devel >= 0.0.1
BuildRequires:	glib2-devel  >= 2.2.0
BuildRequires:	gtk+2-devel  >= 2.2.0
BuildRequires:	libgadu-devel >= 1.0
BuildRequires:	libtlen-devel
BuildRequires:	libtool
BuildRequires:	intltool
BuildRequires:	gettext-devel >= 0.11.0
BuildRequires:	xosd-devel   >= 2.0.0
BuildRequires:  pkgconfig
BuildRequires:	fontconfig-devel
BuildRoot:	%{tmpdir}/%{name}-%{version}-root-%(id -u -n)

%description
Gadu-Gadu and Tlen.pl and any other instant messanger client with
GTK+2 GUI on GNU/GPL.

%description -l pl
Klient Gadu-Gadu i Tlen.pl oraz innych protoko³ów z GUI pod GTK+2 na
licencji GNU/GPL.

%package gui-gtk+2
Summary:	GTK+2 GUI plugin
Summary(pl):	Wtyczka z GUI w GTK+2
Group:		Applications/Communications
Requires:	%{name} = %{version}

%description gui-gtk+2
GTK+2 GUI plugin for GNU Gadu 2.

%description gui-gtk+2 -l pl
Wtyczka z GUI w GTK+2 do GNU Gadu 2.

%package emoticons
Summary:	Emoticons
Summary(pl):	Emotikony
Group:		Applications/Communications
Requires:	%{name} = %{version}

%description emoticons
Emotions icons and description files.

%description emoticons -l pl
Zestaw ikon z emotikonami, oraz plikiem konfiguracyjnym.

%package gadu-gadu
Summary:	Gadu-Gadu plugin
Summary(pl):	Wtyczka protoko³u Gadu-Gadu
Group:		Applications/Communications
Requires:	%{name} = %{version}

%description gadu-gadu
Gadu-Gadu protocol plugin.

%description gadu-gadu -l pl
Wtyczka protoko³u Gadu-Gadu.

%package tlen
Summary:	Tlen.pl plugin
Summary(pl):	Wtyczka protoko³u Tlen.pl
Group:		Applications/Communications
Requires:	%{name} = %{version}

%description tlen
Tlen.pl protocol plugin.

%description tlen -l pl
Wtyczka protoko³u Tlen.pl.

%package jabber
Summary:	Jabber.org plugin
Summary(pl):	Wtyczka protoko³u Jabber
Group:		Applications/Communications
Requires:	%{name} = %{version}

%description jabber
Jabber protocol plugin.

%description jabber -l pl
Wtyczka protoko³u Jabber.org.

%package sound-esd
Summary:	Sound support with ESD
Summary(pl):	Obs³uga d¼wiêku poprzez ESD
Group:		Applications/Communications
Requires:	%{name} = %{version}

%description sound-esd
Sound support with ESD.

%description sound-esd -l pl
Obs³uga d¼wiêku poprzez ESD.

%package sound-oss
Summary:	OSS sound support
Summary(pl):	Obs³uga d¼wiêku OSS
Group:		Applications/Communications
Requires:	%{name} = %{version}

%description sound-oss
OSS sound support.

%description sound-oss -l pl
Obs³uga d¼wiêku OSS.

%package sound-external
Summary:	Sound support with external player
Summary(pl):	Obs³uga d¼wiêku przez zewnêtrzny program
Group:		Applications/Communications
Requires:	%{name} = %{version}

%description sound-external
Sound support with external player.

%description sound-external -l pl
Obs³uga d¼wiêku przez zewnêtrzny program

#%package sound-aRts
#Summary:	Sound support with aRts
#Summary(pl):	Obs³uga d¼wiêku poprzez aRts
#Group:		Applications/Communications
#Requires:	%{name} = %{version}

#%description sound-aRts
#Sound support with aRts.

#%description sound-aRts -l pl
#Obs³uga d¼wiêku poprzez aRts.

%package xosd
Summary:	Support for X On Screen Display
Summary(pl):	Wy¶wietlanie komunikatów na ekranie X
Group:		Applications/Communications
Requires:	%{name} = %{version}

%description xosd
Support for X On Screen Display.

%description xosd -l pl
Wy¶wietlanie komunikatów na ekranie X.

%package docklet
Summary:	Support for Window Managers docklets
Summary(pl):	Obs³uga dokletów w ró¿nych zarz±dcach okien
Group:		Applications/Communications
Requires:	%{name} = %{version}

%description docklet
Support for Window Managers docklets (GNOME, KDE).

%description docklet -l pl
Obs³uga dokletów w ró¿nych zarz±dcach okien (GNOME, KDE).

%package sms
Summary:        SMS Gateway
Summary(pl):    Bramka SMS
Group:          Applications/Communications
Requires:       %{name} = %{version}

%description sms
Send SMS to cellurar phones via web gateways.

%description sms -l pl
Wtyczka wysy³aj±ca SMS-y na telefony komórkowe przez bramki WWW.

%package remote
Summary:        Remote access from other applications
Summary(pl):    Dostêp do programu z innych aplikacji
Group:          Applications/Communications
Requires:       %{name} = %{version} 

%description remote
Make possible exchange data with other applications.

%description remote -l pl
Wtyczka umo¿liwiaj±ca wymianê informacji z innymi aplikacjami.

%package themes
Summary:	Themes for GnuGadu 2 GUI
Summary(pl):	Motywy graficzne dla GUI GnuGadu 2
Group:		Applications/Communications
Requires:       %{name} = %{version}
Requires:	%{name}-gui-gtk+2

%description themes
Themes for GnuGadu 2 GUI.

%description themes
Motywy graficzne dla GUI GnuGadu 2.

%prep
%setup -q -n %{name}-%{version}%{_pre}

%build
rm -f missing
intltoolize --copy --force
%{__libtoolize}
%{__aclocal}
%{__automake}
%{__autoconf}

%configure \
	--disable-gdb \
	--disable-debug \
	--with-gui \
	--with-gadu \
	--with-tlen \
	--with-jabber \
	--with-xosd \
	--with-docklet \
	--with-esd \
	--with-oss \
	--with-sms \
	--with-external \
	--with-remote
#	--with-arts

%{__make}

%install
rm -rf $RPM_BUILD_ROOT

%{__make} install \
	DESTDIR=$RPM_BUILD_ROOT

install -d $RPM_BUILD_ROOT%{_datadir}/applications
install %{SOURCE1} $RPM_BUILD_ROOT%{_datadir}/applications
install -d $RPM_BUILD_ROOT%{_pixmapsdir}
install -d $RPM_BUILD_ROOT%{_datadir}/%{name}/sounds
install $RPM_BUILD_ROOT%{_datadir}/%{name}/pixmaps/online.png $RPM_BUILD_ROOT%{_pixmapsdir}/%{name}.png

%find_lang %{name} --all-name --with-gnome

%clean
rm -rf $RPM_BUILD_ROOT

%files -f %{name}.lang
%defattr(644,root,root,755)
%doc A* C* N* R* T* contrib doc/*
%attr(755,root,root) %{_bindir}/gg2
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
%{_datadir}/applications/gg2.desktop

%files emoticons
%defattr(644,root,root,755)
%attr(755,root,root) %{_datadir}/gg2/pixmaps/emoticons

%files gadu-gadu
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libgadu_gadu_plugin.so

%files tlen
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libtlen_plugin.so

%files jabber
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libjabber_plugin.so

%files sound-esd
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libsound_esd_plugin.so

%files sound-oss
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libsound_oss_plugin.so

%files sound-external
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libsound_external_plugin.so

%files xosd
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libxosd_plugin.so

%files docklet
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libdocklet_plugin.so

%files sms
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libsms_plugin.so

%files remote
%defattr(644,root,root,755)
%attr(755,root,root) %{_libdir}/gg2/libremote_plugin.so

%files themes
%defattr(644,root,root,755)
%dir %{_datadir}/gg2/themes
%{_datadir}/gg2/themes/*.theme
%dir %{_datadir}/gg2/pixmaps/icons
%dir %{_datadir}/gg2/pixmaps/icons/bubble
%dir %{_datadir}/gg2/pixmaps/icons/classic
%dir %{_datadir}/gg2/pixmaps/icons/modern
%dir %{_datadir}/gg2/pixmaps/icons/rozgwiazda
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
%{_datadir}/gg2/pixmaps/icons/tlen-3d/README
%{_datadir}/gg2/pixmaps/icons/tlen-3d/*.png


%define date	%(echo `LC_ALL="C" date +"%a %b %d %Y"`)
%changelog
* %{date} PLD Team <feedback@pld.org.pl>
All persons listed below can be reached at <cvs_login>@pld.org.pl

$Log: gg2.spec,v $
Revision 1.2  2003/04/15 18:03:47  krzyzak
- sync with PLD repository

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
