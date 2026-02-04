Name:           scratchrobin
Version:        1.0.0
Release:        1%{?dist}
Summary:        Database administration tool for ScratchBird

License:        IDPL
URL:            https://github.com/DaltonCalford/ScratchRobin
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.20
BuildRequires:  gcc-c++ >= 11
BuildRequires:  wxGTK3-devel >= 3.2
BuildRequires:  libsecret-devel

Requires:       wxGTK3 >= 3.2
Requires:       libsecret

Recommends:     libpq5
Recommends:     mariadb-connector-c

%description
ScratchRobin is a lightweight, ScratchBird-native database
administration tool inspired by FlameRobin. It provides a
comprehensive GUI for managing ScratchBird databases with
support for PostgreSQL, MySQL, and Firebird.

%prep
%autosetup

%build
%cmake -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install

# Desktop file
install -D -m 0644 packaging/rpm/scratchrobin.desktop \
    %{buildroot}/%{_datadir}/applications/scratchrobin.desktop

# Icon
install -D -m 0644 resources/icons/scratchrobin.png \
    %{buildroot}/%{_datadir}/pixmaps/scratchrobin.png

%files
%license LICENSE
%doc README.md CHANGELOG.md
%{_bindir}/scratchrobin
%{_datadir}/applications/scratchrobin.desktop
%{_datadir}/pixmaps/scratchrobin.png

%changelog
* Mon Feb 03 2026 Dalton Calford <dcalford@stacktrace.ca> - 1.0.0-1
- Initial release
