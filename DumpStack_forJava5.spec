%define prefix /usr
%define version 1.0.0

Summary: Ccat, an error analysing tool for Java VM
Name: DumpStack_forJava5
Version: %{version}
Release: 1
License: Commercial
Group: Development/Debuggers
URL: http://www.ipride.co.jp
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
Packager: Hiroshi ABE <abe@ipride.co.jp>
Requires: log4cxx >= 0.9.7, openssl >= 0.9.7, xerces-c >= 2.7.0
BuildRequires: log4cxx-devel >= 0.9.7, xerces-c-devel >= 2.7.0, license >= 0.0.2

%description
 Ccat is an error analysing tool for Java VM.
 It enables JVM to dump local variables with its name, class and values
whenever an exception is thrown.

%prep
%setup -q

%build
make

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=${RPM_BUILD_ROOT}/ install

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc readme.txt
%{prefix}/lib/libDumpStack*
%{prefix}/share/*


%changelog
* Wed Nov 22 2006 Hiroshi ABE <abe@ipride.co.jp> - 
- Initial build.

