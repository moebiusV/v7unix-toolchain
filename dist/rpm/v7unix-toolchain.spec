Name:           v7unix-toolchain
Version:        0.1.0
Release:        1%{?dist}
Summary:        V7 Unix PDP-11 C toolchain, modernized for a modern host
License:        Caldera
URL:            https://github.com/moebiusV/v7unix-toolchain
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  bison
BuildRequires:  python3

Suggests:       filsys
Suggests:       prebsd
Suggests:       simh

%description
A modern-host port of the Seventh Edition (V7) Unix PDP-11 C toolchain
(cc, cpp/c0/c1/c2, as/as2, ld) that cross-compiles PDP-11 code.

%prep
%setup -q

%build
%configure
make %{?_smp_mflags}

%install
%make_install

%files
%{_libexecdir}/v7unix/
%{_bindir}/v7cc
%{_bindir}/v7as
%{_bindir}/v7ld
%{_libdir}/v7unix/
%{_mandir}/v7unix/man1/
%{_mandir}/man1/v7cc.1
%{_mandir}/man1/v7as.1
%{_mandir}/man1/v7ld.1

%changelog
* Thu Aug 28 2026 maintainer <email> - 0.1.0-1
- Initial package
