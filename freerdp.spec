# make dist # creates freerdp-0.0.1.tar.gz
# rpmbuild -ta freerdp-0.0.1.tar.gz

Summary: Remote Desktop Protocol client
Name: freerdp
Version: 0.0.1
Release: 1%{?dist}
License: GPLv2
Group: Applications/Communications
Url: http://freerdp.sourceforge.net/
Source: http://downloads.sourceforge.net/%{name}/%{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:  openssl-devel, libX11-devel

%description
xfreerdp is a client for Remote Desktop Protocol (RDP), used in a number of
Microsoft products including Windows NT Terminal Server, Windows 2000 Server,
Windows XP and Windows 2003 Server.

%package devel
Summary: Freerdp development support

%description devel
Support for building freerdp plug-ins

%prep
%setup -q

%build
%configure --with-ipv6 --enable-smartcard --with-sound
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
rm -f $RPM_BUILD_ROOT%{_libdir}/lib*.{a,la,so} # FIXME: They shouldn't be installed in the first place

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc COPYING doc/AUTHORS doc/ipv6.txt doc/ChangeLog
%{_bindir}/xfreerdp
%{_libdir}/lib*.so.*
%{_datadir}/freerdp/

%files devel
%defattr(-,root,root)
%{_includedir}/freerdp/

%changelog

* Tue Mar 16 2010 Mads Kiilerich <mads@kiilerich.com> - 0.0.1-1
- Initial "upstream" freerdp spec - made and tested for Fedora 12
