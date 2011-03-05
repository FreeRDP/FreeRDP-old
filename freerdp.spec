# ./autogen.sh
# ./configure
# make dist # creates freerdp-0.0.1.tar.gz
# rpmbuild -ta freerdp-UNKNOWN.tar.gz

Summary: Remote Desktop Protocol functionality
Name: freerdp
Version: UNKNOWN
Release: 1%{?dist}
License: Apache License 2.0
Group: Applications/Communications
URL: http://freerdp.sourceforge.net/
Source: http://downloads.sourceforge.net/%{name}/%{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:  openssl-devel, libX11-devel, libXcursor-devel, cups-devel, alsa-lib-devel

%description
freerdp implements Remote Desktop Protocol (RDP), used in a number of Microsoft
products.

%package -n xfreerdp
Summary: Remote Desktop Protocol client
Requires: %{name}-libs = %{version}-%{release}, %{name}-plugins-standard = %{version}-%{release}
%description -n xfreerdp
xfreerdp is a client for Remote Desktop Protocol (RDP), used in a number of
Microsoft products.

%package libs
Summary: Core libraries implementing the RDP protocol
%description libs
libfreerdp can be embedded in applications.

libfreerdpchanman and libfreerdpkbd might be convenient to use in X
applications together with libfreerdp.

libfreerdp can be extended with plugins handling RDP channels.

%package plugins-standard
Summary: Plugins for handling the standard RDP channels
Requires: %{name}-libs = %{version}-%{release}
%description plugins-standard
A set of plugins to the channel manager implementing the standard virtual
channels extending RDP core functionality.  For example, sounds, clipboard
sync, disk/printer redirection, etc.

%package devel
Summary: Libraries and header files for embedding and extending freerdp
Requires: %{name}-libs = %{version}-%{release}
Requires: pkgconfig
%description devel
Header files and unversioned libraries for libfreerdp, libfreerdpchanman and
libfreerdpkbd.

%prep
%setup -q

%build
%configure --with-ipv6 --enable-smartcard --with-sound --with-sound=alsa --with-crypto=openssl --with-gnu-ld
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
rm -f $RPM_BUILD_ROOT%{_libdir}/{freerdp/,lib}*.{a,la} # FIXME: They shouldn't be installed in the first place

%post libs -p /sbin/ldconfig

%postun libs -p /sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files -n xfreerdp
%defattr(-,root,root)
%{_bindir}/xfreerdp
%{_mandir}/*/*

%files libs
%defattr(-,root,root)
%doc COPYING AUTHORS doc/ipv6.txt doc/ChangeLog
%{_libdir}/lib*.so.*
%dir %{_libdir}/freerdp
%{_datadir}/freerdp/

%files plugins-standard
%defattr(-,root,root)
%{_libdir}/freerdp/*.so

%files devel
%defattr(-,root,root)
%{_includedir}/freerdp/
%{_libdir}/lib*.so
%{_libdir}/pkgconfig/*

%changelog

* Tue Mar 16 2010 Mads Kiilerich <mads@kiilerich.com> - 0.0.1-1
- Initial "upstream" freerdp spec - made and tested for Fedora 12
