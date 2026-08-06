%global pkg_version 1.1.0

Name:           torghostng
Version:        %{pkg_version}
Release:        1%{?dist}
Summary:        Tor proxy management tool

License:        GPLv3
URL:            https://github.com/tkmxqrdxddd/Torghostng-C
Source0:        %{name}-%{pkg_version}.tar.gz

BuildRequires:  gcc
BuildRequires:  libcurl-devel
BuildRequires:  pkgconfig
BuildRequires:  make

Requires:       tor
Requires:       iptables
Requires:       procps-ng
Requires:       libcurl

%description
TorGhostNG redirects TCP traffic through the Tor network (SOCKS5
127.0.0.1:9050), routes DNS through Tor's DNSPort, and optionally
restricts traffic to a specific exit country.

It manages the required system changes (iptables chain, resolv.conf,
torrc) itself and rolls all of them back on failure or when stopped.

%prep
%setup -q

%build
%make_build

%install
%make_install PREFIX=/usr
install -Dm0644 packaging/man/torghostng.1 %{buildroot}%{_mandir}/man1/torghostng.1

%files
%{_bindir}/torghostng
%{_mandir}/man1/torghostng.1
%doc README.md LICENSE

%changelog
* Thu Aug 06 2026 tkmxqrdxddd <tkmxqrd@gmail.com> - 1.1.0-1
- Initial packaging
